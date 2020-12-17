//
//  StageController.cpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-10.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#include "StageController.hpp"
#include "Sounds/SoundPlayer.hpp"

/// A stage cache to allow lazy initialization
Stage StageController::stages[TOTAL_NUM_STAGES + 1];

///
/// [Constructor] Create a stage controller
///
/// @param entityManager A reference to the entity manager to make entities for each stage
///
StageController::StageController(EntityManager* entityManager)
{
    this->entityManager = entityManager;
    
    this->player = entityManager->componentsForType<Player>();
    
    this->player->setDelegate(this);
    
    // Initialize the random number generators for facing direction and y-coordinate
    this->drandom.init(0, 1);
    this->fishRandom.init(200.f, 700.f);

    sinceSpawn = 1000;
    betweenSpawns = 1000;
    sinceSmokeSpawn = 500;
    sinceSmokeSpawn = 500;
    subsDead = 0;
    loading = false;
    fishCount = 0;
    storeInit = false;
    storeEnded = false;
}

void StageController::signalGameActive(bool active)
{
    gameIsActive = active;
    this->entityManager->signalGameOver(!active);
}

bool StageController::isGameActive()
{
    return gameIsActive;
}

///
/// Update the stage at each game tick
///
/// @param elapsed_ms The elapsed time since the last tick
///
void StageController::update(float elapsed_ms)
{
    if (stageType == 0) {
        updateNormalStage(elapsed_ms);
    } else if (stageType == 1) {
        updateStoreStage(elapsed_ms);
    }
}

///
/// Update helper for a normal stage
///
/// @param elapsed_ms The elapsed time since last tick
///
void StageController::updateNormalStage(float elapsed_ms) {
    // Spawn one submarine of each type that must be spawned
    uint32_t typeIToSpawn = this->resSubCounts[Submarine::Type::I];
    uint32_t typeIIToSpawn = this->resSubCounts[Submarine::Type::II];
    uint32_t typeIIIToSpawn = this->resSubCounts[Submarine::Type::III];

    uint32_t fishToSpawn = this->resFishCount;

    if (sinceSpawn + elapsed_ms > betweenSpawns) {
        sinceSpawn = 0;
        if (typeIToSpawn > 0) { // Spawn a type I submarine
            spawnSubmarine(Submarine::Type::I);
            this->resSubCounts[Submarine::Type::I] -= 1; // Dec by 1
        }
        if (typeIIToSpawn > 0) { // Spawn a type II submarine
            spawnSubmarine(Submarine::Type::II);
            this->resSubCounts[Submarine::Type::II] -= 1; // Dec by 1
        }
        if (typeIIIToSpawn > 0) { // Spawn a type III submarine
            spawnSubmarine(Submarine::Type::III);
            this->resSubCounts[Submarine::Type::III] -= 1; // Dec by 1
        }
        if (fishToSpawn > 0 && fishCount <= totalFish) { // Spawn a fish
            spawnFish();
            //printf("Spawning fish from here");
            this->resFishCount -= 1;
            fishCount += 1; // Inc by 1
        }
    } else {
        sinceSpawn += elapsed_ms;
    }

    if (sinceSpawn + elapsed_ms > betweenSpawns && gameIsActive == true) {
        sinceSpawn = 0;
        spawnSmoke();
    } else {
        sinceSpawn += elapsed_ms;
    }

    // Check if all submarines have been removed and advance stage
    if (isStageClear() && gameIsActive == true /*&& loading == false*/)
    {
        // TODO: Not Robust
        //loading = true;
        nextStage();

        this->entityManager->updateStageLabel(this->currentStageNumber);
    }
}

///
/// Update helper for a store stage
///
/// @param elapsed_ms The elapsed time since last tick
///
void StageController::updateStoreStage(float elapsed_ms) {
    if (!storeInit) { // Init the stage
        this->entityManager->removeAllEntities(); // Clear the stage

        Position pos1;
        Position pos2;
        Position pos3;

        pos1.y = 360.f;
        pos2.y = 360.f;
        pos3.y = 360.f;

        pos1.x = 320.f;
        pos2.x = 640.f;
        pos3.x = 960.f;

        this->spawnBuyLives(pos1);
        this->spawnBuyMissiles(pos2);
        this->spawnEndStore(pos3);
        storeInit = true;
    }

    if (storeEnded) {
        this->entityManager->removeAllEntities(); // Clear the stage

        // Move to next stage
        nextStage();

        this->entityManager->updateStageLabel(this->currentStageNumber);
    }
}

//
// MARK:- Entity Spawning IMP
//

///
/// [Convenient] Spawn a bomb at the given position
///
/// @param position The bomb position
/// @return A positive entity identifier on success, `0` otherwise.
/// @note This is a convenient wrapper for calling `makeBomb()` and then `addBomb()`.
///
Entity::Identifier StageController::spawnBomb(Position& position, vec2 boatVel)
{
    // Guard: Check whether the player has reached the limit
    if (!this->player->hasAvailableBombs())
    {
        pinfo("The player has reached the limit of the number of bombs.");
        
        return 0;
    }
    
    Bomb bomb;
    
    if (!this->entityManager->makeBomb(bomb, position, boatVel))
    {
        return 0;
    }
    
    this->entityManager->addBomb(bomb);
    
    this->player->decrementNumAvailableBombs();
    
    return bomb.getIdentifier();
}

///
/// [Convenient] Spawn an explosion at the given position
///
/// @param position The explosion position
/// @return A positive entity identifier on success, `0` otherwise.
/// @note This is a convenient wrapper for calling `makeExplosion()` and then `addExplosion()`.
///
Entity::Identifier StageController::spawnExplosion(Position& position)
{
    Explosion explosion;

    if (!this->entityManager->makeExplosion(explosion, position))
    {
        return 0;
    }
    
    this->entityManager->addExplosion(explosion);
    
    return explosion.getIdentifier();
}

///
/// [Convenient] Spawn a submarine of the given type
///
/// @param type The submarine type
/// @return A positive entity identifier on success, `0` otherwise.
/// @note This is a convenient wrapper for calling `makeSubmarine()` and then `addSubmarine()`.
///       This method will spawn the given type of submarine based on the current stage control data.
///       e.g. The velocity of a submarine will be determined by the corresponding velocity range.
///
Entity::Identifier StageController::spawnSubmarine(Submarine::Type type)
{
    // Generate a direction and a position
    Position position(0, this->yrandoms[type].generate());
    
    Direction direction = Direction::Right;
    
    if (this->drandom.generate() == 0)
    {
        direction = Direction::Left;

        position.x = 1280; // TODO: AVOID HARDCODED VALUE
    }
    
    // Create the submarine
    if (type == Submarine::Type::I){
        return spawnSubIHelper(position, direction);
    } else if (type == Submarine::Type::II) {
        return spawnSubIIHelper(position, direction);
    } else {
        return spawnSubIIIHelper(position, direction);
    }
}

///
/// Helper to spawn a submarine of type I.
///
/// @param pos The position of the submarine
/// @param dir The direction of the submarine
/// @return The ID of the new submarine
///
Entity::Identifier StageController::spawnSubIHelper(Position pos, Direction dir) {
    // Create the submarine
    SubmarineI submarine;

    Submarine::Type type = Submarine::Type::I;

    //printf("\n\n\n SPAWNING SUBMARINE. TYPE IS %c AND CLASS TYPE IS %s\n\n\n", type, typeid(submarine).name());

    // TODO: Read the score for each submarine type from the stage control data
    // TODO: Read the radar radius for each submarine type from the stage control data
    if (!this->entityManager->makeSubmarine(submarine, pos, dir, this->vrandoms[type].generate(), type, 3, FLT_MAX))
    {
        return 0;
    }

    // Add the submarine
    this->entityManager->addSubmarine(submarine, type);

    return submarine.getIdentifier();
}

///
/// Helper to spawn a submarine of type II.
///
/// @param pos The position of the submarine
/// @param dir The direction of the submarine
/// @return The ID of the new submarine
///
Entity::Identifier StageController::spawnSubIIHelper(Position pos, Direction dir) {
    // Create the submarine
    SubmarineII submarine;

    Submarine::Type type = Submarine::Type::II;

    // TODO: Read the score for each submarine type from the stage control data
    // TODO: Read the radar radius for each submarine type from the stage control data
    if (!this->entityManager->makeSubmarine(submarine, pos, dir, this->vrandoms[type].generate(), type, 3, FLT_MAX))
    {
        return 0;
    }

    // Add the submarine
    this->entityManager->addSubmarine(submarine, type);

    return submarine.getIdentifier();
}

///
/// Helper to spawn a submarine of type III.
///
/// @param pos The position of the submarine
/// @param dir The direction of the submarine
/// @return The ID of the new submarine
///
Entity::Identifier StageController::spawnSubIIIHelper(Position pos, Direction dir) {
    // Create the submarine
    SubmarineIII submarine;

    Submarine::Type type = Submarine::Type::III;

    // TODO: Read the score for each submarine type from the stage control data
    // TODO: Read the radar radius for each submarine type from the stage control data
    if (!this->entityManager->makeSubmarine(submarine, pos, dir, this->vrandoms[type].generate(), type, 3, FLT_MAX))
    {
        return 0;
    }

    // Add the submarine
    this->entityManager->addSubmarine(submarine, type);

    return submarine.getIdentifier();
}

///
/// [Convenient] Spawn a fish
///
/// @return A positive entity identifier on success, `0` otherwise.
/// @note This is a convenient wrapper for calling `makeFish()` and then `addFish()`.
///       This method will spawn a fish
///
Entity::Identifier StageController::spawnFish()
{
    // Generate a direction and a position

    Position position(0, fishRandom.generate());

    Direction direction = Direction::Right;

    if (this->drandom.generate() == 0)
    {
        direction = Direction::Left;

        position.x = 1998; // TODO: AVOID HARDCODED VALUE
    }

    // Create the fish
    Fish fish;

    if (!this->entityManager->makeFish(fish, position, direction, 3.0))
    {
        return 0;
    }

    // Add the submarine
    this->entityManager->addFish(fish);

    return fish.getIdentifier();
}

/// [Convenient] Spawn a torpedo at the given position
///
/// @param position The torpedo position
/// @param initVel The initial velocity of the torpedo
/// @return A positive entity identifier on success, `0` otherwise.
/// @note This is a convenient wrapper for calling `makeTorpedo()` and then `addTorpedo()`.
///
Entity::Identifier StageController::spawnTorpedo(Position &position, vec2 initVel)
{
    Torpedo torpedo;

    if (!this->entityManager->makeTorpedo(torpedo, position, initVel))
    {
        return 0;
    }

    this->entityManager->addTorpedo(torpedo);

    return torpedo.getIdentifier();
}

///
/// [Convenient] Spawn smoke at boat position
///
/// @return 'true' on sucess, 'false' otherwise
/// @note This is a convenient wrapper for calling `makeSmoke()` and then `addSmoke()`.
///
bool StageController::spawnSmoke()
{
    Smoke smoke;

    if (!this->entityManager->makeSmoke(smoke))
    {
        return false;
    }

    this->entityManager->addSmoke(smoke);

    return true;
}

///
/// [Convenient] Spawn a missile at the given position
///
/// @param position The missile position
/// @return A positive entity identifier on success, `0` otherwise.
/// @note This is a convenient wrapper for calling `makeMissile()` and then `addMissile()`.
///
Entity::Identifier StageController::spawnMissile(Position &position)
{
    Missile missile;

    if(!this->entityManager->makeMissile(missile, position))
    {
        return 0;
    }

    this->entityManager->addMissile(missile);

    return missile.getIdentifier();
}

///
/// [Convenient] Spawn a boat missile at the given position
///
/// @param position The boat missile position
/// @param target The boat missile's target
/// @return A positive entity identifier on success, `0` otherwise.
/// @note This is a convenient wrapper for calling `makeBoatMissile()` and then `addBoatMissile()`.
///
Entity::Identifier StageController::spawnBoatMissile(Position &position, Position& target)
{
    BoatMissile boatMissile;

    // Guard: Check whether the player has reached the limit
    if (!this->player->hasAvailableMissiles())
    {
        pinfo("The player has run out of boat missiles.");

        return 0;
    }

    // Remove a missile
    this->player->decrementNumAvailableMissiles();

    // Update missile label
    psoftassert(this->entityManager->updateBoatMissilesLabel(player->getNumAvailableMissiles()), "Failed to update the missiles label.");

    if(!this->entityManager->makeBoatMissile(boatMissile, position, target))
    {
        return 0;
    }

    this->entityManager->addBoatMissile(boatMissile);

    return boatMissile.getIdentifier();
}

///
/// Spawn a store item that allows the player to buy lives
///
/// @param position The position of the icon
/// @return A positive entity identifier on success, `0` otherwise.
///
Entity::Identifier StageController::spawnBuyLives(Position& position) {

    BuyLives buyLives;

    if(!this->entityManager->makeBuyLives(buyLives, position))
    {
        return 0;
    }

    this->entityManager->addBuyLives(buyLives);

    return buyLives.getIdentifier();
}

///
/// Spawn a store item that allows the player to buy missiles
///
/// @param position The position of the icon
/// @return A positive entity identifier on success, `0` otherwise.
///
Entity::Identifier StageController::spawnBuyMissiles(Position& position) {
    BuyMissiles buyMissiles;

    if(!this->entityManager->makeBuyMissiles(buyMissiles, position))
    {
        return 0;
    }

    this->entityManager->addBuyMissiles(buyMissiles);

    return buyMissiles.getIdentifier();
}

///
/// Spawn a store item that allows the player to move on from the store
///
/// @param position The position of the icon
/// @return A positive entity identifier on success, `0` otherwise.
///
Entity::Identifier StageController::spawnEndStore(Position& position) {
    EndStore endStore;

    if(!this->entityManager->makeEndStore(endStore, position))
    {
        return 0;
    }

    this->entityManager->addEndStore(endStore);

    return endStore.getIdentifier();
}

void StageController::saveGame(SC_SaveGameData* data)
{
    data->stage = this->currentStageNumber;
    data->subsDead = this->subsDead;
    data->totalSubs = this->totalSubs;
    data->score = this->player->getPlayerScore();
    data->money = this->player->getPlayerMoney();
    data->lives = this->player->getPlayerLives();
    data->resSubCountsI    = this->resSubCounts[Submarine::Type::I];
    data->resSubCountsII   = this->resSubCounts[Submarine::Type::II];
    data->resSubCountsIII  = this->resSubCounts[Submarine::Type::III];
    data->resSubCountsSPEC = this->resSubCounts[Submarine::Type::SPEC];
}

bool StageController::loadGame(SC_SaveGameData data)
{
    this->currentStageNumber = data.stage;
    this->subsDead = data.subsDead;
    this->totalSubs = data.totalSubs;
    this->player->incrementScore(data.score - this->player->getPlayerScore());
    this->player->spendPlayerMoney(this->player->getPlayerMoney() - data.money);
    if(this->player->getPlayerLives() < data.lives){
        while(this->player->getPlayerLives() < data.lives){
            this->player->incrementNumLives();
        }
    } else {
        while(this->player->getPlayerLives() > data.lives){
            this->player->decrementNumLives();
        }
    }
    this->playerBoatLivesDidChange(data.lives);
    this->resSubCounts[Submarine::Type::I] = data.resSubCountsI;
    this->resSubCounts[Submarine::Type::II] = data.resSubCountsII;
    this->resSubCounts[Submarine::Type::III] = data.resSubCountsIII;
    this->resSubCounts[Submarine::Type::SPEC] = data.resSubCountsSPEC;
    
    Stage* nextStage = &StageController::stages[this->currentStageNumber];
    
    if (!nextStage->isLoaded())
    {
        if (!nextStage->load(this->currentStageNumber))
        {
            pserror("Failed to load the next stage (#%d).", this->currentStageNumber);
            
            return false;
        }
    }
    
    // Reinitialize the random number generators
    this->initRandomNumGen(nextStage);
    
    return true;
}

//
// MARK:- Manages Game Stages
//

///
/// Return `true` if there is a next stage
///
bool StageController::hasNextStage()
{
    return this->currentStageNumber < StageController::TOTAL_NUM_STAGES;
}

///
/// Helper function to intialize the required random numbers
///
/// @param stage the stage you want to initalize the random numbers for
///
void StageController::initRandomNumGen(Stage* stage)
{
    for (auto& type : Submarine::allTypes)
    {
        this->vrandoms[type].init(stage->getSubmarineVelocityRange(type));
        
        this->yrandoms[type].init(stage->getSubmarineYcoordRange(type));
        
        this->rrrandoms[type].init(stage->getSubmarineRadarRadiusRange(type));
        
        // TODO: Might also need to reinitialize other control data
        //       e.g. Rate of initiating attacks
        //            Missile path
        //       All control type could be defined as a map<Type, Any>
    }
}

///
/// Activate the next game stage
///
/// @return `true` if the next stage is ready, `false` otherwise.
///
bool StageController::nextStage()
{
    // Guard: Must have a next stage
    passert(this->hasNextStage(), "[Fatal] Must have a next stage.");
    
    pinfo("Current stage is #%d.", this->currentStageNumber);
    
    // Load the stage if necessary
    uint32_t next = this->currentStageNumber + 1;
    
    Stage* nextStage = &StageController::stages[next];
    
    if (!nextStage->isLoaded())
    {
        if (!nextStage->load(next))
        {
            pserror("Failed to load the next stage (#%d).", next);
            
            return false;
        }
    }
    
    // The next stage has been successfully loaded
    this->currentStageNumber = next;
    
    pinfo("The next stage (#%d) has been loaded successfully.", this->currentStageNumber);
    
    // Reinitialize the random number generators
    this->initRandomNumGen(nextStage);
    
    this->resSubCounts = nextStage->getSubmarineCountLimits();

    this->resFishCount = nextStage->getFishCount(); // TODO: Implement this information in the stage data!!

    this->stageType = nextStage->getStageType();

    storeInit = false;
    storeEnded = false;

    waterCurr = nextStage->getCurrent();

    //loading = false;
    subsDead = 0;
    totalSubs = this->resSubCounts[Submarine::Type::I] +
                this->resSubCounts[Submarine::Type::II] +
                this->resSubCounts[Submarine::Type::III];

    totalFish = this->resFishCount;

    this->player->setCurrent(waterCurr);

    return true;
}

///
/// Return `true` if the current stage is clear
///
/// @note A stage is considered to be clear if all submarines are either destroyed or escaped.
///
bool StageController::isStageClear()
{
    return subsDead >= totalSubs;
}

//
// MARK:- Collision Delegate IMP
//

///
/// Called when the collision system starts to find the collision
///
/// @warning This method should only be called once during an update session
///
void StageController::beginUpdates()
{
    // Clear all identifiers to be removed
    this->rmbombs.clear();
    
    this->rmsubmarines.clear();
    
    this->rmfishes.clear();
    
    this->rmtorpedoes.clear();
    
    this->rmmissiles.clear();

    this->rmboatmissiles.clear();
}

//
// MARK: Player attacks enemies and fishes
//

///
/// Called when a bomb collides with a destroyable entity and consequently causes an explosion
///
/// @param bomb The identifier of the bomb that collides with a submarine
/// @param pos The identifier of the submarine that will be destroyed by the bomb
///
void StageController::bombDidGenerateExplosion(Entity::Identifier bomb)
{
    // Spawn an explosion at the bomb position
    psoftassert(this->spawnExplosion(this->entityManager->componentsForType<Position>()[bomb]), "Failed to spawn an explosion.");
    
    // Play the explosion sound effect
    psoftassert(SoundPlayer::shared()->playExplosionSoundEffect(), "Failed to play the explosion sound effect.");
    
    // Set the bomb to be removed
    this->rmbombs.insert(bomb);
    
    // We now have one more available bomb
    // No need to worry about updating the bomb status label
    // as these will be handled by the PlayerDelegate (i.e. delegate chaining)
    this->player->incrementNumAvailableBombs();
}

///
/// Called when a boat missile reaches the end of its path and consequently causes an explosion
///
/// @param boatMissile The identifier of the boat missile that's exploding
///
void StageController::boatMissileDidGenerateExplosion(Entity::Identifier boatMissile) {
    this->spawnExplosion(this->entityManager->componentsForType<Position>()[boatMissile]);

    SoundPlayer::shared()->playExplosionSoundEffect();

    this->rmboatmissiles.insert(boatMissile);
}

///
/// Called when an explosion collides with submarines
///
/// @param submarines Identifiers of submarines that will be destroyed by the bomb
/// @note This delegate method enables the collision handling for chaining explosions on submarines.
///
void StageController::explosionDidCollideWithSubmarines(std::vector<Entity::Identifier> submarines)
{
    // Remove the submarines
    this->rmsubmarines.insert(submarines.begin(), submarines.end());
    
    // Add the player score
    std::for_each(submarines.begin(), submarines.end(), [this] (auto& id) { this->player->incrementScore(this->entityManager->componentsForType<Score>()[id].score); });
    
    // Commit the score
    // No need to worry about updating the score label,
    // as these will be handled by the PlayerDelegate (i.e. delegate chaining)
    this->player->commitScore();
}

///
/// Called when an explosion collides with fish
///
/// @param fishes Identifiers of fishes that will be destroyed by the bomb
/// @note This delegate method enables the collision handling for chaining explosions on fishes.
///
void StageController::explosionDidCollideWithFishes(std::vector<Entity::Identifier> fishes)
{
    // Remove the fishes
    this->rmfishes.insert(fishes.begin(), fishes.end());
    
    // Add the player score
    std::for_each(fishes.begin(), fishes.end(), [this] (auto& id) { this->player->incrementScore(this->entityManager->componentsForType<Score>()[id].score); });
    
    // Commit the score
    // No need to worry about updating the score label,
    // as these will be handled by the PlayerDelegate (i.e. delegate chaining)
    this->player->commitScore();
}

///
/// Called when an explosion collides with missile
///
/// @param missiles Identifiers of missiles that will be destroyed by the bomb
/// @note This delegate method enables the collision handling for chaining explosions on missiles.
///
void StageController::explosionDidCollideWithMissiles(std::vector<Entity::Identifier> missiles) {
    // Remove the missiles
    this->rmmissiles.insert(missiles.begin(), missiles.end());

    // Spawn an explosion for each missile
    std::for_each(missiles.begin(), missiles.end(), [this] (auto& id) {
        this->spawnExplosion(this->entityManager->componentsForType<Position>()[id]);
        SoundPlayer::shared()->playExplosionSoundEffect();
    });
}

///
/// Called when an explosion collides with torpedo
///
/// @param torpedoes Identifiers of torpedoes that will be destroyed by the bomb
/// @note This delegate method enables the collision handling for chaining explosions on torpedoes.
///
void StageController::explosionDidCollideWithTorpedoes(std::vector<Entity::Identifier> torpedoes) {
    // Remove the torpedoes
    this->rmtorpedoes.insert(torpedoes.begin(), torpedoes.end());

    // Spawn an explosion for each torpedo
    std::for_each(torpedoes.begin(), torpedoes.end(), [this] (auto& id) {
        this->spawnExplosion(this->entityManager->componentsForType<Position>()[id]);
        SoundPlayer::shared()->playExplosionSoundEffect();
    });
}

///
/// Called when an explosion collides with store icon
///
/// @param storeIcons Identifiers of store icons that will be destroyed by the bomb
/// @note This delegate method enables the collision handling for chaining explosions on store icons.
///
void StageController::explosionDidCollideWithStoreIcons(std::vector<Entity::Identifier> storeIcons) {
    auto storeArr = this->entityManager->componentsForType<Store>(); // Get all entities with store components

    for (auto it = storeIcons.begin(); it != storeIcons.end(); ++it) {
        switch(storeArr[*it].type) {
            case Store::sType::boatMissile :
                playerDidBuyMissile();
                break;
            case Store::sType::life :
                playerDidBuyLife();
                break;
            case Store::sType::end :
                playerDidExitStore();
                break;
            default:
                break;
        }
    }
}

//
// MARK: Scenario: The player boat gets destroyed by the enemy projectiles
//

///
/// Called when a torpedo collides with the player boat
///
/// @param torpedo The identifier of the torpedo that collides with the player boat
/// @param boat The identifier of the player boat
///
void StageController::torpedoDidCollideWithBoat(Entity::Identifier torpedo, Entity::Identifier boat)
{
    // Remove the torpedoes
    this->rmtorpedoes.insert(torpedo);
    
    // Call the common helper method
    this->projectileDidCollideWithBoat(boat);
}

///
/// Called when a missile collides with the player boat
///
/// @param missile The identifier of the missile that collides with the player boat
/// @param boat The identifier of the player boat
///
void StageController::missileDidCollideWithBoat(Entity::Identifier missile, Entity::Identifier boat)
{
    // Remove the missiles
    this->rmmissiles.insert(missile);
    
    // Call the common helper method
    this->projectileDidCollideWithBoat(boat);
}

///
/// [Private Helper] Called when the enemy projectile collides with the player boat
///
/// @param boat The identifier of the player boat
///
void StageController::projectileDidCollideWithBoat(Entity::Identifier boat)
{
    // Make an explosion at the player boat position
    psoftassert(this->spawnExplosion(this->entityManager->componentsForType<Position>()[boat]), "Failed to spawn an explosion.");
    
    // Play the explosion sound effect
    psoftassert(SoundPlayer::shared()->playExplosionSoundEffect(), "Failed to play the explosion sound effect.");
    
    // Set the player boat destroyed
    // No need to worry about the rest of lives, resetting the stage, updating the label, etc.
    // as these will be handled by the PlayerDelegate (i.e. delegate chaining)
    this->player->setBoatDestroyed();
}

//
// MARK: Scenario: Enemies and their projectiles move out of screen
//

///
/// Called when a submarine did move out of screen
///
/// @param submarine The identifier of the submarine that collides with the left/right screen boundary
///
void StageController::submarineDidMoveOutOfScreen(Entity::Identifier submarine)
{
    // Just remove the submarine; No need to update the score in this case
    this->rmsubmarines.insert(submarine);
}

///
/// Called when a bomb did move out of screen
///
/// @param bomb The identifier of the bomb that collides with the screen bottom
///
void StageController::bombDidMoveOutOfScreen(Entity::Identifier bomb)
{
    // Just remove the bomb; No need to update the score in this case
    this->rmbombs.insert(bomb);
    
    this->player->incrementNumAvailableBombs();
}

///
/// Called when a missile did move out of screen
///
/// @param missile The identifier of the missile that collides with the screen top
///
void StageController::missileDidMoveOutOfScreen(Entity::Identifier missile)
{
    // Just remove the missile; No need to reset the player boat
    this->rmmissiles.insert(missile);
}

///
/// Called when a torpedo did move out of the ocean surface
///
/// @param torpedo The identifier of the torpedo that collides with the water surface
///
void StageController::torpedoDidMoveOutOfOceanSurface(Entity::Identifier torpedo)
{
    // Just remove the torpedo No need to reset the player boat
    this->rmtorpedoes.insert(torpedo);
}

///
/// Called when a smoke moves out of screen
///
/// @param smoke The identifier of the torpedo that with top of screen
///
void StageController::smokeDidMoveOutOfScreen(Entity::Identifier smoke)
{
    // Just remove the torpedo No need to reset the player boat
    this->rmsmoke.insert(smoke);
}

///
/// Called when the collision system finishes finding all collisions
///
void StageController::endUpdates()
{
    if(!this->entityManager->checkIfGameOver())
    {
        // Remove those entities from the entity manager
        std::for_each(this->rmbombs.begin(), this->rmbombs.end(), [this] (auto& id) { this->entityManager->removeBomb(id); });

        std::for_each(this->rmboatmissiles.begin(), this->rmboatmissiles.end(), [this] (auto& id) { this->entityManager->removeBoatMissile(id); });

        std::for_each(this->rmmissiles.begin(), this->rmmissiles.end(), [this] (auto& id) { this->entityManager->removeMissile(id); });
    
        std::for_each(this->rmtorpedoes.begin(), this->rmtorpedoes.end(), [this] (auto& id) { this->entityManager->removeTorpedo(id); });
    
        std::for_each(this->rmsubmarines.begin(), this->rmsubmarines.end(), [this] (auto& id) { this->entityManager->removeSubmarine(id);
                                                                                                            subsDead += 1; });

        std::for_each(this->rmfishes.begin(), this->rmfishes.end(), [this] (auto& id) { this->entityManager->removeFish(id);
                                                                                                                    fishCount -= 1;});

        std::for_each(this->rmsmoke.begin(), this->rmsmoke.end(),[this](auto& id) {    this->entityManager->removeSmoke(id); });
    }
}

//
// MARK:- Player Delegate IMP
//

///
/// Called when the player score did change
///
/// @param newScore The new player score
///
void StageController::playerScoreDidChange(uint32_t newScore)
{
    // Tells the Entity Manager to update the score label
    psoftassert(this->entityManager->updateScoreLabel(newScore), "Failed to update the score label.");
}

///
/// Called when the player money did change
///
/// @param newValue The new money value
///
void StageController::playerMoneyDidChange(uint32_t newValue)
{
    // Tells the Entity Manager to update the money label
    psoftassert(this->entityManager->updateMoneyLabel(newValue), "Failed to update the money label.");
}

///
/// Called when the player boat lives did change
///
/// @param lives The latest number of lives left
///
void StageController::playerBoatLivesDidChange(uint32_t lives)
{
    // Tells the Entity Manager to update the boat lives label
    psoftassert(this->entityManager->updateBoatLivesLabel(lives), "Failed to update the lives label.");
    
    // TODO: Must reset and restart the current stage
    
}

///
/// Called when the player did lose all lives
///
/// @note When the player lost the game, `playerBoatLivesDidChange()` would not be invoked.
///
void StageController::playerDidLoseAllLives()
{
    // TODO: Game Over -> Transfer the scene to score board or anything else
    printf("LOST ALL LIVES\n");
    this->entityManager->signalGameOver(true);
}

///
/// Called when the player purchases a missile
///
void StageController::playerDidBuyMissile() {
    if (player->spendPlayerMoney(this->entityManager->missilePrice)) { // Attempt to buy missile
        // Add missile
        player->incrementNumAvailableMissiles();

        // Update label
        psoftassert(this->entityManager->updateBoatMissilesLabel(player->getNumAvailableMissiles()), "Failed to update the missiles label.");
        // TODO: add sound?
        psoftassert(SoundPlayer::shared()->playPurchaseSoundEffect(), "Failed to play the explosion sound effect.");
    } else { // Buy failed
        // TODO: play sound?
    }
}

///
/// Called when the player purchases a life
///
void StageController::playerDidBuyLife() {
    printf("Player has %d, trying to purchase for %d", player->getPlayerMoney(), this->entityManager->lifePrice);
    if (player->spendPlayerMoney(this->entityManager->lifePrice)) { // Attempt to buy life
        // Add life
        player->incrementNumAvailableLives();

        // Update label
        psoftassert(this->entityManager->updateBoatLivesLabel(player->getPlayerLives()), "Failed to update the lives label.");
        // TODO: add sound?
        psoftassert(SoundPlayer::shared()->playPurchaseSoundEffect(), "Failed to play the explosion sound effect.");
    } else { // Buy failed
        // TODO: play sound?
    }
}

///
/// Called when the player exits the store
///
void StageController::playerDidExitStore() {
    this->storeEnded = true;
}


void StageController::enterTutorial()
{
    tutorialActive = true;
    
    Position pos;
    int i = 0;
    
    // Add enemy descriptions
    pos = {200,240};
    this->entityManager->makeStringLabel(tutorialTextArray[i++], pos, Character::Font::SFMonoRegular, Color::black, 24, "ENEMYS");
    
    pos = {235,300};
    this->entityManager->makeSubmarine(this->entityManager->tutorialSub, pos, Direction::Right, 0, Submarine::Type::I, 0);
    this->entityManager->addSubmarine(this->entityManager->tutorialSub, Submarine::Type::I);
    
    pos.y += 100;
    this->entityManager->makeFish(this->entityManager->tutorialFish, pos, Direction::Right, 0);
    this->entityManager->addFish(this->entityManager->tutorialFish);
    
    // Add attack descriptions
    pos = {587,240};
    this->entityManager->makeStringLabel(tutorialTextArray[i++], pos, Character::Font::SFMonoRegular, Color::black, 24, "DEFENCES");
    
    pos = {640,300};
    this->entityManager->makeBomb(this->entityManager->tutorialBomb, pos, {0,0});
    this->entityManager->addBomb(this->entityManager->tutorialBomb);
    
    pos.y += 100;
    this->entityManager->makeBoatMissile(this->entityManager->tutorialBM, pos, pos);
    this->entityManager->addBoatMissile(this->entityManager->tutorialBM);
    
    // TODO: Add other attack descriptions
    pos = {970,240};
    this->entityManager->makeStringLabel(tutorialTextArray[i++], pos, Character::Font::SFMonoRegular, Color::black, 24, "ATTACKERS");
    
    pos = {1030,300};
    this->entityManager->makeTorpedo(this->entityManager->tutorialTorpedo, pos, {0,0});
    this->entityManager->addTorpedo(this->entityManager->tutorialTorpedo);
    
    pos.y += 100;
    this->entityManager->makeMissile(this->entityManager->tutorialMissile, pos);
    this->entityManager->addMissile(this->entityManager->tutorialMissile);
    
    // TODO: Add a start button
    pos = {435,570};
    this->entityManager->makeStringLabel(tutorialTextArray[i++], pos, Character::Font::SFMonoRegular, Color::black, 50, "CLICK TO START");
}

void StageController::exitTutorial()
{
    if(tutorialActive)
    {
        // Remove all tutorial items on screen
        for(StringLabel sl: tutorialTextArray)
        {
            for(int c: sl.getIdentifiers())
            {
                this->entityManager->removeCharacter(c);
            }
        }
        this->entityManager->removeSubmarine(this->entityManager->tutorialSub.getIdentifier());
        this->entityManager->removeFish(this->entityManager->tutorialFish.getIdentifier());
        this->entityManager->removeBomb(this->entityManager->tutorialBomb.getIdentifier());
        this->entityManager->removeBoatMissile(this->entityManager->tutorialBM.getIdentifier());
        this->entityManager->removeTorpedo(this->entityManager->tutorialTorpedo.getIdentifier());
        this->entityManager->removeMissile(this->entityManager->tutorialMissile.getIdentifier());
        
        
        tutorialActive = false;
        this->signalGameActive(true);
        this->currentStageNumber = -1;
        entityManager->resetGame();
        this->nextStage();
    }
}
