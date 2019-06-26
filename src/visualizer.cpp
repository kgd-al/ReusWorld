#include <QApplication>
#include <QMainWindow>

#include "kgd/external/cxxopts.hpp"

#include "visu/controller.h"

#include "config/dependencies.h"

int main(int argc, char *argv[]) {
  // ===========================================================================
  // == Command line arguments parsing

  using Verbosity = config::Verbosity;

  std::string configFile = "auto";  // Default to auto-config
  Verbosity verbosity = Verbosity::SHOW;

  std::string envGenomeFile, plantGenomeFile;
  std::string loadSaveFile, loadConstraints;

  std::string morphologiesSaveFolder;

  int speed = 0;
  bool autoQuit = false;

  std::string duration = "=100";
  std::string outputFolder = "";

  cxxopts::Options options("ReusWorld",
                           "2D simulation of plants in a changing environment");
  options.add_options()
    ("h,help", "Display help")
    ("a,auto-config", "Load configuration data from default location")
    ("c,config", "File containing configuration data",
     cxxopts::value(configFile))
    ("v,verbosity", "Verbosity level. " + config::verbosityValues(),
     cxxopts::value(verbosity))
    ("d,duration", "Simulation duration. ",
     cxxopts::value(duration))
    ("f,data-folder", "Folder under which to store the computational outputs",
     cxxopts::value(outputFolder))
    ("e,environment", "Environment's genome", cxxopts::value(envGenomeFile))
    ("p,plant", "Plant genome to start from", cxxopts::value(plantGenomeFile))
    ("l,load", "Load a previously saved simulation",
     cxxopts::value(loadSaveFile))
    ("load-constraints", "Constraints to apply on dependencies check",
     cxxopts::value(loadConstraints))
    ("r,run", "Immediatly start running. Optionnally specify at which speed",
      cxxopts::value(speed))
    ("q,auto-quit", "Quit as soon as the simulation ends",
     cxxopts::value(autoQuit))
    ("collect-morphologies", "Save morphologies in the provided folder",
     cxxopts::value(morphologiesSaveFolder))
    ;

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help()
              << "\n\nOption 'auto-config' is the default and overrides 'config'"
                 " if both are provided"
              << "\nEither both 'plant' and 'environment' options are used or "
                 "a valid file is to be provided to 'load' (the former has "
                 "precedance in case all three options are specified)"
              << "\n\n" << config::Dependencies::Help{}
              << std::endl;
    return 0;
  }

  bool missingArgument = (!result.count("environment") || !result.count("plant"))
      && !result.count("load");

  if (missingArgument) {
    if (result.count("environment"))
      utils::doThrow<std::invalid_argument>("No value provided for the plant's genome");

    else if (result.count("plant"))
      utils::doThrow<std::invalid_argument>("No value provided for the environment's genome");

    else
      utils::doThrow<std::invalid_argument>(
        "No starting state provided. Either provide both an environment and plant genomes"
        " or load a previous simulation");
  }

  if (!morphologiesSaveFolder.empty() && loadSaveFile.empty())
    utils::doThrow<std::invalid_argument>(
        "Generating morphologies is only meaningful when loading a simulation");

  if (result.count("auto-config") && result["auto-config"].as<bool>())
    configFile = "auto";

  config::Simulation::setupConfig(configFile, verbosity);
  if (configFile.empty()) config::Simulation::printConfig("");

  // ===========================================================================
  // == Qt setup

  QApplication a(argc, argv);
  setlocale(LC_NUMERIC,"C");

  visu::GraphicSimulation s;
  if (!outputFolder.empty()) s.setDataFolder(outputFolder);

  QMainWindow *w = new QMainWindow;
  gui::MainView *v = new gui::MainView(s.environment(), w);
  visu::Controller c (s, *w, v);

  if (loadSaveFile.empty()) {
    genotype::Environment e = genotype::Environment::fromFile(envGenomeFile);
    genotype::Plant p = genotype::Plant::fromFile(plantGenomeFile);

    std::cout << "Environment:\n" << e
              << "\nPlant:\n" << p
              << std::endl;

    s.init(e, p);

  } else
    visu::GraphicSimulation::load(loadSaveFile, s, loadConstraints);

  if (!duration.empty()) {
    if (duration.size() < 2)
      utils::doThrow<std::invalid_argument>(
        "Invalid duration '", duration, "'. [+|=]<years>");

    uint dvalue = 0;
    if (!(std::istringstream (duration.substr(1)) >> dvalue))
      utils::doThrow<std::invalid_argument>(
        "Failed to parse '", duration.substr(1), "' as a duration (uint)");

    s.setDuration(simu::Environment::DurationSetType(duration[0]), dvalue);
  }

  if (morphologiesSaveFolder.empty()) { // Regular simulation
    w->setAttribute(Qt::WA_QuitOnClose);
    w->show();

    c.nextPlant();
    c.setAutoQuit(autoQuit);

    if (speed != 0) {
      QTimer::singleShot(500, [&c, speed] {
        if (speed > 0)  c.play(true);
        c.setSpeed(std::abs(speed));
      });
    }

    auto ret = a.exec();

    s.abort();
    s.step();

    return ret;

  } else {
    v->saveMorphologies(QString::fromStdString(morphologiesSaveFolder));
    return 0;
  }
}
