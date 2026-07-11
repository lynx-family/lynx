// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_RUST_CRATES_STARLIGHT_FFI_INCLUDE_STARLIGHT_RUST_FFI_H_
#define CORE_RENDERER_STARLIGHT_RUST_CRATES_STARLIGHT_FFI_INCLUDE_STARLIGHT_RUST_FFI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t SLRustNodeId;

#define SLRustAbiVersionMajor 1u
#define SLRustAbiVersionMinor 15u
#define SLRustAbiVersionPatch 0u

typedef enum SLRustStatus {
  SLRustStatusOk = 0,
  SLRustStatusNullPointer = 1,
  SLRustStatusMissingCallback = 2,
  SLRustStatusInvalidStyle = 3,
  SLRustStatusInvalidTree = 4,
  SLRustStatusPanic = 5,
  SLRustStatusAbiMismatch = 6,
  SLRustStatusDisabled = 7,
  SLRustStatusUnsupportedTree = 8,
  SLRustStatusFixedNodeSetMismatch = 9,
} SLRustStatus;

typedef struct SLRustAbiInfo {
  uint32_t version_major;
  uint32_t version_minor;
  uint32_t version_patch;
  size_t size_of_abi_info;
  size_t align_of_abi_info;
  size_t size_of_length;
  size_t align_of_length;
  size_t size_of_constraints;
  size_t align_of_constraints;
  size_t size_of_layout_result;
  size_t align_of_layout_result;
  size_t size_of_style;
  size_t align_of_style;
  size_t size_of_tree_callbacks;
  size_t align_of_tree_callbacks;
} SLRustAbiInfo;

typedef enum SLRustMeasureMode {
  SLRustMeasureModeIndefinite = 0,
  SLRustMeasureModeDefinite = 1,
  SLRustMeasureModeAtMost = 2,
} SLRustMeasureMode;

typedef enum SLRustLengthKind {
  SLRustLengthAuto = 0,
  SLRustLengthPoints = 1,
  SLRustLengthPercent = 2,
  SLRustLengthCalc = 3,
  SLRustLengthFr = 4,
  SLRustLengthMaxContent = 5,
  SLRustLengthFitContent = 6,
  SLRustLengthMinContent = 7,
} SLRustLengthKind;

enum {
  SLRustOptionalUnset = -1,
  SLRustGridLineAuto = 0,
};

typedef enum SLRustDisplay {
  SLRustDisplayNone = 0,
  SLRustDisplayBlock = 1,
  SLRustDisplayFlex = 2,
  SLRustDisplayLinear = 3,
  SLRustDisplayRelative = 4,
  SLRustDisplayGrid = 5,
} SLRustDisplay;

typedef enum SLRustPositionType {
  SLRustPositionStatic = 0,
  SLRustPositionRelative = 1,
  SLRustPositionAbsolute = 2,
  SLRustPositionFixed = 3,
  SLRustPositionSticky = 4,
} SLRustPositionType;

typedef enum SLRustBoxSizing {
  SLRustBoxSizingContentBox = 0,
  SLRustBoxSizingBorderBox = 1,
} SLRustBoxSizing;

typedef enum SLRustDirection {
  SLRustDirectionLtr = 0,
  SLRustDirectionRtl = 1,
} SLRustDirection;

typedef enum SLRustVisibility {
  SLRustVisibilityVisible = 0,
  SLRustVisibilityHidden = 1,
  SLRustVisibilityCollapse = 2,
} SLRustVisibility;

typedef enum SLRustFlexDirection {
  SLRustFlexDirectionRow = 0,
  SLRustFlexDirectionRowReverse = 1,
  SLRustFlexDirectionColumn = 2,
  SLRustFlexDirectionColumnReverse = 3,
} SLRustFlexDirection;

typedef enum SLRustLinearOrientation {
  SLRustLinearOrientationHorizontal = 0,
  SLRustLinearOrientationHorizontalReverse = 1,
  SLRustLinearOrientationVertical = 2,
  SLRustLinearOrientationVerticalReverse = 3,
  SLRustLinearOrientationRow = 4,
  SLRustLinearOrientationRowReverse = 5,
  SLRustLinearOrientationColumn = 6,
  SLRustLinearOrientationColumnReverse = 7,
} SLRustLinearOrientation;

typedef enum SLRustFlexWrap {
  SLRustFlexWrapNoWrap = 0,
  SLRustFlexWrapWrap = 1,
  SLRustFlexWrapWrapReverse = 2,
} SLRustFlexWrap;

typedef enum SLRustJustifyContent {
  SLRustJustifyContentStretch = 0,
  SLRustJustifyContentFlexStart = 1,
  SLRustJustifyContentStart = 2,
  SLRustJustifyContentCenter = 3,
  SLRustJustifyContentFlexEnd = 4,
  SLRustJustifyContentEnd = 5,
  SLRustJustifyContentSpaceBetween = 6,
  SLRustJustifyContentSpaceAround = 7,
  SLRustJustifyContentSpaceEvenly = 8,
} SLRustJustifyContent;

typedef enum SLRustAlignItems {
  SLRustAlignItemsStretch = 0,
  SLRustAlignItemsFlexStart = 1,
  SLRustAlignItemsStart = 2,
  SLRustAlignItemsCenter = 3,
  SLRustAlignItemsFlexEnd = 4,
  SLRustAlignItemsEnd = 5,
  SLRustAlignItemsBaseline = 6,
} SLRustAlignItems;

typedef enum SLRustAlignContent {
  SLRustAlignContentFlexStart = 0,
  SLRustAlignContentCenter = 1,
  SLRustAlignContentFlexEnd = 2,
  SLRustAlignContentSpaceBetween = 3,
  SLRustAlignContentSpaceAround = 4,
  SLRustAlignContentSpaceEvenly = 5,
  SLRustAlignContentStretch = 6,
  SLRustAlignContentStart = 7,
  SLRustAlignContentEnd = 8,
} SLRustAlignContent;

typedef enum SLRustJustifyItems {
  SLRustJustifyItemsAuto = 0,
  SLRustJustifyItemsStretch = 1,
  SLRustJustifyItemsStart = 2,
  SLRustJustifyItemsCenter = 3,
  SLRustJustifyItemsEnd = 4,
} SLRustJustifyItems;

typedef enum SLRustGridAutoFlow {
  SLRustGridAutoFlowRow = 0,
  SLRustGridAutoFlowColumn = 1,
  SLRustGridAutoFlowDense = 2,
  SLRustGridAutoFlowRowDense = 3,
  SLRustGridAutoFlowColumnDense = 4,
} SLRustGridAutoFlow;

typedef enum SLRustRelativeCenter {
  SLRustRelativeCenterNone = 0,
  SLRustRelativeCenterHorizontal = 1,
  SLRustRelativeCenterVertical = 2,
  SLRustRelativeCenterBoth = 3,
} SLRustRelativeCenter;

typedef enum SLRustLinearGravity {
  SLRustLinearGravityNone = 0,
  SLRustLinearGravityTop = 1,
  SLRustLinearGravityBottom = 2,
  SLRustLinearGravityLeft = 3,
  SLRustLinearGravityRight = 4,
  SLRustLinearGravityCenterVertical = 5,
  SLRustLinearGravityCenterHorizontal = 6,
  SLRustLinearGravitySpaceBetween = 7,
  SLRustLinearGravityStart = 8,
  SLRustLinearGravityEnd = 9,
  SLRustLinearGravityCenter = 10,
} SLRustLinearGravity;

typedef enum SLRustLinearLayoutGravity {
  SLRustLinearLayoutGravityNone = 0,
  SLRustLinearLayoutGravityTop = 1,
  SLRustLinearLayoutGravityBottom = 2,
  SLRustLinearLayoutGravityLeft = 3,
  SLRustLinearLayoutGravityRight = 4,
  SLRustLinearLayoutGravityCenterVertical = 5,
  SLRustLinearLayoutGravityCenterHorizontal = 6,
  SLRustLinearLayoutGravityFillVertical = 7,
  SLRustLinearLayoutGravityFillHorizontal = 8,
  SLRustLinearLayoutGravityCenter = 9,
  SLRustLinearLayoutGravityStretch = 10,
  SLRustLinearLayoutGravityStart = 11,
  SLRustLinearLayoutGravityEnd = 12,
} SLRustLinearLayoutGravity;

typedef enum SLRustLinearCrossGravity {
  SLRustLinearCrossGravityNone = 0,
  SLRustLinearCrossGravityStart = 1,
  SLRustLinearCrossGravityEnd = 2,
  SLRustLinearCrossGravityCenter = 3,
  SLRustLinearCrossGravityStretch = 4,
} SLRustLinearCrossGravity;

typedef enum SLRustListComponentType {
  SLRustListComponentTypeNone = -1,
  SLRustListComponentTypeHeader = 0,
  SLRustListComponentTypeFooter = 1,
  SLRustListComponentTypeListRow = 2,
  SLRustListComponentTypeDefault = 3,
} SLRustListComponentType;

typedef struct SLRustLength {
  int32_t kind;
  float value;
  float percent;
  bool has_base;
  bool has_percentage;
} SLRustLength;

typedef struct SLRustRectLength {
  SLRustLength left;
  SLRustLength right;
  SLRustLength top;
  SLRustLength bottom;
} SLRustRectLength;

typedef struct SLRustRectF32 {
  float left;
  float right;
  float top;
  float bottom;
} SLRustRectF32;

typedef struct SLRustSize {
  float width;
  float height;
} SLRustSize;

typedef struct SLRustPoint {
  float x;
  float y;
} SLRustPoint;

typedef struct SLRustSideConstraint {
  float size;
  int32_t mode;
} SLRustSideConstraint;

typedef struct SLRustConstraints {
  SLRustSideConstraint width;
  SLRustSideConstraint height;
} SLRustConstraints;

typedef struct SLRustLayoutResult {
  SLRustPoint offset;
  SLRustSize size;
  float baseline;
  bool has_baseline;
  SLRustRectF32 margin;
  SLRustRectF32 padding;
  SLRustRectF32 border;
  SLRustRectF32 sticky_pos;
} SLRustLayoutResult;

typedef struct SLRustStyle {
  int32_t display;
  int32_t position;
  int32_t box_sizing;
  int32_t direction;
  int32_t visibility;
  bool has_explicit_direction;
  SLRustLength width;
  SLRustLength height;
  SLRustLength min_width;
  SLRustLength min_height;
  SLRustLength max_width;
  SLRustLength max_height;
  float aspect_ratio;
  bool has_aspect_ratio;
  SLRustLength left;
  SLRustLength right;
  SLRustLength top;
  SLRustLength bottom;
  SLRustRectLength margin;
  SLRustRectLength padding;
  SLRustRectF32 border;
  int32_t flex_direction;
  int32_t flex_wrap;
  int32_t justify_content;
  int32_t align_items;
  int32_t align_self;
  bool has_align_self;
  int32_t align_content;
  int32_t justify_items;
  int32_t justify_self;
  float flex_grow;
  float flex_shrink;
  SLRustLength flex_basis;
  int32_t order;
  SLRustLength row_gap;
  SLRustLength column_gap;
  int32_t linear_orientation;
  int32_t linear_gravity;
  int32_t linear_layout_gravity;
  int32_t linear_cross_gravity;
  float linear_weight;
  float linear_weight_sum;
  int32_t linear_column_count;
  SLRustLength list_main_axis_gap;
  SLRustLength list_cross_axis_gap;
  int32_t list_component_type;
  const SLRustLength* grid_template_columns;
  size_t grid_template_columns_len;
  const SLRustLength* grid_template_rows;
  size_t grid_template_rows_len;
  const SLRustLength* grid_template_columns_max;
  size_t grid_template_columns_max_len;
  const SLRustLength* grid_template_rows_max;
  size_t grid_template_rows_max_len;
  const SLRustLength* grid_auto_columns;
  size_t grid_auto_columns_len;
  const SLRustLength* grid_auto_rows;
  size_t grid_auto_rows_len;
  const SLRustLength* grid_auto_columns_max;
  size_t grid_auto_columns_max_len;
  const SLRustLength* grid_auto_rows_max;
  size_t grid_auto_rows_max_len;
  int32_t grid_auto_flow;
  int32_t grid_column_start;
  int32_t grid_column_end;
  int32_t grid_row_start;
  int32_t grid_row_end;
  size_t grid_column_span;
  size_t grid_row_span;
  int32_t relative_id;
  int32_t relative_align_top;
  int32_t relative_align_right;
  int32_t relative_align_bottom;
  int32_t relative_align_left;
  int32_t relative_top_of;
  int32_t relative_right_of;
  int32_t relative_bottom_of;
  int32_t relative_left_of;
  bool relative_layout_once;
  int32_t relative_center;
} SLRustStyle;

typedef size_t (*SLRustChildCountFunc)(void* context, SLRustNodeId node);
typedef SLRustNodeId (*SLRustChildAtFunc)(void* context, SLRustNodeId node,
                                          size_t index);
typedef bool (*SLRustStyleFunc)(void* context, SLRustNodeId node,
                                SLRustStyle* out_style);
typedef void (*SLRustSetLayoutFunc)(void* context, SLRustNodeId node,
                                    SLRustLayoutResult layout);
typedef void (*SLRustSetLayoutWithConstraintsFunc)(
    void* context, SLRustNodeId node, SLRustConstraints constraints,
    SLRustLayoutResult layout);
typedef bool (*SLRustHasMeasureFunc)(void* context, SLRustNodeId node);
typedef bool (*SLRustMeasureFunc)(void* context, SLRustNodeId node,
                                  SLRustConstraints constraints,
                                  SLRustSize* out_size);
typedef bool (*SLRustBaselineFunc)(void* context, SLRustNodeId node,
                                   SLRustSize content_size,
                                   float* out_baseline);
typedef bool (*SLRustPhysicalPixelsPerLayoutUnitFunc)(void* context,
                                                      SLRustNodeId node,
                                                      float* out_value);

typedef struct SLRustTreeCallbacks {
  void* context;
  SLRustChildCountFunc child_count;
  SLRustChildAtFunc child_at;
  SLRustStyleFunc style;
  SLRustSetLayoutFunc set_layout;
  SLRustSetLayoutWithConstraintsFunc set_layout_with_constraints;
  SLRustHasMeasureFunc has_measure;
  SLRustMeasureFunc measure;
  SLRustBaselineFunc baseline;
  SLRustPhysicalPixelsPerLayoutUnitFunc physical_pixels_per_layout_unit;
} SLRustTreeCallbacks;

static inline SLRustLength SLRustMakeLength(int32_t kind, float value,
                                            float percent, bool has_base,
                                            bool has_percentage) {
  SLRustLength length;
  length.kind = kind;
  length.value = value;
  length.percent = percent;
  length.has_base = has_base;
  length.has_percentage = has_percentage;
  return length;
}

static inline SLRustLength SLRustMakeAutoLength(void) {
  return SLRustMakeLength(SLRustLengthAuto, 0.0f, 0.0f, false, false);
}

static inline SLRustLength SLRustMakePointsLength(float value) {
  return SLRustMakeLength(SLRustLengthPoints, value, 0.0f, false, false);
}

static inline SLRustLength SLRustMakePercentLength(float percent) {
  return SLRustMakeLength(SLRustLengthPercent, percent, 0.0f, false, false);
}

static inline SLRustLength SLRustMakeCalcLength(float value, float percent) {
  return SLRustMakeLength(SLRustLengthCalc, value, percent, true, true);
}

static inline SLRustLength SLRustMakeFrLength(float value) {
  return SLRustMakeLength(SLRustLengthFr, value, 0.0f, false, false);
}

static inline SLRustLength SLRustMakeMaxContentLength(void) {
  return SLRustMakeLength(SLRustLengthMaxContent, 0.0f, 0.0f, false, false);
}

static inline SLRustLength SLRustMakeMinContentLength(void) {
  return SLRustMakeLength(SLRustLengthMinContent, 0.0f, 0.0f, false, false);
}

static inline SLRustLength SLRustMakeFitContentLength(void) {
  return SLRustMakeLength(SLRustLengthFitContent, 0.0f, 0.0f, false, false);
}

static inline SLRustLength SLRustMakeFitContentLengthWithBase(
    float value, float percent, bool has_percentage) {
  return SLRustMakeLength(SLRustLengthFitContent, value, percent, true,
                          has_percentage);
}

static inline SLRustSize SLRustMakeSize(float width, float height) {
  SLRustSize size;
  size.width = width;
  size.height = height;
  return size;
}

static inline SLRustPoint SLRustMakePoint(float x, float y) {
  SLRustPoint point;
  point.x = x;
  point.y = y;
  return point;
}

static inline SLRustRectF32 SLRustMakeRectF32(float left, float right,
                                             float top, float bottom) {
  SLRustRectF32 rect;
  rect.left = left;
  rect.right = right;
  rect.top = top;
  rect.bottom = bottom;
  return rect;
}

static inline SLRustSideConstraint SLRustMakeSideConstraint(float size,
                                                           int32_t mode) {
  SLRustSideConstraint constraint;
  constraint.size = size;
  constraint.mode = mode;
  return constraint;
}

static inline SLRustSideConstraint SLRustMakeIndefiniteConstraint(void) {
  return SLRustMakeSideConstraint(0.0f, SLRustMeasureModeIndefinite);
}

static inline SLRustSideConstraint SLRustMakeDefiniteConstraint(float size) {
  return SLRustMakeSideConstraint(size, SLRustMeasureModeDefinite);
}

static inline SLRustSideConstraint SLRustMakeAtMostConstraint(float size) {
  return SLRustMakeSideConstraint(size, SLRustMeasureModeAtMost);
}

static inline SLRustConstraints SLRustMakeConstraints(
    SLRustSideConstraint width, SLRustSideConstraint height) {
  SLRustConstraints constraints;
  constraints.width = width;
  constraints.height = height;
  return constraints;
}

#ifdef __cplusplus
#define SLRUST_ALIGNOF(type) alignof(type)
#else
#define SLRUST_ALIGNOF(type) _Alignof(type)
#endif

static inline SLRustAbiInfo SLRustMakeCallerAbiInfo(void) {
  SLRustAbiInfo info;
  info.version_major = SLRustAbiVersionMajor;
  info.version_minor = SLRustAbiVersionMinor;
  info.version_patch = SLRustAbiVersionPatch;
  info.size_of_abi_info = sizeof(SLRustAbiInfo);
  info.align_of_abi_info = SLRUST_ALIGNOF(SLRustAbiInfo);
  info.size_of_length = sizeof(SLRustLength);
  info.align_of_length = SLRUST_ALIGNOF(SLRustLength);
  info.size_of_constraints = sizeof(SLRustConstraints);
  info.align_of_constraints = SLRUST_ALIGNOF(SLRustConstraints);
  info.size_of_layout_result = sizeof(SLRustLayoutResult);
  info.align_of_layout_result = SLRUST_ALIGNOF(SLRustLayoutResult);
  info.size_of_style = sizeof(SLRustStyle);
  info.align_of_style = SLRUST_ALIGNOF(SLRustStyle);
  info.size_of_tree_callbacks = sizeof(SLRustTreeCallbacks);
  info.align_of_tree_callbacks = SLRUST_ALIGNOF(SLRustTreeCallbacks);
  return info;
}

#undef SLRUST_ALIGNOF

SLRustStatus SLRustGetAbiInfo(SLRustAbiInfo* out_info);
const char* SLRustStatusName(SLRustStatus status);
void SLRustStyleDefault(SLRustStyle* out_style);
SLRustStatus SLRustLayoutExternal(const SLRustTreeCallbacks* callbacks,
                                  SLRustNodeId root,
                                  SLRustConstraints constraints,
                                  SLRustSize* out_size);
SLRustStatus SLRustLayoutExternalWithOwnerConstraints(
    const SLRustTreeCallbacks* callbacks,
    SLRustNodeId root,
    SLRustConstraints constraints,
    SLRustSize* out_size);
SLRustStatus SLRustLayoutExternalWithOwnerConstraintsAndDirection(
    const SLRustTreeCallbacks* callbacks,
    SLRustNodeId root,
    SLRustConstraints constraints,
    int32_t owner_direction,
    SLRustSize* out_size);
SLRustStatus SLRustLayoutExternalWithNodeConstraints(
    const SLRustTreeCallbacks* callbacks,
    SLRustNodeId root,
    SLRustConstraints constraints,
    SLRustSize* out_size);
SLRustStatus SLRustLayoutExternalChecked(const SLRustAbiInfo* caller_abi,
                                         const SLRustTreeCallbacks* callbacks,
                                         SLRustNodeId root,
                                         SLRustConstraints constraints,
                                         SLRustSize* out_size);
SLRustStatus SLRustLayoutExternalWithOwnerConstraintsChecked(
    const SLRustAbiInfo* caller_abi,
    const SLRustTreeCallbacks* callbacks,
    SLRustNodeId root,
    SLRustConstraints constraints,
    SLRustSize* out_size);
SLRustStatus SLRustLayoutExternalWithOwnerConstraintsAndDirectionChecked(
    const SLRustAbiInfo* caller_abi,
    const SLRustTreeCallbacks* callbacks,
    SLRustNodeId root,
    SLRustConstraints constraints,
    int32_t owner_direction,
    SLRustSize* out_size);
SLRustStatus SLRustLayoutExternalWithNodeConstraintsChecked(
    const SLRustAbiInfo* caller_abi,
    const SLRustTreeCallbacks* callbacks,
    SLRustNodeId root,
    SLRustConstraints constraints,
    SLRustSize* out_size);

#ifdef __cplusplus
}
#endif

#endif  // CORE_RENDERER_STARLIGHT_RUST_CRATES_STARLIGHT_FFI_INCLUDE_STARLIGHT_RUST_FFI_H_
