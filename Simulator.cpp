#include "Simulator.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include "Scenes/SceneIndex.h"
#include <chrono>

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
    ImGui::Begin("Game Physics");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Step: %.3f ms, DrawPrep: %.3f, Draw: %.3f ms", lastStepTime * 1000, lastDrawPrepTime * 1000, renderer.lastDrawTime * 1000);
    ImGui::Text("%d objects, %d lines", renderer.objectCount(), renderer.lineCount());
    ImGui::Separator();
    ImGui::Text("Scene");
    if (ImGui::BeginCombo("Scene", currentSceneName.c_str()))
    {
        for (auto &sceneName : sceneNames)
        {
            bool isSelected = (currentSceneName == sceneName);
            if (ImGui::Selectable(sceneName.c_str(), isSelected))
            {
                currentSceneName = sceneName;
                currentScene = scenesCreators[currentSceneName]();
                currentScene->init();
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Reload Scene"))
    {
        currentScene = scenesCreators[currentSceneName]();
        currentScene->init();
    }
    ImGui::Separator();
    if (ImGui::CollapsingHeader(currentSceneName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        currentScene->onGUI();
    }
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Rendering"))
    {
        if (ImGui::Checkbox("Limit FPS", &limitFPS))
        {
            if (limitFPS)
                renderer.setPresentMode(wgpu::PresentMode::Fifo);
            else
                renderer.setPresentMode(wgpu::PresentMode::Immediate);
        }

        ImGui::ColorEdit3("Background Color", glm::value_ptr(renderer.backgroundColor));
        ImGui::Separator();

        ImGui::DragFloat3("Light Position", glm::value_ptr(renderer.m_lightingUniforms.direction), 0.01f);
        ImGui::Separator();
        ImGui::ColorEdit3("Ambient Color", glm::value_ptr(renderer.m_lightingUniforms.ambient));
        ImGui::DragFloat("Ambient Intensity", &renderer.m_lightingUniforms.ambient_intensity, 0.01f, 0.0, 1.0);
        ImGui::Separator();
        ImGui::ColorEdit3("Specular Color", glm::value_ptr(renderer.m_lightingUniforms.specular));
        ImGui::DragFloat("Specular Intensity", &renderer.m_lightingUniforms.specular_intensity, 0.01f, 0.0, 1.0);
        ImGui::DragFloat("Specular Alpha", &renderer.m_lightingUniforms.alpha, 0.1f, 0.0, 100.0);
        ImGui::Separator();
        ImGui::DragFloat("Diffuse Intensity", &renderer.m_lightingUniforms.diffuse_intensity, 0.01f, 0.0, 1.0);
    }

    ImGui::End();
}

void Simulator::onDraw()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    renderer.drawWireCube({0, 0, 0}, {5, 5, 5}, {1, 1, 1});
    currentScene->onDraw(renderer);
    lastDrawPrepTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime).count();
};