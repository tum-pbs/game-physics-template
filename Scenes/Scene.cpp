#include "Scene.h"
#include <imgui.h>

void Scene::onDraw(Renderer &renderer)
{
    renderer.drawWireCube(glm::vec3(0), glm::vec3(5), glm::vec3(1));

    renderer.drawCube(glm::vec3(0,0,0), glm::angleAxis(glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 0.0f)), glm::vec3(0.5,0.5,0.5), glm::vec4(1,0,0,1));
}
