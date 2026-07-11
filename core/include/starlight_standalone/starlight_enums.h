// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_ENUMS_H_
#define CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_ENUMS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SLDisplay {
  SLDisplayNone = 0,
  // default value
  SLDisplayFlex = 1,
  SLDisplayGrid = 2,
  SLDisplayLinear = 3,
  SLDisplayRelative = 4,
  SLDisplayBlock = 5,
} SLDisplay;

// for align-self: auto is default value
// for align-items: stretch is default value
typedef enum SLFlexAlign {
  // auto is not supported in align-items
  SLFlexAlignAuto = 0,
  SLFlexAlignStretch = 1,
  SLFlexAlignFlexStart = 2,
  SLFlexAlignFlexEnd = 3,
  SLFlexAlignCenter = 4,
  SLFlexAlignBaseline = 5,
  SLFlexAlignStart = 6,
  SLFlexAlignEnd = 7,
} SLFlexAlign;

// stretch is default value.
typedef enum SLAlignContent {
  SLAlignContentFlexStart = 0,
  SLAlignContentFlexEnd = 1,
  SLAlignContentCenter = 2,
  SLAlignContentStretch = 3,
  SLAlignContentSpaceBetween = 4,
  SLAlignContentSpaceAround = 5,
  SLAlignContentSpaceEvenly = 6,
} SLAlignContent;

typedef enum SLJustifyContent {
  SLJustifyContentFlexStart = 0,
  SLJustifyContentCenter = 1,
  SLJustifyContentFlexEnd = 2,
  SLJustifyContentSpaceBetween = 3,
  SLJustifyContentSpaceAround = 4,
  SLJustifyContentSpaceEvenly = 5,
  SLJustifyContentStretch = 6,
  SLJustifyContentStart = 7,
  SLJustifyContentEnd = 8,
} SLJustifyContent;

typedef enum SLFlexDirection {
  SLFlexDirectionColumn = 0,
  SLFlexDirectionRow = 1,
  SLFlexDirectionRowReverse = 2,
  SLFlexDirectionColumnReverse = 3,
} SLFlexDirection;

typedef enum SLFlexWrap {
  SLFlexWrapWrap = 0,
  SLFlexWrapNowrap = 1,
  SLFlexWrapWrapReverse = 2,
} SLFlexWrap;

typedef enum SLLinearOrientation {
  SLLinearOrientationHorizontal = 0,
  SLLinearOrientationVertical = 1,
  SLLinearOrientationHorizontalReverse = 2,
  SLLinearOrientationVerticalReverse = 3,
  SLLinearOrientationRow = 4,
  SLLinearOrientationColumn = 5,
  SLLinearOrientationRowReverse = 6,
  SLLinearOrientationColumnReverse = 7,
} SLLinearOrientation;

typedef enum SLLinearGravity {
  SLLinearGravityNone = 0,
  SLLinearGravityTop = 1,
  SLLinearGravityBottom = 2,
  SLLinearGravityLeft = 3,
  SLLinearGravityRight = 4,
  SLLinearGravityCenterVertical = 5,
  SLLinearGravityCenterHorizontal = 6,
  SLLinearGravitySpaceBetween = 7,
  SLLinearGravityStart = 8,
  SLLinearGravityEnd = 9,
  SLLinearGravityCenter = 10,
} SLLinearGravity;

typedef enum SLLinearLayoutGravity {
  SLLinearLayoutGravityNone = 0,
  SLLinearLayoutGravityTop = 1,
  SLLinearLayoutGravityBottom = 2,
  SLLinearLayoutGravityLeft = 3,
  SLLinearLayoutGravityRight = 4,
  SLLinearLayoutGravityCenterVertical = 5,
  SLLinearLayoutGravityCenterHorizontal = 6,
  SLLinearLayoutGravityFillVertical = 7,
  SLLinearLayoutGravityFillHorizontal = 8,
  SLLinearLayoutGravityCenter = 9,
  SLLinearLayoutGravityStretch = 10,
  SLLinearLayoutGravityStart = 11,
  SLLinearLayoutGravityEnd = 12,
} SLLinearLayoutGravity;

typedef enum SLLinearCrossGravity {
  SLLinearCrossGravityNone = 0,
  SLLinearCrossGravityStart = 1,
  SLLinearCrossGravityEnd = 2,
  SLLinearCrossGravityCenter = 3,
  SLLinearCrossGravityStretch = 4,
} SLLinearCrossGravity;

typedef enum SLListComponentType {
  SLListComponentTypeDefault = 0,
  SLListComponentTypeHeader = 1,
  SLListComponentTypeFooter = 2,
  SLListComponentTypeListRow = 3,
} SLListComponentType;

typedef enum SLRelativeCenter {
  SLRelativeCenterNone = 0,
  SLRelativeCenterVertical = 1,
  SLRelativeCenterHorizontal = 2,
  SLRelativeCenterBoth = 3,
} SLRelativeCenter;

typedef enum SLGridAutoFlow {
  SLGridAutoFlowRow = 0,
  SLGridAutoFlowColumn = 1,
  SLGridAutoFlowDense = 2,
  SLGridAutoFlowRowDense = 3,
  SLGridAutoFlowColumnDense = 4,
} SLGridAutoFlow;

typedef enum SLJustifyItem {
  SLJustifyItemAuto = 0,
  SLJustifyItemStretch = 1,
  SLJustifyItemStart = 2,
  SLJustifyItemEnd = 3,
  SLJustifyItemCenter = 4,
} SLJustifyItem;

typedef enum SLDirection {
  SLDirectionNormal = 0,
  SLDirectionLynxRTL = 1,
  SLDirectionRTL = 2,
  SLDirectionLTR = 3,
} SLDirection;

typedef enum SLPositionType {
  SLPositionTypeAbsolute = 0,
  SLPositionTypeRelative = 1,
  SLPositionTypeFixed = 2,
  SLPositionTypeSticky = 3,
} SLPositionType;

typedef enum SLBoxSizing {
  SLBoxSizingBorderBox = 0,
  SLBoxSizingContentBox = 1,
} SLBoxSizing;

typedef enum SLEdge {
  SLEdgeLeft = 0,
  SLEdgeRight = 1,
  SLEdgeTop = 2,
  SLEdgeBottom = 3,
  SLEdgeStart = 4,
  SLEdgeEnd = 5,
  SLEdgeHorizontal = 6,
  SLEdgeVertical = 7,
  SLEdgeAll = 8,
} SLEdge;

typedef enum SLGap { SLGapColumn = 0, SLGapRow = 1, SLGapAll = 2 } SLGap;

typedef enum SLDimension {
  SLHorizontal = 0,
  SLVertical = 1,
  SLDimensionCount = 2
} SLDimension;

typedef enum SLNodeMeasureMode {
  SLNodeMeasureModeUndefined = 0,
  SLNodeMeasureModeExactly = 1,
  SLNodeMeasureModeAtMost = 2,
} SLNodeMeasureMode;

typedef enum SLUnit {
  SLUnitPoint = 0,
  SLUnitPercent = 1,
  SLUnitAuto = 2,
  SLUnitMaxContent = 3,
  SLUnitFitContent = 4,
  SLUnitFr = 5,
  SLUnitCalc = 6,
} SLUnit;

#ifdef __cplusplus
}
#endif

#endif  // CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_ENUMS_H_
