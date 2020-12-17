//
//  World.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-09-27.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef World_hpp
#define World_hpp

#include "Foundations/Foundations.hpp"
#include "Systems/RenderSystem.hpp"
#include "Systems/MotionSystem.hpp"
#include "Systems/InputSystem.hpp"
#include "Systems/CollisionSystem.hpp"
#include "Systems/AttackSystem.hpp"
#include "Systems/AnimationSystem.hpp"
#include "Systems/PathingSystem.hpp"
#include "WindowController.hpp"
#include "StageController.hpp"

/// Submarine Wars World
class World
{
public:
    ///
    /// Initialize the game world
    ///
    /// @param size The screen size (width, height)
    /// @return `true` on successfully initialized the world, `false` otherwise.
    ///
    bool init(ScreenSize size);
    
    ///
    /// Destory the game world and release allocated resources
    ///
    void destroy();
    
    ///
    /// Ticks the game ahead by the given milliseconds
    ///
    /// @param ms The elapsed time since the last tick
    /// @return `true` on a successful tick, `false` otherwise.
    ///
    bool update(float ms);
    
    ///
    /// Check whether the game is over
    ///
    /// @return `true` if the game is over and the window should close.
    ///
    bool isOver() const;

    EntityManager* entityManager;
    
private:
    /// The window controller
    WindowController* windowController;
    
    /// The render system
    RenderSystem* renderSystem;
    
    /// The motion system
    MotionSystem* motionSystem;
    
    /// The input system
    InputSystem* inputSystem;
    
    /// The collision system
    CollisionSystem* collisionSystem;

    /// The attack system
    AttackSystem* attackSystem;
    
    /// The pathing system
    PathingSystem* pathingSystem;
    
    /// The animation system
    AnimationSystem* animationSystem;
    
    /// The entity manager
    //TODO move this back here
    //EntityManager* entityManager;
    
    /// The stage controller
    StageController* stageController;

    /// Holding left key
    bool lKey;

    /// Holding right key
    bool rKey;
    
    /// Indicates the mouse is over the "new game" button on the intro UI
    bool mouseIsOverNewGame;
    
    /// Indicates the mouse is over the "load game" button on the intro UI
    bool mouseIsOverLoadGame;
    
    struct SaveGame
    {
        StageController::SC_SaveGameData scData;
        EntityManager::EM_SaveData emData;
    };
    
    void saveGame();
    
    bool ReadSaveFromFile(SaveGame* saveData);
    
    bool loadGame();

    //
    // MARK:- User Input Events Callbacks
    //
    
    ///
    /// Callback function on a key event
    ///
    /// @param window The window that received the event
    /// @param key The keyboard key that was pressed or released
    /// @param scancode The system-specific scancode of the key
    /// @param action One of key action `GLFW_PRESS`, `GLFW_RELEASE` or `GLFW_REPEAT`
    /// @param mods Bit field describing which modifier keys were held down
    ///
    void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
    
    ///
    /// Callback function on a mouse move event
    ///
    /// @param window The window that received the event
    /// @param xpos The new cursor x-coordinate, relative to the left edge of the content area
    /// @param ypos The new cursor x-coordinate, relative to the left edge of the content area
    ///
    void onMouseMoveEvent(GLFWwindow* window, double xpos, double ypos);
    
    ///
    /// Callback function on a mouse button event
    ///
    /// @param window The window that received the event
    /// @param button The mouse button that was pressed or released.
    /// @param action One of button action `GLFW_PRESS` or `GLFW_RELEASE`
    /// @param mods Bit field describing which modifier keys were held down
    ///
    void onMouseButtonEvent(GLFWwindow* window, int button, int action, int mods);
};

#endif /* World_hpp */
