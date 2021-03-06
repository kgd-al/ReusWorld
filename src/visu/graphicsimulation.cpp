#include <QCoreApplication>
#include <QFileDialog>

#include "graphicsimulation.h"

#include "controller.h"

namespace visu {

GraphicSimulation::GraphicSimulation (void)
  : _pviewer(nullptr, _ptree, PViewer::Direction::LeftToRight) {}

void GraphicSimulation::setController(visu::Controller *c) {
  _controller = c;
//  _pviewer.setParent(c->view());
}

simu::Plant*
GraphicSimulation::addPlant(const PGenome &p, float x, float biomass) {
  simu::Plant *plant = Simulation::addPlant(p, x, biomass);
  if (plant)  _controller->view()->addPlantItem(*plant, p.species());
  return plant;
}

void GraphicSimulation::delPlant(simu::Plant &p, simu::Plant::Seeds &seeds) {
  _controller->view()->delPlantItem(p);
  Simulation::delPlant(p, seeds);
}

void GraphicSimulation::updatePlantAltitude(simu::Plant &p, float h) {
  Simulation::updatePlantAltitude(p, h);
  _controller->view()->updatePlantItem(p);
}

bool GraphicSimulation::init(const EGenome &env, PGenome plant) {
  bool ok = Simulation::init(env, plant);
  emit initialized(ok);
  _controller->view()->updateEnvironment();

  connect(&_pviewer, &PViewer::onSpeciesHoverEvent,
          _controller->view(), &gui::MainView::speciesHovered);

  return ok;
}

void GraphicSimulation::graphicalStep (uint speed) {
  for (uint i=0; i<speed && !finished(); i++) {
    step();
    QCoreApplication::processEvents();  // A bit ugly but what the hell
  }

  _controller->view()->update();

  if (finished()) emit completed();
}

void GraphicSimulation::saveAs(void) const {
  QFileDialog dialog (_controller->view());
  dialog.setWindowTitle("Save simulation as");
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setNameFilter("Save file (*.save)");
  dialog.setViewMode(QFileDialog::Detail);
  dialog.selectFile(QString::fromStdString(periodicSaveName()));
  if (dialog.exec())
    save(dialog.selectedFiles()[0].toStdString());
}

void GraphicSimulation::savePhylogeny (void) const {
  QString file = QFileDialog::getSaveFileName(_controller->view(), "Save PTree as");
  if (!file.isEmpty())  _ptree.saveTo(file.toStdString());
}

void GraphicSimulation::doScreenshot(const QSize &size, stdfs::path path) const {
  static const std::string screenshotFolder = "screenshots/";

  if (_env.time() == _env.startTime()) stdfs::remove_all(screenshotFolder);
  stdfs::create_directory(screenshotFolder);

  QPixmap p = _controller->view()->screenshot(size);

  if (path.empty()) {
    path = screenshotFolder;
    path += "sc_";
    path += _env.time().pretty();
    path += ".png";
  }

  p.save(QString::fromStdString(path.string()));
}

void GraphicSimulation::load (const std::string &file, GraphicSimulation &s,
                              const std::string &constraints,
                              const std::string &fields) {

  Simulation::load(file, s, constraints, fields);
  s._controller->view()->updateEnvironment();
  for (const auto &p: s._plants)
    s._controller->view()->addPlantItem(*p.second,
                                        p.second->species());

  s._pviewer.build();

  connect(&s._pviewer, &PViewer::onSpeciesHoverEvent,
          s._controller->view(), &gui::MainView::speciesHovered);

  emit s.initialized(true);
}

} // end of namespace visu
