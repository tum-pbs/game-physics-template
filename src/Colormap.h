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
    /// @brief Create a colormap
    /// @param name
    ///    The name of the colormap. All colormaps from matplotlib are available, for more info look into resources/colormaps.txt
    ///
    ///    Common colormaps are: hot, gray, viridis, plasma, inferno, magma, cividis...
    Colormap(std::string name);

    /// @brief Calculate the y-offset of the colormap in the colormap texture
    /// @return
    ///    The y-offset of the colormap in the colormap texture. Range: [0, 1]
    float textureOffset();

    /// @brief Apply the colormap
    /// @param value
    ///    The value to apply the colormap to. values will be clamped to [0, 1]
    /// @return
    ///    The color of the value in the colormap
    glm::vec3 operator()(float value);

    /// @brief Load the colormaps from resources/colormaps.png and resources/colormaps.txt
    ///
    ///     Only has to be called once, this happens automatically when the first colormap is created.
    static void init();

    /// All colormaps in on texture, line by line
    static ResourceManager::Image colormaps;

private:
    int index;
    glm::vec3 color(float value);
    static std::map<std::string, int> indices;
    static std::vector<std::string> names;
};