//
//  World.cpp
//  SubmarineWars
//
//  Created by FireWolf on 2019-09-27.
//  Copyright © 2019 FireWolf. All rights reserved.
//

#include "World.hpp"
#include "Foundations/Foundations.hpp"
#include "Entities/Submarine.hpp"
#include "Sounds/SoundPlayer.hpp"
#include <iostream>
#include <fstream>

using JSON = nlohmann::json;

static void glfwErrorCallback(int error, const char* description)
{
    pserror("OpenGL Error %d: %s", error, description);
}

bool World::init(ScreenSize size)
{
    // Initialize OpenGL and GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    
    if (!glfwInit())
    {
        pserror("Failed to initialize GLFW.");
        return false;
    }
        
    // Initialize the shared sound player
    if (!SoundPlayer::sharedInit())
    {
        pserror("Failed to initialize the shared sound player.");
        
        return false;
    }
    
    // Initialize the window controller
    this->windowController = WindowController::create("Submarine Wars", size);
    
    if (this->windowController == nullptr)
    {
        pserror("Failed to initialize the window controller.");
        
        return false;
    }
    
    // Setup the callback functions
    GLFWwindow* window = this->windowController->getMainWindow();
    
    glfwSetWindowUserPointer(this->windowController->getMainWindow(), this);
    
    auto movcb = [](GLFWwindow* window, double xpos, double ypos)
    {
        ((World*) glfwGetWindowUserPointer(window))->onMouseMoveEvent(window, xpos, ypos);
    };
    
    auto mobcb = [](GLFWwindow* window, int button, int action, int mods)
    {
        ((World*) glfwGetWindowUserPointer(window))->onMouseButtonEvent(window, button, action, mods);
    };
    
    glfwSetCursorPosCallback(window, movcb);
    
    glfwSetMouseButtonCallback(window, mobcb);
    
    // Initialize the entity manager
    this->entityManager = new EntityManager();
    
    if (this->entityManager == nullptr)
    {
        pserror("Failed to initialize the entity manager.");
        
        return false;
    }
    
    // Initialize the stage controller
    this->stageController = new StageController(this->entityManager);
    
    if (this->stageController == nullptr)
    {
        pserror("Failed to initialize the stage controller.");
        
        return false;
    }
    
    // Initialize systems
    this->renderSystem = new RenderSystem(Components::makeBitMap<Sprite, Color, Position, Rotation, Physics>(), this->entityManager, this->windowController);
    
    if (this->renderSystem == nullptr)
    {
        pserror("Failed to initialize the render system.");
        
        return false;
    }
    
    this->motionSystem = new MotionSystem(Components::makeBitMap<Position, Velocity>(), this->entityManager);
    
    if (this->motionSystem == nullptr)
    {
        pserror("Failed to initialize the motion system.");
        
        return false;
    }
    
    this->inputSystem = new InputSystem(Components::makeBitMap<Input>(), this->entityManager, this->stageController);
    
    if (this->inputSystem == nullptr)
    {
        pserror("Failed to initialize the input system.");
        
        return false;
    }
    
    this->collisionSystem = new CollisionSystem(Components::makeBitMap<Collision, Velocity, Position>(), this->entityManager, this->stageController);
    
    if (this->collisionSystem == nullptr)
    {
        pserror("Failed to initialize the collision system.");
        
        return false;
    }

    this->attackSystem = new AttackSystem(Components::makeBitMap<Attack>(), this->entityManager, this->stageController, this->windowController);

    if (this->attackSystem == nullptr)
    {
        pserror("Failed to initialize the attack system.");

        return false;
    }

    this->pathingSystem = new PathingSystem(Components::makeBitMap<Pathing, Position, Rotation>(), this->entityManager);

    if (this->pathingSystem == nullptr)
    {
        pserror("Failed to initialize the animation system.");

        pserror("Failed to initialize the pathing system.");

        return false;
    }

    this->animationSystem = new AnimationSystem(Components::makeBitMap<Sprite, Animation>(), this->entityManager);

    if (this->animationSystem == nullptr)
    {
        pserror("Failed to initialize the animation system.");

        return false;
    }
    
    // Register systems with the entity manager
    this->entityManager->registerDelegates(this->renderSystem,
                                           this->motionSystem,
                                           this->inputSystem,
                                           this->collisionSystem,
                                           this->attackSystem,
                                           this->pathingSystem,
                                           this->animationSystem);
    
    // Setup the background ocean
    if (!this->entityManager->setupOcean())
    {
        pserror("Failed to set up the ocean.");

        return false;
    }

    // Setup the player boat
    if (!this->entityManager->resetBoat())
    {
        pserror("Failed to set up the player boat.");
        
        return false;
    }
    //printf("Setup player boat. mass is %f\n\n", this->entityManager->componentsForType<Physics>()[0].mass);
    
    // Setup the player boat lives indicator
    if (!this->entityManager->setupBoatLivesIndicator())
    {
        pserror("Failed to set up the boat lives indicator.");
        
        return false;
    }

    // Setup the player boat missiles indicator
    if (!this->entityManager->setupBoatMissilesIndicator()) {
        pserror("Failed to set up the boat missiles indicator.");

        return false;
    }

    // Setup the score label
    if (!this->entityManager->setupScoreLabel())
    {
        pserror("Failed to set up the score label.");
        
        return false;
    }

    // Setup the money label
    if (!this->entityManager->setupMoneyLabel())
    {
        pserror("Failed to set up the money label.");
        
        return false;
    }

    // Setup the stage label
    if (!this->entityManager->setupStageLabel())
    {
        pserror("Failed to set up the stage label.");
        
        return false;
    }

    // Set up the intro UI
    if(!this->entityManager->setupIntroUI())
    {
        pserror("Failed to set up the intro UI.");
        
        return false;
    }

    // Setup

    lKey = false;
    rKey = false;
    mouseIsOverNewGame = false;
    
    passert(SoundPlayer::shared()->playBackgroundMusic(), "Failed to play the BGM.");

    return true;
}

///
/// Destory the game world and release allocated resources
///
void World::destroy()
{
    // TODO: IMP THIS
    delete this->entityManager;
    
    delete this->stageController;
    
    delete this->renderSystem;
    
    delete this->motionSystem;
    
    delete this->inputSystem;
    
    delete this->collisionSystem;

    delete this->attackSystem;

    delete this->pathingSystem;

    delete this->animationSystem;
    
    SoundPlayer::sharedFinalize();
    
    WindowController::destory(this->windowController);
}

///
/// Ticks the game ahead by the given milliseconds
///
/// @param ms The elapsed time since the last tick
/// @return `true` on a successful tick, `false` otherwise.
///
bool World::update(float ms)
{
    // TODO: IMP THIS
    if(!this->entityManager->checkIfGameOver())
    {
        this->stageController->update(ms);
    } else
    {
        this->stageController->signalGameActive(false);
    }
    this->motionSystem->update(ms);

    this->inputSystem->update(ms);

    this->collisionSystem->update(ms);

    this->renderSystem->update(ms);

    this->attackSystem->update(ms);

    if(!this->entityManager->checkIfGameOver())
    {
        this->pathingSystem->update(ms);
    }
    
    this->animationSystem->update(ms);

    return true;
}

void World::saveGame()
{
    SaveGame saveGameData;
    // Save data from the StageController
    stageController->saveGame(&saveGameData.scData);
    
    // Save data from the EnitityManager
    entityManager->saveGame(&saveGameData.emData);
    
    char* path = new char[1024]();
    
    snprintf(path, 1024, "%s/SaveData.json", SWDataPath);
    
    // Write a new save file
    std::ofstream sfile(path);
    
    delete [] path;
    
    if (!sfile.good())
    {
        pserror("Failed to save the game");
        
        sfile.close();
        
        return;
    }
    
    JSON saveFile;
    
    saveFile["stage"] = saveGameData.scData.stage;
    saveFile["subsDead"] = saveGameData.scData.subsDead;
    saveFile["totalSubs"] = saveGameData.scData.totalSubs;
    saveFile["score"] = saveGameData.scData.score;
    saveFile["money"] = saveGameData.scData.money;
    saveFile["lives"] = saveGameData.scData.lives;
    saveFile["resSubCountsI"] = saveGameData.scData.resSubCountsI;
    saveFile["resSubCountsII"] = saveGameData.scData.resSubCountsII;
    saveFile["resSubCountsIII"] = saveGameData.scData.resSubCountsIII;
    saveFile["resSubCountsSPEC"] = saveGameData.scData.resSubCountsSPEC;
    
    saveFile["boatId"] = saveGameData.emData.boatId;
    
    JSON fishJSON = JSON::array();
    JSON bombJSON = JSON::array();
    
    JSON positionJSON = JSON::array();
    JSON velocityJSON = JSON::array();
    JSON rotationsJSON = JSON::array();
    JSON physicsJSON = JSON::array();
    JSON scoresJSON = JSON::array();
    JSON collisionsJSON = JSON::array();
    JSON attacksJSON = JSON::array();
    JSON missileJSON = JSON::array();
    JSON torpJSON = JSON::array();
    JSON subIJSON = JSON::array();
    JSON subIIJSON = JSON::array();
    JSON subIIIJSON = JSON::array();
    JSON subSPECJSON = JSON::array();
    
    
    char* Val1 = new char[1024]();
    char* Val2 = new char[1024]();
    char* Val3 = new char[1024]();
    char* Val4 = new char[1024]();
    char* Val5 = new char[1024]();
    
    for(auto fish: saveGameData.emData.fishes)
    {
        snprintf(Val1, 1024, "%d", fish);
        fishJSON.push_back(Val1);
    }
    saveFile["Fishes"] = fishJSON;
    
    for(auto bomb: saveGameData.emData.bombs)
    {
        snprintf(Val1, 1024, "%d", bomb);
        bombJSON.push_back(Val1);
    }
    saveFile["Bombs"] = bombJSON;
    
    for(auto missile: saveGameData.emData.missiles)
    {
        snprintf(Val1, 1024, "%d", missile);
        missileJSON.push_back(Val1);
    }
    saveFile["Missiles"] = missileJSON;
    
    for(auto torp: saveGameData.emData.torpedoes)
    {
        snprintf(Val1, 1024, "%d", torp);
        torpJSON.push_back(Val1);
    }
    saveFile["Torpedos"] = torpJSON;
    
    for(auto sub: saveGameData.emData.submarines[0])
    {
        snprintf(Val1, 1024, "%d", sub);
        subIJSON.push_back(Val1);
    }
    saveFile["SubI"] = subIJSON;
    
    for(auto sub: saveGameData.emData.submarines[1])
    {
        snprintf(Val1, 1024, "%d", sub);
        subIIJSON.push_back(Val1);
    }
    saveFile["SubII"] = subIIJSON;
    
    for(auto sub: saveGameData.emData.submarines[2])
    {
        snprintf(Val1, 1024, "%d", sub);
        subIIIJSON.push_back(Val1);
    }
    saveFile["SubIII"] = subIIIJSON;
    
    for(auto sub: saveGameData.emData.submarines[3])
    {
        snprintf(Val1, 1024, "%d", sub);
        subSPECJSON.push_back(Val1);
    }
    saveFile["SubSPEC"] = subSPECJSON;
    
    for(int i = 0; i < EntityManager::MAX_NUM_ON_SCREEN_ENTITIES; i++)
    {
        snprintf(Val1, 1024, "%f", saveGameData.emData.positions[i].x);
        snprintf(Val2, 1024, "%f", saveGameData.emData.positions[i].y);
        positionJSON.push_back({Val1,Val2});
        
        snprintf(Val1, 1024, "%f", saveGameData.emData.velocities[i].vx);
        snprintf(Val2, 1024, "%f", saveGameData.emData.velocities[i].vy);
        velocityJSON.push_back({Val1,Val2});
        
        snprintf(Val1, 1024, "%f", saveGameData.emData.rotations[i].radians);
        rotationsJSON.push_back(Val1);
        
        snprintf(Val1, 1024, "%f", saveGameData.emData.physics[i].force.x);
        snprintf(Val2, 1024, "%f", saveGameData.emData.physics[i].force.y);
        snprintf(Val3, 1024, "%f", saveGameData.emData.physics[i].mass);
        snprintf(Val4, 1024, "%f", saveGameData.emData.physics[i].scale.x);
        snprintf(Val5, 1024, "%f", saveGameData.emData.physics[i].scale.y);
        physicsJSON.push_back({Val1,Val2,Val3,Val4,Val5});
        
        snprintf(Val1, 1024, "%u", saveGameData.emData.scores[i].score);
        scoresJSON.push_back(Val1);
        
        snprintf(Val1, 1024, "%u", saveGameData.emData.collisions[i].type);
        collisionsJSON.push_back(Val1);
        
        snprintf(Val1, 1024, "%u", saveGameData.emData.attacks[i].type);
        attacksJSON.push_back(Val1);
    }
    delete [] Val1;
    delete [] Val2;
    delete [] Val3;
    delete [] Val4;
    delete [] Val5;
    
    saveFile["Positions"] = positionJSON;
    saveFile["Velocities"] = velocityJSON;
    saveFile["Rotations"] = rotationsJSON;
    saveFile["Physics"] = physicsJSON;
    saveFile["Scores"] = scoresJSON;
    saveFile["Collisions"] = collisionsJSON;
    saveFile["Attack"] = attacksJSON;
    saveFile["HasSavedData"] = true;

    sfile << saveFile << std::endl;
    
    sfile.close();
}

bool World::ReadSaveFromFile(SaveGame* saveData)
{
    char* path = new char[1024]();
    
    snprintf(path, 1024, "%s/SaveData.json", SWDataPath);
    
    // Write a new save file
    std::ifstream sfile(path);
    
    delete [] path;
    
    if (!sfile.good())
    {
        pserror("No saved game found");
        
        sfile.close();
        
        return false;
    }
    
    JSON saveFile;
    sfile >> saveFile;
    
    try
    {
        if(saveFile["HasSavedData"] == true) {
            // There is saved data to load, lets load it

            // Stage controller data
            saveData->scData.lives = saveFile["lives"];
            saveData->scData.money = saveFile["money"];
            saveData->scData.resSubCountsI = saveFile["resSubCountsI"];
            saveData->scData.resSubCountsII = saveFile["resSubCountsII"];
            saveData->scData.resSubCountsIII = saveFile["resSubCountsIII"];
            saveData->scData.resSubCountsSPEC = saveFile["resSubCountsSPEC"];
            saveData->scData.score = saveFile["score"];
            saveData->scData.stage = saveFile["stage"];
            saveData->scData.subsDead = saveFile["subsDead"];
            saveData->scData.totalSubs = saveFile["totalSubs"];
            
            // EnitityManager data
            saveData->emData.boatId = saveFile["boatId"];
        
            for(int i = 0; i < EntityManager::MAX_NUM_ON_SCREEN_ENTITIES; i++)
            {
                /*
                 * Get the position array
                 */
                auto s = saveFile["Positions"][i][0].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                float x = std::stof(s);
                s = saveFile["Positions"][i][1].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                float y = std::stof(s);
                saveData->emData.positions[i] = {x,y};
            
                /*
                 * Get the velocities array
                 */
                s = saveFile["Velocities"][i][0].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                x = std::stof(s);
                s = saveFile["Velocities"][i][1].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                y = std::stof(s);
                saveData->emData.velocities[i] = {x,y};
            
                /*
                 * Get the rotations array
                 */
                s = saveFile["Rotations"][i].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                float r = std::stof(s);
                saveData->emData.rotations[i].radians = r;
            
                /*
                 * Get the Physics array
                 */
                s = saveFile["Physics"][i][0].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                x = std::stof(s);
                s = saveFile["Physics"][i][1].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                y = std::stof(s);
                saveData->emData.physics[i].scale = {x,y};
                s = saveFile["Physics"][i][2].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                float m = std::stof(s);
                saveData->emData.physics[i].mass = m;
                s = saveFile["Physics"][i][3].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                x = std::stof(s);
                s = saveFile["Physics"][i][4].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                y = std::stof(s);
                saveData->emData.physics[i].force = {x,y};
            
                /*
                 * Get the scores array
                 */
                s = saveFile["Scores"][i].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                int score = std::stoi(s);
                saveData->emData.scores[i].score = score;
            
                /*
                 * Get the collision array
                 */
                s = saveFile["Collisions"][i].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                int t = std::stoi(s);
                if(t >= Collision::NUM_ENTITIES){
                    passert(false, "Save file (Collision array) corrupted, value >= max value of Collision::NUM_ENTITIES");
                    return false;
                }
                saveData->emData.collisions[i].type = (Collision::eType)t;
            
                /*
                 * Get the attack array
                 */
                s = saveFile["Attack"][i].dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                int a = std::stoi(s);
                if(a > Attack::missile){
                    passert(false, "Save file (attack array) corrupted, value exceeds max value of Attack::missile");
                    return false;
                }
                saveData->emData.attacks[i].type = (Attack::aType)a;
            }
                
            for(auto bomb: saveFile["Bombs"])
            {
                auto s = bomb.dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                Entity::Identifier e = (Entity::Identifier)std::stoi(s);
                saveData->emData.bombs.insert(e);
            }
            
            for(auto torp: saveFile["Torpedos"])
            {
                auto s = torp.dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                Entity::Identifier e = (Entity::Identifier)std::stoi(s);
                saveData->emData.torpedoes.insert(e);
            }
                
            for(auto fish: saveFile["Fishes"])
            {
                auto s = fish.dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                Entity::Identifier e = (Entity::Identifier)std::stoi(s);
                saveData->emData.fishes.insert(e);
            }
                
            for(auto missile: saveFile["Missiles"])
            {
                auto s = missile.dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                Entity::Identifier e = (Entity::Identifier)std::stoi(s);
                saveData->emData.missiles.insert(e);
            }
                
            for(auto sub: saveFile["SubI"])
            {
                auto s = sub.dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                Entity::Identifier e = (Entity::Identifier)std::stoi(s);
                saveData->emData.submarines[0].insert(e);
            }
                
            for(auto sub: saveFile["SubII"])
            {
                auto s = sub.dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                Entity::Identifier e = (Entity::Identifier)std::stoi(s);
                saveData->emData.submarines[1].insert(e);
            }
                
            for(auto sub: saveFile["SubIII"])
            {
                auto s = sub.dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                Entity::Identifier e = (Entity::Identifier)std::stoi(s);
                saveData->emData.submarines[2].insert(e);
            }
                
            for(auto sub: saveFile["SubSPEC"])
            {
                auto s = sub.dump().substr(1,100);
                s = s.substr(0, s.size()-1);
                Entity::Identifier e = (Entity::Identifier)std::stoi(s);
                saveData->emData.submarines[3].insert(e);
            }
        } else {
            // No saved data, nothing to load
            printf("No data to load\n");
            return false;
        }
    } catch (const std::exception) {
        sfile.close();
        printf("Data is corrupt\n");
        return false;
    }
        
    sfile.close();
    return true;
}

bool World::loadGame()
{
    SaveGame saveData;
    if(ReadSaveFromFile(&saveData))
    {
        /// Now we know we have good data we can procede to save it
        stageController->loadGame(saveData.scData);
        entityManager->loadGame(saveData.emData);
        
        entityManager->updateStageLabel(saveData.scData.stage);
        entityManager->updateMoneyLabel(saveData.scData.money);
        entityManager->updateScoreLabel(saveData.scData.score);
        return true;
    }
    SoundPlayer::shared()->playErrorSoundEffect();
    return false;
}

///
/// Check whether the game is over
///
/// @return `true` if the game is over and the window should close.
///
bool World::isOver() const
{
    return glfwWindowShouldClose(this->windowController->getMainWindow());
}

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
void World::onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // If key == d or right arrow key, move boats left
    if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) {
        if (action == GLFW_PRESS) {
            rKey = true;
            //this->inputSystem->doRightPress();
        } else if (action == GLFW_RELEASE) {
            rKey = false;
            //this->inputSystem->stopMoving();
        }
    } else if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT) {
        if (action == GLFW_PRESS) {
            lKey = true;
            //this->inputSystem->doLeftPress();
        } else if (action == GLFW_RELEASE) {
            lKey = false;
            //this->inputSystem->stopMoving();
        }
    } else if (key == GLFW_KEY_SPACE) {
        if (action == GLFW_PRESS) {
            this->inputSystem->doSpacePress();
        }
    } else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        saveGame();
    } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        loadGame();
    }

    if (lKey && rKey) {
        this->inputSystem->stopMoving();
    } else if (lKey) {
        this->inputSystem->doLeftPress();
    } else if (rKey) {
        this->inputSystem->doRightPress();
    } else {
        this->inputSystem->stopMoving();
    }
}

///
/// Callback function on a mouse move event
///
/// @param window The window that received the event
/// @param xpos The new cursor x-coordinate, relative to the left edge of the content area
/// @param ypos The new cursor x-coordinate, relative to the left edge of the content area
///
void World::onMouseMoveEvent(GLFWwindow* window, double xpos, double ypos)
{
    // TODO: IMP THIS
    mouseIsOverNewGame = false;
    if(xpos > 394.f && xpos < 628.f)
    {
        if(ypos > 517.f && ypos < 566.f)
        {
            mouseIsOverNewGame = true;
        }
    }
    
    mouseIsOverLoadGame = false;
    if(xpos > 652.5 && xpos < 887.5)
    {
        if(ypos > 517.f && ypos < 566.f)
        {
            mouseIsOverLoadGame = true;
        }
    }
}

///
/// Callback function on a mouse button event
///
/// @param window The window that received the event
/// @param button The mouse button that was pressed or released.
/// @param action One of button action `GLFW_PRESS` or `GLFW_RELEASE`
/// @param mods Bit field describing which modifier keys were held down
///
void World::onMouseButtonEvent(GLFWwindow* window, int button, int action, int mods)
{
    if (this->stageController->isGameActive() && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xPos;
        double yPos;
        glfwGetCursorPos(window, &xPos, &yPos);
        float xPosF = (float) xPos;
        float yPosF = (float) yPos;
        this->inputSystem->doLeftClick({xPosF, yPosF});
    }
    
    // If we are in the tutorial then we should exit as the user has clicked
    // NOTE: This line should come after the BoatMissile check above and
    //       before the new/load game check below
    this->stageController->exitTutorial();

    // TODO: IMP THIS
    if(!this->stageController->isGameActive() && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        if(mouseIsOverNewGame || mouseIsOverLoadGame)
        {
            passert(mouseIsOverNewGame != mouseIsOverLoadGame,
                    "Mouse cannot be over both buttons at the same time");
            
            if(mouseIsOverLoadGame)
            {
                // Load up the saved data if we can
                if(!this->loadGame())
                {
                    return;
                }
                // Signal for the game to start
                this->stageController->signalGameActive(true);
            }
            
            // Set up the key call back so key presses will start working
            GLFWwindow* window = this->windowController->getMainWindow();
            auto keycb = [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                ((World*) glfwGetWindowUserPointer(window))->onKeyEvent(window, key, scancode, action, mods);
            };
            glfwSetKeyCallback(window, keycb);
        
            // Remove intro UI so game can start
            this->entityManager->removeIntroUI();
            this->entityManager->removeOutroUI();
            
            if(mouseIsOverNewGame)
            {
                this->stageController->enterTutorial();
            }
        }
    }
}
