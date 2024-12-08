#include "Scene.h"
#include <map>

#include "Scene1.h"
#include "Scene1_SingleStep.h"
#include "Scene2_Simulation.h"
#include "Scene3_Collision.h"
#include "Scene4_ComplexSimulation.h"

using SceneCreator = std::function<std::unique_ptr<Scene>()>;

template <typename T>
SceneCreator creator()
{
    return []()
    { return std::make_unique<T>(); };
}

std::map<std::string, SceneCreator> scenesCreators = {
    {"Demo Scene", creator<Scene1>()},
    {"Demo Scene (Single-Step)", creator<Scene1_SingleStep>()},
    {"Demo Scene (Simulation)", creator<Scene2_Simulation>()},
    {"Demo Scene (Collision)", creator<Scene3_Collision>()},
    {"Demo Scene (Complex Simulation)", creator<Scene4_ComplexSimulation>()},
    // add more Scene types here
};
