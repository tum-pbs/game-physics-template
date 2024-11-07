#include "Renderer.h"
#include "Simulator.h"

int main(int, char **)
{
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
		renderer.onFrame();
		renderer.clearScene();
	}
	return 0;
}
