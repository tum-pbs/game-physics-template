#include "Scene.h"

class Scene2 : public Scene
{
public:
    void simulateStep() override;

private:
    void diffuseImplicit(float dt);
};