//
//  ComponentsDataProvider.h
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-01.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef ComponentsDataProvider_hpp
#define ComponentsDataProvider_hpp

#include <typeindex>

/// A set of methods implemented by the data provider of components arrays to provide components of different types
class ComponentsDataProvider
{
public:
    /// Virtual destructor
    virtual ~ComponentsDataProvider() {}
    
    ///
    /// [Convenient] Retrieve the components array of the given type
    ///
    /// @param Type T is the component type
    /// @return The component array of type T.
    ///
    template <typename T>
    T* componentsForType()
    {
        return dynamic_cast<T*>(this->components(typeid(T)));
    }
    
    ///
    /// [Convenient] Retrieve the indirect components array of the given type
    ///
    /// @param Type T is the component type
    /// @return The component array of type T*.
    ///
    template <typename T>
    T** indirectComponentsForType()
    {
        return reinterpret_cast<T**>(this->indirectComponents(typeid(T)));
    }
    
private:
    ///
    /// Retrieve the components array
    ///
    /// @param id The type id of the component type
    /// @return A `Component` pointer to the component array.
    ///
    virtual SWComponent* components(std::type_index id) = 0;
    
    ///
    /// Retrieve the indirect components array
    ///
    /// @param id The type id of the component type
    /// @return A `Component*` pointer to the component array.
    ///
    virtual SWComponent** indirectComponents(std::type_index id) = 0;
};

#endif /* ComponentsDataProvider_hpp */
