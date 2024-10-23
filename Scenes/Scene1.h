#include "Scene.h"
#include <random>

struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float lifetime;
};

class Scene1 : public Scene
{
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<float> dis;

    float pitch = 0.f;
    float roll = 0.f;
    float yaw = 0.f;

    float pitch_increment = 0.005f;
    float roll_increment = 0.005f;
    float yaw_increment = 0.005f;
    
    int32_t launch_delay = 32;
    int32_t lastLaunch = 0;
    void launchSphere();

    std::vector<Particle> particles;

    virtual void onDraw(Renderer &renderer) override;
    virtual void simulateStep() override;
    virtual void onGUI() override;

    struct inputState{
        bool space = false;
        bool w = false, a = false, s = false, d = false;
        bool e = false, q = false;
    };
    inputState keyState;

    virtual void onKeyInput(GLFWwindow *window, int key, int scancode, int action, int mods) override;

public:
    Scene1() : gen(rd()), dis(0.f, 1.f) {}
};