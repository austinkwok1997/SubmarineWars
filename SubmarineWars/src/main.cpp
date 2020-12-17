//
//  main.cpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-09-27.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#include <iostream>
#include <chrono>

#define GL3W_IMPLEMENTATION
#include <gl3w.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include "Foundations/Foundations.hpp"
#include "World.hpp"

using Clock = std::chrono::high_resolution_clock;

int main(int argc, const char * argv[])
{
    World world;
    
    ScreenSize size = { 1280, 720 };
    
    if (!world.init(size))
    {
        pserror("Failed to initialize the game world.");
        
        return EXIT_FAILURE;
    }
    //printf("Boat mass is now %f\n\n", world.entityManager->componentsForType<Physics>()[0].mass);
    
    auto t = Clock::now();

    //printf("First world update. ");

    // variable timestep loop.. can be improved (:
    while (!world.isOver())
    {
        // Processes system messages, if this wasn't present the window would become unresponsive
        glfwPollEvents();
        
        // Calculating elapsed times in milliseconds from the previous iteration
        auto now = Clock::now();
        
        float elapsed_sec = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
        
        t = now;
        //printf("Boat mass or bass if you will is %f before update, and ", world.entityManager->componentsForType<Physics>()[0].mass);
        world.update(elapsed_sec);
        //printf(" %f after. \n\n", world.entityManager->componentsForType<Physics>()[0].mass);

    }
    
    world.destroy();
    
    return 0;
}
