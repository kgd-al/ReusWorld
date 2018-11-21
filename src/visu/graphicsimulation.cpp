#include "graphicsimulation.h"

#include "controller.h"

namespace visu {

void GraphicSimulation::setController(visu::Controller *c) {
  _controller = c;
}

void GraphicSimulation::addPlant(const genotype::Plant &p, float x) {
  Simulation::addPlant(p, x);
  _controller->view()->addPlantItem(*_plants.at(x));
}

void GraphicSimulation::delPlant(float x) {
  _controller->view()->delPlantItem(x);
  Simulation::delPlant(x);
}

} // end of namespace visu
