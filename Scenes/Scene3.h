#include "Scene.h"

class Scene3 : public Scene
{
public:
    void init() override;
    void simulateStep() override;

private:
    void resetValues();
};