#include <glm/glm.hpp>

class Camera
{
public:
    /// @brief Camera update function, called once per frame. Specifies the camera behavior.
    ///
    /// Updates the camera position and rotation based on user input. Edit this function for different camera behavior.
    void update();

    /// Camera viewpoint
    glm::vec3 position;
    /// Field of view in degrees
    float fov;
    /// Near clipping plane
    float near;
    /// Far clipping plane
    float far;
    /// Width of the window
    int width;
    /// Height of the window
    int height;
    Camera();

    /// Forward vector of the current camera rotation
    glm::vec3 forward();
    /// Up vector of the current camera rotation
    glm::vec3 up();
    /// Right vector of the current camera rotation
    glm::vec3 right();
    /// The view matrix of the camera
    glm::mat4 viewMatrix;
    /// The projection matrix of the camera
    glm::mat4 projectionMatrix();
    /// Update the camera view matrix by looking at the target from the current position with the up vector (0,0,1)
    void lookAt(glm::vec3 target);
    /// Compute the aspect ratio of the window
    float aspectRatio();

private:
    const glm::vec3 worldRight = glm::vec3(1, 0, 0);
    const glm::vec3 worldForward = glm::vec3(0, 1, 0);
    const glm::vec3 worldUp = glm::vec3(0, 0, 1);

    glm::vec2 cameraAngles = glm::vec2(-0.1, 0.2);
    float zoom = -2;
    float zoomSensitivity = 0.1;
    float cameraSensitivity = 0.01;
};