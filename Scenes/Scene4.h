#include "Scene.h"
#include <string>

class Scene4 : public Scene
{
public:
    void init() override;
    void simulateStep() override;
    void onGUI() override;

private:
    bool pause = false;
    float timescale = 1.0f;
    float dt = 0;
    int cubeRes = 3;
    float k = 80;
    std::string integrator = "Euler";
    void resolveCollisions();
    void initCube(int res, float stiffness);
};
