#include "Scene.h"
#include <map>

#include "Scene1.h"
#include "Scene2.h"

using SceneCreator = std::function<std::unique_ptr<Scene>()>;

std::map<std::string, SceneCreator> scenesCreators = {
    {"Explicit Euler", []()
     { return std::make_unique<Scene1>(); }},
    {"Implicit Euler", []()
     { return std::make_unique<Scene2>(); }},
    // add more Scene types here
};
