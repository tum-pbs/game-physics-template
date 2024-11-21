#include "Renderer.h"
#include "Simulator.h"
#include "PathFinder.h"

int main(int argc, char ** argv)
{
	namespace fs = std::filesystem;
	workingDirectory = fs::absolute(fs::path(argv[0])).remove_filename();
    binaryDirectory = fs::current_path();

	Renderer renderer = Renderer();
	Simulator simulator(renderer);

	simulator.init();
	while (renderer.isRunning())
	{
		simulator.simulateStep();
		simulator.onDraw();
		renderer.onFrame();
		renderer.clearScene();
	}
	return 0;
}
