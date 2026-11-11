// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/path_parser.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>

namespace lynx {
namespace tasm {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr size_t kMaxCommands = 65536;
struct Point {
  double x = 0, y = 0;
};
Point operator+(Point a, Point b) { return {a.x + b.x, a.y + b.y}; }
Point operator-(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
Point operator*(Point p, double s) { return {p.x * s, p.y * s}; }
void Emit(Path& out, PathVerb verb, Point p = {}, Point a = {}, Point b = {}) {
  out.commands.push_back({verb, static_cast<float>(p.x),
                          static_cast<float>(p.y), static_cast<float>(a.x),
                          static_cast<float>(a.y), static_cast<float>(b.x),
                          static_cast<float>(b.y)});
}

// SVG endpoint-parameterized arc to cubic segments of at most a quarter turn.
void Arc(Path& out, Point from, Point end, double rx, double ry, double angle,
         bool large, bool sweep) {
  rx = std::abs(rx);
  ry = std::abs(ry);
  if (from.x == end.x && from.y == end.y) return;
  if (!rx || !ry) {
    Emit(out, PathVerb::kLine, end);
    return;
  }
  const double phi = std::fmod(angle, 360.) * (kPi / 180.), c = std::cos(phi),
               s = std::sin(phi);
  Point half = (from - end) * .5;
  double x = c * half.x + s * half.y, y = -s * half.x + c * half.y;
  double lambda = std::hypot(x / rx, y / ry);
  if (lambda > 1) {
    rx *= lambda;
    ry *= lambda;
  }
  double u = x / rx, v = y / ry, q = u * u + v * v;
  double k = (large == sweep ? -1 : 1) * std::sqrt(std::max(0., (1 - q) / q));
  double cx = k * rx * v, cy = -k * ry * u;
  Point center = (from + end) * .5 + Point{c * cx - s * cy, s * cx + c * cy};
  double t = std::atan2((y - cy) / ry, (x - cx) / rx);
  double delta = std::atan2((-y - cy) / ry, (-x - cx) / rx) - t;
  if (sweep && delta < 0) delta += 2 * kPi;
  if (!sweep && delta > 0) delta -= 2 * kPi;
  if (!std::isfinite(delta)) {
    Emit(out, PathVerb::kLine, end);
    return;
  }
  int n =
      std::max(1, static_cast<int>(std::ceil(std::abs(delta) / (kPi * .5))));
  double step = delta / n;
  auto map = [&](double a, double b) {
    return center + Point{rx * c * a - ry * s * b, rx * s * a + ry * c * b};
  };
  for (int i = 0; i < n; i++) {
    double t1 = t + step, h = 4. / 3. * std::tan(step * .25);
    Emit(out, PathVerb::kCubic,
         i + 1 == n ? end : map(std::cos(t1), std::sin(t1)),
         map(std::cos(t) - h * std::sin(t), std::sin(t) + h * std::cos(t)),
         map(std::cos(t1) + h * std::sin(t1), std::sin(t1) - h * std::cos(t1)));
    t = t1;
  }
}

bool Digit(char c) { return c >= '0' && c <= '9'; }
// Decode the SVG path data in CSS path() into absolute Path commands.
class PathParser {
 public:
  explicit PathParser(const char* p) : p_(p) {}
  bool Read(Path& out) {
    char cmd = 0, prev = 0;
    Point p{}, start{}, control{};
    while (true) {
      Skip();
      if (!*p_) return true;
      if ((*p_ >= 'A' && *p_ <= 'Z') || (*p_ >= 'a' && *p_ <= 'z')) {
        cmd = *p_++;
        allow_comma_ = false;
      }
      bool relative = cmd >= 'a' && cmd <= 'z';
      char op = relative ? cmd - ('a' - 'A') : cmd;
      if ((!prev && op != 'M') || out.commands.size() >= kMaxCommands)
        return false;
      Point a, b, end;
      auto point = [&](Point& v) {
        if (!Number(v.x) || !Number(v.y)) return false;
        if (relative) v = v + p;
        return true;
      };
      switch (op) {
        case 'M':
        case 'L':
          if (!point(end)) return false;
          Emit(out, op == 'M' ? PathVerb::kMove : PathVerb::kLine, end);
          if (op == 'M') {
            start = end;
            cmd = relative ? 'l' : 'L';
          }
          break;
        case 'H':
        case 'V': {
          double v;
          if (!Number(v)) return false;
          end = p;
          if (op == 'H')
            end.x = (relative ? p.x : 0) + v;
          else
            end.y = (relative ? p.y : 0) + v;
          Emit(out, PathVerb::kLine, end);
          break;
        }
        case 'C':
        case 'S':
          if (op == 'C') {
            if (!point(a)) return false;
          } else
            a = (prev == 'C' || prev == 'S') ? p * 2 - control : p;
          if (!point(b) || !point(end)) return false;
          Emit(out, PathVerb::kCubic, end, a, b);
          control = b;
          break;
        case 'Q':
        case 'T':
          if (op == 'Q') {
            if (!point(a)) return false;
          } else
            a = (prev == 'Q' || prev == 'T') ? p * 2 - control : p;
          if (!point(end)) return false;
          Emit(out, PathVerb::kCubic, end, p + (a - p) * (2. / 3),
               end + (a - end) * (2. / 3));
          control = a;
          break;
        case 'A': {
          double rx, ry, angle;
          bool large, sweep;
          if (!Number(rx) || !Number(ry) || !Number(angle) || !Flag(large) ||
              !Flag(sweep) || !point(end))
            return false;
          Arc(out, p, end, rx, ry, angle, large, sweep);
          break;
        }
        case 'Z':
          Emit(out, PathVerb::kClose);
          end = start;
          cmd = 0;
          break;
        default:
          return false;
      }
      prev = op;
      p = end;
    }
  }

 private:
  void Skip() {
    while (*p_ == ' ' || *p_ == '\t' || *p_ == '\r' || *p_ == '\n') ++p_;
  }
  void SkipArgumentSeparator() {
    Skip();
    if (allow_comma_ && *p_ == ',') {
      ++p_;
      Skip();
    }
    allow_comma_ = true;
  }
  bool Flag(bool& v) {
    SkipArgumentSeparator();
    if (*p_ != '0' && *p_ != '1') return false;
    v = *p_++ == '1';
    return true;
  }
  bool Number(double& v) {
    SkipArgumentSeparator();
    const char* start = p_;
    if (*p_ == '+' || *p_ == '-') ++p_;
    const char* whole = p_;
    while (Digit(*p_)) ++p_;
    bool digits = p_ != whole;
    if (*p_ == '.') {
      ++p_;
      const char* frac = p_;
      while (Digit(*p_)) ++p_;
      digits |= p_ != frac;
    }
    if (!digits) return false;
    if (*p_ == 'e' || *p_ == 'E') {
      ++p_;
      if (*p_ == '+' || *p_ == '-') ++p_;
      if (!Digit(*p_)) return false;
      while (Digit(*p_)) ++p_;
    }
    char* end;
    v = std::strtod(start, &end);
    return end == p_ && std::isfinite(v) &&
           std::abs(v) <= std::numeric_limits<float>::max();
  }
  const char* p_;
  bool allow_comma_ = false;
};

}  // namespace

bool ParseSvgPath(const std::string& data, float scale, Path& out) {
  if (data.find('\0') != std::string::npos) return false;
  Path parsed;
  if (!PathParser(data.c_str()).Read(parsed)) return false;
  for (auto& c : parsed.commands) {
    c.x *= scale;
    c.y *= scale;
    c.x1 *= scale;
    c.y1 *= scale;
    c.x2 *= scale;
    c.y2 *= scale;
    if (!std::isfinite(c.x) || !std::isfinite(c.y) || !std::isfinite(c.x1) ||
        !std::isfinite(c.y1) || !std::isfinite(c.x2) || !std::isfinite(c.y2))
      return false;
  }
  out = std::move(parsed);
  return true;
}

}  // namespace tasm
}  // namespace lynx
