#include "environment.h"
#include "tiniestphysicsengine.h"

#include "../config/simuconfig.h"

namespace simu {

static constexpr bool debugCGP = false;

using UL_EU = EnumUtils<UndergroundLayers>;

// =============================================================================

Environment::Environment(void)
  : _physics(std::make_unique<physics::TinyPhysicsEngine>(*this)) {}

Environment::~Environment(void) {}

void Environment::init (const Genome &g) {
  _genome = g;

  _topology.resize(_genome.voxels+1, 0.f);
  _temperature.resize(_genome.voxels+1, .5 * (_genome.maxT + _genome.minT));

  _hygrometry[UndergroundLayers::SHALLOW].resize(_genome.voxels+1, config::Simulation::baselineShallowWater());
  _hygrometry[UndergroundLayers::DEEP].resize(_genome.voxels+1, 1.f);

  _dice.reset(_genome.rngSeed);
  _updatedTopology = false;

  _start = _time = Time();
}

void Environment::destroy (void) {
  _physics->reset();
}

void Environment::step (void) {
  cgpStep();

#ifndef NDEBUG
  _physics->debug();
#endif

  _time.next();
}

void Environment::cgpStep (void) {
  using CGP = genotype::EnvCGP;
  using CGP_I = genotype::cgp::Inputs;
  using CGP_O = genotype::cgp::Outputs;
  CGP::Inputs inputs;
  CGP::Outputs outputs;

  _updatedTopology = _time.isStartOfYear()
      && (_time.year() % config::Simulation::updateTopologyEvery()) == 0;

  inputs[CGP_I::DAY] = _time.timeOfYear();
  inputs[CGP_I::YEAR] = _time.timeOfWorld();
  for (uint i=0; i<=_genome.voxels; i++) {
    float &A = _topology[i];
    float &T = _temperature[i];
    float &H = _hygrometry[SHALLOW][i];

    inputs[CGP_I::COORDINATE] = float(i) / _genome.voxels;
    inputs[CGP_I::ALTITUDE] = A / _genome.depth;
    inputs[CGP_I::TEMPERATURE] = (_genome.maxT - T) / (_genome.maxT - _genome.minT);
    inputs[CGP_I::HYGROMETRY] = H;

    _genome.cgp.process(inputs, outputs);

    if (_updatedTopology)
      A = outputs[CGP_O::ALTITUDE_] * _genome.depth;

    T = .5 * (outputs[CGP_O::TEMPERATURE_] + 1) * (_genome.maxT - _genome.minT) + _genome.minT;
    H = .5 * outputs[CGP_O::HYGROMETRY_];
  }

  if (debugCGP) {
    std::cerr << __PRETTY_FUNCTION__ << " CGP Stepped" << std::endl;
    std::cerr << "\tTopology:";
    for (uint i=0; i<=_genome.voxels; i++)
      std::cerr << " " << _topology[i];
    std::cerr << "\n\tTemperature:";
    for (uint i=0; i<=_genome.voxels; i++)
      std::cerr << " " << _temperature[i];
    std::cerr << "\n\tHygrometry:";
    for (uint i=0; i<=_genome.voxels; i++)
      std::cerr << " " << _hygrometry[SHALLOW][i];
    std::cerr << std::endl;
  }
}

float Environment::heightAt(float x) const {
  if (!insideXRange(x)) return 0;
  return interpolate(_topology, x);
}

float Environment::temperatureAt(float x) const {
  if (!insideXRange(x)) return 0;
  return interpolate(_temperature, x);
}

float Environment::waterAt(const Point &p) const {
  if (!inside(p)) return 0; // No water outside world

  float h = heightAt(p.x);
  if (h < p.y)  return 0; // No water above ground

  float d = (h - p.y) / (h + yextent());
  assert(0 <= d && d <= 1);
  return interpolate(_hygrometry[SHALLOW], p.x) * (1.f - uint(d))
       + interpolate(_hygrometry[DEEP], p.x) * d;
}

float Environment::lightAt(float) const {
  return config::Simulation::baselineLight();
}

float Environment::interpolate(const Voxels &voxels, float x) const {
  float v = (x + xextent()) * float(_genome.voxels) / width();
  uint v0 = uint(v);
  float d = v - v0;

  assert(v0 <= _genome.voxels);

  if (v0 == _genome.voxels)
        return voxels[v0];
  else  return voxels[v0] * (1.f - d) + voxels[v0+1] * d;
}

const physics::UpperLayer::Items& Environment::canopy(const Plant *p) const {
  return _physics->canopy(p);
}

bool Environment::addCollisionData(Plant *p) {
  return _physics->addCollisionData(*this, p);
}

void Environment::updateCollisionData(Plant *p) {
  _physics->updateCollisions(p);
}

void Environment::updateCollisionDataFinal(Plant *p) {
  _physics->updateFinal(*this, p);
}

void Environment::removeCollisionData(Plant *p) {
  _physics->removeCollisionData(p);
}

bool Environment::isCollisionFree (const Plant *p) const {
  return _physics->isCollisionFree(p);
}

void Environment::disseminateGeneticMaterial(Organ *f) {
  _physics->addPistil(f);
}

void Environment::updateGeneticMaterial(Organ *f, const Point &oldPos) {
  _physics->updatePistil(f, oldPos);
}

void Environment::removeGeneticMaterial(Organ *f) {
  _physics->delPistil(f);
}

physics::Pistil Environment::collectGeneticMaterial(Organ *f) {
  auto itP = _physics->sporesInRange(f);

  if (std::distance(itP.first, itP.second) >= 1)
    return *dice()(itP.first, itP.second);

  else
    return physics::Pistil();
}

void Environment::processNewObjects(void) {
  _physics->processNewObjects();
}

// =============================================================================
// == Binary serialization

void save (nlohmann::json &j, const rng::FastDice &d) {
  std::ostringstream oss;
  serialize(oss, d);
  j = oss.str();
}

void load (const nlohmann::json &j, rng::FastDice &d) {
  std::istringstream iss (j.get<std::string>());
  deserialize(iss, d);
}

void Environment::save (nlohmann::json &j, const Environment &e) {
  nlohmann::json jd;
  simu::save(jd, e._dice);
  j = {
    e._genome, jd,
    e._topology, e._temperature, e._hygrometry,
    { e._time.year(), e._time.day(), e._time.hour() }
  };
}

void Environment::load (const nlohmann::json &j, Environment &e) {
  uint i=0;
  e._genome = j[i++];
  simu::load(j[i++], e._dice);
  e._topology = j[i++].get<Voxels>();
  e._temperature = j[i++].get<Voxels>();
  e._hygrometry = j[i++];

  float y = j[i][0], d = j[i][1], h = j[i][2];
  e._start.set(y, d, h);
  e._time = e._start;
}

} // end of namespace simu
