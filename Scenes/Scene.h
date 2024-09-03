#pragma once
#include "Renderer.h"
#include "Grid.h"

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
    virtual void init();
    /// @brief Simulate a step in the scene. Gets called every frame before onDraw.
    ///
    /// This is where you should update the physics of the scene.
    virtual void simulateStep() {};
    /// @brief Draw the scene. Gets called every frame after simulateStep.
    ///
    /// This is where you should call the Renderer draw functions.
    virtual void onDraw(Renderer &renderer);
    /// @brief Define the GUI for the scene. Gets called every frame after onDraw.
    virtual void onGUI();
    virtual ~Scene() = default;
    Grid grid = Grid(0, 0);

protected:
    void randomInit(size_t width, size_t height, size_t depth = 1);
    float alpha = 1;
    float dt = 0.01;
    int drawRadius = 5;
    size_t resolution = 50;
    void resetGrid();
};
