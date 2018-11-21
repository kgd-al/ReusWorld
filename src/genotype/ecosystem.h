#ifndef ECOSYSTEM_H
#define ECOSYSTEM_H

#include "plant.h"

namespace genotype {

class Environment : public SelfAwareGenome<Environment> {
  APT_SAG()
public:
  uint rngSeed;
  float width, depth;
};

DECLARE_GENOME_FIELD(Environment, uint, rngSeed)
DECLARE_GENOME_FIELD(Environment, float, width)
DECLARE_GENOME_FIELD(Environment, float, depth)

class Ecosystem : public SelfAwareGenome<Ecosystem> {
  APT_SAG()
public:
  uint initSeeds;
  Plant plant;
  Environment env;
};

DECLARE_GENOME_FIELD(Ecosystem, uint, initSeeds)
DECLARE_GENOME_FIELD(Ecosystem, Plant, plant)
DECLARE_GENOME_FIELD(Ecosystem, Environment, env)

} // end of namespace genotype

namespace config {

template <>
struct SAG_CONFIG_FILE(Environment) {
  using Bui = Bounds<uint>;
  using Bf = Bounds<float>;
  DECLARE_PARAMETER(Bui, rngSeedBounds)
  DECLARE_PARAMETER(Bf, widthBounds)
  DECLARE_PARAMETER(Bf, depthBounds)

  DECLARE_PARAMETER(MutationRates, mutationRates)
};

template <>
struct SAG_CONFIG_FILE(Ecosystem) {
  using Bui = Bounds<uint>;
  DECLARE_PARAMETER(Bui, initSeedsBounds)

  DECLARE_PARAMETER(MutationRates, mutationRates)
};

} // end of namespace config

#endif // ECOSYSTEM_H
