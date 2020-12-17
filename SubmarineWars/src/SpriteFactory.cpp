//
//  SpriteFactory.cpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-09-28.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#include "SpriteFactory.hpp"
#include "ProjectPath.hpp"

/// Private instance
SpriteFactory* SpriteFactory::instance = nullptr;

/// Private texture paths map
SpriteFactory::TexturePathsMap SpriteFactory::texturePathsMap =
{
    {typeid(Boat),          {SWTexturesPath("BoatF1.png"),
                             SWTexturesPath("BoatF1.png"),
                             SWTexturesPath("BoatF1.png"),
                             SWTexturesPath("BoatF2.png"),
                             SWTexturesPath("BoatF2.png"),
                             SWTexturesPath("BoatF2.png"),
                             SWTexturesPath("BoatF3.png"),
                             SWTexturesPath("BoatF3.png"),
                             SWTexturesPath("BoatF3.png"),
                             SWTexturesPath("BoatF4.png"),
                             SWTexturesPath("BoatF4.png"),
                             SWTexturesPath("BoatF4.png"),
                                    SWTexturesPath("BoatF5.png"),
                                    SWTexturesPath("BoatF5.png"),
                                    SWTexturesPath("BoatF5.png"),
                                    SWTexturesPath("BoatF6.png"),
                                    SWTexturesPath("BoatF6.png"),
                                    SWTexturesPath("BoatF6.png"),
                                    SWTexturesPath("BoatF7.png"),
                                    SWTexturesPath("BoatF7.png"),
                                    SWTexturesPath("BoatF7.png"),
                                    SWTexturesPath("BoatF8.png"),
                                    SWTexturesPath("BoatF8.png"),
                                    SWTexturesPath("BoatF8.png")}},
    {typeid(Bomb),          {SWTexturesPath("Bomb.png")}},
    {typeid(Torpedo),       {SWTexturesPath("Torpedo.png")}},
    {typeid(Missile),       {SWTexturesPath("Missile.png")}},
    {typeid(BoatMissile),   {SWTexturesPath("BoatMissile1.png"),
                                    SWTexturesPath("BoatMissile1.png"),
                                    SWTexturesPath("BoatMissile1.png"),
                                    SWTexturesPath("BoatMissile1.png"),
                                    SWTexturesPath("BoatMissile1.png"),
                                    SWTexturesPath("BoatMissile1.png"),
                                    SWTexturesPath("BoatMissile2.png"),
                                    SWTexturesPath("BoatMissile2.png"),
                                    SWTexturesPath("BoatMissile2.png"),
                                    SWTexturesPath("BoatMissile2.png"),
                                    SWTexturesPath("BoatMissile2.png"),
                                    SWTexturesPath("BoatMissile2.png"),}},
    {typeid(BuyLives),   {SWTexturesPath("BuyLives.png")}},
    {typeid(BuyMissiles),   {SWTexturesPath("BuyMissiles.png")}},
    {typeid(EndStore),   {SWTexturesPath("EndStore.png")}},
    {typeid(Crosshair),   {SWTexturesPath("Crosshair.png")}},
    {typeid(Submarine),     {SWTexturesPath("Submarine.png")}},
    {typeid(SubmarineI),     {SWTexturesPath("SubmarineTypeI.png")}},
    {typeid(SubmarineII),     {SWTexturesPath("SubmarineTypeII.png")}},
    {typeid(SubmarineIII),     {SWTexturesPath("SubmarineTypeIII.png")}},
    {typeid(Ocean),         {SWTexturesPath("FakeOcean.png")}},
    {typeid(Fish),          {SWTexturesPath("Fish.png")}},
    {typeid(Smoke),         {SWTexturesPath("Smoke.png")}},
    {typeid(Explosion),     {SWTexturesPath("Expl1.png"),
                                    SWTexturesPath("Expl2.png"),
                                    SWTexturesPath("Expl3.png"),
                                    SWTexturesPath("Expl4.png"),
                                    SWTexturesPath("Expl5.png"),
                                    SWTexturesPath("Expl6.png"),
                                    SWTexturesPath("Expl7.png"),
                                    SWTexturesPath("Expl8.png"),
                                    SWTexturesPath("Expl9.png"),
                                    SWTexturesPath("Expl10.png"),
                                    SWTexturesPath("Expl11.png"),
                                    SWTexturesPath("Expl12.png"),
                                    SWTexturesPath("Expl13.png"),
                                    SWTexturesPath("Expl14.png"),
                                    SWTexturesPath("Expl15.png"),}},
    {typeid(BoatLives),     {SWTexturesPath("BoatLives.png")}},
    {typeid(BoatMissiles),     {SWTexturesPath("MissileIndicator.png")}},
    {typeid(IntroUI),       {SWTexturesPath("IntroUI.png")}},
    {typeid(OutroUI),       {SWTexturesPath("OutroUI.png")}}
};

/// Private shader paths map
SpriteFactory::ShaderPathsMap SpriteFactory::shaderPathsMap =
{
    {typeid(Character), std::make_pair(SWShaderPath("textured.vs.glsl"), SWShaderPath("character.fs.glsl"))}
};

/// Default shader paths
constexpr std::pair<const char*, const char*> SpriteFactory::defaultShaderPaths;

/// Get the shared instance
SpriteFactory* SpriteFactory::shared()
{
    if (SpriteFactory::instance == nullptr)
    {
        SpriteFactory::instance = new SpriteFactory();
    }
    
    return SpriteFactory::instance;
}

/// Private constructor
SpriteFactory::SpriteFactory()
{
    // Initialize the FreeType library
    if (FT_Init_FreeType(&this->ftlibrary) != 0)
    {
        pserror("Failed to initialize the FreeType library.");
        
        pserror("All subsequent FreeType calls might fail.");
    }
}

///
/// Make the sprite for Character entity type
/// @param sprite The sprite created on return
/// @param info Additional information needed to make the sprite (See the note below)
/// @return `true` on a successful creation, `false` otherwise.
/// @note The caller could easily make the sprite for a type by invoking make<Character>().
/// @note `info` must be a **non-null** pointer to a `struct Character::Attribute` type.
///
template <> // Concrete: Provides a different implementation for `Character` entity
bool SpriteFactory::make<Character>(Sprite* sprite, void* info)
{
    // Guard: The given info must be non-null
    if (info == nullptr)
    {
        pserror("API Usage Error: Must provide a non-null pointer to a Character::Attribute struct.");
        
        return false;
    }
    
    // Retrieve the character and font info
    auto attribute = reinterpret_cast<Character::Attribute*>(info);
    
    auto& face = this->fontFaceMap[attribute->font];
    
    // Guard: Check the shared font face cache
    if (face == nullptr)
    {
        // No cached font face found
        // Guard: Initialize the face for the request font type
        if (FT_New_Face(this->ftlibrary, Character::pathForFont(attribute->font), 0, &face) != 0)
        {
            pserror("Failed to load the face for the given font.");
            
            return false;
        }
    }
    
    passert(face != nullptr, "Face for the given font should be non-null now.");
    
    // Guard: Set the font size
    if (FT_Set_Pixel_Sizes(face, attribute->pixelSize.width, attribute->pixelSize.height) != 0)
    {
        pserror("Failed to set the font size.");
        
        return false;
    }
    
    passert(attribute->pixelSize.width == 0, "API Usage Error: Font width is not supported. Set the height instead.");
    
    // Retrieve the cached texture for this combination of font and size
    uint64_t key = ((uint64_t) static_cast<std::underlying_type_t<Character::Font>>(attribute->font) << 32) | ((uint32_t) attribute->pixelSize.height);
    
    auto& textures = this->characterTextureMap[key];
    
    auto& texture = textures[attribute->character];
    
    // Guard: Check the shared texture cache
    if (!texture.isValid())
    {
        // No cached texture for this combination of font and size
        // Guard: Load the texture for the requested character
        if (!texture.loadFromFace(attribute->character, face))
        {
            pserror("Failed to load the character texture from the font face.");
            
            return false;
        }
    }
    
    // Populate the character attributes
    attribute->size.width = face->glyph->bitmap.width;
    
    attribute->size.height = face->glyph->bitmap.rows;
    
    attribute->bearing.x = face->glyph->bitmap_left;
    
    attribute->bearing.y = face->glyph->bitmap_top;
    
    attribute->advance = (uint32_t) face->glyph->advance.x;
    
    // Initialize the sprite
    return sprite->initFromTexture(texture, SpriteFactory::shaderPathsForType(typeid(Character)));
}
