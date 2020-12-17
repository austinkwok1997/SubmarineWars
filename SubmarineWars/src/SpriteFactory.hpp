//
//  SpriteFactory.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-09-28.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef SpriteFactory_hpp
#define SpriteFactory_hpp

#include "Foundations/Foundations.hpp"
#include "Entities/Entities.hpp"
#include "Components/Sprite.hpp"
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <initializer_list>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "ProjectPath.hpp"

/// A singleton that creates sprites for different entities conveniently and efficiently
class SpriteFactory
{
public:
    /// Get the shared instance
    static SpriteFactory* shared();
    
    ///
    /// Make the sprite for the given entity type
    ///
    /// @param sprite The sprite created on return
    /// @param info Additional information needed to make the sprite
    /// @return `true` on a successful creation, `false` otherwise.
    /// @note The caller could easily make the sprite for a type by invoking make<Boat>().
    ///
    template <typename T> // Restricted: T must be a subclass of Entity
    std::enable_if_t<std::is_base_of<Entity, T>::value, bool> make(Sprite* sprite, void* info = nullptr)
    {
        // Just a runtime time check
        passert(!(std::is_same<T, Character>::value), "[Runtime] Fatal Error: make<Character>(Sprite*) should not be dispatched to here.");

        // Grab the reference to the cached textures
        // It's OK if the vector of cached textures does not exist,
        // as the map [] operator will create one for us.
        std::vector<Texture>& textures = this->texturesMap[typeid(T)];

        // Guard: Check the shared textures cache [Stage 1]
        if (textures.empty())
        {
            // No cached textures for the given entity type
            // Reserved the memory for the number of textures
            textures.resize(SpriteFactory::texturePathsMap[typeid(T)].size());
        }

        // Guard: Consistency check
        passert(textures.size() == SpriteFactory::texturePathsMap[typeid(T)].size(),
                "Fatal Error: Inconsistency found. Cached textures size must be identical to the size of file paths.");

        // Reset the initialization status of the sprite
        // Optimize the memory usage by reserving the number of sprites
        sprite->reset(textures.size());
        
        // Guard: Check the shared textures cache [Stage 2]
        auto piterator = SpriteFactory::texturePathsMap[typeid(T)].begin();

        for (auto& texture : textures)
        {
			// Guard: Check each cached texture
            if (!texture.isValid())
            {
                // No cached texture for the given entity type
                // Load the texture from the file
                if (!texture.loadFromFile(*piterator))
                {
                    pserror("Failed to load the texture #%ld for entity type %s.", std::distance(SpriteFactory::texturePathsMap[typeid(T)].begin(), piterator), typeid(T).name());

                    return false;
                }
            }

            // Guard: Initialize the sprite with each texture
            if (!sprite->initFromTexture(texture, SpriteFactory::shaderPathsForType(typeid(T))))
            {
                pserror("Failed to initialize the sprite with the current texture.");
                
                return false;
            }

            // Increment the path iterator
            std::advance(piterator, 1);
        }
        
        // All done without errors
        return true;
    }
    
private:
    /// The number of ASCII characters
    static constexpr uint32_t NUM_ASCII_CHARS = 128;

    /// A texture map type that maps the Entity type to its cached texture objects
    typedef std::unordered_map<std::type_index, std::vector<Texture>> TexturesMap;
    
    /// A texture path map that maps the Entity type to its texture file paths
    typedef std::unordered_map<std::type_index, std::vector<const char*>> TexturePathsMap;
    
    /// A shader path map that maps the Entity type to its shader file paths
    typedef std::unordered_map<std::type_index, std::pair<const char*, const char*>> ShaderPathsMap;

    /// A texture map type that maps the pair of font and size to cached texture objects for all ASCII characters
    typedef std::unordered_map<uint64_t, Texture[NUM_ASCII_CHARS]> CharacterTextureMap;

    /// A font face map type that maps the font to cached font face handle
    typedef std::unordered_map<Character::Font, FT_Face> FontFaceMap;
    
    /// A map that contains cached textures
    /// where key is the type id of the entity;
    /// and value is the cached textures.
    /// For static entity, there will be one single cached texture;
    /// For animated entity, there will be multiple cached textures.
    TexturesMap texturesMap;

    /// A map that contains cached textures for all ASCII characters
    /// where key is the pair of font type and size, represented in UInt64;
    /// and value is an array of cached texture indexed by the ASCII character.
    CharacterTextureMap characterTextureMap;

    /// A map that contains cached font face handles
    /// where key is the font type;
    /// and value is the cached font face handle object.
    FontFaceMap fontFaceMap;

    /// The FreeType library
    FT_Library ftlibrary;
    
    /// Private instance
    static SpriteFactory* instance;
    
    /// Private texture paths map
    /// For static entity, there will be one single texture file
    /// For animated entity, there will be a list of texture files
    static TexturePathsMap texturePathsMap;
    
    /// Private shader paths map
    /// Special shader paths are stored in this map
    /// If this map does not contain the given key,
    /// the default vertex and fragment shader will be used
    /// Direct access to this map is not allowed;
    /// All accesses must be coordinated by the `shadersForEntity()` function
    static ShaderPathsMap shaderPathsMap;
    
    /// The default vertex and fragment shader paths
    static constexpr std::pair<const char*, const char*> defaultShaderPaths = std::make_pair(SWShaderPath("textured.vs.glsl"), SWShaderPath("textured.fs.glsl"));
    
    /// [Convenient, Coordinated] Get the shader paths for the given entity type
    static inline const std::pair<const char*, const char*>& shaderPathsForType(std::type_index type)
    {
        return SpriteFactory::shaderPathsMap.find(type) == SpriteFactory::shaderPathsMap.end() ? SpriteFactory::defaultShaderPaths : SpriteFactory::shaderPathsMap[type];
    }
    
    /// Private constructor
    SpriteFactory();
};

///
/// Make the sprite for Character entity type
/// @param sprite The sprite created on return
/// @param info Additional information needed to make the sprite (See the note below)
/// @return `true` on a successful creation, `false` otherwise.
/// @note The caller could easily make the sprite for a type by invoking make<Character>().
/// @note `info` must be a **non-null** pointer to a `struct Character::Attribute` type.
///
template <> // Concrete: Provides a different implementation for `Character` entity
bool SpriteFactory::make<Character>(Sprite* sprite, void* info);

#endif /* SpriteFactory_hpp */
