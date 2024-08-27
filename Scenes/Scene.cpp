#include "Scene.h"
#include <imgui.h>

void Scene::onDraw(Renderer &renderer)
{
    renderer.drawWireCube(glm::vec3(0), glm::vec3(5), glm::vec3(1));
}
