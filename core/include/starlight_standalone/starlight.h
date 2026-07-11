// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_H_
#define CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_H_

#include <stdbool.h>
#include <stdint.h>

#include "starlight_config.h"
#include "starlight_value.h"

#ifdef __cplusplus
extern "C" {
#endif

// a wrapper of LayoutObject.
struct StarlightNode;
typedef struct StarlightNode* SLNodeRef;
struct StarlightSize;
struct StarlightConfig;

/**
 *  1. When `widthMode` or `heightMode` is `SLNodeMeasureModeUndefined`:
 *  The corresponding `width` or `height` value is meaningless, as if setting
 * `width: max-content` or `height: max-content` in Web.
 *
 *  2. When `widthMode` or `heightMode` is `SLNodeMeasureModeExactly`:
 *  The `width` or `height` value specifies the exact size constraint (content
 * bounds) for the node.
 *
 *  3. When `widthMode` or `heightMode` is `SLNodeMeasureModeAtMost`:
 *  The `width` or `height` value specifies the maximum allowable size (content
 * bounds) for the node.
 *
 *  The `width`, `width_mode`, `height`, and `height_mode` parameters
 *  respectively define the width and height constraints (content bounds) of the
 *  node. This function is only required to return the content size of
 *  the node, excluding margins, borders, and padding.
 */
typedef StarlightSize (*StarlightMeasureFunc)(void* manager_node, float width,
                                              SLNodeMeasureMode width_mode,
                                              float height,
                                              SLNodeMeasureMode height_mode);

typedef float (*StarlightBaselineFunc)(void* manager_node, float width,
                                       float height);

typedef struct StarlightMeasureDelegate {
  // measure_func_ is used to measure the node.
  StarlightMeasureFunc measure_func_;
  // When baseline_func_ is set, it will be called to get the distance from the
  // top edge of the content box to the baseline of the node.
  StarlightBaselineFunc baseline_func_;
  // When the measure_func_ or baseline_func_ need to access the manager_node_,
  // set the manager_node_.
  void* manager_node_;
} StarlightMeasureDelegate;

SLNodeRef SLNodeNew();
// User need to manage the memory of config through calling `SLConfigFree` when
// not using.
SLNodeRef SLNodeNewWithConfig(StarlightConfig* config);

void SLNodeInsertChild(const SLNodeRef parent, const SLNodeRef child,
                       int32_t index);
// Recommended to use this function rather than SLNodeInsertChild.
void SLNodeInsertChildBefore(const SLNodeRef parent, const SLNodeRef child,
                             const SLNodeRef reference);
void SLNodeRemoveChild(const SLNodeRef parent, const SLNodeRef child);
void SLNodeRemoveAllChildren(const SLNodeRef parent);
// Nearly resets all of the node, including removing children, reseting layout
// results, reseting CSS styles, and so on.
void SLNodeReset(const SLNodeRef node);
SLNodeRef SLNodeGetChild(const SLNodeRef node, int32_t index);
int32_t SLNodeGetChildCount(const SLNodeRef node);
SLNodeRef SLNodeGetParent(const SLNodeRef node);

void SLNodeFree(const SLNodeRef node);
void SLNodeFreeRecursive(const SLNodeRef node);

bool SLNodeIsDirty(const SLNodeRef node);
void SLNodeMarkDirty(const SLNodeRef node);

// TODO(yuanzhiwen): currently unavailable
// bool SLNodeGetHasNewLayout(const SLNodeRef node);
// void SLNodeSetHasNewLayout(const SLNodeRef node, bool has_new_layout);

bool SLNodeIsRTL(const SLNodeRef node);

// owner_width, owner_height define the containing block for the root node, and
// the percentage for width, height, max-width, max-height, padding, margin will
// refer to this containing block. If width, height are not set, the root node's
// outer box (margin box) will be limited by the owner_width and owner_height.
void SLNodeCalculateLayout(const SLNodeRef node, float owner_width,
                           float owner_height, SLDirection owner_direction);
// Same as SLNodeCalculateLayout, but preserves the measure modes for root owner
// constraints instead of treating every finite owner size as exact.
void SLNodeCalculateLayoutWithMode(const SLNodeRef node, float owner_width,
                                   SLNodeMeasureMode owner_width_mode,
                                   float owner_height,
                                   SLNodeMeasureMode owner_height_mode,
                                   SLDirection owner_direction);

/**
 * @brief Set the measurement delegate of a node.
 *
 * @param delegate The measurement delegate object.
 *
 * This function sets the measurement delegate of a node. The measurement
 * delegate object should inject functions to measure the node:
 *   - StarlightMeasureFunc measure_func_ (optional): for measuring the node.
 *   - StarlightBaselineFunc baseline_func_ (optional): for calculating the
baseline of the node.
 *   - void* manager_node_ (optional): the manager node of the StarlightNode, it
 * usually holds the StarlightNode.
 *
 * Users need to manage the lifecycle of the measurement delegate object
 * themselves to ensure that it is released before the node is destroyed.
 */
void SLNodeSetMeasureDelegate(const SLNodeRef node,
                              StarlightMeasureDelegate* const delegate);
StarlightMeasureDelegate* SLNodeGetMeasureDelegate(const SLNodeRef node);
bool SLNodeHasMeasureFunc(const SLNodeRef node);

// Styles
void SLNodeStyleSetDirection(const SLNodeRef node, SLDirection type);
void SLNodeStyleSetFlexDirection(const SLNodeRef node, SLFlexDirection value);

// alignment
void SLNodeStyleSetJustifyContent(const SLNodeRef node, SLJustifyContent value);
void SLNodeStyleSetAlignContent(const SLNodeRef node, SLAlignContent value);
void SLNodeStyleSetAlignItems(const SLNodeRef node, SLFlexAlign value);
void SLNodeStyleSetAlignSelf(const SLNodeRef node, SLFlexAlign value);

// Defaults to `relative`.
void SLNodeStyleSetPositionType(const SLNodeRef node, SLPositionType value);

// Defaults to `no-wrap`.
void SLNodeStyleSetFlexWrap(const SLNodeRef node, SLFlexWrap value);

void SLNodeStyleSetLinearOrientation(const SLNodeRef node,
                                     SLLinearOrientation value);
void SLNodeStyleSetLinearGravity(const SLNodeRef node, SLLinearGravity value);
void SLNodeStyleSetLinearLayoutGravity(const SLNodeRef node,
                                       SLLinearLayoutGravity value);
void SLNodeStyleSetLinearCrossGravity(const SLNodeRef node,
                                      SLLinearCrossGravity value);
void SLNodeStyleSetLinearColumnCount(const SLNodeRef node, int32_t value);
void SLNodeStyleSetListComponentType(const SLNodeRef node,
                                     SLListComponentType value);
void SLNodeStyleSetListMainAxisGap(const SLNodeRef node, float value);
void SLNodeStyleSetListMainAxisGapPercent(const SLNodeRef node, float value);
void SLNodeStyleSetListMainAxisGapCalc(const SLNodeRef node,
                                       StarlightValue value);
void SLNodeStyleSetListMainAxisGapValue(const SLNodeRef node,
                                        StarlightValue value);
void SLNodeStyleSetListCrossAxisGap(const SLNodeRef node, float value);
void SLNodeStyleSetListCrossAxisGapPercent(const SLNodeRef node, float value);
void SLNodeStyleSetListCrossAxisGapCalc(const SLNodeRef node,
                                        StarlightValue value);
void SLNodeStyleSetListCrossAxisGapValue(const SLNodeRef node,
                                         StarlightValue value);
void SLNodeStyleSetLinearWeight(const SLNodeRef node, float value);
void SLNodeStyleSetLinearWeightSum(const SLNodeRef node, float value);

void SLNodeStyleSetRelativeId(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeAlignTop(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeAlignRight(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeAlignBottom(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeAlignLeft(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeTopOf(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeRightOf(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeBottomOf(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeLeftOf(const SLNodeRef node, int32_t value);
void SLNodeStyleSetRelativeLayoutOnce(const SLNodeRef node, bool value);
void SLNodeStyleSetRelativeCenter(const SLNodeRef node, SLRelativeCenter value);

void SLNodeStyleSetGridTemplateColumns(const SLNodeRef node,
                                       const StarlightValue* values,
                                       int32_t count);
void SLNodeStyleSetGridTemplateColumnsMax(const SLNodeRef node,
                                          const StarlightValue* values,
                                          int32_t count);
void SLNodeStyleSetGridTemplateRows(const SLNodeRef node,
                                    const StarlightValue* values,
                                    int32_t count);
void SLNodeStyleSetGridTemplateRowsMax(const SLNodeRef node,
                                       const StarlightValue* values,
                                       int32_t count);
void SLNodeStyleSetGridAutoColumns(const SLNodeRef node,
                                   const StarlightValue* values,
                                   int32_t count);
void SLNodeStyleSetGridAutoColumnsMax(const SLNodeRef node,
                                      const StarlightValue* values,
                                      int32_t count);
void SLNodeStyleSetGridAutoRows(const SLNodeRef node,
                                const StarlightValue* values, int32_t count);
void SLNodeStyleSetGridAutoRowsMax(const SLNodeRef node,
                                   const StarlightValue* values,
                                   int32_t count);
void SLNodeStyleSetGridAutoFlow(const SLNodeRef node, SLGridAutoFlow value);
void SLNodeStyleSetJustifyItems(const SLNodeRef node, SLJustifyItem value);
void SLNodeStyleSetJustifySelf(const SLNodeRef node, SLJustifyItem value);
void SLNodeStyleSetGridColumnStart(const SLNodeRef node, int32_t value);
void SLNodeStyleSetGridColumnEnd(const SLNodeRef node, int32_t value);
void SLNodeStyleSetGridRowStart(const SLNodeRef node, int32_t value);
void SLNodeStyleSetGridRowEnd(const SLNodeRef node, int32_t value);
void SLNodeStyleSetGridColumnSpan(const SLNodeRef node, int32_t value);
void SLNodeStyleSetGridRowSpan(const SLNodeRef node, int32_t value);

// Defaults to `flex`.
void SLNodeStyleSetDisplay(const SLNodeRef node, SLDisplay value);

// Defaults to `border-box`. Different from Web (`content-box`).
void SLNodeStyleSetBoxSizing(const SLNodeRef node, SLBoxSizing value);

// Defaults to undefined.
void SLNodeStyleSetAspectRatio(const SLNodeRef node, float value);

// Defaults to 0.
void SLNodeStyleSetOrder(const SLNodeRef node, int32_t value);

// flex: 1 is equivalent to flex-grow: 1, flex-shrink: 1, flex-basis: 0 (differs
// from 0% in Web).
void SLNodeStyleSetFlex(const SLNodeRef node, float value);
// Defaults to 0.
void SLNodeStyleSetFlexGrow(const SLNodeRef node, float value);
// Defaults to 1.
void SLNodeStyleSetFlexShrink(const SLNodeRef node, float value);
// flex-basis: defaults to `auto`.
void SLNodeStyleSetFlexBasis(const SLNodeRef node, float value);
void SLNodeStyleSetFlexBasisPercent(const SLNodeRef node, float value);
void SLNodeStyleSetFlexBasisCalc(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetFlexBasisValue(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetFlexBasisAuto(const SLNodeRef node);
void SLNodeStyleSetFlexBasisMaxContent(const SLNodeRef node);
void SLNodeStyleSetFlexBasisFitContent(const SLNodeRef node);
void SLNodeStyleSetFlexBasisFitContentValue(const SLNodeRef node,
                                            StarlightValue value);

// top, bottom, left, right: defaults to `auto`.
void SLNodeStyleSetPosition(const SLNodeRef node, SLEdge edge, float position);
void SLNodeStyleSetPositionPercent(const SLNodeRef node, SLEdge edge,
                                   float position);
void SLNodeStyleSetPositionCalc(const SLNodeRef node, SLEdge edge,
                                StarlightValue position);
void SLNodeStyleSetPositionValue(const SLNodeRef node, SLEdge edge,
                                 StarlightValue position);
void SLNodeStyleSetPositionAuto(const SLNodeRef node, SLEdge edge);

// margin
// Defaults to 0.
void SLNodeStyleSetMargin(const SLNodeRef node, SLEdge edge, float value);
void SLNodeStyleSetMarginPercent(const SLNodeRef node, SLEdge edge,
                                 float value);
void SLNodeStyleSetMarginCalc(const SLNodeRef node, SLEdge edge,
                              StarlightValue value);
void SLNodeStyleSetMarginValue(const SLNodeRef node, SLEdge edge,
                               StarlightValue value);
void SLNodeStyleSetMarginAuto(const SLNodeRef node, SLEdge edge);

// padding
// Defaults to 0.
void SLNodeStyleSetPadding(const SLNodeRef node, SLEdge edge, float value);
void SLNodeStyleSetPaddingPercent(const SLNodeRef node, SLEdge edge,
                                  float value);
void SLNodeStyleSetPaddingCalc(const SLNodeRef node, SLEdge edge,
                               StarlightValue value);
void SLNodeStyleSetPaddingValue(const SLNodeRef node, SLEdge edge,
                                StarlightValue value);

// border width
// Defaults to 0.
void SLNodeStyleSetBorder(const SLNodeRef node, SLEdge edge, float value);

// gap
// Defaults to 0.
void SLNodeStyleSetGap(const SLNodeRef node, SLGap gutter, float value);
void SLNodeStyleSetGapPercent(const SLNodeRef node, SLGap gutter, float value);
void SLNodeStyleSetGapCalc(const SLNodeRef node, SLGap gutter,
                           StarlightValue value);
void SLNodeStyleSetGapValue(const SLNodeRef node, SLGap gutter,
                            StarlightValue value);

// width properties
// width: defaults to `auto`.
void SLNodeStyleSetWidth(const SLNodeRef node, float value);
void SLNodeStyleSetWidthPercent(const SLNodeRef node, float value);
void SLNodeStyleSetWidthCalc(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetWidthValue(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetWidthAuto(const SLNodeRef node);
void SLNodeStyleSetWidthMaxContent(const SLNodeRef node);
void SLNodeStyleSetWidthFitContent(const SLNodeRef node);
void SLNodeStyleSetWidthFitContentValue(const SLNodeRef node,
                                        StarlightValue value);
// min-width: defaults to `0`.
void SLNodeStyleSetMinWidth(const SLNodeRef node, float value);
void SLNodeStyleSetMinWidthPercent(const SLNodeRef node, float value);
void SLNodeStyleSetMinWidthCalc(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetMinWidthValue(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetMinWidthMaxContent(const SLNodeRef node);
void SLNodeStyleSetMinWidthFitContent(const SLNodeRef node);
void SLNodeStyleSetMinWidthFitContentValue(const SLNodeRef node,
                                           StarlightValue value);
// max-width: defaults to no limit.
void SLNodeStyleSetMaxWidth(const SLNodeRef node, float value);
void SLNodeStyleSetMaxWidthPercent(const SLNodeRef node, float value);
void SLNodeStyleSetMaxWidthCalc(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetMaxWidthValue(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetMaxWidthMaxContent(const SLNodeRef node);
void SLNodeStyleSetMaxWidthFitContent(const SLNodeRef node);
void SLNodeStyleSetMaxWidthFitContentValue(const SLNodeRef node,
                                           StarlightValue value);

// height properties
// height: defaults to `auto`.
void SLNodeStyleSetHeight(const SLNodeRef node, float value);
void SLNodeStyleSetHeightPercent(const SLNodeRef node, float value);
void SLNodeStyleSetHeightCalc(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetHeightValue(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetHeightAuto(const SLNodeRef node);
void SLNodeStyleSetHeightMaxContent(const SLNodeRef node);
void SLNodeStyleSetHeightFitContent(const SLNodeRef node);
void SLNodeStyleSetHeightFitContentValue(const SLNodeRef node,
                                         StarlightValue value);
// min-height: defaults to `0`.
void SLNodeStyleSetMinHeight(const SLNodeRef node, float value);
void SLNodeStyleSetMinHeightPercent(const SLNodeRef node, float value);
void SLNodeStyleSetMinHeightCalc(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetMinHeightValue(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetMinHeightMaxContent(const SLNodeRef node);
void SLNodeStyleSetMinHeightFitContent(const SLNodeRef node);
void SLNodeStyleSetMinHeightFitContentValue(const SLNodeRef node,
                                            StarlightValue value);
// max-height: defaults to no limit.
void SLNodeStyleSetMaxHeight(const SLNodeRef node, float value);
void SLNodeStyleSetMaxHeightPercent(const SLNodeRef node, float value);
void SLNodeStyleSetMaxHeightCalc(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetMaxHeightValue(const SLNodeRef node, StarlightValue value);
void SLNodeStyleSetMaxHeightMaxContent(const SLNodeRef node);
void SLNodeStyleSetMaxHeightFitContent(const SLNodeRef node);
void SLNodeStyleSetMaxHeightFitContentValue(const SLNodeRef node,
                                            StarlightValue value);

// get style
SLFlexDirection SLNodeStyleGetFlexDirection(const SLNodeRef node);
// When setting to SLJustifyContentStart/SLJustifyContentEnd, we will get
// SLJustifyContentFlexStart/SLJustifyContentFlexEnd.
SLJustifyContent SLNodeStyleGetJustifyContent(const SLNodeRef node);
SLAlignContent SLNodeStyleGetAlignContent(const SLNodeRef node);
SLFlexAlign SLNodeStyleGetAlignItems(const SLNodeRef node);
SLFlexAlign SLNodeStyleGetAlignSelf(const SLNodeRef node);
SLPositionType SLNodeStyleGetPositionType(const SLNodeRef node);
SLFlexWrap SLNodeStyleGetFlexWrap(const SLNodeRef node);
SLLinearOrientation SLNodeStyleGetLinearOrientation(const SLNodeRef node);
SLLinearGravity SLNodeStyleGetLinearGravity(const SLNodeRef node);
SLLinearLayoutGravity SLNodeStyleGetLinearLayoutGravity(const SLNodeRef node);
SLLinearCrossGravity SLNodeStyleGetLinearCrossGravity(const SLNodeRef node);
SLRelativeCenter SLNodeStyleGetRelativeCenter(const SLNodeRef node);
SLGridAutoFlow SLNodeStyleGetGridAutoFlow(const SLNodeRef node);
SLJustifyItem SLNodeStyleGetJustifyItems(const SLNodeRef node);
SLJustifyItem SLNodeStyleGetJustifySelf(const SLNodeRef node);
SLDisplay SLNodeStyleGetDisplay(const SLNodeRef node);
SLBoxSizing SLNodeStyleGetBoxSizing(const SLNodeRef node);
float SLNodeStyleGetAspectRatio(const SLNodeRef node);
int32_t SLNodeStyleGetOrder(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeId(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeAlignTop(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeAlignRight(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeAlignBottom(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeAlignLeft(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeTopOf(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeRightOf(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeBottomOf(const SLNodeRef node);
int32_t SLNodeStyleGetRelativeLeftOf(const SLNodeRef node);
bool SLNodeStyleGetRelativeLayoutOnce(const SLNodeRef node);
int32_t SLNodeStyleGetGridColumnStart(const SLNodeRef node);
int32_t SLNodeStyleGetGridColumnEnd(const SLNodeRef node);
int32_t SLNodeStyleGetGridRowStart(const SLNodeRef node);
int32_t SLNodeStyleGetGridRowEnd(const SLNodeRef node);
int32_t SLNodeStyleGetGridColumnSpan(const SLNodeRef node);
int32_t SLNodeStyleGetGridRowSpan(const SLNodeRef node);
float SLNodeStyleGetFlexGrow(const SLNodeRef node);
float SLNodeStyleGetFlexShrink(const SLNodeRef node);
float SLNodeStyleGetLinearWeight(const SLNodeRef node);
float SLNodeStyleGetLinearWeightSum(const SLNodeRef node);
float SLNodeStyleGetBorder(const SLNodeRef node, SLEdge edge);
StarlightValue SLNodeStyleGetFlexBasis(const SLNodeRef node);
StarlightValue SLNodeStyleGetPosition(const SLNodeRef node, SLEdge edge);
StarlightValue SLNodeStyleGetMargin(const SLNodeRef node, SLEdge edge);
StarlightValue SLNodeStyleGetPadding(const SLNodeRef node, SLEdge edge);
StarlightValue SLNodeStyleGetGap(const SLNodeRef node, SLGap gutter);
StarlightValue SLNodeStyleGetWidth(const SLNodeRef node);
StarlightValue SLNodeStyleGetHeight(const SLNodeRef node);
StarlightValue SLNodeStyleGetMinWidth(const SLNodeRef node);
StarlightValue SLNodeStyleGetMaxWidth(const SLNodeRef node);
StarlightValue SLNodeStyleGetMinHeight(const SLNodeRef node);
StarlightValue SLNodeStyleGetMaxHeight(const SLNodeRef node);

// layout result
// the distance from the upper left corner of the current node's border box to
// the left edge of the parent node's border box.
float SLNodeLayoutGetLeft(const SLNodeRef node);
// the distance from the upper left corner of the current node's border box to
// the top edge of the parent node's border box.
float SLNodeLayoutGetTop(const SLNodeRef node);

// TODO(yuanzhiwen): offset_right, and offset_bottom
// float SLNodeLayoutGetRight(const SLNodeRef node);
// float SLNodeLayoutGetBottom(const SLNodeRef node);

// return the width and height of border box.
float SLNodeLayoutGetWidth(const SLNodeRef node);
float SLNodeLayoutGetHeight(const SLNodeRef node);
// return the node baseline offset from the top edge of the border box. When no
// baseline was calculated, this returns the border-box height fallback.
float SLNodeLayoutGetBaseline(const SLNodeRef node);
float SLNodeLayoutGetMargin(const SLNodeRef node, SLEdge edge);
float SLNodeLayoutGetPadding(const SLNodeRef node, SLEdge edge);
float SLNodeLayoutGetBorder(const SLNodeRef node, SLEdge edge);
float SLNodeLayoutGetStickyPosition(const SLNodeRef node, SLEdge edge);

#ifdef __cplusplus
}
#endif

#endif  // CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_H_
