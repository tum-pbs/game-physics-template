#include <imgui.h>
#include <glm/gtx/quaternion.hpp>
#include "Scene1.h"

void Scene1::onDraw(Renderer& renderer){
    renderer.drawWireCube(glm::vec3(0), glm::vec3(5), glm::vec3(1));

    renderer.drawCube(glm::vec3(0,0,0), glm::quat(glm::vec3(pitch, roll, yaw)), glm::vec3(0.5,0.5,0.5), glm::vec4(1,0,0,1));

    glm::mat4 rotation = glm::toMat4(glm::quat(glm::vec3(pitch, roll, yaw)));
    glm::vec3 forward = glm::vec3(rotation * glm::vec4(0, 0, 1, 0));
    glm::vec3 right = glm::vec3(rotation * glm::vec4(1, 0, 0, 0));
    glm::vec3 up = glm::vec3(rotation * glm::vec4(0, 1, 0, 0));

    renderer.drawLine(glm::vec3(0), forward, glm::vec4(1, 0, 0, 1));
    renderer.drawLine(glm::vec3(0), right, glm::vec4(0, 1, 0, 1));
    renderer.drawLine(glm::vec3(0), up, glm::vec4(0, 0, 1, 1));

    for (auto& particle : particles){
        renderer.drawSphere(particle.position, 0.1f, particle.color);
    }
    cameraMatrix = renderer.camera.viewMatrix;
    fwd = inverse(cameraMatrix) * glm::vec4(0, 0, 1, 0);
    right = inverse(cameraMatrix) * glm::vec4(1, 0, 0, 0);
    up = inverse(cameraMatrix) * glm::vec4(0, 1, 0, 0);
}

void Scene1::simulateStep(){
    // pitch += pitch_increment;
    // roll += roll_increment;
    // yaw += yaw_increment;

    glm::vec3 gravityAccel = glm::vec3(0, 0, -9.81f);

    for (auto& particle : particles){
        particle.position += 0.01f * particle.velocity;
        particle.lifetime += 0.01f;
        particle.velocity += gravityAccel * 0.01f;
    }
    if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)){   
        auto drag = ImGui::GetMouseDragDelta(1);
        if(!(drag.x == 0 && drag.y == 0)){
            auto dx = drag.x * right;
            auto dy = -drag.y * up;
            for (auto& particle : particles){
                particle.velocity += (dx + dy) * 0.01f;
            }
        }
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& particle){
        return particle.lifetime > 2.f;
    }), particles.end());

    if(keyState.space)
        launchSphere();
    if(keyState.w)
        pitch += pitch_increment;
    if(keyState.s)
        pitch -= pitch_increment;
    if(keyState.a)
        roll += roll_increment;
    if(keyState.d)
        roll -= roll_increment;
    if(keyState.q)
        yaw += yaw_increment;
    if(keyState.e)
        yaw -= yaw_increment;
    lastLaunch++;
}

void Scene1::launchSphere(){
    if( lastLaunch < launch_delay)
        return;
    lastLaunch = 0;
    glm::mat4 rotation = glm::toMat4(glm::quat(glm::vec3(pitch, roll, yaw)));
    glm::vec3 forward = glm::vec3(rotation * glm::vec4(0, 0, 1, 0));
    glm::vec3 right = glm::vec3(rotation * glm::vec4(1, 0, 0, 0));
    glm::vec3 up = glm::vec3(rotation * glm::vec4(0, 1, 0, 0));

    glm::vec4 color = glm::vec4(dis(gen), dis(gen), dis(gen), 1);
    float velocityMagnitude = 4.5f + dis(gen);
    glm::vec3 velocity = forward * velocityMagnitude;

    velocity += right * (dis(gen) - 0.5f) * 2.f;
    velocity += up * (dis(gen) - 0.5f) * 2.f;

    particles.push_back(Particle{glm::vec3(0), velocity, color, .0});
}

void Scene1::onGUI(){
    ImGui::SliderFloat("Pitch Increment", &pitch_increment, -0.01f, 0.01f);
    ImGui::SliderFloat("Roll Increment", &roll_increment, -0.01f, 0.01f);
    ImGui::SliderFloat("Yaw Increment", &yaw_increment, -0.01f, 0.01f);
    ImGui::SliderInt("Launch Delay", &launch_delay, 0, 100);

    auto launch = ImGui::Button("Launch");

    if(launch){
        launchSphere();
    }

}
void Scene1::onKeyInput(GLFWwindow* window,  int key, int scancode, int action, int mods){
    if(action == GLFW_PRESS){
        if(key == GLFW_KEY_W)
            keyState.w = true;
        if(key == GLFW_KEY_A)
            keyState.a = true;
        if(key == GLFW_KEY_S)
            keyState.s = true;        
        if(key == GLFW_KEY_D)
            keyState.d = true;        
        if(key == GLFW_KEY_E)
            keyState.e = true;        
        if(key == GLFW_KEY_Q)
            keyState.q = true;
        if(key == GLFW_KEY_SPACE)
            keyState.space = true;
    }
    if(action == GLFW_RELEASE){
        if(key == GLFW_KEY_W)
            keyState.w = false;
        if(key == GLFW_KEY_A)
            keyState.a = false;
        if(key == GLFW_KEY_S)
            keyState.s = false;        
        if(key == GLFW_KEY_D)
            keyState.d = false;        
        if(key == GLFW_KEY_E)
            keyState.e = false;        
        if(key == GLFW_KEY_Q)
            keyState.q = false;
        if(key == GLFW_KEY_SPACE)
            keyState.space = false;
    }
}