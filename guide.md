# Demo Project for Game Physics

## Setting up the environment
Requirements:

- VsCode: https://code.visualstudio.com/
- Some C++ Compiler, e.g., [Visual Studio on Windows](https://visualstudio.microsoft.com/vs/community/), Xcode on MacOS, or GCC on Linux
- [Git](https://git-scm.com/)

Optionally if you want to be able to run cmake commands manually (you should not need to do this and if you do not know what this means you should not worry about it) you need to install CMake system wide from [here](https://cmake.org/)

Within VsCode you can then install the CMake and C++ Extensions from the extensions menu on the left, Git should be supported by default. To install them just search for the extension name, click on them and press install on the extension page, these are the two relevant extensions:

![alt text](https://github.com/user-attachments/assets/cbfb3374-ba4f-4d48-8ddf-8a9fb52b441c)

Next step is to download the framework. To do this clone the git repository from a terminal:

```
git clone https://github.com/tum-pbs/game-physics-template
```

And Open the folder in VsCode (Chose Open Folder... when launching VsCode and browse to the folder you cloned the repository into).

After doing this you should see a drop down by the CMake extension asking for a _kit_, which is a combination of compiler and build system, where you can pick which compiler you want to use. For Windows choose Visual Studio Community Release with x86_amd64 in the name or pick [Unspecified] to let CMake figure it out (should you get some strange linker errors pick the x86_amd64 one manually and make sure to not use the amd64_x86 as this can lead to issues on some systems):

![alt text](https://github.com/user-attachments/assets/64d42b7e-009f-4776-a0e7-3b93dee80af6)

If this dropdown did not appear or you did not manage to click on the right option, you can open this menu manually by searching for 'CMake: Select a Kit' in the command prompt (ctrl + shift + p hotkey on windows by default, command + shift + p on mac o) and then manually picking the option

![alt text](https://github.com/user-attachments/assets/d1eaf2ab-f7f0-4469-88b9-5bca6559d11a)

For MacOS choose Clang for your respective architecture, e.g., arm64 for M series processors (or leave it as [Unspecified]):
![alt text](https://github.com/user-attachments/assets/c05deee4-74ba-4379-9c42-011e32d97d44)

For Linux chose the newest version of your compiler (or leave it as unspecified):
![alt-text](https://github.com/user-attachments/assets/13a3ca61-b72c-4e76-bc4c-cb7d578ace66)

Doing this should open an output window at the bottom with CMake/Build selected and cmake will setup the kit, which will take a moment to complete

![alt text](https://github.com/user-attachments/assets/dc97b615-25c8-402f-8fbd-a25e05d8ae1f)

When this is done you can run 'CMake: configure' from the command prompt, which will create the build system, which should finish with a line similar to 'Build files have been written':

![alt text](https://github.com/user-attachments/assets/763e0cc8-e43d-47fd-8b33-895236faf4b8)

Now that the project is configured we can build the program using the F7 hotkey (CMake: Build) or Build and Launch (Ctrl + F5 | CMake: Run without Debugging), which will open the demo window in full screen and show something similar to this:

![alt text](https://github.com/user-attachments/assets/976e470c-fad2-4ab4-9e6f-ec230f565cb4)

## Drawing additional objects

For the structure of the project there are _Scenes_ within the _Scenes_ subfolder with a parent Scene in Scene.h/Scene.cpp, which is the default scene that should not be directly used in general but for simplicity we can look into the Scene.cpp file and find this:

```cpp
#include "Scene.h"
#include <imgui.h>

void Scene::onDraw(Renderer &renderer)
{
    renderer.drawWireCube(glm::vec3(0), glm::vec3(5), glm::vec3(1));
}
```

Which is a draw call that is called everytime a new frame is to be _rendered_ where we have created a wire cube at the origin (glm::vec3(x) creates a 3D vector with entries all set to x, i.e., [0,0,0] for this call), a scale of 5 in all directions and an all white color (colors are given as floats). If we want to render more we can simply add a new draw call below the drawWireCube function within the onDraw function:

```cpp

void Scene::onDraw(Renderer &renderer)
{
    renderer.drawWireCube(glm::vec3(0), glm::vec3(5), glm::vec3(1));
    renderer.drawCube(
        glm::vec3(0,0,0), // center position
        glm::angleAxis(glm::radians(45.0f), glm::vec3(1.0f / sqrtf(2.f), 1.0f / sqrtf(2.f), 0.0f)), // rotation
        glm::vec3(0.5,0.5,0.5), // scale
        glm::vec4(1,0,0,1)); // color
}
```

This new call will draw a cube at the origin, rotated by 45 degrees along some axis (note that glm expects angles to be in radians for this function, and the axis should be normalized, that being said, it works fine in most cases without normalization until it doesn't so be careful with functons that require an axis input!), with a scale of 0.5 in all directions and 100% red colored:

![alt text](https://github.com/user-attachments/assets/709e7e33-8501-4478-ab3c-994a645e28e8)

## To Fullscreen or not to fullscreen

A quick note: If you do not want to launch the program as full screen every time you need to modify the src/Renderer.cpp file and change the initWindowAndDevice function from this:

```cpp
void Renderer::initWindowAndDevice(){
    //...
    
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	window = glfwCreateWindow(640, 480, "Game Physics Template", NULL, NULL);
```

To this:
```cpp
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
	window = glfwCreateWindow(1280, 720, "Game Physics Template", NULL, NULL);
```
Where 1280 is the width of the window, 720 the height of the window (in pixels) and the window is not maximized by default. 

__Please note that this file is not part of the code you are submitting when completing your tasks and any changes outside of the scenes folder can NOT be taken into account when grading your submissions!__

## Adding Scenes

If we now want to add/modify our own Scene we have to take a look at Scene1.h and Scene1.hpp. A custom scene inherits from the parent scene by default and is not required to override anything:
```cpp
class Scene1 : public Scene{};
``` 

The scenes are registered with a name in the SceneIndex.h file (here in line 16):
```cpp
// SceneIndex.h
#include "Scene1.h"
// ...

std::map<std::string, SceneCreator> scenesCreators = {
    {"Demo Scene", creator<Scene1>()},
    // add more Scene types here
};
```

If you want to add more scenes you need to first create a new scene header and implementation (Scene2.h and Scene2.cpp) with contents, e.g., like this:
```cpp
// Scene2.h
#include "Scene.h"

class Scene2 : public Scene // Make sure to use a different class name!
{
};
// Scene2.cpp
/* Empty */
```

After you have created these you need to rerun CMake configure (Use the command prompt from VSCode) and register them in the map by adding the include and label:
```cpp
// SceneIndex.h
#include "Scene1.h"
#include "Scene2.h"
// ...

std::map<std::string, SceneCreator> scenesCreators = {
    {"Demo Scene", creator<Scene1>()},
    {"Demo Scene 2", creator<Scene2>()},
    // add more Scene types here
};
```

## Implementing something in our custom scene

But for now we only need to deal with a single Scene. To give a scene its own _onDraw_ method we need to override it in the header file:
```cpp
#include "Scene.h"

class Scene1 : public Scene
{
    virtual void onDraw(Renderer &renderer) override;
};

```

And add a definition to Scene1.cpp:
```cpp
#include "Scene1.h"

void Scene1::onDraw(Renderer& renderer){
    renderer.drawWireCube(glm::vec3(0), glm::vec3(5), glm::vec3(1));

    renderer.drawCube(glm::vec3(0,0,0), glm::angleAxis(glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 0.0f)), glm::vec3(0.5,0.5,0.5), glm::vec4(1,0,0,1));
}
```

Which will produce the same exact output as before!

## Rotating Cube

To add some animation we need to modify the simulateStep() function which performs the integration of the physics system. In this example we create a pitch roll and yaw member in the Scene1 class to represent the rotation of the cube and override the simulateStep function from the parent class:

```cpp
#include "Scene.h"

class Scene1 : public Scene
{
    float pitch = 0.f;
    float roll = 0.f;
    float yaw = 0.f;

    virtual void onDraw(Renderer &renderer) override;
    virtual void simulateStep() override;
};
```
And update the Scene1.cpp file to implement the simulateStep function and update the onDraw function to draw the cube with the given Euler angles:
```cpp
void Scene1::onDraw(Renderer& renderer){
    renderer.drawWireCube(glm::vec3(0), glm::vec3(5), glm::vec3(1));

    renderer.drawCube(  glm::vec3(0,0,0), 
                        glm::quat(glm::vec3(pitch, roll, yaw)), // rotation now given via Euler angles
                        glm::vec3(0.5,0.5,0.5), 
                        glm::vec4(1,0,0,1));
}

void Scene1::simulateStep(){
    pitch += 0.001f;
    roll += 0.002f;
    yaw += 0.003f;
}
```
Which will now make the cube spin. Visualizing rotation can sometimes be useful and we can use the _drawLine_ functions to render the cubes local coordinate system, which is a set of three directions consisting of the _forward_, _right_ and _up_ directions relative to the cubes orientation, which we can compute from a rotation matrix:
```cpp
glm::mat4 rotation = glm::toMat4(glm::quat(glm::vec3(pitch, roll, yaw)));
glm::vec3 forward = glm::vec3(rotation * glm::vec4(0, 0, 1, 0));
glm::vec3 right = glm::vec3(rotation * glm::vec4(1, 0, 0, 0));
glm::vec3 up = glm::vec3(rotation * glm::vec4(0, 1, 0, 0));
``` 
Which we can then put in the onDraw function and drawLines from the origin (which is the cube origin) in the three directions with forward being red, right being green and up being blue:
```cpp
void Scene1::onDraw(Renderer& renderer){
    renderer.drawWireCube(glm::vec3(0), glm::vec3(5), glm::vec3(1));

    renderer.drawCube(  glm::vec3(0,0,0), 
                        glm::quat(glm::vec3(pitch, roll, yaw)), 
                        glm::vec3(0.5,0.5,0.5), 
                        glm::vec4(1,0,0,1));

    glm::mat4 rotation = glm::toMat4(glm::quat(glm::vec3(pitch, roll, yaw)));
    glm::vec3 forward = glm::vec3(rotation * glm::vec4(0, 0, 1, 0));
    glm::vec3 right = glm::vec3(rotation * glm::vec4(1, 0, 0, 0));
    glm::vec3 up = glm::vec3(rotation * glm::vec4(0, 1, 0, 0));

    renderer.drawLine(glm::vec3(0), forward, glm::vec4(1, 0, 0, 1));
    renderer.drawLine(glm::vec3(0), right, glm::vec4(0, 1, 0, 1));
    renderer.drawLine(glm::vec3(0), up, glm::vec4(0, 0, 1, 1));
}
```

You also need to add an include to be able to use the glm::toMat4 function:
```cpp
// At the top of Scene1.cpp
#include <glm/gtx/quaternion.hpp>
```

Which will now show this:

![alt text](https://github.com/user-attachments/assets/9e5408aa-dfe4-41c4-a8bb-b318f8cec333)

## User interaction via the GUI

We now want to add some interactivity to our program, e.g., we want to change how quickly the cube rotates. To do this we need to override the onGUI member of the Scene class and add calls to ImGui (the GUI backend we use for our framework). ImGui is an intermediate mode backend that is fairly straight forward to utilize and we simply need to create a member of the Scene1 class to represent the increments for pitch, roll and yaw and add the override:

```cpp
// Scene1.h
    float pitch_increment = 0.001f;
    float roll_increment = 0.002f;
    float yaw_increment = 0.003f;
    virtual void onGUI() override;
```

After which we can change the simulateStep to use these members instead of hardcoded values:
```cpp
void Scene1::simulateStep(){
    pitch += pitch_increment;
    roll += roll_increment;
    yaw += yaw_increment;
}
```

To add GUI elements for these increments we create _sliders_ which require a label (a C-string), a pointer to the variable we want to adjust from the GUI (you can achieve this by simply getting the address of the class members with &), and setting limits in which we want to adjust the spinning speed:
```cpp
void Scene1::onGUI(){
    ImGui::SliderFloat("Pitch Increment", &pitch_increment, -0.01f, 0.01f);
    ImGui::SliderFloat("Roll Increment", &roll_increment, -0.01f, 0.01f);
    ImGui::SliderFloat("Yaw Increment", &yaw_increment, -0.01f, 0.01f);
}
```

Which also requires including the imgui header:
```cpp
#include <imgui.h>
```

And we now have an interactive "simulation":

![alt text](https://github.com/user-attachments/assets/f35c7849-da3a-45fd-b78b-27295b643ad5)

Note that you can ctrl + left mouse button to directly input values into the sliders as well.

![alt text](https://github.com/user-attachments/assets/7f955773-0833-43a1-a74c-bc0430999348)

## Shooting Particles

As a final step we want to implement some new objects and see how we can handle resources and state in a more _complex_ setup. To do this we would like to _shoot_ spheres from the cube towards the forward direction. Each of these spheres will have a set of physical attributes, i.e., position, velocity, color and a lifetime. We can add a Plain Old Datastructure (POD) to the Scene1.h file:

```cpp
struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float lifetime;
};
``` 

To represent our _particles_. The Scene1 class then contains a list of particles it has emitted, which we store in a std::vector container (part of the C++ standard library). 
```cpp
class Scene1 : public Scene{
    //...
    std::vector<Particle> particles;
    //...
}
```

To have some interesting behavior we want to randomly set the colors of each particle using a random number generator. Random Number Generation (RNG) is a difficult topic in general but within the C++11 vernacular we have a distribution of values we want to generate (for our purposes a uniform distribution from 0 to 1 of floats), a generator that creates _random_ values and an initial seed from some random device on our system. This means we need to include the random header and add three additional members to the Scene1 class:

```cpp
#include <random>

class Scene1 : public Scene{
    //...
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<float> dis;
    //...
}
```

This random generator needs to be initialized in the constructor of the scene so we add a _PUBLIC_ constructor at the end of the class definition:

```cpp
class Scene1 : public Scene{
    //...
public:
    Scene1() : gen(rd()), dis(0.f, 1.f) {}
}
``` 

To now launch particles we add a _launch_ button to our GUI in the onGUI function:
```cpp
// Scene1.cpp
void Scene1::onGUI(){
    //...
    auto launch = ImGui::Button("Launch");
}
``` 

Where launch is true when the button is pressed (launch is only true on the initial press and not while the button is held down). Based on the launch variable we can then create a new particle by first computing the forward direction of the cube (as done before) and then creating a particle at the origin moving with 5 units per second in the forward direction per _time_ and a random color by drawing samples from our distribution:

```cpp
// Scene1.cpp
void Scene1::onGUI(){
    //...
    auto launch = ImGui::Button("Launch");

    if(launch){
        glm::mat4 rotation = glm::toMat4(glm::quat(glm::vec3(pitch, roll, yaw)));
        glm::vec3 forward = glm::vec3(rotation * glm::vec4(0, 0, 1, 0));

        particles.push_back(Particle{
            glm::vec3(0), // Initial Position
            forward * 5.f, // Initial Velocity
            glm::vec4(dis(gen), dis(gen), dis(gen), 1), // Color
            .0 // Particles are created with their own time counter set to 0
        });
    }
```

An important note here is the multiplication with 5.f. On Windows this code will work with 5 due to differences in integer casting behavior but on MacOS and Linux it will require an actual float value! If you want to multiply something with something else, __always make sure you use the right type!__ It is very easy to get accidental behavior or strange behavior due to implicit casting, or to simply lose performance from using double precision floating point numbers (5.0 is a double, 5.0f is a float).

To render these particles we then need to modify the onDraw method using the drawSphere function (which requires position, radius and color):

```cpp
void Scene1::onDraw(Renderer& renderer){
    //...
    for (auto& particle : particles){
        renderer.drawSphere(particle.position, 0.1f, particle.color);
    }
}
```

And add a Euler-like integration with a constant velocity (for simplicity we ignore timstep sizes) and integrate the lifetime of all particles:
```cpp
void Scene1::simulateStep(){
    //...
    for (auto& particle : particles){
        particle.position += 0.01f * particle.velocity;
        particle.lifetime += 0.01f;
    }
```

To remove particles after 1 second we add this call to the end of the simulateStep function where we remove particles if a conditional is true for the respective entry:
```cpp
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& particle){
        return particle.lifetime > 1.5f;
    }), particles.end());
```
All of this gives us this interactive physics simulation:

![alt text](https://github.com/user-attachments/assets/c8728def-ad8a-4d0d-8359-18c4081714c7)

## Gravity

If we want to add gravity we can simply add a gravitational field in the simulateStep function and update the velocity accordingly:

```cpp
glm::vec3 gravityAccel = glm::vec3(0, 0, -9.81f);

for (auto& particle : particles){
    particle.position += 0.01f * particle.velocity;
    particle.lifetime += 0.01f;
    particle.velocity += gravityAccel * 0.01f;
}
```

In this case we set the gravity to be pointing down in the z-direction. Note that some physics engines assign the "down" direction to be the negative y-coordinate (this is an interesting legacy development, e.g., consider 2D games where the down direction is naturally y). Within this framework we always set the z-direction to be up and down and chose the gravity accordingly.

Which gives us our final physics system:

![alt text](https://github.com/user-attachments/assets/80a47bc6-8266-4144-9ec2-74f4447f74cf)

## Keyboard input

To make this a bit more interesting we can now add keyboard controls, which is the last part of this tutorial. Before we do this, however, its a good idea to clean up our code a bit, which we can do by moving the launch functionality into a new function of its own and adding a delay to the launch to limit how many spheres we can create and we also set the default values for the increments to 0.005 for future use!
```cpp
// Scene1.h
class Scene1 : public Scene{
    //...
    float pitch_increment = 0.005f;
    float roll_increment = 0.005f;
    float yaw_increment = 0.005f;

    int32_t launch_delay = 8;
    int32_t lastLaunch = 0;
    void launchSphere();
    //...
}

// Scene1.cpp
void Scene1::simulateStep(){
    //...
    lastLaunch++;
}

void Scene1::launchSphere(){
    if( lastLaunch < launch_delay)
        return;
    lastLaunch = 0;
    glm::mat4 rotation = glm::toMat4(glm::quat(glm::vec3(pitch, roll, yaw)));
    glm::vec3 forward = glm::vec3(rotation * glm::vec4(0, 0, 1, 0));
    glm::vec3 right = glm::vec3(rotation * glm::vec4(1, 0, 0, 0));
    glm::vec3 up = glm::vec3(rotation * glm::vec4(0, 1, 0, 0));

    glm::vec4 color = glm::vec4(dis(gen), dis(gen), dis(gen), 1);
    float velocityMagnitude = 4.5f + dis(gen);
    glm::vec3 velocity = forward * velocityMagnitude;

    velocity += right * (dis(gen) - 0.5f) * 2.f;
    velocity += up * (dis(gen) - 0.5f) * 2.f;

    particles.push_back(Particle{glm::vec3(0), velocity, color, .0});
}

void Scene1::onGUI(){
    // ...
    ImGui::SliderInt("Launch Delay", &launch_delay, 0, 100);
}
```

We have also used this opportunity to randomize the launched spheres a bit to keep things interesting by randomizing the velocity (which is now uniform in magnitude in [4.5,5.5] ). We then added some _spray_ functionality that randomly changes the velocity with respect to the two other local coordinate axes of the cube, i.e., the forward and up direction. In this case this is a simple uniform noise in the range [-0.5,0.5]. Can you think of any improvements here? Hint: Should spray be uniform in x and y or should it be shaped in some way?

## Keyboard input

While this is a nice change, the limit in launching spheres was mostly how fast we can press the button whereas we now would like to just hold down a button to keep firing spheres, for which we will use either immediate mode input via ImGUI (recommended) or an event based input mode via GLFW key inputs ([more information here](https://www.glfw.org/docs/3.3/input_guide.html#input_key)).

Now, what is event based and what is immediate mode input? In general any key on a keyboard (ignoring analog keyboards) can have four stats:
- Held down
- Not Held down
- Was just pressed
- Was just released

The first two states are self-explanatory and describe the state of a key after it was pressed and the state of a key at rest. Note that with most input systems a key is only considered as _held_ down after a short delay, e.g., imagine trying to type a sentence if you could only hold down a key for a single frame. More interesting are the key being pressed and released.

A key being pressed and released is an _event_, i.e., it is a change of state of the keyboard and if we only look at the _state_ of a key then we cannot detect this single event, however, we can utilize our operating system or window system to send us a notification if this _event_ occured and we have to write functions that react to events instead of checking the state of keys. This has some other convenient advantages, e.g., if our game is running at 1 fps then we have to hold a key for an entire second to make sure the key is being pressed while _some_ function checks for input and there is no way to queue events, whereas for an event based system we only need to make sure that the event is sent at _some_ point which is then queued up for processing. While this may be an extreme example, there can be weird interactions or situations where two functions check for key state during a frame and one sees a key as being held and one sees a key as not being held, leading to very subtle and hard to reproduce bugs. On the other hand, for an event based system a common bug is someone _tabbing_ out of the game while holding a key such that the key _release_ event is never sent. Can you think of other issues?

If you want to use input during any submission __DO NOT USE THE GLFW VARIANT__. The GLFW variant requires changes to files outside of the Scene folder which are not part of the code you submit. (Note that this will only mean that the tutor grading your exercise wont be able to press buttons for any interactive element you chose to implement, and you will not be able to use this during the exam)

### ImGUI Immediate mode inputs

Adding inputs via ImGUI is very straightforward due to the immediate design of ImGUI. Accordingly, we can simply check at any point of our code (after including the imgui header) if a key is being held down and adding input handling is as simple as adding a few lines of code to the simulateStep function:

```cpp
// Scene1.cpp
void Scene1::simulateStep(){
    // pitch += pitch_increment;
    // roll += roll_increment;
    // yaw += yaw_increment;

    glm::vec3 gravityAccel = glm::vec3(0, 0, -9.81f);

    for (auto& particle : particles){
        particle.position += 0.01f * particle.velocity;
        particle.lifetime += 0.01f;
        particle.velocity += gravityAccel * 0.01f;
    }
    
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& particle){
        return particle.lifetime > 1.5f;
    }), particles.end());


    if(ImGui::IsKeyDown(ImGuiKey_Space))
        launchSphere();
    if(ImGui::IsKeyDown(ImGuiKey_W))
        pitch += pitch_increment;
    if(ImGui::IsKeyDown(ImGuiKey_S))
        pitch -= pitch_increment;
    if(ImGui::IsKeyDown(ImGuiKey_A))
        roll += roll_increment;
    if(ImGui::IsKeyDown(ImGuiKey_D))
        roll -= roll_increment;
    if(ImGui::IsKeyDown(ImGuiKey_Q))
        yaw += yaw_increment;
    if(ImGui::IsKeyDown(ImGuiKey_E))
        yaw -= yaw_increment;
    lastLaunch++;
}
``` 

Which are all necessary changes. You may still want to try the GLFW based variant as it shows some further C++ features and might be useful in general.

![Example image of the final result showing a spray of many spheres](https://github.com/user-attachments/assets/fb85a57d-fec7-451e-8d85-3baa0d8e38a3)

## Real Time Integration

So far we only used a fixed hardcoded timestep, but this may not be the most appropriate solution for your game. A common way to do physics is to instead implement an adaptive timestepping that takes the framerate of the game into account, which you can simply get via ImGui using `ImGui::GetIO().DeltaTime`. Implementing this in our demo code then is straight forward:

```cpp
// Scene1.cpp

void Scene1::simulateStep(){
    float realtimeDt = ImGui::GetIO().DeltaTime;
    // ...    
    for (auto& particle : particles){
        particle.position += realtimeDt * particle.velocity;
        particle.lifetime += realtimeDt;
        particle.velocity += gravityAccel * realtimeDt;
    }
    // ...
}
```

## Mouse Input


Something that may come in handy is having the ability to have a user click and drag on the screen to apply a force to the objects in the scene, even though it is not that useful for this demo example. 

This can be done as well in our framework but is a bit more involved as this requires camera information that is not readily available during the simulator (a simulation does not have the concept of a camera in most cases). Accordingly we need to save all relevant information during a part of the render process where it is available to make it available during the simulation.

To do this we add a few members to our _Scene_ class:
```cpp
// Scene.h
class Scene{
    //...
    glm::mat4 cameraMatrix = glm::mat4(1);
    glm::vec3 fwd = glm::vec3(1, 0, 0);
    glm::vec3 right = glm::vec3(0, 1, 0);
    glm::vec3 up = glm::vec3(0, 0, 1);
}
```

Which are then used to store all relevant information. In these members we will store the camera matrix and from this also derive the forward (the direction _into_ the screen) and the right and up vectors (relative to the camera coordinate system). The forward direction could also be used to _shoot_ objects into the scene, whereas the right and up will be used for a dragging interaction.

To capture this information we need to modify the _onDraw_ function (make sure to do this in the child scenes if they override the parent function too!):
```cpp
// Scene.cpp
void onDraw(Renderer& renderer){
    //...
    cameraMatrix = renderer.camera.viewMatrix;
    fwd = inverse(cameraMatrix) * glm::vec4(0, 0, 1, 0);
    right = inverse(cameraMatrix) * glm::vec4(1, 0, 0, 0);
    up = inverse(cameraMatrix) * glm::vec4(0, 1, 0, 0);
}
```

To then add a dragging interaction for the right mouse button (the left is used for camera interactivity) we can check if the mouse button was released before the current frame using 
```cpp
ImGui::IsMouseReleased(ImGuiMouseButton_Right)
```

Which returns true in this case (we could also add interactivity where the button is _held_ down instead of released but this is left to the reader as an exercise). We can then get the amount of _pixels_ the mouse was dragged by using 
```cpp
auto drag = ImGui::GetMouseDragDelta(1);
```
Note the 1 in the argument to get the drag delta for the _right_ mouse button. If the drag distance was zero we skip adding a force to an object, e.g., if this is done during a loop over all objects:
```cpp
if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)){   
    auto drag = ImGui::GetMouseDragDelta(1);
    if(drag.x == 0 && drag.y == 0)
        continue;
    //...
}
```

To now calculate the mouse force we multiply the drag in the x-direction with the _right_ vector of the camera and the drag in y-direction with the _up_ vector of the camera. Due to conventions and preferences some users may prefer inverting certain components, e.g., should the mouse being moved left move the object to the left (this makes a lot of sense for dragging interactions) or should we use a _spring_ like interaction where we move the mouse to the right and on releasing the button the distance we moved influences the force applied to the object in the opposite direction (this makes a lot of sense for on release interactions). In a full game engine it makes sense to make this configurable for the user but here we invert the y-coordinate (due to screen coordinate systems) and keep the x as is:
```cpp
if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)){   
    auto drag = ImGui::GetMouseDragDelta(1);
    if(drag.x == 0 && drag.y == 0)
        continue;
    auto dx = drag.x * right;
    auto dy = -drag.y * up;
    object.force += dx + dy;
}
```

You might also want to scale this force or precompute the value once outside of looping over all objects to avoid recomputing the same values over and over again. Also make sure to apply this force in every substep of multistep integrators!