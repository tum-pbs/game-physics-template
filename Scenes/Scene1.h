#include "Scene.h"

class Scene1 : public Scene
{
public:
    void simulateStep() override;

private:
    void diffuseExplicit(float dt);
};