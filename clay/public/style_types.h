// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_PUBLIC_STYLE_TYPES_H_
#define CLAY_PUBLIC_STYLE_TYPES_H_

#include <limits.h>

//------------------------------------------------------------------------------
// Enumerations
//------------------------------------------------------------------------------
enum class ClayOverflowType {
  kVisible = 0,
  kHidden,
  kScroll,
};

enum class ClayTimingFunctionType {
  kLinear = 0,
  kEaseIn,
  kEaseOut,
  kEaseInEaseOut,
  kSquareBezier,
  kCubicBezier,
  kSteps,
};

enum class ClayStepsType {
  kInvalid = 0,
  kStart,
  kEnd,
  kJumpBoth,
  kJumpNone,
};

enum class ClayAnimationPropertyType {
  kNone = 0,
  kOpacity = 1 << 0,
  kScaleX = 1 << 1,
  kScaleY = 1 << 2,
  kScaleXY = 1 << 3,
  kWidth = 1 << 4,
  kHeight = 1 << 5,
  kBackgroundColor = 1 << 6,
  kVisibility = 1 << 7,
  kLeft = 1 << 8,
  kTop = 1 << 9,
  kRight = 1 << 10,
  kBottom = 1 << 11,
  kTransform = 1 << 12,
  kColor = 1 << 13,
  kFilter = 1 << 14,
  kBoxShadow = 1 << 15,
  kAll = kOpacity | kWidth | kHeight | kBackgroundColor | kVisibility | kLeft |
         kTop | kRight | kBottom | kTransform | kColor | kFilter | kBoxShadow
};

enum class ClayTextDecorationType {
  kNone = 0,
  kUnderLine = 1 << 0,
  kLineThrough = 1 << 1,
};

enum class ClayShadowOption {
  kNone = 0,
  kInset = 1,
  kInitial = 2,
  kInherit = 3,
};

enum class ClayBackgroundImageType {
  kNone,
  kUrl,
  kLinearGradient,
  kRadialGradient,
  kConicGradient,
};

enum class ClayBackgroundOriginType {
  kPaddingBox = 0,
  kBorderBox = 1,
  kContentBox = 2,
};

enum class ClayBackgroundRepeatType {
  kRepeat = 0,
  kNoRepeat = 1,
  kRepeatX = 2,
  kRepeatY = 3,
  kRound = 4,
  kSpace = 5,
};

enum class ClayBackgroundSizeType {
  kAuto = (1 << 5),
  kCover,
  kContain,
};

enum class ClayBackgroundClipType {
  kPaddingBox = 0,
  kBorderBox = 1,
  kContentBox = 2,
  kText = 3,
};

enum class ClayMaskClipType {
  kPaddingBox = 0,
  kBorderBox = 1,
  kContentBox = 2,
};

using ClayMaskImageType = ClayBackgroundImageType;
using ClayMaskOriginType = ClayBackgroundOriginType;
using ClayMaskRepeatType = ClayBackgroundRepeatType;
using ClayMaskSizeType = ClayBackgroundSizeType;

enum class ClayPlatformLengthUnit {
  kNumber = 0,
  kPercentage,
};

enum class ClayGradientPositionType {
  kPercent,
  kNumber,
};

enum class ClayFilterType {
  kNone = 0,
  kGrayScale = 1,
  kBlur = 2,
};

enum class ClayImageRendering {
  kAuto,
  kCrispEdges,
  kPixelated,
};

enum class ClayRadialGradientShapeType {
  kEllipse = 0,
  kCircle,
};

enum class ClayRadialGradientSizeType {
  kFarthestCorner = 0,
  kFarthestSide,
  kClosestCorner,
  kClosestSide,
  kLength,
};

enum class ClayRadialGradientCenterType {
  BACKGROUND_POSITION_TOP = -(1 << 5),
  BACKGROUND_POSITION_RIGHT = -(1 << 5) - 1,
  BACKGROUND_POSITION_BOTTOM = -(1 << 5) - 2,
  BACKGROUND_POSITION_LEFT = -(1 << 5) - 3,
  BACKGROUND_POSITION_CENTER = -(1 << 5) - 4,
  RADIAL_CENTER_TYPE_PERCENTAGE = -11,  // according to lynx
};

enum class ClayBasicShapeType {
  kUnknown = 0,
  kCircle,
  kEllipse,
  kPath,
  kSuperEllipse,
  kInset,
};

enum class ClayAnimationDirectionType {
  kNormal = 0,
  kReverse,
  kAlternate,
  kAlternateReverse,
};

enum class ClayAnimationFillModeType {
  kNone = 0,
  kForwards,
  kBackwards,
  kBoth,
};

enum class ClayAnimationPlayStateType {
  kPaused = 0,
  kRunning,
};

enum class ClayVisibilityType {
  kHidden = 0,
  kVisible,
  kNone,
  kCollapse,
};

enum class ClayCursorType {
  kUrl = 0,
  kKeyword,
};

enum class ClayTransformType {
  kNone = 0,
  kTranslate = 1,
  kTranslateX = 1 << 1,
  kTranslateY = 1 << 2,
  kTranslateZ = 1 << 3,
  kTranslate3d = 1 << 4,
  kRotate = 1 << 5,
  kRotateX = 1 << 6,
  kRotateY = 1 << 7,
  kRotateZ = 1 << 8,
  kScale = 1 << 9,
  kScaleX = 1 << 10,
  kScaleY = 1 << 11,
  kSkew = 1 << 12,
  kSkewX = 1 << 13,
  kSkewY = 1 << 14,
  kMatrix = 1 << 15,
  kMatrix3d = 1 << 16,
};

//------------------------------------------------------------------------------
// structs
//------------------------------------------------------------------------------
typedef struct ClayTransformOP {
  ClayTransformType type;
  float value[3];
  ClayPlatformLengthUnit unit[3];
  double matrix[16];
} ClayTransformOP;

typedef struct ClayTransform {
  ClayTransformOP* op;
  int size;
  float transform_origin[2];
} ClayTransform;

typedef struct ClayLayoutStyles {
  int padding_left;
  int padding_top;
  int padding_right;
  int padding_bottom;
  int margin_left;
  int margin_top;
  int margin_right;
  int margin_bottom;
  float width;
  float height;
} ClayLayoutStyles;

typedef struct ClayMeasureOutput {
  float width;
  float height;
} ClayMeasureOutput;

typedef struct ClayPlatformLength {
  double value = 0.0;
  ClayPlatformLengthUnit unit;
} ClayPlatformLength;

typedef struct ClayLinearGradient {
  double degree = 0.0;
  // Owned by embedder.
  unsigned int* colors = nullptr;
  int colors_length = 0;
  // Owned by embedder.
  float* positions = nullptr;
  // Owned by embedder.
  ClayGradientPositionType* position_types = nullptr;
  int positions_length = 0;
} ClayLinearGradient;

typedef struct ClayRadialGradient {
  ClayRadialGradientShapeType shape_type;
  ClayRadialGradientSizeType shape_size;  // according to RadialGradientExtent
  unsigned int* colors = nullptr;
  int colors_length = 0;
  float* positions = nullptr;
  ClayGradientPositionType* position_types = nullptr;
  int positions_length = 0;
  ClayRadialGradientCenterType center_x;
  float center_x_value;
  ClayRadialGradientCenterType center_y;
  float center_y_value;
  ClayPlatformLengthUnit length_x_unit;
  float length_x;
  ClayPlatformLengthUnit length_y_unit;
  float length_y;
} ClayRadialGradient;

typedef struct ClayConicGradient {
  unsigned int* colors = nullptr;
  int colors_length = 0;
  float* positions = nullptr;
  ClayGradientPositionType* position_types = nullptr;
  int positions_length = 0;
  bool x_is_percent;
  float center_x;
  bool y_is_percent;
  float center_y;
  double start_angle = 0;
  double end_angle = 360.0;
} ClayConicGradient;

#endif  // CLAY_PUBLIC_STYLE_TYPES_H_
