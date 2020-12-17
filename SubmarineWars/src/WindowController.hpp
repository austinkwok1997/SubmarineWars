//
//  WindowController.hpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-10-04.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#ifndef WindowController_hpp
#define WindowController_hpp

#include "Foundations/Foundations.hpp"

/// A controller that manages the main window, screen, framebuffer, etc.
class WindowController
{
private:
    /// The main window
    GLFWwindow* window;
    
    /// The main framebuffer
    GLuint framebuffer;
    
    /// The requested screen size
    ScreenSize reqScreenSize;
    
    /// The actual screen size
    ScreenSize actScreenSize;
    
    /// The actual screen scale
    float screenScale;
    
    /// The screen texture
    ScreenTexture screenTexture;
    
    /// Private constructor
    WindowController();
    
    /// Private destructor
    ~WindowController();
    
public:
    ///
    /// [Convenient] Create a window controller
    ///
    /// @param title The window title
    /// @param size The window size
    /// @return A non-null pointer to the newly created window controller on success, `nullptr` otherwise.
    ///
    static WindowController* create(const char* title, ScreenSize& size);
    
    ///
    /// [Convenient] Destroy the given window controller
    ///
    /// @param controller A non-null window controller instance created by WindowController::create().
    ///
    static void destory(WindowController* controller);
    
    // MARK:- Query the window properties
    
    ///
    /// [Fast] Get the main window handle
    ///
    inline GLFWwindow* getMainWindow() { return this->window; }
    
    ///
    /// [Fast] Get the main framebuffer
    ///
    inline GLuint getFramebuffer() { return this->framebuffer; }
    
    ///
    /// [Fast] Get the requested screen size
    ///
    /// @note On a non-HiDPI display, the requested screen size is identical to the actual screen size.
    ///
    inline ScreenSize& getRequestedScreenSize() { return this->reqScreenSize; }
    
    ///
    /// [Fast] Get the actual screen size
    ///
    /// @note On a non-HiDPI display, the requested screen size is identical to the actual screen size.
    ///
    inline ScreenSize& getActualScreenSize() { return this->actScreenSize; }
    
    ///
    /// [Fast] Get the screen scale
    ///
    inline float getScreenScale() { return this->screenScale; }
    
    ///
    /// [Fast] Get the screen texture
    ///
    inline ScreenTexture& getScreenTexture() { return this->screenTexture; }
};

#endif /* WindowController_hpp */
