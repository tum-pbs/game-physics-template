#include "Simulator.h"
#include <imgui.h>
#include "Scenes/SceneIndex.h"

void Simulator::init()
{
    // get vector of all avaiable scene names
    for (auto &scene : scenesCreators)
    {
        sceneNames.push_back(scene.first);
    }
    if (sceneNames.size() <= 0)
    {
        std::cout << "No scenes available! Did you forget to add your scene to SceneIndex.h?" << std::endl;
    }
    else
    {
        currentSceneName = sceneNames[0];
        currentScene = scenesCreators[currentSceneName]();
        currentScene->init();
    }
}

void Simulator::simulateStep()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    if (currentScene != nullptr)
        currentScene->simulateStep();
    lastStepTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime).count();
}

void Simulator::onGUI()
{
    using namespace ImGui;
    if (currentScene == nullptr)
    {
        Begin("Game Physics", nullptr, ImGuiWindowFlags_NoTitleBar);
        Text("No scenes available!");
        Text("Did you forget to add your scene to SceneIndex.h?");
        End();
        return;
    }
    Renderer::camera.update();

    Begin("Game Physics", nullptr, ImGuiWindowFlags_NoTitleBar);
    Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GetIO().Framerate, GetIO().Framerate);
    Text("Step: %.3f ms, DrawPrep: %.3f, Draw: %.3f ms", lastStepTime * 1000, lastDrawPrepTime * 1000, renderer.lastDrawTime * 1000);
    Text("%ld objects, %ld lines, %ld images", renderer.objectCount(), renderer.lineCount(), renderer.imageCount());
    Separator();
    Text("Scene");
    if (BeginCombo("Scene", currentSceneName.c_str()))
    {
        for (auto &sceneName : sceneNames)
        {
            bool isSelected = (currentSceneName == sceneName);
            if (Selectable(sceneName.c_str(), isSelected))
            {
                currentSceneName = sceneName;
                currentScene = scenesCreators[currentSceneName]();
                currentScene->init();
            }
            if (isSelected)
                SetItemDefaultFocus();
        }
        EndCombo();
    }
    if (Button("Reload Scene"))
    {
        currentScene = scenesCreators[currentSceneName]();
        currentScene->init();
    }
    Separator();
    if (CollapsingHeader(currentSceneName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        currentScene->onGUI();
    }
    Separator();
    if (CollapsingHeader("Rendering"))
    {
        if (Checkbox("Limit FPS", &limitFPS))
        {
            if (limitFPS)
                renderer.setPresentMode(wgpu::PresentMode::Fifo);
            else
                renderer.setPresentMode(wgpu::PresentMode::Immediate);
        }

        ColorEdit3("Background Color", glm::value_ptr(renderer.backgroundColor));
        Separator();

        DragFloat3("Light Position", glm::value_ptr(renderer.lightingUniforms.direction), 0.01f);
        Separator();
        ColorEdit3("Ambient Color", glm::value_ptr(renderer.lightingUniforms.ambient));
        DragFloat("Ambient Intensity", &renderer.lightingUniforms.ambient_intensity, 0.01f, 0.0, 1.0);
        Separator();
        ColorEdit3("Specular Color", glm::value_ptr(renderer.lightingUniforms.specular));
        DragFloat("Specular Intensity", &renderer.lightingUniforms.specular_intensity, 0.01f, 0.0, 1.0);
        DragFloat("Specular Alpha", &renderer.lightingUniforms.alpha, 0.1f, 0.0, 100.0);
        Separator();
        DragFloat("Diffuse Intensity", &renderer.lightingUniforms.diffuse_intensity, 0.01f, 0.0, 1.0);
    }

    End();
}

void Simulator::onDraw()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    if (currentScene != nullptr)
        currentScene->onDraw(renderer);
    lastDrawPrepTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime).count();
};