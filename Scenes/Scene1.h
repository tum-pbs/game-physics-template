#include "Scene.h"

class Scene1 : public Scene
{
public:
    void init() override;
    void simulateStep() override;
    void onDraw(Renderer &renderer) override;
    void onGUI() override;
};