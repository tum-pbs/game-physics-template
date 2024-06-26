#include "Scene.h"

class Scene1 : public Scene
{
public:
    void onDraw(Renderer &renderer) override;
    void onGUI() override;
};