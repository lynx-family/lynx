// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_ENUMS_H_
#define CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_ENUMS_H_

namespace starlight {
typedef enum SLDisplayType {
  kSLDisplayTypeNone = 0,
  kSLDisplayTypeFlex,
  kSLDisplayTypeGrid,
  kSLDisplayTypeLinear,
  kSLDisplayTypeRelative,
  kSLDisplayTypeBlock,
  kSLDisplayTypeAuto,
} SLDisplayType;

typedef enum SLFlexAlignType {
  kSLFlexAlignTypeAuto = 0,
  kSLFlexAlignTypeStretch,
  kSLFlexAlignTypeFlexStart,
  kSLFlexAlignTypeFlexEnd,
  kSLFlexAlignTypeCenter,
  kSLFlexAlignTypeBaseline,
  kSLFlexAlignTypeStart,
  kSLFlexAlignTypeEnd,
} SLFlexAlignType;

typedef enum SLAlignContentType {
  kSLAlignContentTypeFlexStart = 0,
  kSLAlignContentTypeFlexEnd,
  kSLAlignContentTypeCenter,
  kSLAlignContentTypeStretch,
  kSLAlignContentTypeSpaceBetween,
  kSLAlignContentTypeSpaceAround,
} SLAlignContentType;

typedef enum SLFlexJustifyType {
  kSLFlexJustifyTypeAuto = 0,
  kSLFlexJustifyTypeStretch,
  kSLFlexJustifyTypeStart,
  kSLFlexJustifyTypeEnd,
  kSLFlexJustifyTypeCenter
} SLFlexJustifyType;

typedef enum SLJustifyContentType {
  kSLJustifyContentTypeFlexStart = 0,
  kSLJustifyContentTypeCenter,
  kSLJustifyContentTypeFlexEnd,
  kSLJustifyContentTypeSpaceBetween,
  kSLJustifyContentTypeSpaceAround,
  kSLJustifyContentTypeSpaceEvenly,
} SLJustifyContentType;

typedef enum SLFlexDirection {
  kSLFlexDirectionColumn = 0,
  kSLFlexDirectionRow,
  kSLFlexDirectionRowReverse,
  kSLFlexDirectionColumnReverse,
} SLFlexDirection;

typedef enum SLFlexWrapType {
  kSLFlexWrapTypeWrap = 0,
  kSLFlexWrapTypeNowrap,
  kSLFlexWrapTypeWrapReverse,
} SLFlexWrapType;

typedef enum SLDirectionType {
  kSLDirectionTypeNormal = 0,
  kSLDirectionTypeRtl = 2,
  kSLDirectionTypeLtr = 3,
} SLDirectionType;

typedef enum SLPositionType {
  kSLPositionTypeAbsolute = 0,
  kSLPositionTypeRelative,
  kSLPositionTypeFixed,
  kSLPositionTypeSticky,
} SLPositionType;

typedef enum SLEdge {
  kSLLeft = 0,
  kSLRight,
  kSLTop,
  kSLBottom,
  kSLInlineStart,
  kSLInlineEnd,
} SLEdge;

typedef enum SLLengthType {
  kSLLengthPPX = 0,
  kSLLengthPX,
  kSLLengthRPX,
  kSLLengthVW,
  kSLLengthVH,
  kSLLengthPercentage,
  kSLLengthAuto,
} SLLengthType;

typedef enum SLDimension {
  kSLHorizontal = 0,
  kSLVertical,
  kSLDimensionCount
} SLDimension;

typedef enum StarlightMeasureMode {
  kStarlightMeasureModeIndefinite = 0,
  kStarlightMeasureModeDefinite,
  kStarlightMeasureModeAtMost
} StarlightMeasureMode;

}  // namespace starlight

#endif  // CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_ENUMS_H_
