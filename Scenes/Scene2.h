#include "Scene.h"

class Scene2 : public Scene
{
public:
    void init() override;
    void simulateStep() override;
    void onGUI() override;

private:
    void resetValues();
};