#include "Scene.h"

class Scene1 : public Scene
{
public:
    void init() override;
    void simulateStep() override;
    void onGUI() override;

private:
    void diffuseExplicit(float dt);
    float alpha = 1;
    float dt = 0.01;
    size_t resolution = 100;
    void resetGrid();
};