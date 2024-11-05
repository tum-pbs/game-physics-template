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
    glm::mat4 cameraMatrix = glm::mat4(1);
    glm::vec3 fwd = glm::vec3(1, 0, 0);
    glm::vec3 right = glm::vec3(0, 1, 0);
    glm::vec3 up = glm::vec3(0, 0, 1);
    
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

public:
    Scene1() : gen(rd()), dis(0.f, 1.f) {}
};