
ImGui::NewFrame();

ImGui::Begin("Game Physics");
ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
}
ImGui::Separator();
if (ImGui::CollapsingHeader(currentSceneName.c_str()))
{
    ImGui::Checkbox("Pause", &pause);
    ImGui::DragFloat("timescale", &timescale, 0.1f, 0.0001f, 100.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
    ImGui::Text("Physics dt: %.4f", dt);
    ImGui::DragFloat3("Gravity", glm::value_ptr(gravity), 0.01f, -50.0f, 50.0f, "%.2f");
    ImGui::BeginCombo("Test", "Test");
    if (ImGui::Selectable("Test", true))
    {
        integrator = "Euler";
    }
    ImGui::EndCombo();
}
ImGui::Separator();
if (ImGui::CollapsingHeader("Rendering"))
{
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

defineGUI();

// Draw the UI
ImGui::EndFrame();
// Convert the UI defined above into low-level drawing commands

ImGui::Render();