#include "Scene.h"
#include <map>

#include "Scene1.h"

using SceneCreator = std::function<std::unique_ptr<Scene>()>;

std::map<std::string, SceneCreator> scenesCreators = {
    {"Demo Scene", []()
     { return std::make_unique<Scene1>(); }},
    // add more Scene types here
};
