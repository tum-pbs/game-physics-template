#pragma once
#include "Renderer.h"

/// @brief Scene base class. **Run `cmake . -B build` after adding new files to the scenes folder**
///
/// This class is the base class for all scenes in the application.
/// When creating a new scene, derive from this class.
/// Add the new scene to the SceneIndex.h file.
///
/// Feel free to modify the non virtual functions for functionality shared between scenes.
class Scene
{
public:
    /// @brief Initialize the scene. Gets called every time the scene is switched to.
    virtual void init() {};
    /// @brief Simulate a step in the scene. Gets called every frame before onDraw.
    ///
    /// This is where you should update the physics of the scene.
    virtual void simulateStep() {};
    /// @brief Draw the scene. Gets called every frame after simulateStep.
    ///
    /// This is where you should call the Renderer draw functions.
    virtual void onDraw(Renderer &renderer);
    /// @brief Define the GUI for the scene. Gets called every frame after onDraw.
    virtual void onGUI() {};
    virtual ~Scene() = default;
    virtual void onKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods){}
};
