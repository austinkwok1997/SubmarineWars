//
//  EntityManagerDelegate.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-09-27.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef EntityManagerDelegate_hpp
#define EntityManagerDelegate_hpp

#include "Entities/Entity.hpp"

/// Contains a set of methods to handle events occured in an entity manager
class EntityManagerDelegate /* protocol EntityManagerDelegate  OR  interface EntityManagerDelegate */
{
public:
    /// Virtual destructor
    virtual ~EntityManagerDelegate() {}
    
    ///
    /// Called when an entity has been added to the entity manager
    ///
    /// @param entity The entity added
    ///
    virtual void didAddEntity(Entity& entity) = 0;
    
    ///
    /// Called when an entity has been removed from the entity manager
    ///
    /// @param entity The entity removed
    ///
    virtual void didRemoveEntity(Entity& entity) = 0;
    
    ///
    /// Called when an entity has been updated
    ///
    /// @param entity The entity updated
    /// @note An entity is updated when a component is added to/removed from it.
    ///
    virtual void didUpdateEntity(Entity& entity) = 0;
};

#endif /* EntityManagerDelegate_hpp */
