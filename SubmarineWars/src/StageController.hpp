//
//  StageController.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-10.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef StageController_hpp
#define StageController_hpp

#include "Foundations/Foundations.hpp"
#include "Entities/Entities.hpp"
#include "Components/Components.hpp"
#include "Components/PlayerDelegate.hpp"
#include "CollisionDelegate.hpp"
#include "Systems/InputSystem.hpp"
#include "EntitySpawning.hpp"
#include "EntityManager.hpp"
#include "Stage.hpp"
#include "ProjectPath.hpp"

#include <SDL.h>
#include <SDL_mixer.h>

#include <unordered_map>
#include <unordered_set>

/// StageController manages game stages and related control data
/// It also acts as an entity spawner to spawn entities based on control data of each stage
class StageController : public EntitySpawning, public CollisionDelegate, public PlayerDelegate
{
public:
    ///
    /// [Constructor] Create a stage controller
    ///
    /// @param entityManager A reference to the entity manager to make entities for each stage
    ///
    StageController(EntityManager* entityManager);
    
    ///
    /// Update the stage at each game tick
    ///
    /// @param elapsed_ms The elapsed time since the last tick
    ///
    void update(float elapsed_ms);

    ///
    /// Update helper for a normal stage
    ///
    /// @param elapsed_ms The elapsed time since last tick
    ///
    void updateNormalStage(float elapsed_ms);

    ///
    /// Update helper for a store stage
    ///
    /// @param elapsed_ms The elapsed time since last tick
    ///
    void updateStoreStage(float elapsed_ms);

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
    Entity::Identifier spawnBomb(Position& position, vec2 boatVel) override;
    
    ///
    /// [Convenient] Spawn an explosion at the given position
    ///
    /// @param position The explosion position
    /// @return A positive entity identifier on success, `0` otherwise.
    /// @note This is a convenient wrapper for calling `makeExplosion()` and then `addExplosion()`.
    ///
    Entity::Identifier spawnExplosion(Position& position) override;
    
    ///
    /// [Convenient] Spawn a submarine of the given type
    ///
    /// @param type The submarine type
    /// @return A positive entity identifier on success, `0` otherwise.
    /// @note This is a convenient wrapper for calling `makeSubmarine()` and then `addSubmarine()`.
    ///       This method will spawn the given type of submarine based on the current stage control data.
    ///       e.g. The velocity of a submarine will be determined by the corresponding velocity range.
    ///
    Entity::Identifier spawnSubmarine(Submarine::Type type) override;

    ///
    /// [Convenient] Spawn a fish
    ///
    /// @return A positive entity identifier on success, `0` otherwise.
    /// @note This is a convenient wrapper for calling `makeFish()` and then `addFish()`.
    ///       This method will spawn a fish
    ///
    Entity::Identifier spawnFish() override;

    ///
    /// [Convenient] Spawn a torpedo at the given position
    ///
    /// @param position The torpedo position
    /// @param initVel The initial velocity of the torpedo
    /// @return A positive entity identifier on success, `0` otherwise.
    /// @note This is a convenient wrapper for calling `makeTorpedo()` and then `addTorpedo()`.
    ///
    Entity::Identifier spawnTorpedo(Position& position, vec2 initVel) override;

    ///
    /// Spawn a missile at the given position
    ///
    /// @param position The missile position
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    Entity::Identifier spawnMissile(Position& position) override;

    ///
    /// Spawn a boat missile at the given position
    ///
    /// @param position The boat missile position
    /// @param target The boat missile's target
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    Entity::Identifier spawnBoatMissile(Position& position, Position& target) override;

    ///
    /// Spawn a store item that allows the player to buy lives
    ///
    /// @param position The position of the icon
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    Entity::Identifier spawnBuyLives(Position& position);

    ///
    /// Spawn a store item that allows the player to buy missiles
    ///
    /// @param position The position of the icon
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    Entity::Identifier spawnBuyMissiles(Position& position);

    ///
    /// Spawn a store item that allows the player to move on from the store
    ///
    /// @param position The position of the icon
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    Entity::Identifier spawnEndStore(Position& position);
    
    //save game
    struct SC_SaveGameData {
        int stage;
        int subsDead;
        int totalSubs;
        uint32_t score;
        uint32_t money;
        uint32_t lives;
        
        /// Record the residual number of submarines to be spawned in this stage
        uint32_t resSubCountsI;
        uint32_t resSubCountsII;
        uint32_t resSubCountsIII;
        uint32_t resSubCountsSPEC;
    };
    void saveGame(SC_SaveGameData* data);
    bool loadGame(SC_SaveGameData data);

    ///
    /// [Convenient] Spawn smoke at boat position
    ///
    /// @return 'true' on sucess, 'false' otherwise
    /// @note This is a convenient wrapper for calling `makeSmoke()` and then `addSmoke()`.
    ///
    bool spawnSmoke();
    
    ///
    /// Helper function to intialize the required random numbers
    ///
    /// @param stage the stage you want to initalize the random numbers for
    ///
    void initRandomNumGen(Stage* stage);
    
    ///
    /// Activate the next game stage
    ///
    /// @return `true` if the next stage is ready, `false` otherwise.
    ///
    bool nextStage();
    
    ///
    /// Signal the game has started
    ///
    /// @param active true if setting game to active, false otherwise
    ///
    void signalGameActive(bool active);
    
    ///
    /// Check is the game is active or not
    ///
    /// @return true if game is active, false otherwise
    bool isGameActive();
    
    ///
    /// Starts the tutorial
    ///
    void enterTutorial();
    
    ///
    /// Ends the tutorial
    ///
    void exitTutorial();

private:
    /// The total number of stages in this game
    static constexpr int TOTAL_NUM_STAGES = 26;
    
    /// A stage cache to allow lazy initialization
    static Stage stages[TOTAL_NUM_STAGES + 1];
    
    bool gameIsActive = false;
    
    bool tutorialActive = false;
    
    /// A reference to the entity manager to make entities
    EntityManager* entityManager;
    
    /// A reference to the player component
    Player* player;
    
    /// The current stage number; -1 if not started
    int currentStageNumber = -1;

    /// A random number generator to determine the submarine facing direction (static)
    Random<int> drandom;

    /// An array of texts that can be displayed during the tutorial
    StringLabel tutorialTextArray[5];

    /// Random number generators to determine the submarine velocity
    std::unordered_map<Submarine::Type, Random<float>> vrandoms;
    
    /// Random number generators to determine the initial submarine y-coordinate
    std::unordered_map<Submarine::Type, Random<float>> yrandoms;
    
    /// Random number generators to determine the submarine radar radius
    std::unordered_map<Submarine::Type, Random<float>> rrrandoms;
    
    /// Record the residual number of submarines to be spawned in this stage
    std::unordered_map<Submarine::Type, uint32_t> resSubCounts;

    /// Record the number of fish to be spawned this stage. Value is 0 before game starts
    uint32_t resFishCount = 0;

    /// The number of fish currently on stage
    int fishCount;

    /// The water current on this stage
    vec2 waterCurr;

    /// The stage type of this stage
    int stageType;

    /// Record identifiers of bombs to be removed due to detected collisions
    std::unordered_set<Entity::Identifier> rmbombs;
    
    /// Record identifiers of submarines to be removed due to detected collisions
    std::unordered_set<Entity::Identifier> rmsubmarines;

    /// Record identifiers of fish to be removed due to detected collisions
    std::unordered_set<Entity::Identifier> rmfishes;
    
    /// Record identifiers of torpedoes to be removed due to detected collisions
    std::unordered_set<Entity::Identifier> rmtorpedoes;
    
    /// Record identifiers of missiles to be removed due to detected collisions
    std::unordered_set<Entity::Identifier> rmmissiles;

    /// Record identifiers of boat missiles to be removed due to exploding
    std::unordered_set<Entity::Identifier> rmboatmissiles;

    /// Record identifiers of smoke to be removed due to detected collisions
    std::unordered_set<Entity::Identifier> rmsmoke;

    /// Time since last sub spawn
    float sinceSpawn;

    /// Time between sub spawns
    float betweenSpawns;

    /// Time since last sub spawn
    float sinceSmokeSpawn;

    /// Time between sub spawns
    float betweenSmokeSpawns;

    /// Total number of submarines in the stage
    int totalSubs;

    /// Total number of fish in the stage
    int totalFish;

    /// Number of submarines that have been removed
    int subsDead;

    /// Next stage being loaded
    bool loading;

    /// Has this store been initialized
    bool storeInit;

    /// Has this store been ended
    bool storeEnded;

    /// Y coord of new fish
    Random<float> fishRandom;

    //
    // MARK:- Manages Game Stages
    //

    ///
    /// Helper to spawn a submarine of type I.
    ///
    /// @param pos The position of the submarine
    /// @param dir The direction of the submarine
    /// @return The ID of the new submarine
    ///
    Entity::Identifier spawnSubIHelper(Position pos, Direction dir);

    ///
    /// Helper to spawn a submarine of type II.
    ///
    /// @param pos The position of the submarine
    /// @param dir The direction of the submarine
    /// @return The ID of the new submarine
    ///
    Entity::Identifier spawnSubIIHelper(Position pos, Direction dir);

    ///
    /// Helper to spawn a submarine of type III.
    ///
    /// @param pos The position of the submarine
    /// @param dir The direction of the submarine
    /// @return The ID of the new submarine
    ///
    Entity::Identifier spawnSubIIIHelper(Position pos, Direction dir);
    
    ///
    /// Return `true` if there is a next stage
    ///
    bool hasNextStage();
    
    ///
    /// Return `true` if the current stage is clear
    ///
    /// @note A stage is considered to be clear if all submarines are either destroyed or escaped.
    ///
    bool isStageClear();


    
    //
    // MARK:- Collision Delegate IMP
    //
    
    ///
    /// Called when the collision system starts to find the collision
    ///
    /// @warning This method should only be called once during an update session
    ///
    void beginUpdates() override;
    
    //
    // MARK: Player attacks enemies and fishes
    //
    
    ///
    /// Called when a bomb collides with a destroyable entity and consequently causes an explosion
    ///
    /// @param bomb The identifier of the bomb that collides with a submarine
    ///
    void bombDidGenerateExplosion(Entity::Identifier bomb) override;

    ///
    /// Called when a boat missile reaches the end of its path and consequently causes an explosion
    ///
    /// @param boatMissile The identifier of the boat missile that's exploding
    ///
    void boatMissileDidGenerateExplosion(Entity::Identifier boatMissile) override;
    
    ///
    /// Called when an explosion collides with submarines
    ///
    /// @param submarines Identifiers of submarines that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on submarines.
    ///
    void explosionDidCollideWithSubmarines(std::vector<Entity::Identifier> submarines) override;
    
    ///
    /// Called when an explosion collides with fish
    ///
    /// @param fishes Identifiers of fishes that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on fishes.
    ///
    void explosionDidCollideWithFishes(std::vector<Entity::Identifier> fishes) override;

    ///
    /// Called when an explosion collides with missile
    ///
    /// @param missiles Identifiers of missiles that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on missiles.
    ///
    void explosionDidCollideWithMissiles(std::vector<Entity::Identifier> missiles) override;

    ///
    /// Called when an explosion collides with torpedo
    ///
    /// @param torpedoes Identifiers of torpedoes that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on torpedoes.
    ///
    void explosionDidCollideWithTorpedoes(std::vector<Entity::Identifier> torpedoes) override;

    ///
    /// Called when an explosion collides with store icon
    ///
    /// @param storeIcons Identifiers of store icons that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on store icons.
    ///
    void explosionDidCollideWithStoreIcons(std::vector<Entity::Identifier> storeIcons) override;


    //
    // MARK: Scenario: The player boat gets destroyed by the enemy projectiles
    //
    
    ///
    /// Called when a torpedo collides with the player boat
    ///
    /// @param torpedo The identifier of the torpedo that collides with the player boat
    /// @param boat The identifier of the player boat
    ///
    void torpedoDidCollideWithBoat(Entity::Identifier torpedo, Entity::Identifier boat) override;
    
    ///
    /// Called when a missile collides with the player boat
    ///
    /// @param missile The identifier of the missile that collides with the player boat
    /// @param boat The identifier of the player boat
    ///
    void missileDidCollideWithBoat(Entity::Identifier missile, Entity::Identifier boat) override;
    
    ///
    /// [Private Helper] Called when the enemy projectile collides with the player boat
    ///
    /// @param boat The identifier of the player boat
    ///
    void projectileDidCollideWithBoat(Entity::Identifier boat);
    
    //
    // MARK: Scenario: Enemies and their projectiles move out of screen
    //
    
    ///
    /// Called when a submarine did move out of screen
    ///
    /// @param submarine The identifier of the submarine that collides with the left/right screen boundary
    ///
    void submarineDidMoveOutOfScreen(Entity::Identifier submarine) override;
    
    ///
    /// Called when a bomb did move out of screen
    ///
    /// @param bomb The identifier of the bomb that collides with the screen bottom
    ///
    void bombDidMoveOutOfScreen(Entity::Identifier bomb) override;
    
    ///
    /// Called when a missile did move out of screen
    ///
    /// @param missile The identifier of the missile that collides with the screen top
    ///
    void missileDidMoveOutOfScreen(Entity::Identifier missile) override;
    
    ///
    /// Called when a torpedo did move out of the ocean surface
    ///
    /// @param torpedo The identifier of the torpedo that collides with the water surface
    ///
    void torpedoDidMoveOutOfOceanSurface(Entity::Identifier torpedo) override;

    ///
    /// Called when smoke moves out of screen
    ///
    /// @param smoke The identifier of the smoke that leaves the screen
    ///
    void smokeDidMoveOutOfScreen(Entity::Identifier smoke) override;
    
    ///
    /// Called when the collision system finishes finding all collisions
    ///
    void endUpdates() override;
    
    //
    // MARK:- Player Delegate IMP
    //
    
    ///
    /// Called when the player score did change
    ///
    /// @param newScore The new player score
    ///
    void playerScoreDidChange(uint32_t newScore) override;
    
    ///
    /// Called when the player money did change
    ///
    /// @param newValue The new money value
    ///
    void playerMoneyDidChange(uint32_t newValue) override;
    
    ///
    /// Called when the player boat lives did change
    ///
    /// @param lives The latest number of lives left
    ///
    void playerBoatLivesDidChange(uint32_t lives) override;
    
    ///
    /// Called when the player did lose all lives
    ///
    /// @note When the player lost the game, `playerBoatLivesDidChange()` would not be invoked.
    ///
    void playerDidLoseAllLives() override;

    ///
    /// Called when the player purchases a missile
    ///
    void playerDidBuyMissile();

    ///
    /// Called when the player purchases a life
    ///
    void playerDidBuyLife();

    ///
    /// Called when the player exits the store
    ///
    void playerDidExitStore();
};

#endif /* StageController_hpp */
