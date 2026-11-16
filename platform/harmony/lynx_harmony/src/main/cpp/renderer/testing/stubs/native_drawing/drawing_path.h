// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_TESTING_STUBS_NATIVE_DRAWING_DRAWING_PATH_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_TESTING_STUBS_NATIVE_DRAWING_DRAWING_PATH_H_
#include <array>
#include <vector>

// A geometry-recording boundary for host tests, not a Drawing rasterizer.
enum OH_Drawing_ErrorCode {
  OH_DRAWING_SUCCESS,
  OH_DRAWING_ERROR_INVALID_PARAMETER
};
enum OH_Drawing_PathDirection { PATH_DIRECTION_CW, PATH_DIRECTION_CCW };
enum OH_Drawing_PathFillType {
  PATH_FILL_TYPE_WINDING,
  PATH_FILL_TYPE_EVEN_ODD
};
enum OH_Drawing_CornerPos {
  CORNER_POS_TOP_LEFT,
  CORNER_POS_TOP_RIGHT,
  CORNER_POS_BOTTOM_RIGHT,
  CORNER_POS_BOTTOM_LEFT
};
struct OH_Drawing_Corner_Radii {
  float x;
  float y;
};
struct OH_Drawing_Rect {
  float left, top, right, bottom;
};
struct OH_Drawing_RoundRect {
  OH_Drawing_Rect rect;
  std::array<float, 8> radii;
};
struct OH_Drawing_Matrix {
  float x, y;
};
struct OH_Drawing_Path {
  std::vector<char> verbs;
  std::vector<float> points;
  std::array<float, 8> radii{};
  OH_Drawing_PathFillType fill = PATH_FILL_TYPE_WINDING;
};
extern "C" {
OH_Drawing_Path* OH_Drawing_PathCreate();
void OH_Drawing_PathDestroy(OH_Drawing_Path*);
void OH_Drawing_PathMoveTo(OH_Drawing_Path*, float, float);
void OH_Drawing_PathLineTo(OH_Drawing_Path*, float, float);
void OH_Drawing_PathQuadTo(OH_Drawing_Path*, float, float, float, float);
void OH_Drawing_PathCubicTo(OH_Drawing_Path*, float, float, float, float, float,
                            float);
void OH_Drawing_PathClose(OH_Drawing_Path*);
void OH_Drawing_PathAddRect(OH_Drawing_Path*, float, float, float, float,
                            OH_Drawing_PathDirection);
void OH_Drawing_PathAddRoundRect(OH_Drawing_Path*, const OH_Drawing_RoundRect*,
                                 OH_Drawing_PathDirection);
bool OH_Drawing_PathBuildFromSvgString(OH_Drawing_Path*, const char*);
void OH_Drawing_PathTransform(OH_Drawing_Path*, const OH_Drawing_Matrix*);
void OH_Drawing_PathSetFillType(OH_Drawing_Path*, OH_Drawing_PathFillType);
void OH_Drawing_PathGetBounds(OH_Drawing_Path*, OH_Drawing_Rect*);
OH_Drawing_ErrorCode OH_Drawing_ErrorCodeGet();
void OH_Drawing_ErrorCodeReset();
OH_Drawing_Rect* OH_Drawing_RectCreate(float, float, float, float);
void OH_Drawing_RectDestroy(OH_Drawing_Rect*);
float OH_Drawing_RectGetLeft(OH_Drawing_Rect*);
float OH_Drawing_RectGetTop(OH_Drawing_Rect*);
float OH_Drawing_RectGetRight(OH_Drawing_Rect*);
float OH_Drawing_RectGetBottom(OH_Drawing_Rect*);
OH_Drawing_RoundRect* OH_Drawing_RoundRectCreate(const OH_Drawing_Rect*, float,
                                                 float);
void OH_Drawing_RoundRectDestroy(OH_Drawing_RoundRect*);
void OH_Drawing_RoundRectSetCorner(OH_Drawing_RoundRect*, OH_Drawing_CornerPos,
                                   OH_Drawing_Corner_Radii);
OH_Drawing_Matrix* OH_Drawing_MatrixCreateScale(float, float, float, float);
void OH_Drawing_MatrixDestroy(OH_Drawing_Matrix*);
}
namespace lynx::testing {
struct DrawingState {
  int creates = 0;
  int destroys = 0;
  int live = 0;
  int parses = 0;
  int auxiliary_live = 0;
  int fail_allocation = -1;
  bool fail_operation = false;
};
DrawingState& GetDrawingState();
}  // namespace lynx::testing
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_TESTING_STUBS_NATIVE_DRAWING_DRAWING_PATH_H_
