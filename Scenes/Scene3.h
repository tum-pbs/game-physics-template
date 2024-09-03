#include "Scene.h"

class Scene3 : public Scene
{
public:
    void init() override;
    void simulateStep() override;
    void onDraw(Renderer &renderer) override;
    void onGUI() override;

protected:
    void resetGrid() override;

private:
    glm::vec3 cullingOffsets;
    bool doCulling = false;
    enum Solver
    {
        Explicit,
        Implicit
    };
    Solver solver = Solver::Explicit;
    float scale = 5;
    bool drawBorder = false;
};