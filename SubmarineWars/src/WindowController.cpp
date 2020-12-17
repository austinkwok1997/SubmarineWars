//
//  WindowController.cpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-04.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#include "WindowController.hpp"

/// Private constructor
WindowController::WindowController()
{
    
}

/// Private destructor
WindowController::~WindowController()
{
    // TODO: Release the resource
}

///
/// [Convenient] Create a window controller
///
/// @param title The window title
/// @param size The window size
/// @return A non-null pointer to the newly created window controller on success, `nullptr` otherwise.
///
WindowController* WindowController::create(const char* title, ScreenSize& size)
{
    // Guard: Create the window controller instance
    WindowController* instance = new WindowController();
    
    if (instance == nullptr)
    {
        pserror("Failed to create the window controller: Insufficient memory.");
        
        return nullptr;
    }
    
    // Setup the main window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 0);
    
    // Create the main window
    instance->window = glfwCreateWindow(size.width, size.height, title, nullptr, nullptr);
    
    if (instance->window == nullptr)
    {
        pserror("Failed to create the main window.");
        
        delete instance;
        
        return nullptr;
    }
    
    glfwMakeContextCurrent(instance->window);
    glfwSwapInterval(1); // vsync
    
    // Load OpenGL function pointers
    gl3w_init();
    
    // Create the framebuffer
    instance->framebuffer = 0;
    
    glGenFramebuffers(1, &instance->framebuffer);
    
    glBindFramebuffer(GL_FRAMEBUFFER, instance->framebuffer);
    
    // Handle the HiDPI display
    glfwGetFramebufferSize(instance->window, &instance->actScreenSize.width, &instance->actScreenSize.height);
    
    instance->screenScale = instance->actScreenSize.width / size.width;
    
    // Initialize the screen texture
    instance->screenTexture.createFromScreen(instance->window);
    
    // Save the requested screen size
    instance->reqScreenSize = size;
    
    return instance;
}

///
/// [Convenient] Destroy the given window controller
///
/// @param controller A non-null window controller instance created by WindowController::create().
///
void WindowController::destory(WindowController* controller)
{
    
}
