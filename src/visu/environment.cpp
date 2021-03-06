#include <QPainter>

#include "environment.h"
#include "../simu/tiniestphysicsengine.h"
#include "../config/simuconfig.h"

#ifdef DEBUG_COLLISIONS
#include "plant.h"
#endif

#include "qtconversions.hpp"

#include <QDebug>

namespace gui {

using ULItems = simu::physics::UpperLayer::Items;
using ULItem = simu::physics::UpperLayer::Item;

const QColor sky = QColor::fromRgbF(.13, .5, .7);
const QColor ground = QColor::fromRgbF(.5, .4, .3);

static constexpr int debugAABB = 0;
static constexpr int debugLeaves = 0;
static constexpr int debugSpores = 0;

QColor Environment::colorForTemperature (float t) {
  static const float maxAlpha = 1;
  const auto minT = _object.minTemperature();
  const auto maxT = _object.maxTemperature();
  const auto avgT = .5 * (maxT + minT);

  if (t < avgT)
    return QColor::fromRgbF(1, 1, 1, maxAlpha * (avgT - t) / (avgT - minT));
  else
    return QColor::fromRgbF(1, 0, 0, maxAlpha * (1 - (maxT - t) / (maxT - avgT)));
}

QColor colorForWater (float w) {
  static const auto baselineWater = config::Simulation::baselineShallowWater();
  return QColor::fromRgbF(0,0,1, .1 * w / baselineWater);
}

void debugPaintAABB (QPainter *painter, const simu::physics::CollisionObject *obj,
                     const QRectF &aabb) {

  QPen pen = painter->pen();
  painter->save();
    if (debugAABB & 1) {
      pen.setColor(Qt::red);
      painter->setPen(pen);
      painter->drawRect(aabb);
    }

    if (debugAABB & 2) {
      pen.setColor(Qt::blue);
      painter->setPen(pen);
      for (const simu::Organ *o: obj->plant->organs())
        painter->drawRect(toQRect(o->globalCoordinates().boundingRect));
    }

  painter->restore();
}

void debugPaintLeaves (QPainter *painter, const ULItems &items,
                       const QRectF &rect, Qt::GlobalColor c) {

  QPen basePen = painter->pen();
  basePen.setColor(c);
  QPen dotPen = basePen;
  dotPen.setStyle(Qt::DashLine);

  auto y = rect.top() - .05 * rect.height();
  auto dy = .025 * rect.height();

  painter->save();
  for (const ULItem &item: items) {
    auto dx = .05 * (item.r - item.l);

    if (debugLeaves & 1) {  // Draw upper layer segments
      painter->setPen(basePen);
      painter->drawLine(QPointF(item.l, y-dy), QPointF(item.l, y+dy));
        painter->drawLine(QPointF(item.l, y-dy), QPointF(item.l+dx, y-dy));
        painter->drawLine(QPointF(item.l, y+dy), QPointF(item.l+dx, y+dy));
      painter->drawLine(QPointF(item.r, y-dy), QPointF(item.r, y+dy));
        painter->drawLine(QPointF(item.r, y-dy), QPointF(item.r-dx, y-dy));
        painter->drawLine(QPointF(item.r, y+dy), QPointF(item.r-dx, y+dy));
      painter->drawLine(QPointF(item.l, y), QPointF(item.r, y));
    }

    if (debugLeaves & 3) {  // Draw line to associated organ
      painter->setPen(dotPen);
      auto oy = toQRect(item.organ->globalCoordinates().boundingRect).top();
      painter->drawLine(QPointF(.5 * (item.l + item.r), y),
                        QPointF(.5 * (item.l + item.r), oy));
    }
  }

  if ((debugLeaves & 2) && !items.empty()) {  // Draw upper layer enveloppe
    painter->setPen(dotPen);
    ULItem prev = items[0];
    auto prevY = toQRect(prev.organ->globalCoordinates().boundingRect).top();
    painter->drawLine(QPointF(prev.l, prevY), QPointF(prev.r, prevY));
    for (uint i=1; i<items.size(); i++) {
      ULItem curr = items[i];
      auto oy = toQRect(curr.organ->globalCoordinates().boundingRect).top();
      painter->drawLine(QPointF(prev.r, prevY), QPointF(curr.l, oy));
      painter->drawLine(QPointF(curr.l, oy), QPointF(curr.r, oy));
      prev = curr;
      prevY = oy;
    }
  }
  painter->restore();
}

void Environment::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
  // Draw slightly more on the right to hide the junction lines
  static constexpr float OVERDRAW = 0;

  const auto voxels = _object.voxelCount();
  const auto voxelWidth = _object.width() / voxels;
  const auto top = -_object.yextent(), bottom = _object.yextent();

  painter->fillRect(-_object.xextent(), -_object.yextent(),
                    _object.width(), _object.height(), Qt::black);

  for (uint i=0; i<voxels; i++) {
    const float x0 = -_object.xextent() + i * voxelWidth,
                x1 = std::min( _object.xextent(),
                              -_object.xextent() + (i+1+OVERDRAW) * voxelWidth);

    const auto y0 = _object.heightAt(x0), y1 = _object.heightAt(x1);
    const auto t0 = _object.temperatureAt(x0), t1 = _object.temperatureAt(x1);
    const auto w0 = _object.waterAt({x0,y0}), w1 = _object.waterAt({x1,y1});

    QLinearGradient heatGradient;
    heatGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    heatGradient.setStart(0, .5);
    heatGradient.setFinalStop(1, .5);
    heatGradient.setColorAt(0, colorForTemperature(t0));
    heatGradient.setColorAt(1, colorForTemperature(t1));

    QPainterPath above;
    above.moveTo(x0, -y0);
    above.lineTo(x0, top);
    above.lineTo(x1, top);
    above.lineTo(x1, -y1);
    above.closeSubpath();

    painter->fillPath(above, sky);
    painter->fillPath(above, heatGradient);


    QLinearGradient waterHGradient;
    waterHGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    waterHGradient.setStart(0, .5);
    waterHGradient.setFinalStop(1, .5);
    waterHGradient.setColorAt(0, colorForWater(w0));
    waterHGradient.setColorAt(1, colorForWater(w1));

    QLinearGradient waterVGradient;
    waterVGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    waterVGradient.setStart(.5, 0);
    waterVGradient.setFinalStop(.5, 1);
    waterVGradient.setColorAt(0, colorForWater(.5 * (w0 + w1)));
    waterVGradient.setColorAt(1, colorForWater(1));

    QPainterPath below;
    below.moveTo(x0, -y0);
    below.lineTo(x0, bottom);
    below.lineTo(x1, bottom);
    below.lineTo(x1, -y1);
    below.closeSubpath();
    painter->fillPath(below, ground);
    painter->fillPath(below, waterVGradient);
    painter->fillPath(below, waterHGradient);
  }

//  painter->save();
//  QPen debugPen = painter->pen();
//  debugPen.setWidthF(0);
//  debugPen.setColor(Qt::red);
//  painter->setPen(debugPen);
//  for (uint i=0; i<=voxels; i++) {
//    const auto x = -_object.xextent() + i * voxelWidth;
//    painter->drawLine(x, -1, x, 1);
//  }
//  painter->restore();

  painter->save();
  QPen pen = painter->pen();
  pen.setWidthF(0);
  painter->setPen(pen);

  if (debugAABB || debugLeaves) {
    using CData = simu::physics::TinyPhysicsEngine;
    using CObject = simu::physics::CollisionObject;

    painter->save();

    const CData &cd = _object.collisionData();
    for (const CObject *obj: cd.data()) {
      QRectF br = toQRect(obj->boundingRect);

      if (debugAABB)
        debugPaintAABB(painter, obj, br);

      if (debugLeaves) {
        debugPaintLeaves(painter, obj->layer.itemsInIsolation, br, Qt::gray);
        debugPaintLeaves(painter, obj->layer.itemsInWorld, br, Qt::white);
      }
    }
    painter->restore();
  }

  if (debugSpores) {
    painter->save();
    pen.setColor(Qt::blue);
    painter->setPen(pen);
    painter->setBrush(QColor::fromRgbF(0, 0, 1, .25));

    const auto& pistils = _object.collisionData().pistils();
    for (const simu::physics::Pistil &s: pistils)
      if (s.organ->fullness() > .5)
        painter->drawEllipse(toQPoint(s.boundingDisk.center),
                             s.boundingDisk.radius, s.boundingDisk.radius);

    painter->restore();
  }

#ifdef DEBUG_COLLISIONS
  for (const auto &p: _object.collisionData().collisionsDebugData)
    for (const auto &bco: p.second)
      Plant::paint(painter, 1, simu::Organ::OID::INVALID,
                   Qt::red, QColor::fromRgbF(1, 0, 0, .25),
                   bco.pos, bco.rot, bco.symb, bco.w, bco.l);
#endif

  painter->restore();

//  painter->save();
//  painter->setPen(pen);
//  painter->drawLine(-_object.xextent(), 0, _object.xextent(), 0);
//  painter->restore();
}

} // end of namespace gui
