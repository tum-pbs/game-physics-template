#include "Scene.h"

class Scene2 : public Scene
{
public:
    void init() override;
    void simulateStep() override;

private:
    void resetValues();
};