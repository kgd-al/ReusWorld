#ifndef ORGAN_H
#define ORGAN_H

#include "../genotype/plant.h"

#include "types.h"

namespace simu {

struct Plant;

class Organ {
public:
  using Layer = genotype::LSystemType;
  enum OID : uint { INVALID = uint(-1) };

  using Corners = std::array<Point, 4>;

  struct ParentCoordinates {
   float rotation;
  };

  struct PlantCoordinates {
    Point origin, end;
    Point center;
    float rotation;
    Rect boundingRect;
    Corners corners;
  };

  struct GlobalCoordinates {
    Point origin;
    Point center;
    Rect boundingRect;
    Corners corners;
  };

private:
  OID _id;
  Plant *const _plant;

  ParentCoordinates _parentCoordinates;
  PlantCoordinates _plantCoordinates;
  GlobalCoordinates _globalCoordinates;

  float _width, _length;  // Constant for now
  char _symbol;           // To choose color and shape
  Layer _layer;           // To check altitude and symbol

  Organ *_parent;
  std::set<Organ*> _children;
  uint _depth;  ///< Height of the subtree rooted here

  float _surface;
  float _baseBiomass, _accumulatedBiomass, _requiredBiomass;

public:
  Organ (OID id, Plant *plant, float w, float l, float r, char c, Layer t,
         Organ *p = nullptr);

  /// surface, biomass
  void accumulate (float biomass);
  void updateDimensions(bool andTransformations);

  /// _boundingRect
  void updateTransformation (void);
  void updateBoundingBox (void);
  void updateGlobalTransformation (void);
  void updateParent (Organ *newParent);
  void updateRotation (float d_angle);

  void removeFromParent(void);
  void restoreInParent(void);

  auto id (void) const {    return _id;     }
  auto plant (void) const { return _plant;  }

  float localRotation (void) const {
    return _parentCoordinates.rotation;
  }
  const auto& inPlantCoordinates (void) const {  return _plantCoordinates; }
  const auto& globalCoordinates (void) const { return _globalCoordinates; }

  auto width (void) const {   return _width;    }
  auto length (void) const {  return _length;   }
  auto surface (void) const { return _surface;  }

  auto baseBiomass (void) const {
    return _baseBiomass;
  }
  auto accumulatedBiomass (void) const {
    return _accumulatedBiomass;
  }
  auto biomass (void) const {
    return _baseBiomass + _accumulatedBiomass;
  }

  void setRequiredBiomass (float rb) {
    _requiredBiomass = rb;
  }
  auto requiredBiomass (void) const {
    return std::max(0.f, _requiredBiomass);
  }
  float fullness (void) const;  ///< Returns ratio of accumulated biomass [0,1]

  auto symbol (void) const {  return _symbol; }
  auto layer (void) const {   return _layer;  }

  auto parent (void) {  return _parent; }
  const auto parent (void) const {  return _parent; }

  auto& children (void) { return _children; }
  const auto& children (void) const { return _children; }

  void updateDepth (uint newDepth);
  void updateParentDepth (void);
  auto depth (void) const {
    return _depth;
  }

  bool isDead (void) const {
    return biomass() < 0;
  }

  bool isSeed (void) const {
    return _symbol == config::PlantGenome::ls_axiom();
  }
  bool isNonTerminal (void) const {
    return genotype::grammar::Rule_base::isValidNonTerminal(_symbol);
  }
  bool isLeaf (void) const {    return _symbol == 'l';  }
  bool isHair (void) const {    return _symbol == 'h';  }
  bool isFlower (void) const {  return _symbol == 'f';  }
  bool isStructural (void) const {
    return _symbol == 's' || _symbol == 't';
  }

  bool isFruit (void) const {
    return _symbol == genotype::grammar::Rule_base::fruitSymbol();
  }

  friend std::ostream& operator<< (std::ostream &os, const Organ &o);
};

} // end of namespace simu

#endif // ORGAN_H