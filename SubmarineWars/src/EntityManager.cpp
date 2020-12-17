//
//  EntityManager.cpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-09-27.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#include "EntityManager.hpp"
#include "Systems/System.hpp"

constexpr vec2 EntityManager::DEF_BOMB_VELOCITY;

constexpr vec2 EntityManager::DEF_TORPEDO_FORCE;

constexpr vec2 EntityManager::DEF_SUB_FORCE;

constexpr vec2 EntityManager::DEF_FISH_FORCE;

/// Default constructor
EntityManager::EntityManager(): scoreLabel("SCORE:", 8), moneyLabel("MONEY:", 8, 100), livesLabel("x ", 2, 5), missilesLabel("x ", 2, 1), stageLabel("STAGE ", 2, 0)
{
    // Setup the component array map
    // Need attention when a system accesses the sprite component array;
    // It is now an array of Sprite* to allow polymorphism
    this->iregistry[typeid(Sprite)] = reinterpret_cast<SWComponent**>(this->sprites);

    this->registry[typeid(StaticSprite)] = this->ssprites;

    this->registry[typeid(AnimatedSprite)] = this->asprites;
    
    this->registry[typeid(Color)] = this->colors;

    this->registry[typeid(Position)] = this->positions;

    this->registry[typeid(Velocity)] = this->velocities;

    this->registry[typeid(Rotation)] = this->rotations;

    this->registry[typeid(Physics)] = this->physics;

    this->registry[typeid(Collision)] = this->collisions;
    
    this->registry[typeid(Input)] = this->inputs;

    this->registry[typeid(Attack)] = this->attacks;
    
    this->registry[typeid(Player)] = &this->player;

    this->registry[typeid(Score)] = this->scores;

    this->registry[typeid(Pathing)] = this->pathings;

    this->registry[typeid(Animation)] = this->animations;

    this->registry[typeid(Store)] = this->stores;

    this->registry[typeid(Distortion)] = this->distortions;
}

/// Default destructor
EntityManager::~EntityManager() = default;

//
// MARK:- Entity Factory
//

///
/// [Factory] Make a bomb at the given position
///
/// @param bomb The newly created bomb on return
/// @param position The initial position of the bomb
/// @param initVel The initial velocity of the bomb
/// @return `true` on success, `false` otherwise.
/// @note In addition to the basic 4 components required for rendering, this factory method
///       also registers the velocity and collision components for the given `bomb`.
///       The default bomb velocity is stored in `EntityManager::DEF_BOMB_VELOCITY`.
///
bool EntityManager::makeBomb(Bomb& bomb, Position& position, vec2 initVel)
{
    if (!this->make(bomb, position, {1.f, 1.f}))
    {
        pserror("[Fatal] Failed to make a bomb.");
        
        return false;
    }
    
    // Configure the collision and velocity component
    auto id = bomb.getIdentifier();
    
    this->velocities[id].vx = initVel.x;
    
    this->velocities[id].vy = initVel.y;
    
    this->collisions[id].type = Collision::bomb;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    this->physics[id].mass = 0.25f; // TODO: tweak this maybe?

    if(id == this->tutorialBomb.getIdentifier())
    {
        this->physics[id].force = {0,0};
    } else
    {
        this->physics[id].force = {0.f, gravity};
    }

    this->distortions[id].distort = true;

    bomb.registerComponent(this->collisions[id]);
    
    bomb.registerComponent(this->velocities[id]);

    bomb.registerComponent(this->distortions[id]);
    
    return true;
}

///
/// [Factory] Make an explosion at the given position
///
/// @param explosion The newly created explosion on return
/// @param position The initial position of the explosion
/// @return `true` on success, `false` otherwise.
///
bool EntityManager::makeExplosion(Explosion& explosion, Position& position)
{
    if (!this->make(explosion, position, {1.f, 1.f}, Color::defaultColor, 0.0f, true, nullptr))
    {
        pserror("[Fatal] Failed to make an explosion.");
        
        return false;

    }

    // Configure the collision component
    auto id = explosion.getIdentifier();

    this->collisions[id].type = Collision::eType::explosion;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    // Setup the velocity component, required by the CollisionSystem
    this->velocities[id].vx = 0;
    
    this->velocities[id].vy = 0;

    // Configure the animation component
    this->animations[id].setAnimationMode(Animation::Mode::Autoterminating, 0);

    Animation::Callback callback = [](int identifier, void* userptr)
    {
        reinterpret_cast<EntityManager*>(userptr)->removeExplosion(identifier);
    };

    this->animations[id].registerCallback(callback, this);

    // Assign arbitrarily large mass to prevent explosions from being moved by water current
    // TODO: this is pretty janky
    this->physics[id].mass = 999999.f;

    this->physics[id].force = {0.f, 0.f};

    this->distortions[id].distort = true;

    explosion.registerComponent(this->collisions[id]);

    explosion.registerComponent(this->velocities[id]);
    
    explosion.registerComponent(this->animations[id]);

    explosion.registerComponent(this->distortions[id]);
    
    return true;
}

///
/// [Factory] Make a submarine at the given position
///
/// @param submarine The newly created submarine on return
/// @param position The initial position of the submarine
/// @param direction The facing direction of the submarine; either moving `Left` or `Right`
/// @param x_velocity The velocity along the x-axis
/// @param type The collision layer of this submarine
/// @param score The score award if this submarine is destroyed
/// @param radarRadius The radius of the radar to detect the player boat
///                    By default this is the maximum value of a float type;
///                    i.e. The newly created submarine can always detect the player boat.
/// @return `true` on success, `false` otherwise.
///
bool EntityManager::makeSubmarine(Submarine& submarine,
                                  Position& position,
                                  Direction direction,
                                  float x_velocity,
                                  Submarine::Type type,
                                  uint32_t score,
                                  float radarRadius)
{
    // Guard: Check the correctness of the API Usage
    if (x_velocity < 0)
    {
        pserror("API Usage Warning: The given x velocity must be an absolute value.");
    }
    
    if(position.y < 20)
    {
        pserror("[Fatal] Submarine position invalid. An error has occured.");
        
        return false;
    }
    
    vec2 scale = { 1.f, 1.f };
    
    // CollisionSystem seems to implictly did this for us.
    if (direction == Direction::Left)
    {
        scale.x *= -1;
        
        x_velocity = -abs(x_velocity);
    }

    // Fixme: There's gotta be a better way of doing this, but this works for now.
    if (type == Submarine::Type::I) {
        if (!this->make(*getSub1Ptr(submarine), position, scale))
        {
            pserror("[Fatal] Failed to make a submarine.");

            return false;
        }
    } else if (type == Submarine::Type::II) {
        if (!this->make(*getSub2Ptr(submarine), position, scale))
        {
            pserror("[Fatal] Failed to make a submarine.");

            return false;
        }
    } else {
        if (!this->make(*getSub3Ptr(submarine), position, scale))
        {
            pserror("[Fatal] Failed to make a submarine.");

            return false;
        }
    }

    
    // Configure the velocity, collision and score component
    auto id = submarine.getIdentifier();
    
    this->velocities[id].vx = x_velocity;
    
    this->velocities[id].vy = 0;
    
    this->collisions[id].type = Collision::eType::submarine;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    this->physics[id].force.y = DEF_SUB_FORCE.y;
    if (direction == Direction::Left) {
        this->physics[id].force.x = -DEF_SUB_FORCE.x;
    } else {
        this->physics[id].force.x = DEF_SUB_FORCE.x;
    }
    if(id == tutorialSub.getIdentifier())
    {
        this->physics[id].force.x = 0;
    }

    this->physics[id].mass = 1.f;

    this->scores[id].score = score;

    this->distortions[id].distort = true;
    
    submarine.registerComponent(this->collisions[id]);
    
    submarine.registerComponent(this->velocities[id]);
    
    submarine.registerComponent(this->scores[id]);

    submarine.registerComponent(this->distortions[id]);

    // TODO: Revise this (Code Smell)
    
    // Register attack component if submarine is in Layer 2 or Layer 3
    this->attacks[id].radius = radarRadius;

    switch (type)
    {
        case Submarine::Type::II:
            this->attacks[id].type = Attack::torpedo;
            
            submarine.registerComponent(this->attacks[id]);
            
            break;
            
        case Submarine::Type::III:
            this->attacks[id].type = Attack::missile;
            
            submarine.registerComponent(this->attacks[id]);
            
            break;
            
        default:
            break;
    }
    
    return true;
}

///
/// Helper function for make submarine that re-casts sub to proper type
///
/// @param Reference to SubmarineI
/// @return A pointer to sub
///
SubmarineI* EntityManager::getSub1Ptr(Submarine& sub) {
    SubmarineI *ptr = static_cast<SubmarineI*>(&sub);
    return ptr;
}

///
/// Helper function for make submarine that re-casts sub to proper type
///
/// @param Reference to SubmarineII
/// @return A pointer to sub
///
SubmarineII* EntityManager::getSub2Ptr(Submarine& sub) {
    SubmarineII *ptr = static_cast<SubmarineII*>(&sub);
    return ptr;
}

///
/// Helper function for make submarine that re-casts sub to proper type
///
/// @param Reference to SubmarineI
/// @return A pointer to sub
///
SubmarineIII* EntityManager::getSub3Ptr(Submarine& sub) {
    SubmarineIII *ptr = static_cast<SubmarineIII*>(&sub);
    return ptr;
}

///
/// [Factory] Make a fish at the given position
///
/// @param fish The newly created fish on return
/// @param position The initial position of the fish
/// @param direction The facing direction of the fish; either moving `Left` or `Right`
/// @param x_velocity The velocity along the x-axis
/// @return `true` on success, `false` otherwise.
///
bool EntityManager::makeFish(Fish &fish, Position &position, Direction direction, float x_velocity)
{
    vec2 scale = { 1.f, 1.f };
    
    if (!this->make(fish, position, scale))
    {
        pserror("[Fatal] Failed to make a submarine.");
        
        return false;
    }
    
    // Configure the velocity, collision and score component
    auto id = fish.getIdentifier();
    
    this->velocities[id].vx = x_velocity;
    
    this->velocities[id].vy = 0;
    
    this->collisions[id].type = Collision::eType::fish;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    if(id == this->tutorialFish.getIdentifier())
    {
        this->physics[id].force = {0,0};

    } else
    {
        this->physics[id].force.y = DEF_FISH_FORCE.y;
        this->physics[id].force.x = DEF_FISH_FORCE.x;
    }

    this->physics[id].mass = 1.f;

    this->scores[id].score = EntityManager::DEF_FISH_SCORE;

    this->distortions[id].distort = true;
    
    fish.registerComponent(this->collisions[id]);
    
    fish.registerComponent(this->velocities[id]);
    
    fish.registerComponent(this->scores[id]);

    fish.registerComponent(this->distortions[id]);
    
    return true;
}

///
/// [Factory] Make a torpedo at the given position
///
/// @param torpedo The newly created torpedo on return
/// @param position The initial position of the torpedo
/// @param initVel The initial velocity of the torpedo
/// @return `true` on success, `false` otherwise.
/// @note In addition to the basic 4 components required for rendering, this factory method
///       also registers the velocity and collision components for the given `torpedo`.
///       The default torpedo velocity is stored in `EntityManager::DEF_TORPEDO_VELOCITY`.
///
bool EntityManager::makeTorpedo(Torpedo& torpedo, Position& position, vec2 initVel)
{
    if (!this->make(torpedo, position, {1.f, 1.f}))
    {
        pserror("[Fatal] Failed to make a torpedo.");

        return false;
    }

    // Add the collision and velocity component
    auto id = torpedo.getIdentifier();

    this->velocities[id].vx = initVel.x;

    this->velocities[id].vy = initVel.y;

    this->collisions[id].type = Collision::torpedo;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    if(id == this->tutorialTorpedo.getIdentifier())
    {
        this->physics[id].force = {0,0};
        this->rotations[id] = 3.14195/2;
    } else
    {
        this->physics[id].force = EntityManager::DEF_TORPEDO_FORCE;
    }
    
    this->physics[id].mass = 1.f;

    this->distortions[id].distort = true;

    torpedo.registerComponent(this->collisions[id]);

    torpedo.registerComponent(this->velocities[id]);

    torpedo.registerComponent(this->distortions[id]);

    return true;
}


///
/// [Factory] Make a puff of smoke at given location
///
/// @param smoke The newly created smoke on return
/// @return `true` on success, `false` otherwise.
///
bool EntityManager::makeSmoke(Smoke& smoke)
{
    Position position;

    int boatid = this->boat.getIdentifier();

    position.x = positions[boatid].x;

    std::pair<vec2, vec2> thisBB = sprites[boatid]->getBoundingBox(physics[boatid].scale, positions[boatid]);

    position.y = thisBB.first.y;

    if (!this->make(smoke, position, {0.05, 0.05}))
    {
        perror("[Fatal] Failed to make smoke.");

        return false;
    }

    // add the physics component
    auto id = smoke.getIdentifier();

    this->physics[id].mass = 0.005f;

    this->physics[id].force.x = 1.f;

    this->physics[id].force.y = -1.f;

    this->velocities[id].vx = 0.5f;

    this->velocities[id].vy = -0.5f;

    this->collisions[id].type = Collision::smoke;


    smoke.registerComponent(this->physics[id]);

    smoke.registerComponent(this->velocities[id]);

    smoke.registerComponent(this->collisions[id]);

    return true;
}

///
/// [Factory] Make a missile at the given position
///
/// @param missile The newly created missile on return
/// @param position The initial position of the missile
/// @return `true` on success, `false` otherwise.
/// @note In addition to the basic 4 components required for rendering, this factory method
///       also registers the velocity and collision components for the given `missile`.
///       The default missile velocity is stored in `EntityManager::DEF_MISSILE_VELOCITY`.
///
bool EntityManager::makeMissile(Missile& missile, Position& position)
{
    if (!this->make(missile, position, {1.f, 1.f}))
    {
        pserror("[Fatal] Failed to make a torpedo.");

        return false;
    }

    // Add the collision and velocity component
    auto id = missile.getIdentifier();

    this->collisions[id].type = Collision::missile;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    this->physics[id].force = {0.f, 0.f};

    this->physics[id].mass = 99999.f; // Again, arbitratily large because these shouldn't be moved by current

    this->pathings[id].startPosition = position;
    this->pathings[id].targetPosition = this->positions[this->boat.getIdentifier()];
    this->pathings[id].bezier = true;

    this->distortions[id].distort = true;

    missile.registerComponent(this->collisions[id]);

    missile.registerComponent(this->velocities[id]);

    missile.registerComponent(this->pathings[id]);

    missile.registerComponent(this->distortions[id]);

    return true;
}

///
/// [Factory] Make a boat missile targeting the given position
///
/// @param missile The newly created missile on return
/// @param position The initial position of the missile
/// @param target The targeted position of the missile
/// @return `true` on success, `false` otherwise.
/// @note In addition to the basic 4 components required for rendering, this factory method
///       also registers the velocity and collision components for the given `missile`.
///       The default missile velocity is stored in `EntityManager::DEF_MISSILE_VELOCITY`.
///
bool EntityManager::makeBoatMissile(BoatMissile& boatMissile, Position& position, Position& target)
    {

    if (!this->make(boatMissile, position, {1.f, 1.f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0.0f, true))
    {
        pserror("[Fatal] Failed to make a boat missile.");

        return false;
    }

    // Add the collision and velocity component
    auto id = boatMissile.getIdentifier();

    this->collisions[id].type = Collision::boatMissile;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    this->physics[id].force = {0.f, 0.f};

    this->physics[id].mass = 99999.f; // Again, arbitratily large because these shouldn't be moved by current

    this->animations[id].setAnimationMode(Animation::Mode::Loopback, 1);


        // TODO: Pathing for boatMissile
    this->pathings[id].startPosition = position;
    this->pathings[id].targetPosition = target;
    this->pathings[id].bezier = false;
    this->pathings[id].increment = 0.025f;

    this->distortions[id].distort = true;

    boatMissile.registerComponent(this->collisions[id]);

    boatMissile.registerComponent(this->velocities[id]);

    boatMissile.registerComponent(this->pathings[id]);

    boatMissile.registerComponent(this->animations[id]);

    boatMissile.registerComponent(this->distortions[id]);


        return true;
}

///
/// [Factory] Make a store icon allowing the player to buy lives
///
/// @param buyLives The newly created icon on return
/// @param position The position of the icon
/// @return `true` on success, `false` otherwise.
/// @note In addition to the basic 4 components required for rendering, this factory method
///       also registers the velocity and collision components for the given icon.
///
bool EntityManager::makeBuyLives(BuyLives& buyLives, Position& position) {

    if (!this->make(buyLives, position, {1.f, 1.f}))
    {
        pserror("[Fatal] Failed to make a buy lives icon.");

        return false;

    }

    // Configure the collision component
    auto id = buyLives.getIdentifier();

    this->collisions[id].type = Collision::eType::storeIcon;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    // Configure the store component
    this->stores[id].type = Store::sType::life;

    // Setup the velocity component, required by the CollisionSystem
    this->velocities[id].vx = 0;

    this->velocities[id].vy = 0;

    // Assign arbitrarily large mass to prevent icons from being moved by water current
    // TODO: this is pretty janky
    this->physics[id].mass = 999999.f;

    this->physics[id].force = {0.f, 0.f};

    buyLives.registerComponent(this->collisions[id]);

    buyLives.registerComponent(this->velocities[id]);

    buyLives.registerComponent(this->stores[id]);

    //buyLives.registerComponent(this->animations[id]);

    return true;
}

///
/// [Factory] Make a store icon allowing the player to buy missiles
///
/// @param buyMissiles The newly created icon on return
/// @param position The position of the icon
/// @return `true` on success, `false` otherwise.
/// @note In addition to the basic 4 components required for rendering, this factory method
///       also registers the velocity and collision components for the given icon.
///
bool EntityManager::makeBuyMissiles(BuyMissiles& buyMissiles, Position& position) {
    if (!this->make(buyMissiles, position, {1.f, 1.f}))
    {
        pserror("[Fatal] Failed to make a buy lives icon.");

        return false;

    }

    // Configure the collision component
    auto id = buyMissiles.getIdentifier();

    this->collisions[id].type = Collision::eType::storeIcon;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    // Configure the store component
    this->stores[id].type = Store::sType::boatMissile;

    // Setup the velocity component, required by the CollisionSystem
    this->velocities[id].vx = 0;

    this->velocities[id].vy = 0;

    // Assign arbitrarily large mass to prevent icons from being moved by water current
    // TODO: this is pretty janky
    this->physics[id].mass = 999999.f;

    this->physics[id].force = {0.f, 0.f};

    buyMissiles.registerComponent(this->collisions[id]);

    buyMissiles.registerComponent(this->velocities[id]);

    buyMissiles.registerComponent(this->stores[id]);

    //buyMissiles.registerComponent(this->animations[id]);

    return true;
}

///
/// [Factory] Make a store icon allowing the player to exit the store
///
/// @param endStore The newly created icon on return
/// @param position The position of the icon
/// @return `true` on success, `false` otherwise.
/// @note In addition to the basic 4 components required for rendering, this factory method
///       also registers the velocity and collision components for the given icon.
///
bool EntityManager::makeEndStore(EndStore& endStore, Position& position) {
    if (!this->make(endStore, position, {1.f, 1.f}))
    {
        pserror("[Fatal] Failed to make a buy lives icon.");

        return false;

    }

    // Configure the collision component
    auto id = endStore.getIdentifier();

    this->collisions[id].type = Collision::eType::storeIcon;

    this->collisions[id].gridCoords.clear();

    this->collisions[id].ignore.clear();

    this->collisions[id].prevCellCount = -1;

    // Configure the store component
    this->stores[id].type = Store::sType::end;

    // Setup the velocity component, required by the CollisionSystem
    this->velocities[id].vx = 0;

    this->velocities[id].vy = 0;

    // Assign arbitrarily large mass to prevent icons from being moved by water current
    // TODO: this is pretty janky
    this->physics[id].mass = 999999.f;

    this->physics[id].force = {0.f, 0.f};

    endStore.registerComponent(this->collisions[id]);

    endStore.registerComponent(this->velocities[id]);

    endStore.registerComponent(this->stores[id]);

    //endStore.registerComponent(this->animations[id]);

    return true;
}

///
/// [Factory] Make a character at the given position
///
/// @param character The newly created character on return
/// @param position The position to place the character
/// @param c The ASCII character to render
/// @param font The font used to render the character
/// @param color The font color used to render the character, by default it is black
/// @param size Height of the character
/// @return `true` on success, `false` otherwise.
///
bool EntityManager::makeCharacter(Character& character, Position& position, char c, Character::Font font, vec4 color, uint32_t size)
{
    // Create attributes for the requested character
    character.attribute.character = c;

    character.attribute.font = font;

    character.attribute.pixelSize.width = 0;

    character.attribute.pixelSize.height = size;

    // Guard: Make the sprite
    return this->make(character, position, {1.0f, 1.0f}, color, 0.0f, false, &character.attribute);
}

///
/// [Factory] Make a string label at the given position
///
/// @param stringLabel The newly created string label on return
/// @param position The position to place the first character in the string label
/// @param font The font used to render the string
/// @param color The font color used to render the string, by default it is black
/// @param psize Height of each character in pixels
/// @param format Format of the string
/// @return `true` on success, `false` otherwise.
///
bool EntityManager::makeStringLabel(StringLabel& stringLabel, Position& position, Character::Font font, vec4 color, uint32_t psize, const char* format, ...)
{
    // Construct the final string
    char string[128] = {};

    va_list args;

    va_start(args, format);

    auto numChars = vsnprintf(string, size(string), format, args);

    // Initialize the given string label
    stringLabel.resetIdentifiers();

    // Make the character entity for each character in the string
    for (int index = 0; index < numChars; index++)
    {
        Character ce;

        if (!this->makeCharacter(ce, position, string[index], font, color, psize))
        {
            pserror("[Fatal] Failed to make the character entity for [%c].", string[index]);

            return false;
        }

        this->addCharacter(ce);

        stringLabel.addIdentifiers(ce.getIdentifier());

        position.x += (ce.attribute.advance >> 6);
    }

    va_end(args);

    return true;
}

//
// MARK:- Manage Entities
//

///
/// Setup the background ocean
///
/// @return `true` on success, `false` otherwise.
/// @note The caller should call this method to set up the background ocean for the first time.
/// @warning Subsequent calls are silently ignored as the ocean is already initialized.
///
bool EntityManager::setupOcean()
{
    // Guard: If the ocean is already initialized, don't need to set it up again
    if (this->ocean.getIdentifier() != 0)
    {
        return true;
    }
    
    // Initialize the ocean
    Position position(640, 360);

    if (!this->make(this->ocean, position))
    {
        pserror("Failed to make the background ocean.");
        
        return false;
    }
    
    // Add the ocean
    this->addEntity(this->ocean);

    return true;
}

bool EntityManager::setupOutroUI()
{
    // Add the game over text to the outro UI
    Position position = {512,210};
    makeStringLabel(this->OutroLabel1, position, Character::Font::SFMonoRegular, Color::black, 52,
                    "GAME OVER");

    position = {365,320};
    makeStringLabel(this->OutroLabel2, position, Character::Font::SFMonoRegular, Color::black, 15,
                    "THANKS FOR PLAYING");
    
    position = {200,360};
    makeStringLabel(this->OutroLabel3, position, Character::Font::SFMonoRegular, Color::black, 15,
                    "WOULD YOU LIKE TO TRY AGAIN?");
    
    /// Add the new/load game text
    position = {440,541.5};
    makeStringLabel(this->newGameLabel, position, Character::Font::SFMonoRegular, Color::white, 35,
                    "N E W  G A M E");

    position = {680,541.5};
    makeStringLabel(this->loadGameLabel, position, Character::Font::SFMonoRegular, Color::white, 35,
                    "L O A D  G A M E");
    
    return true;
}

void EntityManager::removeOutroUI()
{
    this->removeEntity(this->outroUI);
    
    for(Entity::Identifier ident: OutroLabel1.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: OutroLabel2.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: OutroLabel3.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: newGameLabel.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: loadGameLabel.getIdentifiers())
    {
        removeCharacter(ident);
    }
}

bool EntityManager::setupIntroUI()
{
    // Initialize the intro UI
    Position position(645, 360);
    
    if (!this->make(this->introUI, position, {0.6,0.6}))
    {
        pserror("Failed to make the intro UI.");
        
        return false;
    }
    
    // Add the intro UI
    this->addEntity(this->introUI);
    
    /// Add the text to the UI
    
    /// Add the game title to the intro UI
    position = {431,210};
    makeStringLabel(this->titleLabel, position, Character::Font::SFMonoRegular, Color::black, 52,
                    GAME_TITLE);
    
    /// Add the welcome message to the intro UI
    position = {494,288};
    makeStringLabel(this->welcomeLabel, position, Character::Font::SFMonoRegular, Color::black, 23,
                    WELCOME_MESSAGE);
    
    /// Add the instructions to the intro UI
    position = {385,320};
    makeStringLabel(this->instructionsLabel1, position, Character::Font::SFMonoRegular, Color::black, 15,
                    INSTRUCTION_DESC1);
    
    position = {510,340};
    makeStringLabel(this->instructionsLabel2, position, Character::Font::SFMonoRegular, Color::black, 15,
                    INSTRUCTION_DESC2);
    
    position = {603,360};
    makeStringLabel(this->instructionsLabel3, position, Character::Font::SFMonoRegular, Color::black, 15,
                    INSTRUCTION_DESC3);

    /// Add the new/load game text
    position = {440,541.5};
    makeStringLabel(this->newGameLabel, position, Character::Font::SFMonoRegular, Color::white, 35,
                    NEW_GAME_TITLE);
    position = {680,541.5};
    makeStringLabel(this->loadGameLabel, position, Character::Font::SFMonoRegular, Color::white, 35,
                    LOAD_GAME_TITLE);

    return true;
}

void EntityManager::removeIntroUI()
{
    this->removeEntity(this->introUI);
    for(Entity::Identifier ident: titleLabel.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: welcomeLabel.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: instructionsLabel1.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: instructionsLabel2.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: instructionsLabel3.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: newGameLabel.getIdentifiers())
    {
        removeCharacter(ident);
    }
    for(Entity::Identifier ident: loadGameLabel.getIdentifiers())
    {
        removeCharacter(ident);
    }
}

///
/// Disable the boat
///
/// @note The caller should invoke this method when a boat is exploded
///
void EntityManager::disableBoat()
{
    // Notify the delegate that this boat has "disappeared"
    for (auto delegate : this->delegates)
    {
        delegate->didRemoveEntity(this->boat);
    }
    
    // Unlike Entity::remove(entity:) we don't deinitialize its component
    // Instead, we will reuse them in EntityManager::resetBoat().
}

///
/// Reset the player boat
///
/// @note This method resets the boat location to the default value
/// @note This method reuses its existing component so no extra overhead is introduced
/// @note The caller should call this method to set up the player boat for the first time.
/// @return `true` on success, `false` otherwise.
///
bool EntityManager::resetBoat()
{
    Entity::Identifier id = this->boat.getIdentifier();
    
    // Guard: Check whether the boat has been initialized
    if (id == 0)
    {
        // Not initialized
        // Make a new one
        // Setup the boat
        Position position(640, 155);

        if (!this->make(this->boat, position, {1.f, 1.f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0.0f, true))
        {
            pserror("[Fatal] Failed to make the player boat.");
            
            return false;
        }

        Entity::Identifier newID = this->boat.getIdentifier();

        // Configure the animation component
        this->animations[newID].setAnimationMode(Animation::Mode::Loopback, 1);

        this->physics[newID].force = {0.f, 0.f};

        this->physics[newID].mass = 0.5f;
        
        // Add the input, collision and velocity component to the boat
        this->boat.registerComponent(this->inputs[newID]);

        this->boat.registerComponent(this->collisions[newID]);
        
        this->boat.registerComponent(this->velocities[newID]);

        this->boat.registerComponent(this->animations[newID]);

        this->boat.registerComponent(this->physics[newID]);

        // Setup the player component (No need to register it)
        this->player.pid = this->boat.getIdentifier();
        
        this->addEntity(this->boat);
        
        return true;
    }
    
    // Already initialized; Reset the position
    this->positions[this->boat.getIdentifier()].x = 640;
    
    this->positions[this->boat.getIdentifier()].y = 180;

    return true;
}

void EntityManager::saveGame(EM_SaveData* data)
{
    data->boatId = this->boat.getIdentifier();
    for(auto fish: fishes) data->fishes.insert(fish.first);
    for(auto bomb: bombs) data->bombs.insert(bomb.first);
    for(auto missile: missiles) data->missiles.insert(missile.first);
    for(auto torp: torpedoes) data->torpedoes.insert(torp.first);
    for(int i = 0; i < Submarine::TOTAL_NUM_SUBMARINE_TYPES; i++)
    {
        for(auto sub: submarines[i]) data->submarines[i].insert(sub.first);
    }
    for(int i = 0; i < MAX_NUM_ON_SCREEN_ENTITIES; i++)
    {
        data->physics[i] = physics[i];
        data->positions[i] = positions[i];
        data->rotations[i] = rotations[i];
        data->velocities[i] = velocities[i];
        data->scores[i] = scores[i];
        data->collisions[i] = collisions[i];
        data->attacks[i] = attacks[i];
    }
}

void EntityManager::signalGameOver(bool over)
{
    this->gameIsOver = over;
    if(!gameIsOver)
    {
        gameIsRunning = true;
    }
}

bool EntityManager::checkIfGameOver()
{
    if(gameIsOver)
    {
        if(gameIsRunning)
        {
            // Signal the game is not currrently running
            gameIsRunning = false;
            
            // Remove all entities from the screen
            this->removeAllEntities();
            
            // Spawn end of game UI
            this->setupOutroUI();
        }
        return true;
    }
    else
    {
        return false;
    }
}

void EntityManager::removeAllEntities()
{
    /// Remove all fish
    while(fishes.size() > 0)
    {
        removeFish(fishes.begin()->first);
    }
    /// Remove all bombs
    while(bombs.size() > 0)
    {
        removeBomb(bombs.begin()->first);
        this->player.incrementNumAvailableBombs();
    }
    /// Remove all torpedos
    while(torpedoes.size() > 0)
    {
        removeTorpedo(torpedoes.begin()->first);
    }
    /// Remove all missiles
    while(missiles.size() > 0)
    {
        removeMissile(missiles.begin()->first);
    }

    /// Remove all boat missiles
    while(boatMissiles.size() > 0) {
        removeBoatMissile(boatMissiles.begin()->first);
    }

    /// Remove all explosions
    while(explosions.size() > 0) {
        removeExplosion(explosions.begin()->first);
    }
    for(int i = 0; i < Submarine::TOTAL_NUM_SUBMARINE_TYPES; i++)
    {
        while(submarines[i].size() > 0)
        {
            removeSubmarine(submarines[i].begin()->first);
        }
    }

    /// Remove all store icons
    while(buyLivesIcons.size() > 0)
    {
        removeBuyLives(buyLivesIcons.begin()->first);
    }
    while(buyMissilesIcons.size() > 0)
    {
        removeBuyMissiles(buyMissilesIcons.begin()->first);
    }
    while(endStoreIcons.size() > 0) {
        removeEndStore(endStoreIcons.begin()->first);
    }
}

void EntityManager::resetGame()
{
    //TODO: update all lables
    this->player.money = 100;
    this->updateMoneyLabel(this->player.getPlayerMoney());
    
    this->player.score = 0;
    this->updateScoreLabel(this->player.getPlayerScore());
    
    this->player.lives = 5;
    this->updateBoatLivesLabel(this->player.getPlayerLives());
    
    this->player.numAvailableMissiles = 1;
    this->updateBoatMissilesLabel(this->player.getNumAvailableMissiles());
    
    this->updateStageLabel(0);
}

bool EntityManager::loadGame(EM_SaveData data)
{
    removeAllEntities();
    
    //Set boat position
    positions[data.boatId] = data.positions[data.boatId];
    
    /*
     * Add saved bombs
     */
    for(Entity::Identifier bombID: data.bombs)
    {
        Bomb bombToCreate;
        
        Position pos = data.positions[bombID];
        
        if (!this->makeBomb(bombToCreate, pos, {0.f, 0.f})) //TODO: make bomb velocities persist
        {
            return 0;
        }
        
        this->addBomb(bombToCreate);
        
        this->player.decrementNumAvailableBombs();
    }
    
    
    /*
     * Add saved submarines
     */
    for(int i = 0; i < Submarine::TOTAL_NUM_SUBMARINE_TYPES; i++)
    {
        for(Entity::Identifier subID: data.submarines[i])
        {
            Submarine subToCreate;
            
            Position pos = data.positions[subID];
            float x_vel = data.velocities[subID].vx;
            uint32_t score = data.scores[subID].score;
            Direction dir = Direction::Right;
            if(x_vel < 0)
            {
                x_vel = x_vel * (-1);
                dir = Direction::Left;
            }
            
            this->makeSubmarine(subToCreate, pos, dir, x_vel, (Submarine::Type)i, score);
            this->addSubmarine(subToCreate, (Submarine::Type)i);
        }
    }
    
    /*
     * Add saved Fishes
     */
    for(Entity::Identifier fishID: data.fishes)
    {
        Fish fishToCreate;
        
        Position pos = data.positions[fishID];
        float x_vel = data.velocities[fishID].vx;
        Direction dir = Direction::Right;
        if(x_vel < 0)
        {
            dir = Direction::Left;
            x_vel = x_vel * (-1);
        }
        
        if (!this->makeFish(fishToCreate, pos, dir, x_vel))
        {
            return 0;
        }
        
        this->addFish(fishToCreate);
    }
    
    /*
     * Add saved torpedos
     */
    for(Entity::Identifier torpID: data.torpedoes)
    {
        Torpedo torpToCreate;
        
        Position pos = data.positions[torpID];
        
        if(!this->makeTorpedo(torpToCreate, pos, {0.f, 0.f})) // TODO: Make torpedo velocity persist
        {
            return 0;
        }
        
        this->addTorpedo(torpToCreate);
    }
    
    
    /*
     * Add saved missiles
     */
    for(Entity::Identifier missileID: data.missiles)
    {
        Missile missileToCreate;
        
        Position pos = data.positions[missileID];
        
        if(!this->makeMissile(missileToCreate, pos))
        {
            return 0;
        }
        
        this->addMissile(missileToCreate);
    }
    
    return true;
}

///
/// Setup the formatted number label
///
/// @param numberLabel The score label, the money label, the lives label, etc
/// @param position The position to place the first character
/// @param size The font size; by default 32px
/// @return `true` on success, `false` otherwise.
/// @warning Subsequent calls are silently ignored if the given formatted number label is already initialized.
///          The caller might want to invoke `updateFormattedNumberLabel()` instead to update the value in the label.
///
bool EntityManager::setupFormattedNumberLabel(FormattedNumberLabel& numberLabel, Position& position, uint32_t size)
{
    // Guard: If the given number label is already initialized, no need to set it up again
    if (numberLabel.getIdentifier() != 0)
    {
        pwarning("API Usage Error: The given formatted number label is already initialized.");
        
        return true;
    }
    
    // Compute the format string
    auto length = numberLabel.getFormatStringLength();
    
    char* format = (char*) alloca(length);
    
    // Format "Prefix%0\(width)llu"
    snprintf(format, length, "%s%%0%llullu", numberLabel.getStringPrefix(), numberLabel.getWidth());
    
    // Make the label
    return this->makeStringLabel(numberLabel, position, Character::Font::SFMonoRegular, Color::black, size, format, numberLabel.getValue());
}

///
/// Setup the boat lives indicator
///
/// @note The caller should invoke this method to set up the boat lives indicator for the first time.
///
bool EntityManager::setupBoatLivesIndicator()
{
    // Guard: If the ocean is already initialized, don't need to set it up again
    if (this->boatLives.getIdentifier() != 0)
    {
        pwarning("API Usage Error: The boat lives indicator is already initialized.");
        
        return true;
    }
    
    // Initialize the ocean
    Position position(24, 24);

    // Scale to 48 x 48
    if (!this->make(this->boatLives, position, {1.f, 1.f}))
    {
        pserror("Failed to make the boat lives indicator.");
        
        return false;
    }
    
    // Add the boat lives indicator
    this->addEntity(this->boatLives);

    // Setup the number of lives
    position.x += 32;
    
    return this->setupFormattedNumberLabel(this->livesLabel, position);
}

///
/// Setup the boat missiles indicator
///
/// @note The caller should invoke this method to set up the boat missiles indicator for the first time.
///
bool EntityManager::setupBoatMissilesIndicator()
{
    // Guard: If the ocean is already initialized, don't need to set it up again
    if (this->boatMissilesIndicator.getIdentifier() != 0)
    {
        pwarning("API Usage Error: The boat lives indicator is already initialized.");

        return true;
    }

    // Initialize the ocean
    Position position(24, 56);

    // Scale to 48 x 48
    if (!this->make(this->boatMissilesIndicator, position, {1.f, 1.f}))
    {
        pserror("Failed to make the boat lives indicator.");

        return false;
    }

    // Add the boat lives indicator
    this->addEntity(this->boatMissilesIndicator);

    // Setup the number of lives
    position.x += 32;

    return this->setupFormattedNumberLabel(this->missilesLabel, position);
}

///
/// Update the numeric value in the given formatted number label
///
/// @param numberLabel An **initialized** label to be updated
/// @param newValue The new numeric value to be rendered
/// @return `true` on success, `false` otherwise.
/// @note This method smartly detects the characters needed to be updated.
///
bool EntityManager::updateFormattedNumberLabel(FormattedNumberLabel& numberLabel, uint64_t newValue)
{
    // Guard: Check whether the new value is actually different
    if (numberLabel.getValue() == newValue)
    {
        return true;
    }
    
    // Get the identifiers of the character sequence
    auto& identifiers = numberLabel.getIdentifiers();
    
    // Compute the format string (excluding the prefix part)
    // i.e. "%0\(width)llu" (e.g. "%08llu")
    char format[8] = {0};
    
    auto width = numberLabel.getWidth();
    
    snprintf(format, size(format), "%%0%llullu", width);
    
    // Construct both the old and the new numeric part of the label
    // The number of digits is fixed and is `width` in the given `numberLabel`
    // Allocate the memory on stack to keep this method less heavy
    // and there won't be a stack overflow issue as width is guaranteed to be <= 16
    auto size = width + 8; // + 8 for aligned access
    
    char* numericOld = (char*) alloca(size);
    
    snprintf(numericOld, size, format, numberLabel.getValue());
    
    char* numericNew = (char*) alloca(size);
    
    snprintf(numericNew, size, format, newValue);
    
    auto base = numberLabel.getStringPrefixLength();
    
    // Here is the algorithm
    // Find the differences
    for (int index = 0; index < width; index++)
    {
        // Guard: Find which character differs
        if (numericNew[index] != numericOld[index])
        {
            // Found a mismatched character
            // Find its corresponding Character entity
            auto id = identifiers[base + index];
            
            auto& ch = this->characters[id];
            
            // Replace its current underlying sprite with the new character sprite
            // using the modified version of character attribute
            // Keep the font, font size unchanged
            ch.attribute.character = numericNew[index];
            
            // Found the current (static) sprite
            auto sprite = &this->ssprites[id];
            
            // Deinitialize the old sprite
            sprite->deinit();
            
            // Guard: Make a new sprite with the new character, passing the new attribute
            if (!SpriteFactory::shared()->make<Character>(sprite, &ch.attribute))
            {
                pserror("Failed to make the sprite for the new character '%c'.", numericNew[index]);
                
                return false;
            }
            
            // All done
            // The new sprite is now effective
            // The entity identifier does not change, keeping the spatial locality
        }
    }
    
    // Update the label value
    numberLabel.setValue(newValue);
    
    return true;
}

//
// MARK:- Adding / Removing Entities
//

///
/// Add a submarine to the system
///
/// @param submarine The **initialized** submarine to be added
/// @param type Specify the type of the given submarine
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addSubmarine(Submarine& submarine, Submarine::Type type)
{
    // No need to check the index because it is bounded by a typed enum
    uint32_t index = Submarine::typeToIndex(type);
    
    this->submarines[index][submarine.getIdentifier()] = submarine;
    
    this->addEntity(submarine);
}

///
/// Add a fish to the system
///
/// @param fish The **initialized** fish to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addFish(Fish& fish)
{
    this->fishes[fish.getIdentifier()] = fish;

    this->addEntity(fish);
}

///
/// Add a bomb to the system
///
/// @param bomb The **initialized** bomb to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addBomb(Bomb& bomb)
{
    this->bombs[bomb.getIdentifier()] = bomb;
    
    this->addEntity(bomb);
}

///
/// Add a torpedo to the system
///
/// @param torpedo The **initialized** torpedo to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addTorpedo(Torpedo& torpedo)
{
    this->torpedoes[torpedo.getIdentifier()] = torpedo;
    
    this->addEntity(torpedo);
}

///
/// Add a missile to the system
///
/// @param missile The **initialized** torpedo to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addMissile(Missile& missile)
{
    this->missiles[missile.getIdentifier()] = missile;
    
    this->addEntity(missile);
}

///
/// Add a boat missile to the system
///
/// @param missile The **initialized** torpedo to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addBoatMissile(BoatMissile &boatMissile){
    this->boatMissiles[boatMissile.getIdentifier()] = boatMissile;

    this->addEntity(boatMissile);
}

///
/// Add a buy lives icon to the system
///
/// @param buyLives The **initialized** BuyLives to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addBuyLives(BuyLives& buyLives) {
    this->buyLivesIcons[buyLives.getIdentifier()] = buyLives;

    this->addEntity(buyLives);
}

///
/// Add a buy missiles icon to the system
///
/// @param buyMissiles The **initialized** BuyMissiles to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addBuyMissiles(BuyMissiles& buyMissiles) {
    this->buyMissilesIcons[buyMissiles.getIdentifier()] = buyMissiles;

    this->addEntity(buyMissiles);
}

///
/// Add an end store icon to the system
///
/// @param endStore The **initialized** EndStore to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addEndStore(EndStore& endStore) {
    this->endStoreIcons[endStore.getIdentifier()] = endStore;

    this->addEntity(endStore);
}

///
/// Add an explosion to the system
///
/// @param explosion The **initialized** explosion to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addExplosion(Explosion& explosion)
{
    this->explosions[explosion.getIdentifier()] = explosion;
    
    this->addEntity(explosion);
}

///
/// Add a puff of smoke to the system
///
/// @param smoke The **initialized** puff of smoke to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addSmoke(Smoke& smoke)
{
    this->smokes[smoke.getIdentifier()] = smoke;

    this->addEntity(smoke);
}

///
/// Add a character to the system
///
/// @param character The **initialized** character to be added
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::addCharacter(Character& character)
{
    this->characters[character.getIdentifier()] = character;

    this->addEntity(character);
}

///
/// Add a string label to the system
///
/// @param stringLabel The **initialized** string label to be added
///
void EntityManager::addStringLabel(StringLabel& stringLabel)
{
    this->stringLabels[stringLabel.getIdentifier()] = stringLabel;

    // No need to notify the delegates
    // as StringLabel is just a wrapper of characters
}

///
/// Remove a submarine from the system
///
/// @param identifier Specify the identifier of the submarine to be removed
/// @warning If the system contains the entity that has the same identifier as the given one,
///          this method will replace the old one with the given one.
///
void EntityManager::removeSubmarine(Entity::Identifier identifier)
{
    for (size_t index = 0; index < size(this->submarines); index++)
    {
        auto iterator = this->submarines[index].find(identifier);
        
        if (iterator != this->submarines[index].end())
        {
            // Found
            this->removeEntity(iterator->second);
            
            this->submarines[index].erase(iterator);
            
            return;
        }
    }
}

///
/// Remove a fish from the system
///
/// @param identifier Specify the identifier of the fish to be removed
///
void EntityManager::removeFish(Entity::Identifier identifier)
{
    auto iterator = this->fishes.find(identifier);

    if (iterator != this->fishes.end())
    {
        // Found
        this->removeEntity(iterator->second);

        this->fishes.erase(iterator);
    }
}

///
/// Remove a submarine from the system
///
/// @param identifier Specify the identifier of the submarine to be removed
///
void EntityManager::removeBomb(Entity::Identifier identifier)
{
    auto iterator = this->bombs.find(identifier);
    
    if (iterator != this->bombs.end())
    {
        // Found
        this->removeEntity(iterator->second);
        
        this->bombs.erase(iterator);
    }
}

///
/// Remove a torpedo from the system
///
/// @param identifier Specify the identifier of the torpedo to be removed
///
void EntityManager::removeTorpedo(Entity::Identifier identifier)
{
    auto iterator = this->torpedoes.find(identifier);
    
    if (iterator != this->torpedoes.end())
    {
        // Found
        this->removeEntity(iterator->second);
        
        this->torpedoes.erase(iterator);
    }
}

///
/// Remove a missile from the system
///
/// @param identifier Specify the identifier of the missile to be removed
///
void EntityManager::removeMissile(Entity::Identifier identifier)
{
    auto iterator = this->missiles.find(identifier);

    if (iterator != this->missiles.end())
    {
        // Found
        this->removeEntity(iterator->second);

        this->missiles.erase(iterator);
    }
}

///
/// Remove a boat missile from the system
///
/// @param identifier Specify the identifier of the boat missile to be removed
///
void EntityManager::removeBoatMissile(Entity::Identifier identifier)
{
    auto iterator = this->boatMissiles.find(identifier);

    if (iterator != this->boatMissiles.end())
    {
        // Found
        this->removeEntity(iterator->second);

        this->boatMissiles.erase(iterator);
    }
}

///
/// Remove a buy lives icon from the system
///
/// @param identifier Specify the identifier of the buy lives icon to be removed
///
void EntityManager::removeBuyLives(Entity::Identifier identifier) {
    auto iterator = this->buyLivesIcons.find(identifier);

    if (iterator != this->buyLivesIcons.end())
    {
        // Found
        this->removeEntity(iterator->second);

        this->buyLivesIcons.erase(iterator);
    }
}

///
/// Remove a buy missiles icon from the system
///
/// @param identifier Specify the identifier of the buy missiles icon to be removed
///
void EntityManager::removeBuyMissiles(Entity::Identifier identifier) {
    auto iterator = this->buyMissilesIcons.find(identifier);

    if (iterator != this->buyMissilesIcons.end())
    {
        // Found
        this->removeEntity(iterator->second);

        this->buyMissilesIcons.erase(iterator);
    }
}

///
/// Remove an end store icon from the system
///
/// @param identifier Specify the identifier of the end store icon to be removed
///
void EntityManager::removeEndStore(Entity::Identifier identifier) {
    auto iterator = this->endStoreIcons.find(identifier);

    if (iterator != this->endStoreIcons.end())
    {
        // Found
        this->removeEntity(iterator->second);

        this->endStoreIcons.erase(iterator);
    }
}

///
/// Remove an explosion from the system
///
/// @param identifier Specify the identifier of the explosion to be removed
///
void EntityManager::removeExplosion(Entity::Identifier identifier)
{
    auto iterator = this->explosions.find(identifier);
    
    if (iterator != this->explosions.end())
    {
        // Found
        this->removeEntity(iterator->second);
        
        this->explosions.erase(iterator);
    }
}

///
/// Remove smoke from the system
///
/// @param identifier Specify the identifier of the smoke to be removed
///
void EntityManager::removeSmoke(Entity::Identifier identifier)
{
    auto iterator = this->smokes.find(identifier);

    if (iterator != this->smokes.end())
    {
        // Found
        this->removeEntity(iterator->second);

        this->smokes.erase(iterator);
    }
}

///
/// Remove a character from the system
///
/// @param identifier Specify the identifier of the character to be removed
///
void EntityManager::removeCharacter(Entity::Identifier identifier)
{
    auto iterator = this->characters.find(identifier);

    if (iterator != this->characters.end())
    {
        // Found
        this->removeEntity(iterator->second);

        this->characters.erase(iterator);
    }
}

///
/// Remove a string label from the system
///
/// @param identifier Specify the identifier of the string label to be removed
///
void EntityManager::removeStringLabel(Entity::Identifier identifier)
{
    auto iterator = this->stringLabels.find(identifier);

    if (iterator != this->stringLabels.end())
    {
        // Found
        auto& identifiers = iterator->second.getIdentifiers();

        std::for_each(identifiers.begin(), identifiers.end(), [this] (auto& id) { this->removeCharacter(id); });

        // No need to notify the delegate
        this->stringLabels.erase(iterator);
    }
}

//
// MARK:- Adding / Removing Entities (Private)
//

///
/// [PRIVATE] Add an entity to the system
///
/// @param an entity added
/// @note This method is called AFTER the upper level APIs have added the given entity.
///       This method will then notify delegates to register this entity if necessary.
///
void EntityManager::addEntity(Entity& entity)
{
    for (auto delegate : this->delegates)
    {
        delegate->didAddEntity(entity);
    }
}

///
/// [PRIVATE] Remove an entity from the system
///
/// @param an entity to be removed
/// @note This method is called BEFORE the upper level APIs remove the given entity.
///       This method will first notify delegates to deregister this entity if necessary.
///
void EntityManager::removeEntity(Entity& entity)
{
    //pinfo("Removing entity # %d.", entity.getIdentifier());

    for (auto delegate : this->delegates)
    {
        delegate->didRemoveEntity(entity);
    }
    
    // Deinitialize the corresponding components to avoid memory leak
    // Search from the least significant bit
    BitOptions bitmap = entity.getComponents();
    
    while (!bitmap.isEmpty())
    {
        // Find the current least significant bit
        uint32_t index = findlsb(bitmap.flatten());
        
        // A reference to the corresponding component
        SWComponent* component = nullptr;
        
        // TDOO: Improve the logic here
        if (index == 0)
        {
            // Indirect Component
            SWComponent** components = this->indirectComponents(Components::getTypeIndex(index));
            
            component = components[entity.getIdentifier()];
        }
        else
        {
            // Direct Component
            SWComponent* components = this->components(Components::getTypeIndex(index));
            
            component = reinterpret_cast<SWComponent*>(reinterpret_cast<uint8_t*>(components) + Components::getTypeSize(index) * entity.getIdentifier());
        }
        
        // Deinitialize the component
        component->deinit();

        // Clear this bit and continue
        bitmap.mutativeRemove(BitOptionCreate(index));
    }
    
    // Release the entity identifier
    this->freelist.free(entity.getIdentifier());

    // Reset distortion effect
    this->distortions[entity.getIdentifier()].distort = false;
}

//
// MARK:- Manage Delegates
//

///
/// Register the given delegate that listens on events of entity addition and removal
///
/// @param delegate The listener to be added
///
void EntityManager::registerDelegate(EntityManagerDelegate* delegate)
{
    this->delegates.push_back(delegate);
}

//
// MARK:- Entity Delegate IMP
//

///
/// [IMP] Called on a component has been added to the entity
///
/// @param entity The entity to which the component is added
/// @param component The component added
///
void EntityManager::didAddComponent(Entity& entity, SWComponent& component)
{
    // Save the given component at the appropriate spot
    SWComponent* components = this->components(typeid(component));
    
    components[entity.getIdentifier()] = component;
    
    // Notify all systems that the given entity has been updated
    for (auto delegate : this->delegates)
    {
        delegate->didUpdateEntity(entity);
    }
}

///
/// [IMP] Called on a component has been removed from the entity
///
/// @param entity The entity from which the component is removed
/// @param componentBitMapIndex The bit map index of the component removed
///
void EntityManager::didRemoveComponent(Entity& entity, uint32_t componentBitMapIndex)
{
    // Deinitialize the component
    SWComponent* components = this->components(Components::getTypeIndex(componentBitMapIndex));
    
    components[entity.getIdentifier()].deinit();
    
    // Notify all systems that the given entity has been updated
    for (auto delegate : this->delegates)
    {
        delegate->didUpdateEntity(entity);
    }
}

/// Create a free list with the given lower and upper bound
EntityManager::FreeList::FreeList() : freeids(MAX_NUM_ON_SCREEN_ENTITIES - 1)
{
    /// Identifier 0 is reserved and is never free
    /// because an "uninitialized" entity has an identifier of 0.
    std::generate(this->freeids.begin(), this->freeids.end(), [n = 1] () mutable { return n++; });
}

/// Allocate an identifier
int EntityManager::FreeList::alloc()
{
    // Guard: Free list must be non-empty
    if (this->freeids.empty())
    {
        return 0;
    }
    
    int first = this->freeids.front();
    
    this->freeids.pop_front();

    return first;
}

/// Release an identifier
void EntityManager::FreeList::free(int id)
{
    this->freeids.push_back(id);
}
