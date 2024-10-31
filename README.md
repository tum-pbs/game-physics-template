
# The Game Physics Template - An Introduction
## Software Setup:
1. install [CMake](https://cmake.org/download/)
2. install a C++ compiler (if you don't have one already), for example:

    - Windows: [Visual Studio](https://visualstudio.microsoft.com/downloads/)
    - macOS: [Xcode](https://developer.apple.com/xcode/)
    - Linux: [GCC](https://gcc.gnu.org/)  

    ... or whatever you prefer

3. install [Git](https://git-scm.com/downloads)
4. clone this repository
    ```
    git clone https://github.com/tum-pbs/game-physics-template.git
    ```
5. Open the project in your IDE, for example [Visual Studio Code]
(https://code.visualstudio.com/), and add [src](src) and [thirdparty](thirdparty) to the include directories. This is just for autocomplete. The build will work without it.

## Building and Running the Project
1. Everytime you add new files, and once in the beginning, run
```
cmake . -B build
```
2. Build the project
```
cmake --build build
```
The output will tel you where the executable is located. You can run it from the command line or from your IDE.

# Project Structure
Each exercise has its own branch, usually only providing some additional code needed for the exercise.  
The intro branch already has the completed tutorial code, so you can start from the main branch to go along.

The relevant directories:
- [Scenes](Scenes) should contain all code relevant to your submission.
- [src](src) contains the engine source code. Feel free to take a look and play around with it. Make sure your submissions run without changes to theses files.
- [thirdparty](thirdparty) contains external libraries, including `glm` for vector math and `imgui` for UI.
- [resources](resources) contains all resources loaded at runtime, such as shaders and colormaps. Especially the postprocessing shader is fun to play with.


# Basics
## `glm` math library
This library is used for all vector, matrix and quaternion math.  

Include `<glm/glm.hpp>` to get started.  

All vectors and matrices have loads of useful overloads, such as `glm::vec3(0)` for a zero vector. `vec4` can be implicitly cast to `vec3` and `vec2`, and so on.  

### Useful knowledge:
- The quaternion constructor is `glm::quat(w,x,y,z)`, while the list initializer is `glm::quat test = {x,y,z,w}`
- You can include `<glm/gtx/string_cast.hpp>` to print vectors, matrices and quaternions with `glm::to_string(vec)`
- The `glm::` namespace contains many useful functions, such as `glm::length(vec)`, `glm::normalize(vec)`, `glm::dot(vec1, vec2)`, `glm::cross(vec1, vec2)`, `glm::inverse(matrix)`, `glm::transpose(matrix)` and many more.
- `glm` has math functions that work on vectors, e.g. `glm::sin(vec)`
- Instead of `using namespace glm`, you can use private `using vec3 = glm::vec3` in your class header to avoid conflicts with other libraries.

## `imgui` UI library
This library is used for all UI elements. It should only be used in the `onGui` method of a scene. 

Include `<imgui.h>` to get started.

### Useful knowledge:
- You can use logarithmic sliders with `ImGui::SliderFloat("Slider", &value, min, max, "%.3f", ImGuiSliderFlags_Logarithmic)`
- ImGui has many functions for interactiviy, such as `ImGui::IsKeyDown()`, `ImGui::GetMousePos()` and many more.
- `ImGui::GetIO()` contains more information about the input, even `PenPressure` and `DisplaySize`.
- Use `using namespace ImGui::` **inside** the `onGui` method to avoid writing `ImGui::` all the time. 

## Drawing objects
The draw methods are documented in the [Renderer documantation](https://davidxdydz.github.io/GamephysicsDocs/class_renderer.html#details).
The `renderer` object is provided in the onDraw method of a scene.  
You can use it to draw lines, ellipsoids, spheres, cubes, quads, and images.

### You have to keep track of the objects yourself!

How you do this is up to you.  
A memory safe way to do this would be to store the objects in a vector in your scene class. Initialize the objects in `init()`, modify them in the `simulateStep()` method, and visualize them in the `onDraw()` method.
```cpp
#include <vector>
#include <glm/glm.hpp>
std::vector<YourObject> objects;
// or
std::vector<glm::vec3> positions;
// ... or whatever you need
```

### Drawing Images
You can draw Images with the `drawImage` method.
```cpp
void onDraw(Renderer &renderer) override {
    // the data should not be declared in the draw method
    std::vector<float> data = {1,0,0,0,1,0};
    int width = 3;
    int height = 2;
    renderer.drawImage(data, height, width);
    // This will draw an image to the full screen looking like this:
    // 1 0 0
    // 0 1 0
    // The colors are defined by the optional colormap parameter.
    // Note that the pixels will be stretched, if the aspect ratio of the image does not match the aspect ratio of the screen.
}
```
## Adding a new scene or source file

### Add scene:
1. Create a new class that inherits from `Scene` in the [Scenes](Scenes) directory.
2. Add the scene to the `SceneIndex` in [Scenes/SceneIndex.h](Scenes/SceneIndex.h).
```cpp
{"The Scene's Name", creator<TheSceneClass>()},
```
3. Run `cmake . -B build` again before building the project.
4. override only the functions you need and implement them in the scene's source file.
```cpp
...
public:
    void onDraw(Renderer &renderer) override;
    void onGui() override;
```
5. After building, you can select the scene in the UI.

### Add source file:
1. Add the source file to the [Scenes](Scenes) directory.
2. Run `cmake . -B build` again before building the project.

## Further reading:

If you would like to know more, or work through a demo example look at our guide in this repository, which you can find in the [guide.md file](https://github.com/tum-pbs/game-physics-template/blob/main/guide.md). This guide works through input handling, extending scenes and further details like adaptive timestepping and will be updated over time too.