#ifndef SIMU_TYPES_H
#define SIMU_TYPES_H

#include <cmath>

#include <ostream>

#include "kgd/external/json.hpp"

namespace simu {

struct Point {
  float x, y;

  static Point fromPolar (float a, float r) {
    return {  float(r * cos(a)), float(r * sin(a))  };
  }

  friend Point operator* (const Point &p, float f) {
    return Point{ f * p.x, f * p.y };
  }

  friend Point operator* (float f, const Point &p) {
    return p * f;
  }

  friend Point operator+ (const Point &lhs, const Point &rhs) {
    return { lhs.x + rhs.x, lhs.y + rhs.y };
  }

  Point& operator+= (const Point &that) {
    x += that.x;
    y += that.y;
    return *this;
  }

  friend Point operator- (const Point &lhs, const Point &rhs) {
    return Point { lhs.x - rhs.x, lhs.y - rhs.y };
  }

  friend bool operator== (const Point &lhs, const Point &rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }

  friend bool operator!= (const Point &lhs, const Point &rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y;
  }

  friend std::ostream& operator<< (std::ostream &os, const Point &p) {
    return os << "{" << p.x << "," << p.y << "}";
  }

  friend void to_json (nlohmann::json &j, const Point &p) {
    j = { p.x, p.y };
  }

  friend void from_json (const nlohmann::json &j, Point &p) {
    p.x = j[0]; p.y = j[1];
  }

  friend void assertEqual (const Point &lhs, const Point &rhs, bool deepcopy);
};

struct Rect {
  Point ul, br;

  static Rect invalid (void) {
    using L = std::numeric_limits<float>;
    return { { L::max(), -L::max() }, { -L::max(), L::max() } };
  }

  Rect& uniteWith (const Rect &that) {
    ul.x = std::min(ul.x, that.ul.x);
    ul.y = std::max(ul.y, that.ul.y);
    br.x = std::max(br.x, that.br.x);
    br.y = std::min(br.y, that.br.y);
    return *this;
  }

  auto r (void) const { return br.x;  }
  auto t (void) const { return ul.y;  }
  auto l (void) const { return ul.x;  }
  auto b (void) const { return br.y;  }

  auto width (void) const {
    return br.x - ul.x;
  }

  auto height (void) const {
    return ul.y - br.y;
  }

  auto center (void) const {
    return Point{ .5f * (ul.x + br.x), .5f * (ul.y + br.y) };
  }

  Rect translated (const Point &p) const {
    return { { ul.x + p.x, ul.y + p.y }, { br.x + p.x, br.y + p.y } };
  }

  bool isValid (void) const {
    return width() >= 0 && height() >= 0;
  }

  friend bool intersects (const Rect &lhs, const Rect &rhs) {
    return lhs.l() <= rhs.r() && lhs.r() >= rhs.l()
        && lhs.t() >= rhs.b() && lhs.b() <= rhs.t();
  }

  friend Rect intersection (const Rect &lhs, const Rect &rhs) {
    Rect intersection;
    intersection.ul.x = std::max(lhs.l(), rhs.l());
    intersection.ul.y = std::min(lhs.t(), rhs.t());
    intersection.br.x = std::min(lhs.r(), rhs.r());
    intersection.br.y = std::max(lhs.b(), rhs.b());
    return intersection;
  }

  friend std::ostream& operator<< (std::ostream &os, const Rect &r) {
    return os << "{ " << r.ul << ", " << r.br << " }";
  }

  friend void to_json (nlohmann::json &j, const Rect &r) {
    j = {r.ul, r.br};
  }

  friend void from_json (const nlohmann::json &j, Rect &r) {
    r.ul = j[0];  r.br = j[1];
  }

  friend void assertEqual (const Rect &lhs, const Rect &rhs, bool deepcopy);
};

struct Disk {
  Point center;
  float radius;

  friend bool intersects (const Disk &lhs, const Disk &rhs) {
    double dXSquared = lhs.center.x - rhs.center.x; // calc. delta X
    dXSquared *= dXSquared; // square delta X

    double dYSquared = lhs.center.y - lhs.center.y; // calc. delta Y
    dYSquared *= dYSquared; // square delta Y

    // Calculate the sum of the radii, then square it
    double sumRadiiSquared = lhs.radius + rhs.radius;
    sumRadiiSquared *= sumRadiiSquared;

    return dXSquared + dYSquared <= sumRadiiSquared;
  }

  friend std::ostream& operator<< (std::ostream &os, const Disk &d) {
    return os << "{" << d.center << ", " << d.radius << "}";
  }

  friend void assertEqual (const Disk &lhs, const Disk &rhs, bool deepcopy);
};

struct Position {
  Point start, end;
  float rotation;
};

struct Time {
  Time (void);

  const auto& year (void) const { return _year; }
  const auto& hour (void) const { return _hour; }
  const auto& day (void) const {  return _day;  }

  float timeOfDay (void) const;
  float timeOfYear (void) const;

  void set (uint y, uint d, uint h) {
    _year = y;  _day = d; _hour = h;
  }

  Time& next (void);
  std::string pretty (void) const;

  bool isStartOfYear (void) const {
    return _day == 0 && _hour == 0;
  }

  static Time fromTimestamp (uint step);
  uint toTimestamp (void) const;

  friend bool operator== (const Time &lhs, const Time &rhs) {
    return lhs._year == rhs._year
        && lhs._day == rhs._day
        && lhs._hour == rhs._hour;
  }

  friend bool operator< (const Time &lhs, const Time &rhs) {
    if (lhs._year != rhs._year) return lhs._year < rhs._year;
    if (lhs._day != rhs._day) return lhs._day < rhs._day;
    return lhs._hour < rhs._hour;
  }

  friend bool operator<= (const Time &lhs, const Time &rhs) {
    return lhs < rhs || lhs == rhs;
  }

  friend Time operator- (const Time &lhs, const Time &rhs) {
    return fromTimestamp(lhs.toTimestamp() - rhs.toTimestamp());
  }

  friend Time operator+ (const Time &lhs, uint years) {
    return Time(lhs._year+years, lhs._day, lhs._hour);
  }

  friend std::ostream& operator<< (std::ostream &os, const Time &t) {
    return os << "y" << t._year << "d" << t._day << "h" << t._hour;
  }

  friend std::istream& operator>> (std::istream &is, Time &t) {
    char c;
    is >> c >> t._year >> c >> t._day >> c >> t._hour;
    return is;
  }

  friend void to_json (nlohmann::json &j, const Time &t);
  friend void from_json (const nlohmann::json &j, Time &t);
  friend void assertEqual (const Time &lhs, const Time &rhs, bool deepcopy);

private:
  uint _year, _day, _hour;

  Time (uint y, uint d, uint h) : _year(y), _day(d), _hour(h) {}
};

} // end of namespace simu

#endif // SIMU_TYPES_H
