#include "Renderer.h"
#include "Simulator.h"
#include "PathFinder.h"
#include <GLFW/glfw3.h>

int main(int argc, char ** argv)
{
	namespace fs = std::filesystem;
	workingDirectory = fs::absolute(fs::path(argv[0])).remove_filename();
    binaryDirectory = fs::current_path();
  
	bool verbose = false;
	Renderer renderer = Renderer(verbose);
	if(verbose)	
		std::cout << "Renderer created" << std::endl;
	Simulator simulator(renderer);
	if(verbose)
		std::cout << "Simulator created" << std::endl;

	simulator.init();
	if(verbose)
		std::cout << "Simulator initialized" << std::endl;
	while (renderer.isRunning())
	{
		simulator.simulateStep();
		simulator.onDraw();
		glfwPollEvents();
		renderer.onFrame();
		renderer.clearScene();

	}
	return 0;
}
