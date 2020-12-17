//
//  EntityManager.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-09-27.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef EntityManager_hpp
#define EntityManager_hpp

#include "Foundations/Foundations.hpp"
#include "Entities/Entities.hpp"
#include "EntityManagerDelegate.hpp"
#include "Components/Components.hpp"
#include "ComponentsDataProvider.hpp"
#include "SpriteFactory.hpp"
#include <list>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <typeindex>
#include <float.h>

/// Manages all game entities
class EntityManager: public EntityDelegate, public ComponentsDataProvider
{
public:
    /// Default constructor
    /// @note Upon completion, a default boat and the background ocean are added automatically
    EntityManager();

    /// Default destructor
    ~EntityManager();

    //
    // MARK:- Entity Factory
    //
    
    ///
    /// [Factory] Make an entity at the given position
    ///
    /// @param entity The newly created entity of type T on return
    /// @param position The initial position of the entity
    /// @param scale Apply the scale on the entity, by default it is {1.0f, 1.0f} (i.e not scale).
    /// @param color Apply the color transformations on the entity,
    ///              by default it is {R = 1.0f, G = 1.0f, B = 1.0f, A = 1.0f} (i.e. no color transformation)
    /// @param radians Apply the rotation radians on the entity, by default it is 0.0f (i.e. no rotation)
    /// @param isAnimated Pass `true` if the entity is animated, by default it is `false` (i.e. no animation)
    /// @param info Additional information needed to make the sprite, by default it is `nullptr` (i.e. no additional info)
    /// @return `true` on success, `false` otherwise.
    /// @note This is a generic and basic version of making an entity;
    ///       The caller might want use a more specific version to make certain types of entity.
    ///       In any case, this method will be called eventually to initialize the entity.
    /// @warning This method does not add the initialized entity to the entity manager;
    ///          It only allocates an identifier for the given entity and initializes the Sprite,
    ///          Position, Rotation and Physics components in the underlying component array.
    ///          (i.e. Initialize all basic required components for rendering.)
    ///          In order to add the entity to the manager, see Entity::addXXX() methods.
    ///          EntityManager only provides two convenient methods to set up the boat and ocean.
    ///          All other entities will follow the make-and-add procedure.
    ///
    template <typename T> // Restricted: T must be a subclass of Entity.
    std::enable_if_t<std::is_base_of<Entity, T>::value, bool>
    make(T& entity, Position& position, vec2 scale = {1.0f, 1.0f}, vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}, float radians = 0.0f, bool isAnimated = false, void* info = nullptr)
    {
        // Allocate a free identifier
        Entity::Identifier identifier = this->freelist.alloc();
        
        if (identifier == 0)
        {
            pserror("[FATAL] No free identifier available.");
            
            return false;
        }
        
        // Make the sprite for the given entity type
        Sprite* sprite = isAnimated ? (Sprite*) &this->asprites[identifier] : (Sprite*) &this->ssprites[identifier];

        // Record the indirection
        this->sprites[identifier] = sprite;
        
        //pinfo("Making sprite for entity #%d.", identifier);

        if (!SpriteFactory::shared()->make<T>(sprite, info))
        {
            pserror("Failed to make the sprite for the entity type %s.", typeid(T).name());
            
            return false;
        }
        
        // Save the entity color, position, scale and rotation radians
        this->colors[identifier].setColor(color);
        
        this->positions[identifier] = position;
        
        this->physics[identifier].scale = scale;
        
        this->rotations[identifier].radians = radians;

        // We have three components right now
        BitOptions components = Components::makeBitMap<Sprite, Color, Position, Rotation, Physics>();

        // Initialize the entity by assigning the identifier, components and delegate
        entity.init(identifier, components, this);
        
        // All done
        return true;
    }
    
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
    bool makeBomb(Bomb& bomb, Position& position, vec2 initVel);
    
    ///
    /// [Factory] Make an explosion at the given position
    ///
    /// @param explosion The newly created explosion on return
    /// @param position The initial position of the explosion
    /// @return `true` on success, `false` otherwise.
    ///
    bool makeExplosion(Explosion& explosion, Position& position);
    
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
    bool makeSubmarine(Submarine& submarine, Position& position, Direction direction, float x_velocity, Submarine::Type type, uint32_t score, float radarRadius = FLT_MAX);

    ///
    /// [Factory] Make a fish at the given position
    ///
    /// @param fish The newly created fish on return
    /// @param position The initial position of the fish
    /// @param direction The facing direction of the fish; either moving `Left` or `Right`
    /// @param x_velocity The **absolute** velocity along the x-axis
    /// @return `true` on success, `false` otherwise.
    ///
    bool makeFish(Fish &fish, Position &position, Direction direction, float x_velocity);

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
    bool makeTorpedo(Torpedo& torpedo, Position& position, vec2 initVel);

    ///
    /// [Factory] Make a puff of smoke at given location
    ///
    /// @param smoke The newly created smoke on return
    /// @return `true` on success, `false` otherwise.
    ///
    bool makeSmoke(Smoke& smoke);

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
    bool makeMissile(Missile& missile, Position& position);

    ///
    /// [Factory] Make a boat missile at the given position
    ///
    /// @param missile The newly created missile on return
    /// @param position The initial position of the missile
    /// @param target The target of the missile
    /// @return `true` on success, `false` otherwise.
    /// @note In addition to the basic 4 components required for rendering, this factory method
    ///       also registers the velocity and collision components for the given `missile`.
    ///       The default missile velocity is stored in `EntityManager::DEF_MISSILE_VELOCITY`.
    ///
    bool makeBoatMissile(BoatMissile& boatMissile, Position& position, Position& target);

    ///
    /// [Factory] Make a store icon allowing the player to buy lives
    ///
    /// @param buyLives The newly created icon on return
    /// @param position The position of the icon
    /// @return `true` on success, `false` otherwise.
    /// @note In addition to the basic 4 components required for rendering, this factory method
    ///       also registers the velocity and collision components for the given icon.
    ///
    bool makeBuyLives(BuyLives& buyLives, Position& position);

    ///
    /// [Factory] Make a store icon allowing the player to buy missiles
    ///
    /// @param buyMissiles The newly created icon on return
    /// @param position The position of the icon
    /// @return `true` on success, `false` otherwise.
    /// @note In addition to the basic 4 components required for rendering, this factory method
    ///       also registers the velocity and collision components for the given icon.
    ///
    bool makeBuyMissiles(BuyMissiles& buyMissiles, Position& position);

    ///
    /// [Factory] Make a store icon allowing the player to exit the store
    ///
    /// @param endStore The newly created icon on return
    /// @param position The position of the icon
    /// @return `true` on success, `false` otherwise.
    /// @note In addition to the basic 4 components required for rendering, this factory method
    ///       also registers the velocity and collision components for the given icon.
    ///
    bool makeEndStore(EndStore& endStore, Position& position);

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
    bool makeCharacter(Character& character, Position& position, char c, Character::Font font = Character::Font::SFMonoRegular, vec4 color = Color::black, uint32_t size = 48);

    ///
    /// [Factory] Make a string label at the given position
    ///
    /// @param stringLabel The newly created string label on return
    /// @param position The position to place the first character in the string label
    /// @param font The font used to render the string
    /// @param color The font color used to render the string
    /// @param psize Height of each character
    /// @param format Format of the string
    /// @return `true` on success, `false` otherwise.
    ///
    bool makeStringLabel(StringLabel& stringLabel, Position& position, Character::Font font, vec4 color, uint32_t psize, const char* format, ...);
    
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
    bool setupOcean();
    
    bool setupOutroUI();
    
    void removeOutroUI();
    
    bool setupIntroUI();
    
    void removeIntroUI();
    
    ///
    /// Disable the boat
    ///
    /// @note The caller should invoke this method when a boat is exploded
    ///
    void disableBoat();
    
    ///
    /// Reset the player boat
    ///
    /// @note This method resets the boat location to the default value.
    /// @note This method reuses its existing component so no extra overhead is introduced.
    /// @note The caller should call this method to set up the player boat for the first time.
    /// @return `true` on success, `false` otherwise.
    ///
    bool resetBoat();
    
    /// Assume the maximum number of entities on screen is MAX_NUM_ON_SCREEN_ENTITIES
    static constexpr uint32_t MAX_NUM_ON_SCREEN_ENTITIES = 1024;
    
    struct EM_SaveData {
        /// The boat ID
        Entity::Identifier boatId;
        
        /// Submarines
        std::unordered_set<Entity::Identifier> submarines[Submarine::TOTAL_NUM_SUBMARINE_TYPES];

        /// Fishes
        std::unordered_set<Entity::Identifier> fishes;

        /// Bombs
        std::unordered_set<Entity::Identifier> bombs;
        
        /// Torpedoes
        std::unordered_set<Entity::Identifier> torpedoes;
        
        /// Missiles
        std::unordered_set<Entity::Identifier> missiles;

        /// Boat Missiles
        // TODO: Set this up
        //std::unordered_set<Entity::Identifier> boatMissiles;
        
        /// Component Array - Position
        Position positions[MAX_NUM_ON_SCREEN_ENTITIES];
        
        /// Component Array - Velocity
        Velocity velocities[MAX_NUM_ON_SCREEN_ENTITIES];
        
        /// Component Array - Rotations
        Rotation rotations[MAX_NUM_ON_SCREEN_ENTITIES];
        
        /// Component Array - Physics
        Physics physics[MAX_NUM_ON_SCREEN_ENTITIES];
        
        /// Component Array - Scores
        Score scores[MAX_NUM_ON_SCREEN_ENTITIES];
        
        /// Component Array - Collision
        Collision collisions[MAX_NUM_ON_SCREEN_ENTITIES];
        
        /// Component Array - Attack
        Attack attacks[MAX_NUM_ON_SCREEN_ENTITIES];
    };
    void saveGame(EM_SaveData* data);
    bool loadGame(EM_SaveData data);
    void removeAllEntities();
    void signalGameOver(bool over);
    bool checkIfGameOver();
    
    bool gameIsOver = true;
    bool gameIsRunning = false;
    
    ///
    /// Setup the formatted number label
    ///
    /// @param numberLabel The score label, the money label, the lives label, etc
    /// @param position The position to place the first character
    /// @param size The font size; by default 32px.
    /// @return `true` on success, `false` otherwise.
    /// @note The caller should call this method to set up the global player status label for the first time.
    /// @warning Subsequent calls are silently ignored if the given formatted number label is already initialized.
    ///          The caller might want to invoke `updateFormattedNumberLabel()` instead to update the value in the label.
    ///
    bool setupFormattedNumberLabel(FormattedNumberLabel& numberLabel, Position& position, uint32_t size = 32);
    
    ///
    /// Setup the boat lives indicator
    ///
    /// @note The caller should invoke this method to set up the boat lives indicator for the first time.
    ///
    bool setupBoatLivesIndicator();

    ///
    /// Setup the boat missiles indicator
    ///
    /// @note The caller should invoke this method to set up the boat missiles indicator for the first time.
    ///
    bool setupBoatMissilesIndicator();
    
    /// [Convenient] Setup the score label
    inline bool setupScoreLabel()
    {
        // Score: 00000000
        Position position(1280 - 16 * 17, 16); // TODO: AVOID HARDCODED SCREEN SIZE VALUE
        
        return this->setupFormattedNumberLabel(this->scoreLabel, position);
    }
    
    /// [Convenient] Setup the money label
    inline bool setupMoneyLabel()
    {
        // Money: 00000000.
        Position position(1280 - 16 * 17, 16 + 32); // TODO: AVOID HARDCODED SCREEN SIZE VALUE
        
        return this->setupFormattedNumberLabel(this->moneyLabel, position);
    }
    
    /// [Convenient] Setup the stage label
    inline bool setupStageLabel()
    {
        // STAGE 0
        Position position(560, 16); // TODO: AVOID HARDCODED SCREEN SIZE VALUE
        
        return this->setupFormattedNumberLabel(this->stageLabel, position);
    }
    
    ///
    /// Update the numeric value in the given formatted number label
    ///
    /// @param numberLabel An **initialized** label to be updated
    /// @param newValue The new numeric value to be rendered
    /// @return `true` on success, `false` otherwise.
    /// @note This method smartly detects the characters needed to be updated.
    ///
    bool updateFormattedNumberLabel(FormattedNumberLabel& numberLabel, uint64_t newValue);
    
    ///
    /// [Convenient] Update the score label
    ///
    /// @param newScore The new score value
    /// @return `true` on success, `false` otherwise.
    /// @note This method is a convenient wrapper of `updateFormattedNumberLabel()`.
    ///
    inline bool updateScoreLabel(uint64_t newScore)
    {
        return this->updateFormattedNumberLabel(this->scoreLabel, newScore);
    }
    
    ///
    /// [Convenient] Update the money label
    ///
    /// @param newValue The new money value
    /// @return `true` on success, `false` otherwise.
    /// @note This method is a convenient wrapper of `updateFormattedNumberLabel()`.
    ///
    inline bool updateMoneyLabel(uint64_t newValue)
    {
        return this->updateFormattedNumberLabel(this->moneyLabel, newValue);
    }
    
    ///
    /// [Convenient] Update the boat lives label
    ///
    /// @param lives The new number of lives
    /// @return `true` on success, `false` otherwise.
    /// @note This method is a convenient wrapper of `updateFormattedNumberLabel()`.
    ///
    inline bool updateBoatLivesLabel(uint64_t lives)
    {
        return this->updateFormattedNumberLabel(this->livesLabel, lives);
    }

    ///
    /// [Convenient] Update the boat missiles label
    ///
    /// @param missiles The new number of missiles
    /// @return `true` on success, `false` otherwise.
    /// @note This method is a convenient wrapper of `updateFormattedNumberLabel()`.
    ///
    inline bool updateBoatMissilesLabel(uint64_t numMissiles)
    {
        return this->updateFormattedNumberLabel(this->missilesLabel, numMissiles);
    }
    
    ///
    /// [Convenient] Update the stage label
    ///
    /// @param stage The new stage number
    /// @return `true` on success, `false` otherwise.
    /// @note This method is a convenient wrapper of `updateFormattedNumberLabel()`.
    ///
    inline bool updateStageLabel(uint64_t stage)
    {
        return this->updateFormattedNumberLabel(this->stageLabel, stage);
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
    void addSubmarine(Submarine& submarine, Submarine::Type type);

    ///
    /// Add a fish to the system
    ///
    /// @param fish The **initialized** fish to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addFish(Fish& fish);
    
    ///
    /// Add a bomb to the system
    ///
    /// @param bomb The **initialized** bomb to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addBomb(Bomb& bomb);
    
    ///
    /// Add an explosion to the system
    ///
    /// @param explosion The **initialized** explosion to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addExplosion(Explosion& explosion);
    
    ///
    /// Add a torpedo to the system
    ///
    /// @param torpedo The **initialized** torpedo to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addTorpedo(Torpedo& torpedo);
    
    ///
    /// Add a missile to the system
    ///
    /// @param missile The **initialized** torpedo to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addMissile(Missile& missile);

    ///
    /// Add a boat missile to the system
    ///
    /// @param boatMissile The **initialized** torpedo to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addBoatMissile(BoatMissile& boatMissile);

    ///
    /// Add a buy lives icon to the system
    ///
    /// @param buyLives The **initialized** BuyLives to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addBuyLives(BuyLives& buyLives);

    ///
    /// Add a buy missiles icon to the system
    ///
    /// @param buyMissiles The **initialized** BuyMissiles to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addBuyMissiles(BuyMissiles& buyMissiles);

    ///
    /// Add an end store icon to the system
    ///
    /// @param endStore The **initialized** EndStore to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addEndStore(EndStore& endStore);

    ///
    /// Add a puff of smoke to the system
    ///
    /// @param smoke The **initialized** puff of smoke to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addSmoke(Smoke& smoke);
    
    ///
    /// Add a character to the system
    ///
    /// @param character The **initialized** character to be added
    /// @warning If the system contains the entity that has the same identifier as the given one,
    ///          this method will replace the old one with the given one.
    ///
    void addCharacter(Character& character);

    ///
    /// Add a string label to the system
    ///
    /// @param stringLabel The **initialized** string label to be added
    ///
    void addStringLabel(StringLabel& stringLabel);

    ///
    /// Remove a submarine from the system
    ///
    /// @param identifier Specify the identifier of the submarine to be removed
    ///
    void removeSubmarine(Entity::Identifier identifier);

    ///
    /// Remove a fish from the system
    ///
    /// @param identifier Specify the identifier of the fish to be removed
    ///
    void removeFish(Entity::Identifier identifier);
    
    ///
    /// Remove a submarine from the system
    ///
    /// @param identifier Specify the identifier of the submarine to be removed
    ///
    void removeBomb(Entity::Identifier identifier);

    ///
    /// Remove a torpedo from the system
    ///
    /// @param identifier Specify the identifier of the torpedo to be removed
    ///
    void removeTorpedo(Entity::Identifier identifier);

    ///
    /// Remove a missile from the system
    ///
    /// @param identifier Specify the identifier of the missile to be removed
    ///
    void removeMissile(Entity::Identifier identifier);

    ///
    /// Remove a boat missile from the system
    ///
    /// @param identifier Specify the identifier of the boat missile to be removed
    ///
    void removeBoatMissile(Entity::Identifier identifier);

    ///
    /// Remove a buy lives icon from the system
    ///
    /// @param identifier Specify the identifier of the buy lives icon to be removed
    ///
    void removeBuyLives(Entity::Identifier identifier);

    ///
    /// Remove a buy missiles icon from the system
    ///
    /// @param identifier Specify the identifier of the buy missiles icon to be removed
    ///
    void removeBuyMissiles(Entity::Identifier identifier);

    ///
    /// Remove an end store icon from the system
    ///
    /// @param identifier Specify the identifier of the end store icon to be removed
    ///
    void removeEndStore(Entity::Identifier identifier);
    
    ///
    /// Remove an explosion from the system
    ///
    /// @param identifier Specify the identifier of the explosion to be removed
    ///
    void removeExplosion(Entity::Identifier identifier);

    ///
    /// Remove smoke from the system
    ///
    /// @param identifier Specify the identifier of the smoke to be removed
    ///
    void removeSmoke(Entity::Identifier identifier);

    ///
    /// Remove a character from the system
    ///
    /// @param identifier Specify the identifier of the character to be removed
    ///
    void removeCharacter(Entity::Identifier identifier);

    ///
    /// Remove a string label from the system
    ///
    /// @param identifier Specify the identifier of the string label to be removed
    ///
    void removeStringLabel(Entity::Identifier identifier);

    //
    // MARK:- Manage Delegates
    //
    
    ///
    /// Register the given delegate that listens on events of entity addition and removal
    ///
    /// @param delegate The listener to be added
    ///
    void registerDelegate(EntityManagerDelegate* delegate);
    
    ///
    /// [Base Case] Register multiple delegates that listen on events of entity addition and removal
    ///
    void registerDelegates() {}
    
    ///
    /// Register multiple delegates that listen on events of entity addition and removal
    ///
    /// @param delegate A listener to be added
    /// @param others Other listeners to be added
    ///
    template <class... D>
    void registerDelegates(EntityManagerDelegate* delegate, D*... others)
    {
        this->registerDelegate(delegate);
        
        this->registerDelegates(others...);
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
    void didAddComponent(Entity& entity, SWComponent& component) override;
    
    ///
    /// [IMP] Called on a component has been removed from the entity
    ///
    /// @param entity The entity from which the component is removed
    /// @param componentBitMapIndex The bit map index of the component removed
    ///
    void didRemoveComponent(Entity& entity, uint32_t componentBitMapIndex) override;

    /// The price of a life at the store
    // TODO: find better way of storing this, maybe in store component itself
    int lifePrice = 50;

    /// The price of a missile at the store
    int missilePrice = 50;
    
    /// Entities used in the tutorial
    Submarine tutorialSub;
    Fish tutorialFish;
    Bomb tutorialBomb;
    BoatMissile tutorialBM;
    Missile tutorialMissile;
    Torpedo tutorialTorpedo;
    
    void resetGame();

private:
    /// The default velocity of a bomb
    static constexpr vec2 DEF_BOMB_VELOCITY = { 0, 5 };

    /// The gravity value for objects that should fall
    float gravity = 200.f;

    /// The default velocity of a torpedo
    static constexpr vec2 DEF_TORPEDO_FORCE = {0, -100.f };

    /// The default force of a submarine engine
    // TODO: Diversify for other sub types?
    static constexpr vec2 DEF_SUB_FORCE = {50.f, 0.f}; // Was 50

    /// The default force of a fish's swimming
    static constexpr vec2 DEF_FISH_FORCE = {25.f, 0.f}; // Was 25

    /// The default score award of a fish
    static constexpr uint32_t DEF_FISH_SCORE = 1;

    /// Represents a free list that keeps track of free identifiers
    struct FreeList
    {
        /// The internal storage that stores all free identifiers
        std::list<int> freeids;
        
        /// Create a free list with the given lower and upper bound
        FreeList();
        
        ///
        /// Allocate an identifier
        ///
        /// @return A non-zero identifier on success, 0 if no free identifier available.
        /// @complexity O(1)
        ///
        int alloc();
        
        ///
        /// Release an identifier
        ///
        /// @param id The identifier to be released
        /// @complexity O(1)
        ///
        void free(int id);
    };

    /// A free list that keeps track of free identifiers
    FreeList freelist;

    /// Delegates that listen on events occurred in this manager
    /// Entity manager does not manage the memory of this delegate
    std::vector<EntityManagerDelegate*> delegates; // used to notify all systems
    
    // MARK:- Manage Entities
    
    /// The player boat
    Boat boat;
    
    /// The player boat lives indicator
    BoatLives boatLives;

    /// The player boat missiles indicator
    BoatMissiles boatMissilesIndicator;
    
    /// The screen displayed when the game starts up
    IntroUI introUI;
    
    /// The screen displayed when the game starts up
    OutroUI outroUI;

    /// Submarines
    std::unordered_map<Entity::Identifier, Submarine> submarines[Submarine::TOTAL_NUM_SUBMARINE_TYPES];

    /// Fishes
    std::unordered_map<Entity::Identifier, Fish> fishes;

    /// Bombs
    std::unordered_map<Entity::Identifier, Bomb> bombs;
    
    /// Torpedoes
    std::unordered_map<Entity::Identifier, Torpedo> torpedoes;
    
    /// Missiles
    std::unordered_map<Entity::Identifier, Missile> missiles;

    /// Boat Missiles
    std::unordered_map<Entity::Identifier, BoatMissile> boatMissiles;

    /// Buy lives icons
    std::unordered_map<Entity::Identifier, BuyLives> buyLivesIcons;

    /// Buy missiles icons
    std::unordered_map<Entity::Identifier, BuyMissiles> buyMissilesIcons;

    /// End store icons
    std::unordered_map<Entity::Identifier, EndStore> endStoreIcons;

    /// Explosions
    std::unordered_map<Entity::Identifier, Explosion> explosions;

    /// Smoke
    std::unordered_map<Entity::Identifier, Smoke> smokes;
    
    /// Characters
    std::unordered_map<Entity::Identifier, Character> characters;

    /// String Labels
    std::unordered_map<Entity::Identifier, StringLabel> stringLabels;

    /// The background ocean
    Ocean ocean;
    
    /// The score label
    FormattedNumberLabel scoreLabel;
    
    /// The money label
    FormattedNumberLabel moneyLabel;
    
    /// The number of lives label
    FormattedNumberLabel livesLabel;

    /// The number of missiles label
    FormattedNumberLabel missilesLabel;
    
    /// The stage label
    FormattedNumberLabel stageLabel;
    
    /// The intro title label
    StringLabel titleLabel;
    const char* GAME_TITLE = "SUBMARINE WARS";

    /// The welcome label
    StringLabel welcomeLabel;
    const char* WELCOME_MESSAGE = "WELCOME ABOARD CAPTAIN";
    
    /// The instructions labels
    StringLabel instructionsLabel1;
    const char* INSTRUCTION_DESC1 = "THERE ARE ENEMY SHIPS INCOMING WE MUST PREPARE FOR BATTLE!";
    StringLabel instructionsLabel2;
    const char* INSTRUCTION_DESC2 = "YOUR CONTROLS ARE LISTED BELOW";
    StringLabel instructionsLabel3;
    const char* INSTRUCTION_DESC3 = "GOOD LUCK";
    
    /// The New Game label
    StringLabel newGameLabel;
    Entity newGameButton;
    const char* NEW_GAME_TITLE = "NEW GAME";
    
    /// The Load Game label
    StringLabel loadGameLabel;
    const char* LOAD_GAME_TITLE = "LOAD GAME";
    
    StringLabel OutroLabel1;
    StringLabel OutroLabel2;
    StringLabel OutroLabel3;
    StringLabel OutroLabel4;

    ///
    /// Helper function for make submarine that re-casts sub to proper type
    ///
    /// @param Reference to SubmarineI
    /// @return A pointer to sub
    ///
    SubmarineI* getSub1Ptr(Submarine& sub);

    ///
    /// Helper function for make submarine that re-casts sub to proper type
    ///
    /// @param Reference to SubmarineI
    /// @return A pointer to sub
    ///
    SubmarineII* getSub2Ptr(Submarine& sub);

    ///
    /// Helper function for make submarine that re-casts sub to proper type
    ///
    /// @param Reference to SubmarineI
    /// @return A pointer to sub
    ///
    SubmarineIII* getSub3Ptr(Submarine& sub);

    /// The money label - a reference to the string
    
    ///
    /// [PRIVATE] Add an entity to the system
    ///
    /// @param entity entity added
    /// @note This method is called AFTER the upper level APIs have added the given entity.
    ///       This method will then notify delegates to register this entity if necessary.
    ///
    void addEntity(Entity& entity);
    
    ///
    /// [PRIVATE] Remove an entity from the system
    ///
    /// @param entity entity to be removed
    /// @note This method is called BEFORE the upper level APIs remove the given entity.
    ///       This method will first notify delegates to deregister this entity if necessary.
    ///
    void removeEntity(Entity& entity);

    // MARK:- Manage Component Arrays

    /// A Component Array Registry type that maps the component type id to its corresponding component array
    typedef std::unordered_map<std::type_index, SWComponent*> CARegistry;

    /// An Indirect Component Array Registry type that maps the component type id to its corresponding indirect component array
    typedef std::unordered_map<std::type_index, SWComponent**> ICARegistry;

    /// A map that maps the Component type id to its component array
    /// allowing fast and convenient accesses to different component array
    CARegistry registry;

    /// A map that maps the Component type id to its indirect component array
    /// allowing fast and convenient accesses to component arrays of polymorphism-enabled component type
    ICARegistry iregistry;

    /// Component Array - Sprite (Indirection)
    Sprite* sprites[MAX_NUM_ON_SCREEN_ENTITIES];
    
    /// Component Array - Static Sprite
    StaticSprite ssprites[MAX_NUM_ON_SCREEN_ENTITIES];

    /// Component Array - Animated Sprite
    AnimatedSprite asprites[MAX_NUM_ON_SCREEN_ENTITIES];

    /// Component Array - Color
    Color colors[MAX_NUM_ON_SCREEN_ENTITIES];
    
    /// Component Array - Position
    Position positions[MAX_NUM_ON_SCREEN_ENTITIES];
    
    /// Component Array - Velocity
    Velocity velocities[MAX_NUM_ON_SCREEN_ENTITIES];
    
    /// Component Array - Rotations
    Rotation rotations[MAX_NUM_ON_SCREEN_ENTITIES];
    
    /// Component Array - Physics
    Physics physics[MAX_NUM_ON_SCREEN_ENTITIES];

    /// Component Array - Collision
    Collision collisions[MAX_NUM_ON_SCREEN_ENTITIES];
    
    /// Component Array - Input
    Input inputs[MAX_NUM_ON_SCREEN_ENTITIES];

    /// Component Array - Attack
    Attack attacks[MAX_NUM_ON_SCREEN_ENTITIES];
    
    /// Component Array - Player (Only single player is supported)
    Player player;

    /// Component Array - Score
    Score scores[MAX_NUM_ON_SCREEN_ENTITIES];

    /// Component Array - Pathing
    Pathing pathings[MAX_NUM_ON_SCREEN_ENTITIES];

    /// Component Array - Animation
    Animation animations[MAX_NUM_ON_SCREEN_ENTITIES];

    /// Component Array - Store
    Store stores[MAX_NUM_ON_SCREEN_ENTITIES];

    /// Component Array - Distortion
    Distortion distortions[MAX_NUM_ON_SCREEN_ENTITIES];

    //
    // MARK:- Components Data Provider IMP
    //
    //

    ///
    /// Retrieve the components array
    ///
    /// @param id The type id of the component type
    /// @return A `Component` pointer to the component array.
    ///
    inline SWComponent* components(std::type_index id) override
    {
        passert(this->registry.find(id) != this->registry.end(), "[Fatal] Error: Found unregistered components array.");

        return this->registry[id];
    }

    ///
    /// Retrieve the indirect components array
    ///
    /// @param id The type id of the component type
    /// @return A `Component*` pointer to the component array.
    ///
    inline SWComponent** indirectComponents(std::type_index id) override
    {
        passert(this->iregistry.find(id) != this->iregistry.end(), "[Fatal] Error: Found unregistered indirect components array.");

        return this->iregistry[id];
    }
};

#endif /* EntityManager_hpp */
