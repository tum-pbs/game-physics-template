#pragma once
#include <string>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <filesystem>
#include "ResourceManager.h"

class Colormap
{
public:
    Colormap(std::string name);
    float textureOffset();
    glm::vec3 operator()(float value);
    static void init();
    static ResourceManager::Image colormaps;

private:
    int index;
    glm::vec3 color(float value);
    static std::map<std::string, int> indices;
    static std::vector<std::string> names;
};