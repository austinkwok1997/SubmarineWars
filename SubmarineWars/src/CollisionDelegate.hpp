//
//  CollisionDelegate.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-10.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef CollisionDelegate_hpp
#define CollisionDelegate_hpp

#include "Entities/Entities.hpp"

/// A set of methods implemented by the collision handler to perform
/// essential actions on a collision detected by the Collision System
class CollisionDelegate
{
public:
    /// Virtual destructor
    virtual ~CollisionDelegate() {}
    
    ///
    /// Called when the collision system starts to find the collision
    ///
    /// @warning This method should only be called once during an update session
    ///
    virtual void beginUpdates() = 0;

    //
    // MARK:- Player attacks enemies and fishes
    //

    ///
    /// Called when a bomb collides with a destroyable entity and consequently causes an explosion
    ///
    /// @param bomb The identifier of the bomb that collides with a submarine
    ///
    virtual void bombDidGenerateExplosion(Entity::Identifier bomb) = 0;

    ///
    /// Called when a boat missile reaches the end of its path and consequently causes an explosion
    ///
    /// @param boatMissile The identifier of the boat missile that's exploding
    ///
    virtual void boatMissileDidGenerateExplosion(Entity::Identifier boatMissile) = 0;
    
    ///
    /// Called when an explosion collides with submarines
    ///
    /// @param submarines Identifiers of submarines that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on submarines.
    ///
    virtual void explosionDidCollideWithSubmarines(std::vector<Entity::Identifier> submarines) = 0;
    
    ///
    /// Called when an explosion collides with fish
    ///
    /// @param fishes Identifiers of fishes that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on fishes.
    ///
    virtual void explosionDidCollideWithFishes(std::vector<Entity::Identifier> fishes) = 0;

    ///
    /// Called when an explosion collides with missile
    ///
    /// @param missiles Identifiers of missiles that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on missiles.
    ///
    virtual void explosionDidCollideWithMissiles(std::vector<Entity::Identifier> missiles) = 0;

    ///
    /// Called when an explosion collides with torpedo
    ///
    /// @param torpedoes Identifiers of torpedoes that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on torpedoes.
    ///
    virtual void explosionDidCollideWithTorpedoes(std::vector<Entity::Identifier> torpedoes) = 0;

    ///
    /// Called when an explosion collides with store icon
    ///
    /// @param storeIcons Identifiers of store icons that will be destroyed by the bomb
    /// @note This delegate method enables the collision handling for chaining explosions on store icons.
    ///
    virtual void explosionDidCollideWithStoreIcons(std::vector<Entity::Identifier> storeIcons) = 0;

    //
    // MARK:- Scenario: The player boat gets destroyed by the enemy projectiles
    //
    ///
    /// Called when a torpedo collides with the player boat
    ///
    /// @param torpedo The identifier of the torpedo that collides with the player boat
    /// @param boat The identifier of the player boat
    ///
    virtual void torpedoDidCollideWithBoat(Entity::Identifier torpedo, Entity::Identifier boat) = 0;
    
    ///
    /// Called when a missile collides with the player boat
    ///
    /// @param missile The identifier of the missile that collides with the player boat
    /// @param boat The identifier of the player boat
    ///
    virtual void missileDidCollideWithBoat(Entity::Identifier missile, Entity::Identifier boat) = 0;

    //
    // MARK:- Scenario: Enemies and their projectiles move out of screen
    //
    ///
    /// Called when a submarine did move out of screen
    ///
    /// @param submarine The identifier of the submarine that collides with the left/right screen boundary
    ///
    virtual void submarineDidMoveOutOfScreen(Entity::Identifier submarine) = 0;

    ///
    /// Called when a bomb did move out of screen
    ///
    /// @param bomb The identifier of the bomb that collides with the screen bottom
    ///
    virtual void bombDidMoveOutOfScreen(Entity::Identifier bomb) = 0;

    ///
    /// Called when a missile did move out of screen
    ///
    /// @param missile The identifier of the missile that collides with the screen top
    ///
    virtual void missileDidMoveOutOfScreen(Entity::Identifier missile) = 0;

    ///
    /// Called when a torpedo did move out of the ocean surface
    ///
    /// @param torpedo The identifier of the torpedo that collides with the water surface
    ///
    virtual void torpedoDidMoveOutOfOceanSurface(Entity::Identifier torpedo) = 0;

    ///
    /// Called when smoke moves above top of screen
    ///
    /// @param smoke the identifier of the smoke when it collides with top of screen
    ///
    virtual void smokeDidMoveOutOfScreen(Entity::Identifier smoke) = 0;
    
    ///
    /// Called when the collision system finishes finding all collisions
    ///
    virtual void endUpdates() = 0;
};

#endif /* CollisionDelegate_hpp */
