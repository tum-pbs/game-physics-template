#include <glm/glm.hpp>

class Camera
{
public:
    glm::vec3 position;
    float fov;
    float near;
    float far;
    int width;
    int height;
    Camera();
    glm::vec3 forward();
    glm::vec3 up();
    glm::vec3 right();
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix();
    void lookAt(glm::vec3 target);
    float aspectRatio();

private:
    const glm::vec3 worldRight = glm::vec3(1, 0, 0);
    const glm::vec3 worldForward = glm::vec3(0, 1, 0);
    const glm::vec3 worldUp = glm::vec3(0, 0, 1);
};