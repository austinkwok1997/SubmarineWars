//
//  EntitySpawning.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-10.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef EntitySpawning_hpp
#define EntitySpawning_hpp

#include "Entities/Entities.hpp"
#include "Components/Components.hpp"

/// A set of methods for spawning game entities
class EntitySpawning
{
public:
    /// Virtual destructor
    virtual ~EntitySpawning() {}
    
    ///
    /// Spawn a bomb at the given position
    ///
    /// @param position The bomb position
    /// @param boatVel The velocity of the boat creating this bomb, which becomes the initial velocity of it
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    virtual Entity::Identifier spawnBomb(Position& position, vec2 boatVel) = 0;
    
    ///
    /// Spawn an explosion at the given position
    ///
    /// @param position The explosion position
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    virtual Entity::Identifier spawnExplosion(Position& position) = 0;
    // TODO: Probably need an extra param `scale` to reflect the explosion radius
    
    ///
    /// Spawn a submarine of the given type
    ///
    /// @param type The submarine type
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    virtual Entity::Identifier spawnSubmarine(Submarine::Type type) = 0;

    ///
    /// Spawn a fish
    ///
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    virtual Entity::Identifier spawnFish() = 0;

    ///
    /// Spawn a torpedo at the given position
    ///
    /// @param position The torpedo position
    /// @param initVel The initial velocity of the torpedo
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    virtual Entity::Identifier spawnTorpedo(Position& position, vec2 initVel) = 0;
    
    ///
    /// Spawn a missile at the given position
    ///
    /// @param position The missile position
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    virtual Entity::Identifier spawnMissile(Position& position) = 0;

    ///
    /// Spawn a boat missile at the given position
    ///
    /// @param position The boat missile position
    /// @param target The boat missile's target
    /// @return A positive entity identifier on success, `0` otherwise.
    ///
    virtual Entity::Identifier spawnBoatMissile(Position& position, Position& target) = 0;
};

#endif /* EntitySpawning_hpp */
