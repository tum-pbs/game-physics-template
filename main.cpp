#include "Renderer.h"
#include "Simulator.h"

int main(int, char **)
{
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
