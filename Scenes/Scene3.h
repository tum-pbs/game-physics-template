#include "Scene.h"

class Scene3 : public Scene
{
public:
    void init() override;
    void simulateStep() override;
    void onGUI() override;

private:
    bool pause = false;
};
