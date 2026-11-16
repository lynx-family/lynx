// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>

#include "gfx/public/path_parser.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/testing/stubs/native_drawing/drawing_path.h"

namespace lynx::testing {
DrawingState& GetDrawingState() {
  static DrawingState state;
  return state;
}

}  // namespace lynx::testing
namespace {
auto& State() { return lynx::testing::GetDrawingState(); }
OH_Drawing_ErrorCode error = OH_DRAWING_SUCCESS;
bool FailAllocation() {
  if (State().fail_allocation < 0) return false;
  if (State().fail_allocation-- == 0) return true;
  return false;
}
void Add(OH_Drawing_Path* path, char verb,
         std::initializer_list<float> points) {
  path->verbs.push_back(verb);
  path->points.insert(path->points.end(), points);
  if (State().fail_operation) error = OH_DRAWING_ERROR_INVALID_PARAMETER;
}
}  // namespace
extern "C" {
OH_Drawing_Path* OH_Drawing_PathCreate() {
  ++State().creates;
  if (FailAllocation()) return nullptr;
  ++State().live;
  return new OH_Drawing_Path;
}
void OH_Drawing_PathDestroy(OH_Drawing_Path* p) {
  ++State().destroys;
  --State().live;
  delete p;
}
void OH_Drawing_PathMoveTo(OH_Drawing_Path* p, float x, float y) {
  Add(p, 'M', {x, y});
}
void OH_Drawing_PathLineTo(OH_Drawing_Path* p, float x, float y) {
  Add(p, 'L', {x, y});
}
void OH_Drawing_PathQuadTo(OH_Drawing_Path* p, float x, float y, float u,
                           float v) {
  Add(p, 'Q', {x, y, u, v});
}
void OH_Drawing_PathCubicTo(OH_Drawing_Path* p, float x, float y, float u,
                            float v, float a, float b) {
  Add(p, 'C', {x, y, u, v, a, b});
}
void OH_Drawing_PathClose(OH_Drawing_Path* p) { Add(p, 'Z', {}); }
void OH_Drawing_PathAddRect(OH_Drawing_Path* p, float l, float t, float r,
                            float b, OH_Drawing_PathDirection) {
  Add(p, 'R', {l, t, r, b});
}
void OH_Drawing_PathAddRoundRect(OH_Drawing_Path* p,
                                 const OH_Drawing_RoundRect* r,
                                 OH_Drawing_PathDirection) {
  Add(p, 'R', {r->rect.left, r->rect.top, r->rect.right, r->rect.bottom});
  p->radii = r->radii;
}
bool OH_Drawing_PathBuildFromSvgString(OH_Drawing_Path* p, const char* data) {
  ++State().parses;
  LBSPathConsumer c{};
  c.ctx = p;
  c.MoveToPoint = [](void* p, float x, float y) {
    OH_Drawing_PathMoveTo(static_cast<OH_Drawing_Path*>(p), x, y);
  };
  c.LineToPoint = [](void* p, float x, float y) {
    OH_Drawing_PathLineTo(static_cast<OH_Drawing_Path*>(p), x, y);
  };
  c.QuadToPoint = [](void* p, float x, float y, float u, float v) {
    OH_Drawing_PathQuadTo(static_cast<OH_Drawing_Path*>(p), x, y, u, v);
  };
  c.CubicToPoint = [](void* p, float x, float y, float u, float v, float a,
                      float b) {
    OH_Drawing_PathCubicTo(static_cast<OH_Drawing_Path*>(p), x, y, u, v, a, b);
  };
  c.ClosePath = [](void* p) {
    OH_Drawing_PathClose(static_cast<OH_Drawing_Path*>(p));
  };
  LBSParsePathWithConsumer(data, &c);
  return c.error == 0;
}
void OH_Drawing_PathTransform(OH_Drawing_Path* p, const OH_Drawing_Matrix* m) {
  for (size_t i = 0; i < p->points.size(); ++i)
    p->points[i] *= i % 2 ? m->y : m->x;
}
void OH_Drawing_PathSetFillType(OH_Drawing_Path* p, OH_Drawing_PathFillType f) {
  p->fill = f;
}
void OH_Drawing_PathGetBounds(OH_Drawing_Path* p, OH_Drawing_Rect* bounds) {
  if (p->points.empty()) {
    *bounds = {0, 0, 0, 0};
    return;
  }
  *bounds = {p->points[0], p->points[1], p->points[0], p->points[1]};
  for (size_t i = 2; i < p->points.size(); i += 2) {
    bounds->left = std::min(bounds->left, p->points[i]);
    bounds->top = std::min(bounds->top, p->points[i + 1]);
    bounds->right = std::max(bounds->right, p->points[i]);
    bounds->bottom = std::max(bounds->bottom, p->points[i + 1]);
  }
}
OH_Drawing_ErrorCode OH_Drawing_ErrorCodeGet() { return error; }
void OH_Drawing_ErrorCodeReset() { error = OH_DRAWING_SUCCESS; }
OH_Drawing_Rect* OH_Drawing_RectCreate(float l, float t, float r, float b) {
  if (FailAllocation()) return nullptr;
  ++State().auxiliary_live;
  return new OH_Drawing_Rect{l, t, r, b};
}
void OH_Drawing_RectDestroy(OH_Drawing_Rect* p) {
  --State().auxiliary_live;
  delete p;
}
float OH_Drawing_RectGetLeft(OH_Drawing_Rect* rect) { return rect->left; }
float OH_Drawing_RectGetTop(OH_Drawing_Rect* rect) { return rect->top; }
float OH_Drawing_RectGetRight(OH_Drawing_Rect* rect) { return rect->right; }
float OH_Drawing_RectGetBottom(OH_Drawing_Rect* rect) { return rect->bottom; }
OH_Drawing_RoundRect* OH_Drawing_RoundRectCreate(const OH_Drawing_Rect* r,
                                                 float x, float y) {
  if (FailAllocation()) return nullptr;
  ++State().auxiliary_live;
  return new OH_Drawing_RoundRect{*r, {x, y, x, y, x, y, x, y}};
}
void OH_Drawing_RoundRectDestroy(OH_Drawing_RoundRect* p) {
  --State().auxiliary_live;
  delete p;
}
void OH_Drawing_RoundRectSetCorner(OH_Drawing_RoundRect* r,
                                   OH_Drawing_CornerPos pos,
                                   OH_Drawing_Corner_Radii v) {
  r->radii[2 * pos] = v.x;
  r->radii[2 * pos + 1] = v.y;
}
OH_Drawing_Matrix* OH_Drawing_MatrixCreateScale(float x, float y, float,
                                                float) {
  if (FailAllocation()) return nullptr;
  ++State().auxiliary_live;
  return new OH_Drawing_Matrix{x, y};
}
void OH_Drawing_MatrixDestroy(OH_Drawing_Matrix* p) {
  --State().auxiliary_live;
  delete p;
}
}
