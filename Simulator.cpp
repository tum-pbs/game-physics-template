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
        throw std::runtime_error("No scenes available");
    currentSceneName = sceneNames[0];
    currentScene = scenesCreators[currentSceneName]();
    currentScene->init();
}

void Simulator::updateCamera()
{
    ImVec2 screenDelta = ImGui::GetMouseDragDelta();
    vec2 angleDelta = vec2(-screenDelta.x, screenDelta.y) * cameraSensitivity;
    vec2 tmpAngles = cameraAngles + angleDelta;
    tmpAngles.y = glm::clamp(tmpAngles.y, -glm::half_pi<float>() + 1e-5f, glm::half_pi<float>() - 1e-5f);
    float cx = cos(tmpAngles.x);
    float sx = sin(tmpAngles.x);
    float cy = cos(tmpAngles.y);
    float sy = sin(tmpAngles.y);
    zoom += ImGui::GetIO().MouseWheel * zoomSensitivity;
    Renderer::camera.position = vec3(cx * cy, sx * cy, sy) * std::exp(-zoom);
    Renderer::camera.lookAt(vec3(0.0f));
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        auto delta = ImGui::GetMouseDragDelta();
        cameraAngles = cameraAngles + angleDelta;
    }
}

void Simulator::simulateStep()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    currentScene->simulateStep();
    lastStepTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime).count();
}

void Simulator::onGUI()
{
    updateCamera();

    using namespace ImGui;
    Begin("Game Physics");
    Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GetIO().Framerate, GetIO().Framerate);
    Text("Step: %.3f ms, DrawPrep: %.3f, Draw: %.3f ms", lastStepTime * 1000, lastDrawPrepTime * 1000, renderer.lastDrawTime * 1000);
    Text("%d objects, %d lines", renderer.objectCount(), renderer.lineCount());
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

        DragFloat3("Light Position", glm::value_ptr(renderer.m_lightingUniforms.direction), 0.01f);
        Separator();
        ColorEdit3("Ambient Color", glm::value_ptr(renderer.m_lightingUniforms.ambient));
        DragFloat("Ambient Intensity", &renderer.m_lightingUniforms.ambient_intensity, 0.01f, 0.0, 1.0);
        Separator();
        ColorEdit3("Specular Color", glm::value_ptr(renderer.m_lightingUniforms.specular));
        DragFloat("Specular Intensity", &renderer.m_lightingUniforms.specular_intensity, 0.01f, 0.0, 1.0);
        DragFloat("Specular Alpha", &renderer.m_lightingUniforms.alpha, 0.1f, 0.0, 100.0);
        Separator();
        DragFloat("Diffuse Intensity", &renderer.m_lightingUniforms.diffuse_intensity, 0.01f, 0.0, 1.0);
    }

    End();
}

void Simulator::onDraw()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    currentScene->onDraw(renderer);
    lastDrawPrepTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime).count();
};