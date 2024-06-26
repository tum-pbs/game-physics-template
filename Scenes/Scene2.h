#include "Scene.h"

class Scene2 : public Scene
{
public:
    void init() override;
    void simulateStep() override;
    void onDraw(Renderer &renderer) override;
    void onGUI() override;
};
