// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::cell::Cell;
use std::ffi::{c_int, c_void};
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::ptr::NonNull;

use starlight_layout::{
    AlignContent, AlignItems, BaseLength, BoxSizing, Constraints, Direction, Display, Edges,
    FlexDirection, FlexWrap, GridAutoFlow, JustifyContent, JustifyItems, LayoutResult, LayoutTree,
    Length, LinearCrossGravity, LinearGravity, LinearLayoutGravity, LinearOrientation,
    ListComponentType, MeasureMode, Point, PositionType, RelativeCenter, Size, Style, Visibility,
};

use crate::{
    CppBaselineError, StandalonePublicBoxAspectLayoutSnapshot, StandalonePublicConfigSnapshot,
    StandalonePublicDimensionLayoutSnapshot, StandalonePublicDimensionStyleSnapshot,
    StandalonePublicDirectionLayoutSnapshot, StandalonePublicDirectionSnapshot,
    StandalonePublicDirtySnapshot, StandalonePublicDisplayLayoutSnapshot,
    StandalonePublicEdgeLayoutSnapshot, StandalonePublicEdgeStyleSnapshot,
    StandalonePublicEdgeStyleVariantSnapshot, StandalonePublicFlexLayoutSnapshot,
    StandalonePublicGridAlignmentLayoutSnapshot, StandalonePublicGridTrackLayoutSnapshot,
    StandalonePublicLayoutEntrypointSnapshot, StandalonePublicLayoutEntrypointStageSnapshot,
    StandalonePublicLayoutGetterSnapshot, StandalonePublicLayoutNodeSnapshot,
    StandalonePublicLengthValue, StandalonePublicLinearLayoutSnapshot,
    StandalonePublicLinearListLayoutSnapshot, StandalonePublicMeasureDelegateSnapshot,
    StandalonePublicPositionLayoutSnapshot, StandalonePublicRelativeLayoutSnapshot,
    StandalonePublicScalarStyleSnapshot, StandalonePublicStyleStage, StandalonePublicTreeSnapshot,
    StandalonePublicTreeStage,
};

const SL_UNDEFINED: f32 = 10E20_f32;

type SLNodeMeasureMode = c_int;
const SL_NODE_MEASURE_MODE_UNDEFINED: SLNodeMeasureMode = 0;
const SL_NODE_MEASURE_MODE_EXACTLY: SLNodeMeasureMode = 1;
const SL_NODE_MEASURE_MODE_AT_MOST: SLNodeMeasureMode = 2;

type SLDisplay = c_int;
const SL_DISPLAY_NONE: SLDisplay = 0;
const SL_DISPLAY_FLEX: SLDisplay = 1;
const SL_DISPLAY_GRID: SLDisplay = 2;
const SL_DISPLAY_LINEAR: SLDisplay = 3;
const SL_DISPLAY_RELATIVE: SLDisplay = 4;
const SL_DISPLAY_BLOCK: SLDisplay = 5;

type SLFlexAlign = c_int;
const SL_FLEX_ALIGN_AUTO: SLFlexAlign = 0;
const SL_FLEX_ALIGN_STRETCH: SLFlexAlign = 1;
const SL_FLEX_ALIGN_FLEX_START: SLFlexAlign = 2;
const SL_FLEX_ALIGN_FLEX_END: SLFlexAlign = 3;
const SL_FLEX_ALIGN_CENTER: SLFlexAlign = 4;
const SL_FLEX_ALIGN_BASELINE: SLFlexAlign = 5;
const SL_FLEX_ALIGN_START: SLFlexAlign = 6;
const SL_FLEX_ALIGN_END: SLFlexAlign = 7;

type SLAlignContent = c_int;
const SL_ALIGN_CONTENT_FLEX_START: SLAlignContent = 0;
const SL_ALIGN_CONTENT_FLEX_END: SLAlignContent = 1;
const SL_ALIGN_CONTENT_CENTER: SLAlignContent = 2;
const SL_ALIGN_CONTENT_STRETCH: SLAlignContent = 3;
const SL_ALIGN_CONTENT_SPACE_BETWEEN: SLAlignContent = 4;
const SL_ALIGN_CONTENT_SPACE_AROUND: SLAlignContent = 5;
const SL_ALIGN_CONTENT_SPACE_EVENLY: SLAlignContent = 6;

type SLJustifyContent = c_int;
const SL_JUSTIFY_CONTENT_FLEX_START: SLJustifyContent = 0;
const SL_JUSTIFY_CONTENT_CENTER: SLJustifyContent = 1;
const SL_JUSTIFY_CONTENT_FLEX_END: SLJustifyContent = 2;
const SL_JUSTIFY_CONTENT_SPACE_BETWEEN: SLJustifyContent = 3;
const SL_JUSTIFY_CONTENT_SPACE_AROUND: SLJustifyContent = 4;
const SL_JUSTIFY_CONTENT_SPACE_EVENLY: SLJustifyContent = 5;
const SL_JUSTIFY_CONTENT_STRETCH: SLJustifyContent = 6;
const SL_JUSTIFY_CONTENT_START: SLJustifyContent = 7;
const SL_JUSTIFY_CONTENT_END: SLJustifyContent = 8;

type SLFlexDirection = c_int;
const SL_FLEX_DIRECTION_COLUMN: SLFlexDirection = 0;
const SL_FLEX_DIRECTION_ROW: SLFlexDirection = 1;
const SL_FLEX_DIRECTION_ROW_REVERSE: SLFlexDirection = 2;
const SL_FLEX_DIRECTION_COLUMN_REVERSE: SLFlexDirection = 3;

type SLFlexWrap = c_int;
const SL_FLEX_WRAP_WRAP: SLFlexWrap = 0;
const SL_FLEX_WRAP_NOWRAP: SLFlexWrap = 1;
const SL_FLEX_WRAP_WRAP_REVERSE: SLFlexWrap = 2;

type SLLinearOrientation = c_int;
const SL_LINEAR_ORIENTATION_HORIZONTAL: SLLinearOrientation = 0;
const SL_LINEAR_ORIENTATION_VERTICAL: SLLinearOrientation = 1;
const SL_LINEAR_ORIENTATION_HORIZONTAL_REVERSE: SLLinearOrientation = 2;
const SL_LINEAR_ORIENTATION_VERTICAL_REVERSE: SLLinearOrientation = 3;
const SL_LINEAR_ORIENTATION_ROW: SLLinearOrientation = 4;
const SL_LINEAR_ORIENTATION_COLUMN: SLLinearOrientation = 5;
const SL_LINEAR_ORIENTATION_ROW_REVERSE: SLLinearOrientation = 6;
const SL_LINEAR_ORIENTATION_COLUMN_REVERSE: SLLinearOrientation = 7;

type SLLinearGravity = c_int;
const SL_LINEAR_GRAVITY_NONE: SLLinearGravity = 0;
const SL_LINEAR_GRAVITY_TOP: SLLinearGravity = 1;
const SL_LINEAR_GRAVITY_BOTTOM: SLLinearGravity = 2;
const SL_LINEAR_GRAVITY_LEFT: SLLinearGravity = 3;
const SL_LINEAR_GRAVITY_RIGHT: SLLinearGravity = 4;
const SL_LINEAR_GRAVITY_CENTER_VERTICAL: SLLinearGravity = 5;
const SL_LINEAR_GRAVITY_CENTER_HORIZONTAL: SLLinearGravity = 6;
const SL_LINEAR_GRAVITY_SPACE_BETWEEN: SLLinearGravity = 7;
const SL_LINEAR_GRAVITY_START: SLLinearGravity = 8;
const SL_LINEAR_GRAVITY_END: SLLinearGravity = 9;
const SL_LINEAR_GRAVITY_CENTER: SLLinearGravity = 10;

type SLLinearLayoutGravity = c_int;
const SL_LINEAR_LAYOUT_GRAVITY_NONE: SLLinearLayoutGravity = 0;
const SL_LINEAR_LAYOUT_GRAVITY_TOP: SLLinearLayoutGravity = 1;
const SL_LINEAR_LAYOUT_GRAVITY_BOTTOM: SLLinearLayoutGravity = 2;
const SL_LINEAR_LAYOUT_GRAVITY_LEFT: SLLinearLayoutGravity = 3;
const SL_LINEAR_LAYOUT_GRAVITY_RIGHT: SLLinearLayoutGravity = 4;
const SL_LINEAR_LAYOUT_GRAVITY_CENTER_VERTICAL: SLLinearLayoutGravity = 5;
const SL_LINEAR_LAYOUT_GRAVITY_CENTER_HORIZONTAL: SLLinearLayoutGravity = 6;
const SL_LINEAR_LAYOUT_GRAVITY_FILL_VERTICAL: SLLinearLayoutGravity = 7;
const SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL: SLLinearLayoutGravity = 8;
const SL_LINEAR_LAYOUT_GRAVITY_CENTER: SLLinearLayoutGravity = 9;
const SL_LINEAR_LAYOUT_GRAVITY_STRETCH: SLLinearLayoutGravity = 10;
const SL_LINEAR_LAYOUT_GRAVITY_START: SLLinearLayoutGravity = 11;
const SL_LINEAR_LAYOUT_GRAVITY_END: SLLinearLayoutGravity = 12;

type SLLinearCrossGravity = c_int;
const SL_LINEAR_CROSS_GRAVITY_NONE: SLLinearCrossGravity = 0;
const SL_LINEAR_CROSS_GRAVITY_START: SLLinearCrossGravity = 1;
const SL_LINEAR_CROSS_GRAVITY_END: SLLinearCrossGravity = 2;
const SL_LINEAR_CROSS_GRAVITY_CENTER: SLLinearCrossGravity = 3;
const SL_LINEAR_CROSS_GRAVITY_STRETCH: SLLinearCrossGravity = 4;

type SLListComponentType = c_int;
const SL_LIST_COMPONENT_TYPE_DEFAULT: SLListComponentType = 0;
const SL_LIST_COMPONENT_TYPE_HEADER: SLListComponentType = 1;
const SL_LIST_COMPONENT_TYPE_FOOTER: SLListComponentType = 2;
const SL_LIST_COMPONENT_TYPE_LIST_ROW: SLListComponentType = 3;

type SLRelativeCenter = c_int;
const SL_RELATIVE_CENTER_NONE: SLRelativeCenter = 0;
const SL_RELATIVE_CENTER_VERTICAL: SLRelativeCenter = 1;
const SL_RELATIVE_CENTER_HORIZONTAL: SLRelativeCenter = 2;
const SL_RELATIVE_CENTER_BOTH: SLRelativeCenter = 3;

type SLGridAutoFlow = c_int;
const SL_GRID_AUTO_FLOW_ROW: SLGridAutoFlow = 0;
const SL_GRID_AUTO_FLOW_COLUMN: SLGridAutoFlow = 1;
const SL_GRID_AUTO_FLOW_DENSE: SLGridAutoFlow = 2;
const SL_GRID_AUTO_FLOW_ROW_DENSE: SLGridAutoFlow = 3;
const SL_GRID_AUTO_FLOW_COLUMN_DENSE: SLGridAutoFlow = 4;

type SLJustifyItem = c_int;
const SL_JUSTIFY_ITEM_AUTO: SLJustifyItem = 0;
const SL_JUSTIFY_ITEM_STRETCH: SLJustifyItem = 1;
const SL_JUSTIFY_ITEM_START: SLJustifyItem = 2;
const SL_JUSTIFY_ITEM_END: SLJustifyItem = 3;
const SL_JUSTIFY_ITEM_CENTER: SLJustifyItem = 4;

type SLDirection = c_int;
// Imported to keep the standalone header enum surface complete. Rust Style
// stores the resolved physical direction as LTR or RTL.
#[allow(dead_code)]
const SL_DIRECTION_NORMAL: SLDirection = 0;
// Imported to keep the standalone header enum surface complete. C++ currently
// treats LynxRTL as an RTL owner direction, while explicit node direction keeps
// the distinct kLynxRtl style value.
#[allow(dead_code)]
const SL_DIRECTION_LYNX_RTL: SLDirection = 1;
const SL_DIRECTION_RTL: SLDirection = 2;
const SL_DIRECTION_LTR: SLDirection = 3;

type SLPositionType = c_int;
const SL_POSITION_TYPE_ABSOLUTE: SLPositionType = 0;
const SL_POSITION_TYPE_RELATIVE: SLPositionType = 1;
const SL_POSITION_TYPE_FIXED: SLPositionType = 2;
const SL_POSITION_TYPE_STICKY: SLPositionType = 3;

type SLBoxSizing = c_int;
const SL_BOX_SIZING_BORDER_BOX: SLBoxSizing = 0;
const SL_BOX_SIZING_CONTENT_BOX: SLBoxSizing = 1;

type SLEdge = c_int;
const SL_EDGE_LEFT: SLEdge = 0;
const SL_EDGE_RIGHT: SLEdge = 1;
const SL_EDGE_TOP: SLEdge = 2;
const SL_EDGE_BOTTOM: SLEdge = 3;
const SL_EDGE_START: SLEdge = 4;
const SL_EDGE_END: SLEdge = 5;
const SL_EDGE_HORIZONTAL: SLEdge = 6;
const SL_EDGE_VERTICAL: SLEdge = 7;
const SL_EDGE_ALL: SLEdge = 8;

type SLGap = c_int;
const SL_GAP_COLUMN: SLGap = 0;
const SL_GAP_ROW: SLGap = 1;
const SL_GAP_ALL: SLGap = 2;

type SLUnit = c_int;
const SL_UNIT_POINT: SLUnit = 0;
const SL_UNIT_PERCENT: SLUnit = 1;
const SL_UNIT_AUTO: SLUnit = 2;
const SL_UNIT_MAX_CONTENT: SLUnit = 3;
const SL_UNIT_FIT_CONTENT: SLUnit = 4;
const SL_UNIT_FR: SLUnit = 5;
const SL_UNIT_CALC: SLUnit = 6;
const SL_VALUE_FLAG_HAS_VALUE: i32 = 1 << 0;
const SL_VALUE_FLAG_HAS_PERCENTAGE: i32 = 1 << 1;

#[repr(C)]
struct StarlightNode {
    _private: [u8; 0],
}

#[repr(C)]
// Imported for public standalone header coverage; the LayoutTree baseline path
// currently uses the default standalone config.
#[allow(dead_code)]
struct StarlightConfig {
    _private: [u8; 0],
}

type SLNodeRef = *mut StarlightNode;

#[repr(C)]
#[derive(Clone, Copy)]
struct StarlightSize {
    width: f32,
    height: f32,
}

#[repr(C)]
struct StarlightMeasureDelegate {
    measure_func: Option<
        extern "C" fn(*mut c_void, f32, SLNodeMeasureMode, f32, SLNodeMeasureMode) -> StarlightSize,
    >,
    baseline_func: Option<extern "C" fn(*mut c_void, f32, f32) -> f32>,
    manager_node: *mut c_void,
}

#[repr(C)]
#[derive(Clone, Copy)]
struct StarlightValue {
    value: f32,
    unit: SLUnit,
    percentage: f32,
    flags: i32,
}

// Keep the public standalone C API imported even when the current baseline
// adapter only calls the subset needed to mirror a `LayoutTree`.
#[allow(dead_code)]
unsafe extern "C" {
    fn SLConfigCreate() -> *mut StarlightConfig;
    fn SLConfigSetPhysicalPixelsPerLayoutUnit(
        config: *mut StarlightConfig,
        physical_pixels_per_layout_unit: f32,
    );
    fn SLConfigFree(config: *mut StarlightConfig);
    fn SLConfigGetPhysicalPixelsPerLayoutUnit(config: *mut StarlightConfig) -> f32;
    fn SLNodeNew() -> SLNodeRef;
    fn SLNodeNewWithConfig(config: *mut StarlightConfig) -> SLNodeRef;
    fn SLNodeInsertChild(parent: SLNodeRef, child: SLNodeRef, index: i32);
    fn SLNodeInsertChildBefore(parent: SLNodeRef, child: SLNodeRef, reference: SLNodeRef);
    fn SLNodeRemoveChild(parent: SLNodeRef, child: SLNodeRef);
    fn SLNodeRemoveAllChildren(parent: SLNodeRef);
    fn SLNodeReset(node: SLNodeRef);
    fn SLNodeGetChild(node: SLNodeRef, index: i32) -> SLNodeRef;
    fn SLNodeGetChildCount(node: SLNodeRef) -> i32;
    fn SLNodeGetParent(node: SLNodeRef) -> SLNodeRef;
    fn SLNodeFree(node: SLNodeRef);
    fn SLNodeFreeRecursive(node: SLNodeRef);
    fn SLNodeIsDirty(node: SLNodeRef) -> bool;
    fn SLNodeMarkDirty(node: SLNodeRef);
    fn SLNodeIsRTL(node: SLNodeRef) -> bool;
    fn SLNodeCalculateLayout(
        node: SLNodeRef,
        owner_width: f32,
        owner_height: f32,
        owner_direction: SLDirection,
    );
    fn SLNodeCalculateLayoutWithMode(
        node: SLNodeRef,
        owner_width: f32,
        owner_width_mode: SLNodeMeasureMode,
        owner_height: f32,
        owner_height_mode: SLNodeMeasureMode,
        owner_direction: SLDirection,
    );
    fn SLNodeSetMeasureDelegate(node: SLNodeRef, delegate: *mut StarlightMeasureDelegate);
    fn SLNodeGetMeasureDelegate(node: SLNodeRef) -> *mut StarlightMeasureDelegate;
    fn SLNodeHasMeasureFunc(node: SLNodeRef) -> bool;

    fn SLNodeStyleSetFlexDirection(node: SLNodeRef, value: SLFlexDirection);
    fn SLNodeStyleSetJustifyContent(node: SLNodeRef, value: SLJustifyContent);
    fn SLNodeStyleSetAlignContent(node: SLNodeRef, value: SLAlignContent);
    fn SLNodeStyleSetAlignItems(node: SLNodeRef, value: SLFlexAlign);
    fn SLNodeStyleSetAlignSelf(node: SLNodeRef, value: SLFlexAlign);
    fn SLNodeStyleSetPositionType(node: SLNodeRef, value: SLPositionType);
    fn SLNodeStyleSetFlexWrap(node: SLNodeRef, value: SLFlexWrap);
    fn SLNodeStyleSetLinearOrientation(node: SLNodeRef, value: SLLinearOrientation);
    fn SLNodeStyleSetLinearGravity(node: SLNodeRef, value: SLLinearGravity);
    fn SLNodeStyleSetLinearLayoutGravity(node: SLNodeRef, value: SLLinearLayoutGravity);
    fn SLNodeStyleSetLinearCrossGravity(node: SLNodeRef, value: SLLinearCrossGravity);
    fn SLNodeStyleSetLinearColumnCount(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetListComponentType(node: SLNodeRef, value: SLListComponentType);
    fn SLNodeStyleSetListMainAxisGap(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetListMainAxisGapPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetListMainAxisGapCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetListMainAxisGapValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetListCrossAxisGap(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetListCrossAxisGapPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetListCrossAxisGapCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetListCrossAxisGapValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetLinearWeight(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetLinearWeightSum(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetRelativeId(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeAlignTop(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeAlignRight(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeAlignBottom(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeAlignLeft(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeTopOf(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeRightOf(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeBottomOf(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeLeftOf(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetRelativeLayoutOnce(node: SLNodeRef, value: bool);
    fn SLNodeStyleSetRelativeCenter(node: SLNodeRef, value: SLRelativeCenter);
    fn SLNodeStyleSetGridTemplateColumns(
        node: SLNodeRef,
        values: *const StarlightValue,
        count: i32,
    );
    fn SLNodeStyleSetGridTemplateColumnsMax(
        node: SLNodeRef,
        values: *const StarlightValue,
        count: i32,
    );
    fn SLNodeStyleSetGridTemplateRows(node: SLNodeRef, values: *const StarlightValue, count: i32);
    fn SLNodeStyleSetGridTemplateRowsMax(
        node: SLNodeRef,
        values: *const StarlightValue,
        count: i32,
    );
    fn SLNodeStyleSetGridAutoColumns(node: SLNodeRef, values: *const StarlightValue, count: i32);
    fn SLNodeStyleSetGridAutoColumnsMax(node: SLNodeRef, values: *const StarlightValue, count: i32);
    fn SLNodeStyleSetGridAutoRows(node: SLNodeRef, values: *const StarlightValue, count: i32);
    fn SLNodeStyleSetGridAutoRowsMax(node: SLNodeRef, values: *const StarlightValue, count: i32);
    fn SLNodeStyleSetGridAutoFlow(node: SLNodeRef, value: SLGridAutoFlow);
    fn SLNodeStyleSetJustifyItems(node: SLNodeRef, value: SLJustifyItem);
    fn SLNodeStyleSetJustifySelf(node: SLNodeRef, value: SLJustifyItem);
    fn SLNodeStyleSetGridColumnStart(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetGridColumnEnd(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetGridRowStart(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetGridRowEnd(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetGridColumnSpan(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetGridRowSpan(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetDisplay(node: SLNodeRef, value: SLDisplay);
    fn SLNodeStyleSetDirection(node: SLNodeRef, value: SLDirection);
    fn SLNodeStyleSetBoxSizing(node: SLNodeRef, value: SLBoxSizing);
    fn SLNodeStyleSetAspectRatio(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetOrder(node: SLNodeRef, value: i32);
    fn SLNodeStyleSetFlex(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetFlexGrow(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetFlexShrink(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetFlexBasis(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetFlexBasisPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetFlexBasisCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetFlexBasisValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetFlexBasisAuto(node: SLNodeRef);
    fn SLNodeStyleSetFlexBasisMaxContent(node: SLNodeRef);
    fn SLNodeStyleSetFlexBasisFitContent(node: SLNodeRef);
    fn SLNodeStyleSetFlexBasisFitContentValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetPosition(node: SLNodeRef, edge: SLEdge, value: f32);
    fn SLNodeStyleSetPositionPercent(node: SLNodeRef, edge: SLEdge, value: f32);
    fn SLNodeStyleSetPositionCalc(node: SLNodeRef, edge: SLEdge, value: StarlightValue);
    fn SLNodeStyleSetPositionValue(node: SLNodeRef, edge: SLEdge, value: StarlightValue);
    fn SLNodeStyleSetPositionAuto(node: SLNodeRef, edge: SLEdge);
    fn SLNodeStyleSetMargin(node: SLNodeRef, edge: SLEdge, value: f32);
    fn SLNodeStyleSetMarginPercent(node: SLNodeRef, edge: SLEdge, value: f32);
    fn SLNodeStyleSetMarginCalc(node: SLNodeRef, edge: SLEdge, value: StarlightValue);
    fn SLNodeStyleSetMarginValue(node: SLNodeRef, edge: SLEdge, value: StarlightValue);
    fn SLNodeStyleSetMarginAuto(node: SLNodeRef, edge: SLEdge);
    fn SLNodeStyleSetPadding(node: SLNodeRef, edge: SLEdge, value: f32);
    fn SLNodeStyleSetPaddingPercent(node: SLNodeRef, edge: SLEdge, value: f32);
    fn SLNodeStyleSetPaddingCalc(node: SLNodeRef, edge: SLEdge, value: StarlightValue);
    fn SLNodeStyleSetPaddingValue(node: SLNodeRef, edge: SLEdge, value: StarlightValue);
    fn SLNodeStyleSetBorder(node: SLNodeRef, edge: SLEdge, value: f32);
    fn SLNodeStyleSetGap(node: SLNodeRef, gutter: SLGap, value: f32);
    fn SLNodeStyleSetGapPercent(node: SLNodeRef, gutter: SLGap, value: f32);
    fn SLNodeStyleSetGapCalc(node: SLNodeRef, gutter: SLGap, value: StarlightValue);
    fn SLNodeStyleSetGapValue(node: SLNodeRef, gutter: SLGap, value: StarlightValue);
    fn SLNodeStyleSetWidth(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetWidthPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetWidthCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetWidthValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetWidthAuto(node: SLNodeRef);
    fn SLNodeStyleSetWidthMaxContent(node: SLNodeRef);
    fn SLNodeStyleSetWidthFitContent(node: SLNodeRef);
    fn SLNodeStyleSetWidthFitContentValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMinWidth(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetMinWidthPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetMinWidthCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMinWidthValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMinWidthMaxContent(node: SLNodeRef);
    fn SLNodeStyleSetMinWidthFitContent(node: SLNodeRef);
    fn SLNodeStyleSetMinWidthFitContentValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMaxWidth(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetMaxWidthPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetMaxWidthCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMaxWidthValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMaxWidthMaxContent(node: SLNodeRef);
    fn SLNodeStyleSetMaxWidthFitContent(node: SLNodeRef);
    fn SLNodeStyleSetMaxWidthFitContentValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetHeight(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetHeightPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetHeightCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetHeightValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetHeightAuto(node: SLNodeRef);
    fn SLNodeStyleSetHeightMaxContent(node: SLNodeRef);
    fn SLNodeStyleSetHeightFitContent(node: SLNodeRef);
    fn SLNodeStyleSetHeightFitContentValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMinHeight(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetMinHeightPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetMinHeightCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMinHeightValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMinHeightMaxContent(node: SLNodeRef);
    fn SLNodeStyleSetMinHeightFitContent(node: SLNodeRef);
    fn SLNodeStyleSetMinHeightFitContentValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMaxHeight(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetMaxHeightPercent(node: SLNodeRef, value: f32);
    fn SLNodeStyleSetMaxHeightCalc(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMaxHeightValue(node: SLNodeRef, value: StarlightValue);
    fn SLNodeStyleSetMaxHeightMaxContent(node: SLNodeRef);
    fn SLNodeStyleSetMaxHeightFitContent(node: SLNodeRef);
    fn SLNodeStyleSetMaxHeightFitContentValue(node: SLNodeRef, value: StarlightValue);

    fn SLNodeStyleGetFlexDirection(node: SLNodeRef) -> SLFlexDirection;
    fn SLNodeStyleGetJustifyContent(node: SLNodeRef) -> SLJustifyContent;
    fn SLNodeStyleGetAlignContent(node: SLNodeRef) -> SLAlignContent;
    fn SLNodeStyleGetAlignItems(node: SLNodeRef) -> SLFlexAlign;
    fn SLNodeStyleGetAlignSelf(node: SLNodeRef) -> SLFlexAlign;
    fn SLNodeStyleGetPositionType(node: SLNodeRef) -> SLPositionType;
    fn SLNodeStyleGetFlexWrap(node: SLNodeRef) -> SLFlexWrap;
    fn SLNodeStyleGetLinearOrientation(node: SLNodeRef) -> SLLinearOrientation;
    fn SLNodeStyleGetLinearGravity(node: SLNodeRef) -> SLLinearGravity;
    fn SLNodeStyleGetLinearLayoutGravity(node: SLNodeRef) -> SLLinearLayoutGravity;
    fn SLNodeStyleGetLinearCrossGravity(node: SLNodeRef) -> SLLinearCrossGravity;
    fn SLNodeStyleGetRelativeCenter(node: SLNodeRef) -> SLRelativeCenter;
    fn SLNodeStyleGetGridAutoFlow(node: SLNodeRef) -> SLGridAutoFlow;
    fn SLNodeStyleGetJustifyItems(node: SLNodeRef) -> SLJustifyItem;
    fn SLNodeStyleGetJustifySelf(node: SLNodeRef) -> SLJustifyItem;
    fn SLNodeStyleGetDisplay(node: SLNodeRef) -> SLDisplay;
    fn SLNodeStyleGetBoxSizing(node: SLNodeRef) -> SLBoxSizing;
    fn SLNodeStyleGetAspectRatio(node: SLNodeRef) -> f32;
    fn SLNodeStyleGetOrder(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeId(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeAlignTop(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeAlignRight(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeAlignBottom(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeAlignLeft(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeTopOf(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeRightOf(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeBottomOf(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeLeftOf(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetRelativeLayoutOnce(node: SLNodeRef) -> bool;
    fn SLNodeStyleGetGridColumnStart(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetGridColumnEnd(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetGridRowStart(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetGridRowEnd(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetGridColumnSpan(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetGridRowSpan(node: SLNodeRef) -> i32;
    fn SLNodeStyleGetFlexGrow(node: SLNodeRef) -> f32;
    fn SLNodeStyleGetFlexShrink(node: SLNodeRef) -> f32;
    fn SLNodeStyleGetLinearWeight(node: SLNodeRef) -> f32;
    fn SLNodeStyleGetLinearWeightSum(node: SLNodeRef) -> f32;
    fn SLNodeStyleGetBorder(node: SLNodeRef, edge: SLEdge) -> f32;
    fn SLNodeStyleGetFlexBasis(node: SLNodeRef) -> StarlightValue;
    fn SLNodeStyleGetPosition(node: SLNodeRef, edge: SLEdge) -> StarlightValue;
    fn SLNodeStyleGetMargin(node: SLNodeRef, edge: SLEdge) -> StarlightValue;
    fn SLNodeStyleGetPadding(node: SLNodeRef, edge: SLEdge) -> StarlightValue;
    fn SLNodeStyleGetGap(node: SLNodeRef, gutter: SLGap) -> StarlightValue;
    fn SLNodeStyleGetWidth(node: SLNodeRef) -> StarlightValue;
    fn SLNodeStyleGetHeight(node: SLNodeRef) -> StarlightValue;
    fn SLNodeStyleGetMinWidth(node: SLNodeRef) -> StarlightValue;
    fn SLNodeStyleGetMaxWidth(node: SLNodeRef) -> StarlightValue;
    fn SLNodeStyleGetMinHeight(node: SLNodeRef) -> StarlightValue;
    fn SLNodeStyleGetMaxHeight(node: SLNodeRef) -> StarlightValue;

    fn SLNodeLayoutGetLeft(node: SLNodeRef) -> f32;
    fn SLNodeLayoutGetTop(node: SLNodeRef) -> f32;
    fn SLNodeLayoutGetWidth(node: SLNodeRef) -> f32;
    fn SLNodeLayoutGetHeight(node: SLNodeRef) -> f32;
    fn SLNodeLayoutGetBaseline(node: SLNodeRef) -> f32;
    fn SLNodeLayoutGetMargin(node: SLNodeRef, edge: SLEdge) -> f32;
    fn SLNodeLayoutGetPadding(node: SLNodeRef, edge: SLEdge) -> f32;
    fn SLNodeLayoutGetBorder(node: SLNodeRef, edge: SLEdge) -> f32;
    fn SLNodeLayoutGetStickyPosition(node: SLNodeRef, edge: SLEdge) -> f32;
}

struct NativeRoot {
    ptr: NonNull<StarlightNode>,
}

impl Drop for NativeRoot {
    fn drop(&mut self) {
        // SAFETY: `NativeRoot` owns the root returned by `SLNodeNew`; all
        // inserted descendants are owned by the native tree and freed
        // recursively exactly once here.
        unsafe { SLNodeFreeRecursive(self.ptr.as_ptr()) };
    }
}

struct NativeNode<N> {
    id: N,
    ptr: NonNull<StarlightNode>,
}

struct ProbeNodes {
    nodes: Vec<NonNull<StarlightNode>>,
}

impl ProbeNodes {
    fn new() -> Self {
        Self { nodes: Vec::new() }
    }

    fn create(&mut self) -> Result<NonNull<StarlightNode>, CppBaselineError> {
        let node = create_native_node(1.0)?;
        self.nodes.push(node);
        Ok(node)
    }

    fn create_with_config(
        &mut self,
        physical_pixels_per_layout_unit: f32,
    ) -> Result<NonNull<StarlightNode>, CppBaselineError> {
        let node = create_native_node(physical_pixels_per_layout_unit)?;
        self.nodes.push(node);
        Ok(node)
    }
}

impl Drop for ProbeNodes {
    fn drop(&mut self) {
        let mut freed = Vec::new();
        for node in &self.nodes {
            if freed.contains(&node.as_ptr()) {
                continue;
            }

            // SAFETY: probe nodes are owned by this RAII container. A node
            // with no parent is a current subtree root; collect descendants
            // before freeing so later listed child pointers are not freed twice.
            unsafe {
                if SLNodeGetParent(node.as_ptr()).is_null() {
                    collect_native_subtree_nodes(*node, &mut freed);
                    SLNodeFreeRecursive(node.as_ptr());
                }
            }
        }
    }
}

fn collect_native_subtree_nodes(
    node: NonNull<StarlightNode>,
    collected: &mut Vec<*mut StarlightNode>,
) {
    if collected.contains(&node.as_ptr()) {
        return;
    }
    collected.push(node.as_ptr());

    // SAFETY: `node` is a live standalone node owned by `ProbeNodes`; child
    // indices are read within the public child-count range.
    let count = unsafe { SLNodeGetChildCount(node.as_ptr()) };
    for index in 0..count {
        // SAFETY: `index` is in range according to `SLNodeGetChildCount`.
        if let Some(child) = NonNull::new(unsafe { SLNodeGetChild(node.as_ptr(), index) }) {
            collect_native_subtree_nodes(child, collected);
        }
    }
}

struct ProbeConfig {
    ptr: NonNull<StarlightConfig>,
}

impl ProbeConfig {
    fn new() -> Result<Self, CppBaselineError> {
        // SAFETY: `SLConfigCreate` returns a newly allocated standalone config
        // or null on allocation failure.
        let ptr = NonNull::new(unsafe { SLConfigCreate() }).ok_or(
            CppBaselineError::NativeLayoutUnavailable("SLConfigCreate returned null"),
        )?;
        Ok(Self { ptr })
    }

    fn set_physical_pixels_per_layout_unit(&mut self, value: f32) {
        // SAFETY: `self.ptr` is a live config owned by this RAII wrapper.
        unsafe { SLConfigSetPhysicalPixelsPerLayoutUnit(self.ptr.as_ptr(), value) };
    }

    fn physical_pixels_per_layout_unit(&self) -> f32 {
        // SAFETY: `self.ptr` is a live config owned by this RAII wrapper.
        unsafe { SLConfigGetPhysicalPixelsPerLayoutUnit(self.ptr.as_ptr()) }
    }
}

impl Drop for ProbeConfig {
    fn drop(&mut self) {
        // SAFETY: `self.ptr` is owned by this RAII wrapper and freed once.
        unsafe { SLConfigFree(self.ptr.as_ptr()) };
    }
}

struct NativeBuildState<T: LayoutTree> {
    tree: NonNull<T>,
    measure_contexts: Vec<MeasureContext<T>>,
    measure_delegates: Vec<StarlightMeasureDelegate>,
}

impl<T: LayoutTree> NativeBuildState<T> {
    fn new(tree: &mut T, measure_count: usize) -> Self {
        Self {
            tree: NonNull::from(tree),
            measure_contexts: Vec::with_capacity(measure_count),
            measure_delegates: Vec::with_capacity(measure_count),
        }
    }
}

struct MeasureContext<T: LayoutTree> {
    tree: NonNull<T>,
    node: T::NodeId,
    measured_content_size: Cell<Option<Size>>,
}

pub fn layout_standalone<T: LayoutTree>(
    tree: &mut T,
    root: T::NodeId,
    constraints: Constraints,
) -> Result<Size, CppBaselineError> {
    layout_standalone_with_owner_direction(tree, root, constraints, tree.style(root).direction)
}

pub fn layout_standalone_with_owner_direction<T: LayoutTree>(
    tree: &mut T,
    root: T::NodeId,
    constraints: Constraints,
    owner_direction: Direction,
) -> Result<Size, CppBaselineError> {
    let (owner_width, owner_width_mode) = owner_constraint_to_native(constraints.width);
    let (owner_height, owner_height_mode) = owner_constraint_to_native(constraints.height);
    let owner_direction = map_direction(owner_direction);
    let mut nodes = Vec::new();
    let measure_count = count_measure_nodes(&*tree, root);
    let mut build_state = NativeBuildState::new(tree, measure_count);
    let root_ptr = build_native_subtree(&*tree, root, &mut nodes, &mut build_state)?;
    let native_root = NativeRoot { ptr: root_ptr };

    // SAFETY: `native_root.ptr` is a live native root and owner sizes use the
    // public standalone sentinel for indefinite constraints. Constraint modes
    // are mapped from the Rust `MeasureMode` enum to the standalone C enum.
    unsafe {
        SLNodeCalculateLayoutWithMode(
            native_root.ptr.as_ptr(),
            owner_width,
            owner_width_mode,
            owner_height,
            owner_height_mode,
            owner_direction,
        );
    }

    let root_size = read_native_size(native_root.ptr);
    for native in nodes {
        let result = read_native_layout(native.ptr);
        tree.set_layout(native.id, result);
    }
    Ok(root_size)
}

pub fn standalone_public_tree_mutation_snapshots(
) -> Result<Vec<StandalonePublicTreeSnapshot>, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let inserted = probe_nodes.create()?;
    let staging = probe_nodes.create()?;
    let nodes = [root, first, second, third, inserted, staging];
    let mut snapshots = Vec::new();

    // SAFETY: all pointers are live standalone nodes owned by `probe_nodes`;
    // index -1 is the public standalone append sentinel.
    unsafe {
        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);
    }
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterAppend,
        &nodes,
        root,
        staging,
    ));

    calculate_probe_layout(root);
    calculate_probe_layout(staging);
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterCleanLayout,
        &nodes,
        root,
        staging,
    ));

    // SAFETY: `second` is not attached to `staging`; this probes the public
    // no-op removal contract.
    unsafe { SLNodeRemoveChild(staging.as_ptr(), second.as_ptr()) };
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterNoopRemove,
        &nodes,
        root,
        staging,
    ));

    // SAFETY: all pointers are live, and index 1 inserts before the current
    // second child through the public standalone API.
    unsafe { SLNodeInsertChild(root.as_ptr(), inserted.as_ptr(), 1) };
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterIndexInsert,
        &nodes,
        root,
        staging,
    ));

    calculate_probe_layout(root);
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterCleanInsertedLayout,
        &nodes,
        root,
        staging,
    ));

    // SAFETY: `inserted` is currently a root child and is reparented to the
    // staging node using the public append sentinel.
    unsafe { SLNodeInsertChild(staging.as_ptr(), inserted.as_ptr(), -1) };
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterReparentToStaging,
        &nodes,
        root,
        staging,
    ));

    // SAFETY: all pointers are live; `inserted` is reparented before `second`.
    unsafe {
        SLNodeInsertChildBefore(root.as_ptr(), inserted.as_ptr(), second.as_ptr());
    }
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterInsertBefore,
        &nodes,
        root,
        staging,
    ));

    // SAFETY: `second` is currently attached to `root`.
    unsafe { SLNodeRemoveChild(root.as_ptr(), second.as_ptr()) };
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterRemoveChild,
        &nodes,
        root,
        staging,
    ));

    // SAFETY: `root` is a live standalone node.
    unsafe { SLNodeRemoveAllChildren(root.as_ptr()) };
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterRemoveAllChildren,
        &nodes,
        root,
        staging,
    ));

    // SAFETY: `staging` is a live standalone node.
    unsafe { SLNodeReset(staging.as_ptr()) };
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterResetStaging,
        &nodes,
        root,
        staging,
    ));

    Ok(snapshots)
}

pub fn standalone_public_dirty_snapshot() -> Result<StandalonePublicDirtySnapshot, CppBaselineError>
{
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let child = probe_nodes.create()?;
    let grandchild = probe_nodes.create()?;
    let nodes = [root, child, grandchild];

    // SAFETY: all pointers are live standalone nodes owned by `probe_nodes`;
    // index -1 is the public standalone append sentinel.
    unsafe {
        SLNodeInsertChild(root.as_ptr(), child.as_ptr(), -1);
        SLNodeInsertChild(child.as_ptr(), grandchild.as_ptr(), -1);
    }

    calculate_probe_layout(root);
    let after_clean_layout = snapshot_public_dirty_nodes(&nodes);

    // SAFETY: `grandchild` is a live standalone node. This probes explicit
    // dirty propagation to ancestors through the public API.
    unsafe { SLNodeMarkDirty(grandchild.as_ptr()) };
    let after_mark_grandchild = snapshot_public_dirty_nodes(&nodes);

    calculate_probe_layout(root);
    let after_reclean_layout = snapshot_public_dirty_nodes(&nodes);

    // SAFETY: `root` is a live standalone node. Marking a clean root should not
    // dirty clean descendants.
    unsafe { SLNodeMarkDirty(root.as_ptr()) };
    let after_mark_root = snapshot_public_dirty_nodes(&nodes);

    Ok(StandalonePublicDirtySnapshot {
        after_clean_layout,
        after_mark_grandchild,
        after_reclean_layout,
        after_mark_root,
    })
}

pub fn standalone_public_edge_style_snapshots(
) -> Result<Vec<StandalonePublicEdgeStyleSnapshot>, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let ltr = probe_nodes.create()?;
    let rtl = probe_nodes.create()?;

    // SAFETY: `ltr` is a live standalone node owned by `probe_nodes`; all
    // setters use public enum values from the standalone C header.
    unsafe {
        SLNodeStyleSetDirection(ltr.as_ptr(), SL_DIRECTION_LTR);
        SLNodeStyleSetPosition(ltr.as_ptr(), SL_EDGE_START, 1.0);
        SLNodeStyleSetPositionPercent(ltr.as_ptr(), SL_EDGE_END, 20.0);
        SLNodeStyleSetPositionCalc(
            ltr.as_ptr(),
            SL_EDGE_VERTICAL,
            starlight_value_from_calc_length(3.0, 30.0),
        );
        SLNodeStyleSetMarginAuto(ltr.as_ptr(), SL_EDGE_HORIZONTAL);
        SLNodeStyleSetMarginPercent(ltr.as_ptr(), SL_EDGE_TOP, 10.0);
        SLNodeStyleSetPadding(ltr.as_ptr(), SL_EDGE_ALL, 4.0);
        SLNodeStyleSetPaddingCalc(
            ltr.as_ptr(),
            SL_EDGE_BOTTOM,
            starlight_value_from_calc_length(2.0, 5.0),
        );
        SLNodeStyleSetBorder(ltr.as_ptr(), SL_EDGE_ALL, 1.0);
        SLNodeStyleSetBorder(ltr.as_ptr(), SL_EDGE_START, 2.0);
        SLNodeStyleSetBorder(ltr.as_ptr(), SL_EDGE_END, 3.0);
        SLNodeStyleSetGap(ltr.as_ptr(), SL_GAP_ALL, 6.0);
        SLNodeStyleSetGapPercent(ltr.as_ptr(), SL_GAP_ROW, 7.0);
    }

    // SAFETY: `rtl` is a live standalone node owned by `probe_nodes`; all
    // setters use public enum values from the standalone C header.
    unsafe {
        SLNodeStyleSetDirection(rtl.as_ptr(), SL_DIRECTION_RTL);
        SLNodeStyleSetPosition(rtl.as_ptr(), SL_EDGE_START, 11.0);
        SLNodeStyleSetPositionPercent(rtl.as_ptr(), SL_EDGE_END, 12.0);
        SLNodeStyleSetPosition(rtl.as_ptr(), SL_EDGE_VERTICAL, 13.0);
        SLNodeStyleSetMarginAuto(rtl.as_ptr(), SL_EDGE_START);
        SLNodeStyleSetMarginPercent(rtl.as_ptr(), SL_EDGE_END, 14.0);
        SLNodeStyleSetPadding(rtl.as_ptr(), SL_EDGE_HORIZONTAL, 15.0);
        SLNodeStyleSetPaddingPercent(rtl.as_ptr(), SL_EDGE_VERTICAL, 16.0);
        SLNodeStyleSetBorder(rtl.as_ptr(), SL_EDGE_ALL, 17.0);
        SLNodeStyleSetBorder(rtl.as_ptr(), SL_EDGE_START, 18.0);
        SLNodeStyleSetBorder(rtl.as_ptr(), SL_EDGE_END, 19.0);
        SLNodeStyleSetGapCalc(
            rtl.as_ptr(),
            SL_GAP_ALL,
            starlight_value_from_calc_length(4.0, 40.0),
        );
        SLNodeStyleSetGap(rtl.as_ptr(), SL_GAP_COLUMN, 21.0);
    }

    Ok(vec![
        snapshot_public_edge_style_stage(StandalonePublicStyleStage::Ltr, ltr),
        snapshot_public_edge_style_stage(StandalonePublicStyleStage::Rtl, rtl),
    ])
}

#[derive(Clone, Copy)]
enum PublicEdgeStyleVariant {
    Points,
    Percent,
    Calc,
    ValueFr,
    ValueMaxContent,
    ValueFitContent,
    Auto,
}

pub fn standalone_public_edge_style_variant_snapshots(
) -> Result<Vec<StandalonePublicEdgeStyleVariantSnapshot>, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let variants = [
        PublicEdgeStyleVariant::Points,
        PublicEdgeStyleVariant::Percent,
        PublicEdgeStyleVariant::Calc,
        PublicEdgeStyleVariant::ValueFr,
        PublicEdgeStyleVariant::ValueMaxContent,
        PublicEdgeStyleVariant::ValueFitContent,
        PublicEdgeStyleVariant::Auto,
    ];
    let mut snapshots = Vec::with_capacity(variants.len());

    for variant in variants {
        let node = probe_nodes.create()?;
        // SAFETY: `node` is live and owned by `probe_nodes`. The applied
        // `StarlightValue` arguments are passed by value through synchronous
        // public C API calls.
        unsafe {
            apply_public_edge_style_variant(node, variant);
        }
        snapshots.push(snapshot_public_edge_style_variant(node));
    }

    Ok(snapshots)
}

pub fn standalone_public_scalar_style_snapshot(
) -> Result<StandalonePublicScalarStyleSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let node = probe_nodes.create()?;

    // SAFETY: `node` is a live standalone node owned by `probe_nodes`; all
    // setters use public enum values from the standalone C header.
    unsafe {
        SLNodeStyleSetFlexDirection(node.as_ptr(), SL_FLEX_DIRECTION_ROW_REVERSE);
        SLNodeStyleSetJustifyContent(node.as_ptr(), SL_JUSTIFY_CONTENT_SPACE_BETWEEN);
        SLNodeStyleSetAlignContent(node.as_ptr(), SL_ALIGN_CONTENT_SPACE_EVENLY);
        SLNodeStyleSetAlignItems(node.as_ptr(), SL_FLEX_ALIGN_BASELINE);
        SLNodeStyleSetAlignSelf(node.as_ptr(), SL_FLEX_ALIGN_END);
        SLNodeStyleSetPositionType(node.as_ptr(), SL_POSITION_TYPE_STICKY);
        SLNodeStyleSetFlexWrap(node.as_ptr(), SL_FLEX_WRAP_WRAP_REVERSE);
        SLNodeStyleSetLinearOrientation(node.as_ptr(), SL_LINEAR_ORIENTATION_VERTICAL_REVERSE);
        SLNodeStyleSetLinearGravity(node.as_ptr(), SL_LINEAR_GRAVITY_CENTER);
        SLNodeStyleSetLinearLayoutGravity(node.as_ptr(), SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL);
        SLNodeStyleSetLinearCrossGravity(node.as_ptr(), SL_LINEAR_CROSS_GRAVITY_STRETCH);
        SLNodeStyleSetRelativeCenter(node.as_ptr(), SL_RELATIVE_CENTER_BOTH);
        SLNodeStyleSetGridAutoFlow(node.as_ptr(), SL_GRID_AUTO_FLOW_COLUMN_DENSE);
        SLNodeStyleSetJustifyItems(node.as_ptr(), SL_JUSTIFY_ITEM_CENTER);
        SLNodeStyleSetJustifySelf(node.as_ptr(), SL_JUSTIFY_ITEM_END);
        SLNodeStyleSetDisplay(node.as_ptr(), SL_DISPLAY_GRID);
        SLNodeStyleSetBoxSizing(node.as_ptr(), SL_BOX_SIZING_CONTENT_BOX);
        SLNodeStyleSetAspectRatio(node.as_ptr(), 1.5);
        SLNodeStyleSetOrder(node.as_ptr(), -2);
        SLNodeStyleSetRelativeId(node.as_ptr(), 17);
        SLNodeStyleSetRelativeAlignTop(node.as_ptr(), 1);
        SLNodeStyleSetRelativeAlignRight(node.as_ptr(), 2);
        SLNodeStyleSetRelativeAlignBottom(node.as_ptr(), 3);
        SLNodeStyleSetRelativeAlignLeft(node.as_ptr(), 4);
        SLNodeStyleSetRelativeTopOf(node.as_ptr(), 5);
        SLNodeStyleSetRelativeRightOf(node.as_ptr(), 6);
        SLNodeStyleSetRelativeBottomOf(node.as_ptr(), 7);
        SLNodeStyleSetRelativeLeftOf(node.as_ptr(), 8);
        SLNodeStyleSetRelativeLayoutOnce(node.as_ptr(), true);
        SLNodeStyleSetGridColumnStart(node.as_ptr(), 2);
        SLNodeStyleSetGridColumnEnd(node.as_ptr(), 4);
        SLNodeStyleSetGridRowStart(node.as_ptr(), 3);
        SLNodeStyleSetGridRowEnd(node.as_ptr(), 5);
        SLNodeStyleSetGridColumnSpan(node.as_ptr(), 6);
        SLNodeStyleSetGridRowSpan(node.as_ptr(), 7);
        SLNodeStyleSetFlex(node.as_ptr(), 2.5);
        SLNodeStyleSetLinearWeight(node.as_ptr(), 3.0);
        SLNodeStyleSetLinearWeightSum(node.as_ptr(), 9.0);
    }

    Ok(snapshot_public_scalar_style(node))
}

pub fn standalone_public_dimension_style_snapshot(
) -> Result<StandalonePublicDimensionStyleSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let node = probe_nodes.create()?;

    // SAFETY: `node` is a live standalone node owned by `probe_nodes`. The
    // `StarlightValue` arguments are passed by value through synchronous C API
    // calls.
    unsafe {
        SLNodeStyleSetFlexBasisPercent(node.as_ptr(), 12.5);
        SLNodeStyleSetWidthAuto(node.as_ptr());
        SLNodeStyleSetHeightCalc(node.as_ptr(), starlight_value_from_calc_length(30.0, 45.0));
        SLNodeStyleSetMinWidthMaxContent(node.as_ptr());
        SLNodeStyleSetMaxWidthFitContentValue(
            node.as_ptr(),
            starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(10.0, 25.0)),
        );
        SLNodeStyleSetMinHeight(node.as_ptr(), 7.0);
        SLNodeStyleSetMaxHeightFitContent(node.as_ptr());
    }

    Ok(snapshot_public_dimension_style(node))
}

#[derive(Clone, Copy)]
enum PublicDimensionStyleVariant {
    Points,
    Percent,
    Calc,
    ValueFr,
    Auto,
    MaxContent,
    FitContent,
    FitContentValue,
}

pub fn standalone_public_dimension_style_variant_snapshots(
) -> Result<Vec<StandalonePublicDimensionStyleSnapshot>, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let variants = [
        PublicDimensionStyleVariant::Points,
        PublicDimensionStyleVariant::Percent,
        PublicDimensionStyleVariant::Calc,
        PublicDimensionStyleVariant::ValueFr,
        PublicDimensionStyleVariant::Auto,
        PublicDimensionStyleVariant::MaxContent,
        PublicDimensionStyleVariant::FitContent,
        PublicDimensionStyleVariant::FitContentValue,
    ];
    let mut snapshots = Vec::with_capacity(variants.len());

    for variant in variants {
        let node = probe_nodes.create()?;
        // SAFETY: `node` is live and owned by `probe_nodes`. The applied
        // `StarlightValue` arguments are passed by value through synchronous
        // public C API calls.
        unsafe {
            apply_public_dimension_style_variant(node, variant);
        }
        snapshots.push(snapshot_public_dimension_style(node));
    }

    Ok(snapshots)
}

pub fn standalone_public_dimension_layout_snapshots(
) -> Result<Vec<StandalonePublicDimensionLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = Vec::new();

    for variant in [
        PublicDimensionStyleVariant::Points,
        PublicDimensionStyleVariant::Percent,
        PublicDimensionStyleVariant::Calc,
        PublicDimensionStyleVariant::ValueFr,
        PublicDimensionStyleVariant::Auto,
        PublicDimensionStyleVariant::MaxContent,
        PublicDimensionStyleVariant::FitContent,
        PublicDimensionStyleVariant::FitContentValue,
    ] {
        snapshots.push(standalone_public_dimension_layout_snapshot(variant)?);
    }

    Ok(snapshots)
}

fn standalone_public_dimension_layout_snapshot(
    variant: PublicDimensionStyleVariant,
) -> Result<StandalonePublicDimensionLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let basis = probe_nodes.create()?;
    let sized = probe_nodes.create()?;
    let clamped = probe_nodes.create()?;
    let trailing = probe_nodes.create()?;
    let nodes = [root, basis, sized, clamped, trailing];
    let mut basis_delegate = standalone_public_dimension_layout_delegate();
    let mut sized_delegate = standalone_public_dimension_layout_delegate();
    let mut clamped_delegate = standalone_public_dimension_layout_delegate();

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`; the
    // measure delegates outlive the synchronous layout call.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 200.0);
        SLNodeStyleSetHeight(root.as_ptr(), 100.0);

        SLNodeStyleSetDisplay(basis.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeSetMeasureDelegate(basis.as_ptr(), &mut basis_delegate);
        SLNodeStyleSetHeight(basis.as_ptr(), 10.0);
        apply_public_dimension_layout_basis_variant(basis, variant);

        SLNodeStyleSetDisplay(sized.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeSetMeasureDelegate(sized.as_ptr(), &mut sized_delegate);
        apply_public_dimension_layout_size_variant(sized, variant);

        SLNodeStyleSetDisplay(clamped.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeSetMeasureDelegate(clamped.as_ptr(), &mut clamped_delegate);
        SLNodeStyleSetWidth(clamped.as_ptr(), 30.0);
        SLNodeStyleSetHeight(clamped.as_ptr(), 12.0);
        apply_public_dimension_layout_clamp_variant(clamped, variant);

        SLNodeStyleSetDisplay(trailing.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(trailing.as_ptr(), 20.0);
        SLNodeStyleSetHeight(trailing.as_ptr(), 12.0);

        SLNodeInsertChild(root.as_ptr(), basis.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), sized.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), clamped.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), trailing.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            200.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicDimensionLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_direction_snapshot(
) -> Result<StandalonePublicDirectionSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let node = probe_nodes.create()?;

    // SAFETY: `node` is a live standalone node owned by `probe_nodes`.
    let default_is_rtl = unsafe { SLNodeIsRTL(node.as_ptr()) };
    // SAFETY: `node` is live and direction values come from the public enum.
    let (rtl_is_rtl, ltr_is_rtl, dirty_after_direction_updates) = unsafe {
        SLNodeStyleSetDirection(node.as_ptr(), SL_DIRECTION_RTL);
        let rtl_is_rtl = SLNodeIsRTL(node.as_ptr());
        SLNodeStyleSetDirection(node.as_ptr(), SL_DIRECTION_LTR);
        let ltr_is_rtl = SLNodeIsRTL(node.as_ptr());
        let dirty_after_direction_updates = SLNodeIsDirty(node.as_ptr());
        (rtl_is_rtl, ltr_is_rtl, dirty_after_direction_updates)
    };

    Ok(StandalonePublicDirectionSnapshot {
        default_is_rtl,
        rtl_is_rtl,
        ltr_is_rtl,
        dirty_after_direction_updates,
    })
}

#[derive(Clone, Copy)]
enum PublicDirectionLayoutVariant {
    Ltr,
    Rtl,
}

impl PublicDirectionLayoutVariant {
    const ALL: [Self; 2] = [Self::Ltr, Self::Rtl];

    const fn native_direction(self) -> SLDirection {
        match self {
            Self::Ltr => SL_DIRECTION_LTR,
            Self::Rtl => SL_DIRECTION_RTL,
        }
    }
}

pub fn standalone_public_direction_layout_snapshots(
) -> Result<Vec<StandalonePublicDirectionLayoutSnapshot>, CppBaselineError> {
    PublicDirectionLayoutVariant::ALL
        .into_iter()
        .map(standalone_public_direction_layout_snapshot)
        .collect()
}

fn standalone_public_direction_layout_snapshot(
    variant: PublicDirectionLayoutVariant,
) -> Result<StandalonePublicDirectionLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let nodes = [root, first, second];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetDirection(root.as_ptr(), variant.native_direction());
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 40.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);
        SLNodeStyleSetMargin(first.as_ptr(), SL_EDGE_START, 7.0);
        SLNodeStyleSetMargin(first.as_ptr(), SL_EDGE_END, 3.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 20.0);
        SLNodeStyleSetHeight(second.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            40.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicDirectionLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_layout_getter_snapshot(
) -> Result<StandalonePublicLayoutGetterSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let child = probe_nodes.create()?;

    // SAFETY: `root` and `child` are live standalone nodes owned by
    // `probe_nodes`. Setters use public standalone enum values and the layout
    // call is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 80.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_ALL, 4.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_LEFT, 6.0);
        SLNodeStyleSetBorder(root.as_ptr(), SL_EDGE_ALL, 2.0);
        SLNodeStyleSetMargin(root.as_ptr(), SL_EDGE_LEFT, 3.0);

        SLNodeStyleSetDisplay(child.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetDirection(child.as_ptr(), SL_DIRECTION_RTL);
        SLNodeStyleSetPositionType(child.as_ptr(), SL_POSITION_TYPE_STICKY);
        SLNodeStyleSetWidth(child.as_ptr(), 40.0);
        SLNodeStyleSetHeight(child.as_ptr(), 20.0);
        SLNodeStyleSetMargin(child.as_ptr(), SL_EDGE_LEFT, 5.0);
        SLNodeStyleSetMargin(child.as_ptr(), SL_EDGE_TOP, 7.0);
        SLNodeStyleSetPadding(child.as_ptr(), SL_EDGE_ALL, 1.0);
        SLNodeStyleSetBorder(child.as_ptr(), SL_EDGE_ALL, 2.0);
        SLNodeStyleSetPosition(child.as_ptr(), SL_EDGE_LEFT, 11.0);
        SLNodeStyleSetPosition(child.as_ptr(), SL_EDGE_RIGHT, 12.0);
        SLNodeStyleSetPosition(child.as_ptr(), SL_EDGE_TOP, 13.0);
        SLNodeStyleSetPosition(child.as_ptr(), SL_EDGE_BOTTOM, 14.0);

        SLNodeInsertChild(root.as_ptr(), child.as_ptr(), 0);
        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLayoutGetterSnapshot {
        root: snapshot_public_layout_node(root),
        child: snapshot_public_layout_node(child),
    })
}

pub fn standalone_public_box_aspect_layout_snapshot(
) -> Result<StandalonePublicBoxAspectLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let content_box = probe_nodes.create()?;
    let border_box = probe_nodes.create()?;
    let clamped = probe_nodes.create()?;
    let nodes = [root, content_box, border_box, clamped];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone enum/edge values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(root.as_ptr(), 120.0);
        SLNodeStyleSetHeight(root.as_ptr(), 130.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_LEFT, 3.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_TOP, 4.0);
        SLNodeStyleSetBorder(root.as_ptr(), SL_EDGE_ALL, 1.0);

        SLNodeStyleSetDisplay(content_box.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetBoxSizing(content_box.as_ptr(), SL_BOX_SIZING_CONTENT_BOX);
        SLNodeStyleSetAspectRatio(content_box.as_ptr(), 2.0);
        SLNodeStyleSetWidth(content_box.as_ptr(), 48.0);
        SLNodeStyleSetPadding(content_box.as_ptr(), SL_EDGE_HORIZONTAL, 4.0);
        SLNodeStyleSetPadding(content_box.as_ptr(), SL_EDGE_VERTICAL, 3.0);
        SLNodeStyleSetBorder(content_box.as_ptr(), SL_EDGE_ALL, 2.0);

        SLNodeStyleSetDisplay(border_box.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetBoxSizing(border_box.as_ptr(), SL_BOX_SIZING_BORDER_BOX);
        SLNodeStyleSetAspectRatio(border_box.as_ptr(), 2.0);
        SLNodeStyleSetWidth(border_box.as_ptr(), 48.0);
        SLNodeStyleSetPadding(border_box.as_ptr(), SL_EDGE_HORIZONTAL, 4.0);
        SLNodeStyleSetPadding(border_box.as_ptr(), SL_EDGE_VERTICAL, 3.0);
        SLNodeStyleSetBorder(border_box.as_ptr(), SL_EDGE_ALL, 2.0);

        SLNodeStyleSetDisplay(clamped.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetBoxSizing(clamped.as_ptr(), SL_BOX_SIZING_CONTENT_BOX);
        SLNodeStyleSetAspectRatio(clamped.as_ptr(), 3.0);
        SLNodeStyleSetWidth(clamped.as_ptr(), 36.0);
        SLNodeStyleSetMinHeight(clamped.as_ptr(), 24.0);
        SLNodeStyleSetPadding(clamped.as_ptr(), SL_EDGE_ALL, 2.0);
        SLNodeStyleSetBorder(clamped.as_ptr(), SL_EDGE_ALL, 1.0);

        SLNodeInsertChild(root.as_ptr(), content_box.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), border_box.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), clamped.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            130.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicBoxAspectLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

#[derive(Clone, Copy)]
enum PublicDisplayLayoutVariant {
    None,
    Block,
    Flex,
    Linear,
    Relative,
    Grid,
}

impl PublicDisplayLayoutVariant {
    fn sl_display(self) -> SLDisplay {
        match self {
            PublicDisplayLayoutVariant::None => SL_DISPLAY_NONE,
            PublicDisplayLayoutVariant::Block => SL_DISPLAY_BLOCK,
            PublicDisplayLayoutVariant::Flex => SL_DISPLAY_FLEX,
            PublicDisplayLayoutVariant::Linear => SL_DISPLAY_LINEAR,
            PublicDisplayLayoutVariant::Relative => SL_DISPLAY_RELATIVE,
            PublicDisplayLayoutVariant::Grid => SL_DISPLAY_GRID,
        }
    }
}

pub fn standalone_public_display_layout_snapshots(
) -> Result<Vec<StandalonePublicDisplayLayoutSnapshot>, CppBaselineError> {
    const VARIANTS: [PublicDisplayLayoutVariant; 6] = [
        PublicDisplayLayoutVariant::None,
        PublicDisplayLayoutVariant::Block,
        PublicDisplayLayoutVariant::Flex,
        PublicDisplayLayoutVariant::Linear,
        PublicDisplayLayoutVariant::Relative,
        PublicDisplayLayoutVariant::Grid,
    ];

    let mut snapshots = Vec::with_capacity(VARIANTS.len());
    for variant in VARIANTS {
        snapshots.push(standalone_public_display_layout_snapshot(variant)?);
    }
    Ok(snapshots)
}

fn standalone_public_display_layout_snapshot(
    variant: PublicDisplayLayoutVariant,
) -> Result<StandalonePublicDisplayLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let container = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, container, first, second, third];
    let template_columns = [Length::points(36.0), Length::points(24.0)];
    let template_rows = [Length::points(18.0), Length::points(16.0)];
    let auto_columns = [Length::points(24.0)];
    let auto_rows = [Length::points(16.0)];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Track vectors stay alive for each synchronous public setter call.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(root.as_ptr(), 180.0);
        SLNodeStyleSetHeight(root.as_ptr(), 130.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_LEFT, 3.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_TOP, 5.0);
        SLNodeStyleSetBorder(root.as_ptr(), SL_EDGE_ALL, 1.0);

        SLNodeStyleSetDisplay(container.as_ptr(), variant.sl_display());
        SLNodeStyleSetWidth(container.as_ptr(), 120.0);
        SLNodeStyleSetHeight(container.as_ptr(), 72.0);
        SLNodeStyleSetMargin(container.as_ptr(), SL_EDGE_LEFT, 7.0);
        SLNodeStyleSetMargin(container.as_ptr(), SL_EDGE_TOP, 6.0);
        SLNodeStyleSetPadding(container.as_ptr(), SL_EDGE_LEFT, 2.0);
        SLNodeStyleSetPadding(container.as_ptr(), SL_EDGE_TOP, 3.0);
        SLNodeStyleSetBorder(container.as_ptr(), SL_EDGE_ALL, 1.0);
        SLNodeStyleSetGap(container.as_ptr(), SL_GAP_COLUMN, 4.0);
        SLNodeStyleSetGap(container.as_ptr(), SL_GAP_ROW, 3.0);
        SLNodeStyleSetFlexDirection(container.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(container.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(container.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(container.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetLinearOrientation(container.as_ptr(), SL_LINEAR_ORIENTATION_HORIZONTAL);
        SLNodeStyleSetGridAutoFlow(container.as_ptr(), SL_GRID_AUTO_FLOW_ROW);
        SLNodeStyleSetJustifyItems(container.as_ptr(), SL_JUSTIFY_ITEM_START);
        set_grid_track_vector(
            container.as_ptr(),
            &template_columns,
            SLNodeStyleSetGridTemplateColumns,
        )?;
        set_grid_track_vector(
            container.as_ptr(),
            &template_columns,
            SLNodeStyleSetGridTemplateColumnsMax,
        )?;
        set_grid_track_vector(
            container.as_ptr(),
            &template_rows,
            SLNodeStyleSetGridTemplateRows,
        )?;
        set_grid_track_vector(
            container.as_ptr(),
            &template_rows,
            SLNodeStyleSetGridTemplateRowsMax,
        )?;
        set_grid_track_vector(
            container.as_ptr(),
            &auto_columns,
            SLNodeStyleSetGridAutoColumns,
        )?;
        set_grid_track_vector(
            container.as_ptr(),
            &auto_columns,
            SLNodeStyleSetGridAutoColumnsMax,
        )?;
        set_grid_track_vector(container.as_ptr(), &auto_rows, SLNodeStyleSetGridAutoRows)?;
        set_grid_track_vector(
            container.as_ptr(),
            &auto_rows,
            SLNodeStyleSetGridAutoRowsMax,
        )?;

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 30.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);
        SLNodeStyleSetRelativeId(first.as_ptr(), 10);
        SLNodeStyleSetGridColumnStart(first.as_ptr(), 1);
        SLNodeStyleSetGridRowStart(first.as_ptr(), 1);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 20.0);
        SLNodeStyleSetHeight(second.as_ptr(), 12.0);
        SLNodeStyleSetRelativeRightOf(second.as_ptr(), 10);
        SLNodeStyleSetGridColumnStart(second.as_ptr(), 2);
        SLNodeStyleSetGridRowStart(second.as_ptr(), 1);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 16.0);
        SLNodeStyleSetHeight(third.as_ptr(), 8.0);
        SLNodeStyleSetRelativeBottomOf(third.as_ptr(), 10);
        SLNodeStyleSetGridColumnStart(third.as_ptr(), 1);
        SLNodeStyleSetGridRowStart(third.as_ptr(), 2);

        SLNodeInsertChild(container.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(container.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(container.as_ptr(), third.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), container.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            180.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            130.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    let snapshot_nodes = if matches!(variant, PublicDisplayLayoutVariant::None) {
        &nodes[..2]
    } else {
        &nodes[..]
    };

    Ok(StandalonePublicDisplayLayoutSnapshot {
        nodes: snapshot_nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_position_layout_snapshot(
) -> Result<StandalonePublicPositionLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let in_flow = probe_nodes.create()?;
    let relative = probe_nodes.create()?;
    let absolute = probe_nodes.create()?;
    let fixed = probe_nodes.create()?;
    let sticky = probe_nodes.create()?;
    let nodes = [root, in_flow, relative, absolute, fixed, sticky];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Position values use public standalone enum/edge values and layout is
    // synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(root.as_ptr(), 140.0);
        SLNodeStyleSetHeight(root.as_ptr(), 120.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_LEFT, 6.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_TOP, 4.0);
        SLNodeStyleSetBorder(root.as_ptr(), SL_EDGE_ALL, 1.0);

        SLNodeStyleSetDisplay(in_flow.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(in_flow.as_ptr(), 24.0);
        SLNodeStyleSetHeight(in_flow.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(relative.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetPositionType(relative.as_ptr(), SL_POSITION_TYPE_RELATIVE);
        SLNodeStyleSetPosition(relative.as_ptr(), SL_EDGE_LEFT, 5.0);
        SLNodeStyleSetPosition(relative.as_ptr(), SL_EDGE_TOP, 7.0);
        SLNodeStyleSetWidth(relative.as_ptr(), 26.0);
        SLNodeStyleSetHeight(relative.as_ptr(), 12.0);

        SLNodeStyleSetDisplay(absolute.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetPositionType(absolute.as_ptr(), SL_POSITION_TYPE_ABSOLUTE);
        SLNodeStyleSetPosition(absolute.as_ptr(), SL_EDGE_LEFT, 10.0);
        SLNodeStyleSetPosition(absolute.as_ptr(), SL_EDGE_TOP, 15.0);
        SLNodeStyleSetWidth(absolute.as_ptr(), 30.0);
        SLNodeStyleSetHeight(absolute.as_ptr(), 14.0);

        SLNodeStyleSetDisplay(fixed.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetPositionType(fixed.as_ptr(), SL_POSITION_TYPE_FIXED);
        SLNodeStyleSetPosition(fixed.as_ptr(), SL_EDGE_RIGHT, 8.0);
        SLNodeStyleSetPosition(fixed.as_ptr(), SL_EDGE_BOTTOM, 9.0);
        SLNodeStyleSetWidth(fixed.as_ptr(), 28.0);
        SLNodeStyleSetHeight(fixed.as_ptr(), 16.0);

        SLNodeStyleSetDisplay(sticky.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetPositionType(sticky.as_ptr(), SL_POSITION_TYPE_STICKY);
        SLNodeStyleSetPosition(sticky.as_ptr(), SL_EDGE_LEFT, 3.0);
        SLNodeStyleSetPosition(sticky.as_ptr(), SL_EDGE_TOP, 4.0);
        SLNodeStyleSetWidth(sticky.as_ptr(), 22.0);
        SLNodeStyleSetHeight(sticky.as_ptr(), 11.0);

        SLNodeInsertChild(root.as_ptr(), in_flow.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), relative.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), absolute.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), fixed.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), sticky.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            140.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicPositionLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_relative_layout_snapshots(
) -> Result<Vec<StandalonePublicRelativeLayoutSnapshot>, CppBaselineError> {
    Ok(vec![
        standalone_public_relative_definite_layout_snapshot()?,
        standalone_public_relative_layout_once_snapshot()?,
    ])
}

fn standalone_public_relative_definite_layout_snapshot(
) -> Result<StandalonePublicRelativeLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let center_none = probe_nodes.create()?;
    let center_horizontal = probe_nodes.create()?;
    let center_vertical = probe_nodes.create()?;
    let center_both = probe_nodes.create()?;
    let parent_end = probe_nodes.create()?;
    let anchor = probe_nodes.create()?;
    let before = probe_nodes.create()?;
    let aligned = probe_nodes.create()?;
    let nodes = [
        root,
        center_none,
        center_horizontal,
        center_vertical,
        center_both,
        parent_end,
        anchor,
        before,
        aligned,
    ];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone enum/edge values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_RELATIVE);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 80.0);

        SLNodeStyleSetDisplay(center_none.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeCenter(center_none.as_ptr(), SL_RELATIVE_CENTER_NONE);
        SLNodeStyleSetWidth(center_none.as_ptr(), 20.0);
        SLNodeStyleSetHeight(center_none.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(center_horizontal.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeCenter(center_horizontal.as_ptr(), SL_RELATIVE_CENTER_HORIZONTAL);
        SLNodeStyleSetWidth(center_horizontal.as_ptr(), 20.0);
        SLNodeStyleSetHeight(center_horizontal.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(center_vertical.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeCenter(center_vertical.as_ptr(), SL_RELATIVE_CENTER_VERTICAL);
        SLNodeStyleSetWidth(center_vertical.as_ptr(), 20.0);
        SLNodeStyleSetHeight(center_vertical.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(center_both.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeCenter(center_both.as_ptr(), SL_RELATIVE_CENTER_BOTH);
        SLNodeStyleSetWidth(center_both.as_ptr(), 20.0);
        SLNodeStyleSetHeight(center_both.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(parent_end.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeAlignRight(parent_end.as_ptr(), 0);
        SLNodeStyleSetRelativeAlignBottom(parent_end.as_ptr(), 0);
        SLNodeStyleSetWidth(parent_end.as_ptr(), 18.0);
        SLNodeStyleSetHeight(parent_end.as_ptr(), 8.0);

        SLNodeStyleSetDisplay(anchor.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeId(anchor.as_ptr(), 20);
        SLNodeStyleSetRelativeAlignRight(anchor.as_ptr(), 0);
        SLNodeStyleSetRelativeAlignBottom(anchor.as_ptr(), 0);
        SLNodeStyleSetWidth(anchor.as_ptr(), 20.0);
        SLNodeStyleSetHeight(anchor.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(before.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeLeftOf(before.as_ptr(), 20);
        SLNodeStyleSetRelativeTopOf(before.as_ptr(), 20);
        SLNodeStyleSetWidth(before.as_ptr(), 10.0);
        SLNodeStyleSetHeight(before.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(aligned.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeAlignLeft(aligned.as_ptr(), 20);
        SLNodeStyleSetRelativeAlignBottom(aligned.as_ptr(), 20);
        SLNodeStyleSetWidth(aligned.as_ptr(), 5.0);
        SLNodeStyleSetHeight(aligned.as_ptr(), 7.0);

        SLNodeInsertChild(root.as_ptr(), center_none.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), center_horizontal.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), center_vertical.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), center_both.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), parent_end.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), before.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), aligned.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), anchor.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            80.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicRelativeLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_relative_layout_once_snapshot(
) -> Result<StandalonePublicRelativeLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let nodes = [root, first, second];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Undefined owner constraints use the public standalone sentinel.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_RELATIVE);
        SLNodeStyleSetRelativeLayoutOnce(root.as_ptr(), true);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeId(first.as_ptr(), 1);
        SLNodeStyleSetRelativeBottomOf(first.as_ptr(), 2);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetRelativeId(second.as_ptr(), 2);
        SLNodeStyleSetRelativeRightOf(second.as_ptr(), 1);
        SLNodeStyleSetWidth(second.as_ptr(), 5.0);
        SLNodeStyleSetHeight(second.as_ptr(), 7.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            SL_UNDEFINED,
            SL_NODE_MEASURE_MODE_UNDEFINED,
            SL_UNDEFINED,
            SL_NODE_MEASURE_MODE_UNDEFINED,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicRelativeLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_linear_layout_snapshots(
) -> Result<Vec<StandalonePublicLinearLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = vec![
        standalone_public_linear_gravity_layout_snapshot()?,
        standalone_public_linear_weight_layout_snapshot()?,
    ];
    snapshots.extend(standalone_public_linear_weight_variant_layout_snapshots()?);
    snapshots.extend(standalone_public_linear_orientation_layout_snapshots()?);
    snapshots.extend(standalone_public_linear_main_gravity_layout_snapshots()?);
    snapshots.extend(standalone_public_linear_cross_gravity_layout_snapshots()?);
    snapshots.extend(standalone_public_linear_layout_gravity_variant_layout_snapshots()?);
    Ok(snapshots)
}

fn standalone_public_linear_gravity_layout_snapshot(
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let end_aligned = probe_nodes.create()?;
    let stretched = probe_nodes.create()?;
    let nodes = [root, first, end_aligned, stretched];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), SL_LINEAR_ORIENTATION_HORIZONTAL);
        SLNodeStyleSetLinearGravity(root.as_ptr(), SL_LINEAR_GRAVITY_CENTER);
        SLNodeStyleSetLinearCrossGravity(root.as_ptr(), SL_LINEAR_CROSS_GRAVITY_CENTER);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 40.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);
        SLNodeStyleSetHeight(first.as_ptr(), 8.0);

        SLNodeStyleSetDisplay(end_aligned.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(end_aligned.as_ptr(), 10.0);
        SLNodeStyleSetHeight(end_aligned.as_ptr(), 6.0);
        SLNodeStyleSetLinearLayoutGravity(end_aligned.as_ptr(), SL_LINEAR_LAYOUT_GRAVITY_END);

        SLNodeStyleSetDisplay(stretched.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(stretched.as_ptr(), 10.0);
        SLNodeStyleSetHeight(stretched.as_ptr(), 5.0);
        SLNodeStyleSetLinearLayoutGravity(stretched.as_ptr(), SL_LINEAR_LAYOUT_GRAVITY_STRETCH);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), end_aligned.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), stretched.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            40.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_linear_weight_layout_snapshot(
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let nodes = [root, first, second];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), SL_LINEAR_ORIENTATION_HORIZONTAL);
        SLNodeStyleSetLinearGravity(root.as_ptr(), SL_LINEAR_GRAVITY_END);
        SLNodeStyleSetLinearWeightSum(root.as_ptr(), 4.0);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(first.as_ptr(), 1.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(second.as_ptr(), 1.0);
        SLNodeStyleSetHeight(second.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            20.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_linear_weight_variant_layout_snapshots(
) -> Result<Vec<StandalonePublicLinearLayoutSnapshot>, CppBaselineError> {
    Ok(vec![
        standalone_public_horizontal_linear_weight_ratio_layout_snapshot()?,
        standalone_public_vertical_linear_weight_ratio_layout_snapshot()?,
        standalone_public_vertical_linear_weight_sum_layout_snapshot()?,
        standalone_public_linear_total_weight_below_one_layout_snapshot()?,
    ])
}

fn standalone_public_horizontal_linear_weight_ratio_layout_snapshot(
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let fixed = probe_nodes.create()?;
    let one_share = probe_nodes.create()?;
    let two_shares = probe_nodes.create()?;
    let nodes = [root, fixed, one_share, two_shares];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), SL_LINEAR_ORIENTATION_HORIZONTAL);
        SLNodeStyleSetWidth(root.as_ptr(), 120.0);
        SLNodeStyleSetHeight(root.as_ptr(), 30.0);

        SLNodeStyleSetDisplay(fixed.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(fixed.as_ptr(), 15.0);
        SLNodeStyleSetHeight(fixed.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(one_share.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(one_share.as_ptr(), 1.0);
        SLNodeStyleSetHeight(one_share.as_ptr(), 12.0);

        SLNodeStyleSetDisplay(two_shares.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(two_shares.as_ptr(), 2.0);
        SLNodeStyleSetHeight(two_shares.as_ptr(), 14.0);

        SLNodeInsertChild(root.as_ptr(), fixed.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), one_share.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), two_shares.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            30.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_vertical_linear_weight_ratio_layout_snapshot(
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let fixed = probe_nodes.create()?;
    let one_share = probe_nodes.create()?;
    let two_shares = probe_nodes.create()?;
    let nodes = [root, fixed, one_share, two_shares];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), SL_LINEAR_ORIENTATION_VERTICAL);
        SLNodeStyleSetWidth(root.as_ptr(), 50.0);
        SLNodeStyleSetHeight(root.as_ptr(), 120.0);

        SLNodeStyleSetDisplay(fixed.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(fixed.as_ptr(), 10.0);
        SLNodeStyleSetHeight(fixed.as_ptr(), 15.0);

        SLNodeStyleSetDisplay(one_share.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(one_share.as_ptr(), 1.0);
        SLNodeStyleSetWidth(one_share.as_ptr(), 12.0);

        SLNodeStyleSetDisplay(two_shares.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(two_shares.as_ptr(), 2.0);
        SLNodeStyleSetWidth(two_shares.as_ptr(), 14.0);

        SLNodeInsertChild(root.as_ptr(), fixed.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), one_share.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), two_shares.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            50.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_vertical_linear_weight_sum_layout_snapshot(
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let fixed = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let nodes = [root, fixed, first, second];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), SL_LINEAR_ORIENTATION_VERTICAL);
        SLNodeStyleSetLinearWeightSum(root.as_ptr(), 4.0);
        SLNodeStyleSetWidth(root.as_ptr(), 40.0);
        SLNodeStyleSetHeight(root.as_ptr(), 100.0);

        SLNodeStyleSetDisplay(fixed.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(fixed.as_ptr(), 10.0);
        SLNodeStyleSetHeight(fixed.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(first.as_ptr(), 1.0);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(second.as_ptr(), 1.0);
        SLNodeStyleSetWidth(second.as_ptr(), 12.0);

        SLNodeInsertChild(root.as_ptr(), fixed.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            40.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_linear_total_weight_below_one_layout_snapshot(
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let child = probe_nodes.create()?;
    let nodes = [root, child];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), SL_LINEAR_ORIENTATION_HORIZONTAL);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(child.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearWeight(child.as_ptr(), 0.5);
        SLNodeStyleSetHeight(child.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), child.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            20.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_linear_orientation_layout_snapshots(
) -> Result<Vec<StandalonePublicLinearLayoutSnapshot>, CppBaselineError> {
    [
        SL_LINEAR_ORIENTATION_HORIZONTAL,
        SL_LINEAR_ORIENTATION_HORIZONTAL_REVERSE,
        SL_LINEAR_ORIENTATION_VERTICAL,
        SL_LINEAR_ORIENTATION_VERTICAL_REVERSE,
        SL_LINEAR_ORIENTATION_ROW,
        SL_LINEAR_ORIENTATION_COLUMN,
        SL_LINEAR_ORIENTATION_ROW_REVERSE,
        SL_LINEAR_ORIENTATION_COLUMN_REVERSE,
    ]
    .into_iter()
    .map(standalone_public_linear_orientation_layout_snapshot)
    .collect()
}

fn standalone_public_linear_orientation_layout_snapshot(
    orientation: SLLinearOrientation,
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), orientation);
        SLNodeStyleSetWidth(root.as_ptr(), 90.0);
        SLNodeStyleSetHeight(root.as_ptr(), 70.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);
        SLNodeStyleSetHeight(first.as_ptr(), 12.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 20.0);
        SLNodeStyleSetHeight(second.as_ptr(), 16.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 15.0);
        SLNodeStyleSetHeight(third.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            90.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            70.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_linear_main_gravity_layout_snapshots(
) -> Result<Vec<StandalonePublicLinearLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = Vec::new();
    for gravity in [
        SL_LINEAR_GRAVITY_NONE,
        SL_LINEAR_GRAVITY_TOP,
        SL_LINEAR_GRAVITY_BOTTOM,
        SL_LINEAR_GRAVITY_LEFT,
        SL_LINEAR_GRAVITY_RIGHT,
        SL_LINEAR_GRAVITY_CENTER_VERTICAL,
        SL_LINEAR_GRAVITY_CENTER_HORIZONTAL,
        SL_LINEAR_GRAVITY_SPACE_BETWEEN,
        SL_LINEAR_GRAVITY_START,
        SL_LINEAR_GRAVITY_END,
        SL_LINEAR_GRAVITY_CENTER,
    ] {
        snapshots.push(standalone_public_linear_main_gravity_layout_snapshot(
            SL_LINEAR_ORIENTATION_VERTICAL,
            gravity,
        )?);
        snapshots.push(standalone_public_linear_main_gravity_layout_snapshot(
            SL_LINEAR_ORIENTATION_HORIZONTAL,
            gravity,
        )?);
    }
    Ok(snapshots)
}

fn standalone_public_linear_main_gravity_layout_snapshot(
    orientation: SLLinearOrientation,
    gravity: SLLinearGravity,
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), orientation);
        SLNodeStyleSetLinearGravity(root.as_ptr(), gravity);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 90.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);
        SLNodeStyleSetHeight(first.as_ptr(), 12.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 20.0);
        SLNodeStyleSetHeight(second.as_ptr(), 16.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 15.0);
        SLNodeStyleSetHeight(third.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            90.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_linear_cross_gravity_layout_snapshots(
) -> Result<Vec<StandalonePublicLinearLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = Vec::new();
    for cross_gravity in [
        SL_LINEAR_CROSS_GRAVITY_NONE,
        SL_LINEAR_CROSS_GRAVITY_START,
        SL_LINEAR_CROSS_GRAVITY_END,
        SL_LINEAR_CROSS_GRAVITY_CENTER,
        SL_LINEAR_CROSS_GRAVITY_STRETCH,
    ] {
        snapshots.push(standalone_public_linear_cross_gravity_layout_snapshot(
            SL_LINEAR_ORIENTATION_VERTICAL,
            cross_gravity,
        )?);
        snapshots.push(standalone_public_linear_cross_gravity_layout_snapshot(
            SL_LINEAR_ORIENTATION_HORIZONTAL,
            cross_gravity,
        )?);
    }
    Ok(snapshots)
}

fn standalone_public_linear_cross_gravity_layout_snapshot(
    orientation: SLLinearOrientation,
    cross_gravity: SLLinearCrossGravity,
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let nodes = [root, first, second];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), orientation);
        SLNodeStyleSetLinearCrossGravity(root.as_ptr(), cross_gravity);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 90.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 20.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 30.0);
        SLNodeStyleSetHeight(second.as_ptr(), 12.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            90.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_linear_layout_gravity_variant_layout_snapshots(
) -> Result<Vec<StandalonePublicLinearLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = Vec::new();
    for layout_gravity in [
        SL_LINEAR_LAYOUT_GRAVITY_NONE,
        SL_LINEAR_LAYOUT_GRAVITY_TOP,
        SL_LINEAR_LAYOUT_GRAVITY_BOTTOM,
        SL_LINEAR_LAYOUT_GRAVITY_LEFT,
        SL_LINEAR_LAYOUT_GRAVITY_RIGHT,
        SL_LINEAR_LAYOUT_GRAVITY_CENTER_VERTICAL,
        SL_LINEAR_LAYOUT_GRAVITY_CENTER_HORIZONTAL,
        SL_LINEAR_LAYOUT_GRAVITY_FILL_VERTICAL,
        SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL,
        SL_LINEAR_LAYOUT_GRAVITY_CENTER,
        SL_LINEAR_LAYOUT_GRAVITY_STRETCH,
        SL_LINEAR_LAYOUT_GRAVITY_START,
        SL_LINEAR_LAYOUT_GRAVITY_END,
    ] {
        snapshots.push(
            standalone_public_linear_layout_gravity_variant_layout_snapshot(
                SL_LINEAR_ORIENTATION_VERTICAL,
                layout_gravity,
            )?,
        );
        snapshots.push(
            standalone_public_linear_layout_gravity_variant_layout_snapshot(
                SL_LINEAR_ORIENTATION_HORIZONTAL,
                layout_gravity,
            )?,
        );
    }
    Ok(snapshots)
}

fn standalone_public_linear_layout_gravity_variant_layout_snapshot(
    orientation: SLLinearOrientation,
    layout_gravity: SLLinearLayoutGravity,
) -> Result<StandalonePublicLinearLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let child = probe_nodes.create()?;
    let nodes = [root, child];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), orientation);
        SLNodeStyleSetLinearCrossGravity(root.as_ptr(), SL_LINEAR_CROSS_GRAVITY_START);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 90.0);

        SLNodeStyleSetDisplay(child.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetLinearLayoutGravity(child.as_ptr(), layout_gravity);
        SLNodeStyleSetWidth(child.as_ptr(), 20.0);
        SLNodeStyleSetHeight(child.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), child.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            90.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_linear_list_layout_snapshot(
) -> Result<StandalonePublicLinearListLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first_regular = probe_nodes.create()?;
    let second_regular = probe_nodes.create()?;
    let explicit_default = probe_nodes.create()?;
    let header = probe_nodes.create()?;
    let footer = probe_nodes.create()?;
    let list_row = probe_nodes.create()?;
    let nodes = [
        root,
        first_regular,
        second_regular,
        explicit_default,
        header,
        footer,
        list_row,
    ];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), SL_LINEAR_ORIENTATION_VERTICAL);
        SLNodeStyleSetWidth(root.as_ptr(), 150.0);
        SLNodeStyleSetLinearColumnCount(root.as_ptr(), 3);
        SLNodeStyleSetListCrossAxisGap(root.as_ptr(), 12.0);
        SLNodeStyleSetListMainAxisGap(root.as_ptr(), 4.0);

        SLNodeStyleSetDisplay(first_regular.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetHeight(first_regular.as_ptr(), 10.0);
        SLNodeStyleSetMargin(first_regular.as_ptr(), SL_EDGE_LEFT, 3.0);
        SLNodeStyleSetMargin(first_regular.as_ptr(), SL_EDGE_RIGHT, 5.0);

        SLNodeStyleSetDisplay(second_regular.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidthAuto(second_regular.as_ptr());
        SLNodeStyleSetHeight(second_regular.as_ptr(), 11.0);

        SLNodeStyleSetDisplay(explicit_default.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetHeight(explicit_default.as_ptr(), 12.0);
        SLNodeStyleSetListComponentType(explicit_default.as_ptr(), SL_LIST_COMPONENT_TYPE_DEFAULT);

        SLNodeStyleSetDisplay(header.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetHeight(header.as_ptr(), 7.0);
        SLNodeStyleSetListComponentType(header.as_ptr(), SL_LIST_COMPONENT_TYPE_HEADER);

        SLNodeStyleSetDisplay(footer.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetHeight(footer.as_ptr(), 8.0);
        SLNodeStyleSetListComponentType(footer.as_ptr(), SL_LIST_COMPONENT_TYPE_FOOTER);

        SLNodeStyleSetDisplay(list_row.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetHeight(list_row.as_ptr(), 9.0);
        SLNodeStyleSetListComponentType(list_row.as_ptr(), SL_LIST_COMPONENT_TYPE_LIST_ROW);

        SLNodeInsertChild(root.as_ptr(), first_regular.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second_regular.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), explicit_default.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), header.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), footer.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), list_row.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            150.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_UNDEFINED,
            SL_NODE_MEASURE_MODE_UNDEFINED,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearListLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

#[derive(Clone, Copy)]
enum PublicListGapVariant {
    Points,
    Percent,
    Calc,
    ValueAuto,
    ValueFr,
    ValueMaxContent,
    ValueFitContent,
}

pub fn standalone_public_list_gap_layout_snapshots(
) -> Result<Vec<StandalonePublicLinearListLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = Vec::new();

    for variant in [
        PublicListGapVariant::Points,
        PublicListGapVariant::Percent,
        PublicListGapVariant::Calc,
        PublicListGapVariant::ValueAuto,
        PublicListGapVariant::ValueFr,
        PublicListGapVariant::ValueMaxContent,
        PublicListGapVariant::ValueFitContent,
    ] {
        snapshots.push(standalone_public_list_gap_layout_snapshot(variant)?);
    }

    Ok(snapshots)
}

fn standalone_public_list_gap_layout_snapshot(
    variant: PublicListGapVariant,
) -> Result<StandalonePublicLinearListLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let fourth = probe_nodes.create()?;
    let nodes = [root, first, second, third, fourth];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_LINEAR);
        SLNodeStyleSetLinearOrientation(root.as_ptr(), SL_LINEAR_ORIENTATION_VERTICAL);
        SLNodeStyleSetWidth(root.as_ptr(), 200.0);
        SLNodeStyleSetLinearColumnCount(root.as_ptr(), 2);
        apply_public_list_gap_variant(root, variant);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidthAuto(first.as_ptr());
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidthAuto(second.as_ptr());
        SLNodeStyleSetHeight(second.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidthAuto(third.as_ptr());
        SLNodeStyleSetHeight(third.as_ptr(), 30.0);

        SLNodeStyleSetDisplay(fourth.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidthAuto(fourth.as_ptr());
        SLNodeStyleSetHeight(fourth.as_ptr(), 40.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), fourth.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            200.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_UNDEFINED,
            SL_NODE_MEASURE_MODE_UNDEFINED,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicLinearListLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_flex_layout_snapshots(
) -> Result<Vec<StandalonePublicFlexLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = vec![
        standalone_public_flex_alignment_order_layout_snapshot()?,
        standalone_public_flex_grow_layout_snapshot()?,
        standalone_public_flex_shrink_layout_snapshot()?,
        standalone_public_flex_align_content_layout_snapshot()?,
    ];
    snapshots.extend(standalone_public_flex_wrap_layout_snapshots()?);
    snapshots.extend(standalone_public_flex_align_content_variant_layout_snapshots()?);
    snapshots.extend(standalone_public_flex_direction_layout_snapshots()?);
    snapshots.extend(standalone_public_flex_justify_content_layout_snapshots()?);
    snapshots.extend(standalone_public_flex_align_items_layout_snapshots()?);
    snapshots.push(standalone_public_flex_align_self_layout_snapshot()?);
    snapshots.extend(standalone_public_flex_align_self_variant_layout_snapshots()?);
    snapshots.push(standalone_public_flex_align_items_baseline_layout_snapshot()?);
    snapshots.push(standalone_public_flex_align_self_baseline_layout_snapshot()?);
    Ok(snapshots)
}

#[derive(Clone, Copy)]
struct StandalonePublicFlexBaselineLeafState {
    width: f32,
    height: f32,
    baseline: f32,
}

fn standalone_public_flex_baseline_leaf_delegate(
    state: &mut StandalonePublicFlexBaselineLeafState,
) -> StarlightMeasureDelegate {
    StarlightMeasureDelegate {
        measure_func: Some(standalone_public_flex_baseline_leaf_measure),
        baseline_func: Some(standalone_public_flex_baseline_leaf_baseline),
        manager_node: (state as *mut StandalonePublicFlexBaselineLeafState).cast::<c_void>(),
    }
}

extern "C" fn standalone_public_flex_baseline_leaf_measure(
    manager_node: *mut c_void,
    _width: f32,
    _width_mode: SLNodeMeasureMode,
    _height: f32,
    _height_mode: SLNodeMeasureMode,
) -> StarlightSize {
    if let Some(state) = NonNull::new(manager_node.cast::<StandalonePublicFlexBaselineLeafState>())
    {
        // SAFETY: `manager_node` is created from a live baseline leaf state
        // before layout and remains live until after layout returns.
        let state = unsafe { &*state.as_ptr() };
        StarlightSize {
            width: state.width,
            height: state.height,
        }
    } else {
        StarlightSize {
            width: 0.0,
            height: 0.0,
        }
    }
}

extern "C" fn standalone_public_flex_baseline_leaf_baseline(
    manager_node: *mut c_void,
    _width: f32,
    _height: f32,
) -> f32 {
    if let Some(state) = NonNull::new(manager_node.cast::<StandalonePublicFlexBaselineLeafState>())
    {
        // SAFETY: `manager_node` is created from a live baseline leaf state
        // before layout and remains live until after layout returns.
        unsafe { &*state.as_ptr() }.baseline
    } else {
        0.0
    }
}

fn standalone_public_flex_align_items_baseline_layout_snapshot(
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];
    let mut first_state = StandalonePublicFlexBaselineLeafState {
        width: 30.0,
        height: 20.0,
        baseline: 15.0,
    };
    let mut second_state = StandalonePublicFlexBaselineLeafState {
        width: 20.0,
        height: 10.0,
        baseline: 4.0,
    };
    let mut third_state = StandalonePublicFlexBaselineLeafState {
        width: 25.0,
        height: 16.0,
        baseline: 8.0,
    };
    let mut first_delegate = standalone_public_flex_baseline_leaf_delegate(&mut first_state);
    let mut second_delegate = standalone_public_flex_baseline_leaf_delegate(&mut second_state);
    let mut third_delegate = standalone_public_flex_baseline_leaf_delegate(&mut third_state);

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Delegates and their state outlive the synchronous layout call.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_BASELINE);
        SLNodeStyleSetWidth(root.as_ptr(), 120.0);
        SLNodeStyleSetHeight(root.as_ptr(), 60.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeSetMeasureDelegate(first.as_ptr(), &mut first_delegate);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeSetMeasureDelegate(second.as_ptr(), &mut second_delegate);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeSetMeasureDelegate(third.as_ptr(), &mut third_delegate);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            60.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_flex_align_self_baseline_layout_snapshot(
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];
    let mut first_state = StandalonePublicFlexBaselineLeafState {
        width: 30.0,
        height: 20.0,
        baseline: 15.0,
    };
    let mut second_state = StandalonePublicFlexBaselineLeafState {
        width: 20.0,
        height: 10.0,
        baseline: 4.0,
    };
    let mut third_state = StandalonePublicFlexBaselineLeafState {
        width: 25.0,
        height: 16.0,
        baseline: 8.0,
    };
    let mut first_delegate = standalone_public_flex_baseline_leaf_delegate(&mut first_state);
    let mut second_delegate = standalone_public_flex_baseline_leaf_delegate(&mut second_state);
    let mut third_delegate = standalone_public_flex_baseline_leaf_delegate(&mut third_state);

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Delegates and their state outlive the synchronous layout call.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 120.0);
        SLNodeStyleSetHeight(root.as_ptr(), 60.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetAlignSelf(first.as_ptr(), SL_FLEX_ALIGN_BASELINE);
        SLNodeSetMeasureDelegate(first.as_ptr(), &mut first_delegate);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetAlignSelf(second.as_ptr(), SL_FLEX_ALIGN_BASELINE);
        SLNodeSetMeasureDelegate(second.as_ptr(), &mut second_delegate);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeSetMeasureDelegate(third.as_ptr(), &mut third_delegate);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            60.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

#[derive(Clone, Copy)]
enum PublicFlexAlignContentLayoutVariant {
    FlexStart,
    FlexEnd,
    Center,
    Stretch,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
    Start,
    End,
}

impl PublicFlexAlignContentLayoutVariant {
    const ALL: [Self; 9] = [
        Self::FlexStart,
        Self::FlexEnd,
        Self::Center,
        Self::Stretch,
        Self::SpaceBetween,
        Self::SpaceAround,
        Self::SpaceEvenly,
        Self::Start,
        Self::End,
    ];

    const fn native_align_content(self) -> SLAlignContent {
        match self {
            Self::FlexStart => SL_ALIGN_CONTENT_FLEX_START,
            Self::FlexEnd => SL_ALIGN_CONTENT_FLEX_END,
            Self::Center => SL_ALIGN_CONTENT_CENTER,
            Self::Stretch => SL_ALIGN_CONTENT_STRETCH,
            Self::SpaceBetween => SL_ALIGN_CONTENT_SPACE_BETWEEN,
            Self::SpaceAround => SL_ALIGN_CONTENT_SPACE_AROUND,
            Self::SpaceEvenly => SL_ALIGN_CONTENT_SPACE_EVENLY,
            Self::Start => SL_ALIGN_CONTENT_FLEX_START,
            Self::End => SL_ALIGN_CONTENT_FLEX_END,
        }
    }
}

fn standalone_public_flex_align_content_variant_layout_snapshots(
) -> Result<Vec<StandalonePublicFlexLayoutSnapshot>, CppBaselineError> {
    PublicFlexAlignContentLayoutVariant::ALL
        .into_iter()
        .map(standalone_public_flex_align_content_variant_layout_snapshot)
        .collect()
}

fn standalone_public_flex_align_content_variant_layout_snapshot(
    variant: PublicFlexAlignContentLayoutVariant,
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_WRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignContent(root.as_ptr(), variant.native_align_content());
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 55.0);
        SLNodeStyleSetHeight(root.as_ptr(), 105.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 30.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 30.0);
        SLNodeStyleSetHeight(second.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 30.0);
        SLNodeStyleSetHeight(third.as_ptr(), 15.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            55.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            105.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_flex_align_self_layout_snapshot(
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let inherited = probe_nodes.create()?;
    let self_start = probe_nodes.create()?;
    let self_end = probe_nodes.create()?;
    let self_stretch = probe_nodes.create()?;
    let nodes = [root, inherited, self_start, self_end, self_stretch];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_CENTER);
        SLNodeStyleSetWidth(root.as_ptr(), 120.0);
        SLNodeStyleSetHeight(root.as_ptr(), 60.0);

        SLNodeStyleSetDisplay(inherited.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(inherited.as_ptr(), 10.0);
        SLNodeStyleSetHeight(inherited.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(self_start.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(self_start.as_ptr(), 10.0);
        SLNodeStyleSetHeight(self_start.as_ptr(), 20.0);
        SLNodeStyleSetAlignSelf(self_start.as_ptr(), SL_FLEX_ALIGN_START);

        SLNodeStyleSetDisplay(self_end.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(self_end.as_ptr(), 10.0);
        SLNodeStyleSetHeight(self_end.as_ptr(), 15.0);
        SLNodeStyleSetAlignSelf(self_end.as_ptr(), SL_FLEX_ALIGN_END);

        SLNodeStyleSetDisplay(self_stretch.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(self_stretch.as_ptr(), 10.0);
        SLNodeStyleSetAlignSelf(self_stretch.as_ptr(), SL_FLEX_ALIGN_STRETCH);

        SLNodeInsertChild(root.as_ptr(), inherited.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), self_start.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), self_end.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), self_stretch.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            60.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_flex_align_self_variant_layout_snapshots(
) -> Result<Vec<StandalonePublicFlexLayoutSnapshot>, CppBaselineError> {
    [
        SL_FLEX_ALIGN_AUTO,
        SL_FLEX_ALIGN_STRETCH,
        SL_FLEX_ALIGN_FLEX_START,
        SL_FLEX_ALIGN_FLEX_END,
        SL_FLEX_ALIGN_CENTER,
        SL_FLEX_ALIGN_BASELINE,
        SL_FLEX_ALIGN_START,
        SL_FLEX_ALIGN_END,
    ]
    .into_iter()
    .map(standalone_public_flex_align_self_variant_layout_snapshot)
    .collect()
}

fn standalone_public_flex_align_self_variant_layout_snapshot(
    align_self: SLFlexAlign,
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let inherited = probe_nodes.create()?;
    let nodes = [root, first, inherited];
    let first_has_height = align_self != SL_FLEX_ALIGN_STRETCH;

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_END);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 50.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 12.0);
        if first_has_height {
            SLNodeStyleSetHeight(first.as_ptr(), 8.0);
        }
        SLNodeStyleSetAlignSelf(first.as_ptr(), align_self);

        SLNodeStyleSetDisplay(inherited.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(inherited.as_ptr(), 14.0);
        SLNodeStyleSetHeight(inherited.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), inherited.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            50.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

#[derive(Clone, Copy)]
enum PublicFlexAlignItemsLayoutVariant {
    Stretch,
    FlexStart,
    FlexEnd,
    Center,
    Baseline,
    Start,
    End,
}

impl PublicFlexAlignItemsLayoutVariant {
    const ALL: [Self; 7] = [
        Self::Stretch,
        Self::FlexStart,
        Self::FlexEnd,
        Self::Center,
        Self::Baseline,
        Self::Start,
        Self::End,
    ];

    const fn native_align_items(self) -> SLFlexAlign {
        match self {
            Self::Stretch => SL_FLEX_ALIGN_STRETCH,
            Self::FlexStart => SL_FLEX_ALIGN_FLEX_START,
            Self::FlexEnd => SL_FLEX_ALIGN_FLEX_END,
            Self::Center => SL_FLEX_ALIGN_CENTER,
            Self::Baseline => SL_FLEX_ALIGN_BASELINE,
            Self::Start => SL_FLEX_ALIGN_START,
            Self::End => SL_FLEX_ALIGN_END,
        }
    }

    const fn uses_auto_cross_size(self) -> bool {
        matches!(self, Self::Stretch)
    }
}

fn standalone_public_flex_align_items_layout_snapshots(
) -> Result<Vec<StandalonePublicFlexLayoutSnapshot>, CppBaselineError> {
    PublicFlexAlignItemsLayoutVariant::ALL
        .into_iter()
        .map(standalone_public_flex_align_items_layout_snapshot)
        .collect()
}

fn standalone_public_flex_align_items_layout_snapshot(
    variant: PublicFlexAlignItemsLayoutVariant,
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), variant.native_align_items());
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 60.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 15.0);

        if !variant.uses_auto_cross_size() {
            SLNodeStyleSetHeight(first.as_ptr(), 10.0);
            SLNodeStyleSetHeight(second.as_ptr(), 20.0);
            SLNodeStyleSetHeight(third.as_ptr(), 15.0);
        }

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            60.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

#[derive(Clone, Copy)]
enum PublicFlexJustifyContentLayoutVariant {
    FlexStart,
    Center,
    FlexEnd,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
    Stretch,
    Start,
    End,
}

impl PublicFlexJustifyContentLayoutVariant {
    const ALL: [Self; 9] = [
        Self::FlexStart,
        Self::Center,
        Self::FlexEnd,
        Self::SpaceBetween,
        Self::SpaceAround,
        Self::SpaceEvenly,
        Self::Stretch,
        Self::Start,
        Self::End,
    ];

    const fn native_justify_content(self) -> SLJustifyContent {
        match self {
            Self::FlexStart => SL_JUSTIFY_CONTENT_FLEX_START,
            Self::Center => SL_JUSTIFY_CONTENT_CENTER,
            Self::FlexEnd => SL_JUSTIFY_CONTENT_FLEX_END,
            Self::SpaceBetween => SL_JUSTIFY_CONTENT_SPACE_BETWEEN,
            Self::SpaceAround => SL_JUSTIFY_CONTENT_SPACE_AROUND,
            Self::SpaceEvenly => SL_JUSTIFY_CONTENT_SPACE_EVENLY,
            Self::Stretch => SL_JUSTIFY_CONTENT_STRETCH,
            Self::Start => SL_JUSTIFY_CONTENT_START,
            Self::End => SL_JUSTIFY_CONTENT_END,
        }
    }
}

fn standalone_public_flex_justify_content_layout_snapshots(
) -> Result<Vec<StandalonePublicFlexLayoutSnapshot>, CppBaselineError> {
    PublicFlexJustifyContentLayoutVariant::ALL
        .into_iter()
        .map(standalone_public_flex_justify_content_layout_snapshot)
        .collect()
}

fn standalone_public_flex_justify_content_layout_snapshot(
    variant: PublicFlexJustifyContentLayoutVariant,
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), variant.native_justify_content());
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 105.0);
        SLNodeStyleSetHeight(root.as_ptr(), 30.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 20.0);
        SLNodeStyleSetHeight(second.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 15.0);
        SLNodeStyleSetHeight(third.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            105.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            30.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

#[derive(Clone, Copy)]
enum PublicFlexDirectionLayoutVariant {
    Column,
    Row,
    RowReverse,
    ColumnReverse,
}

impl PublicFlexDirectionLayoutVariant {
    const ALL: [Self; 4] = [
        Self::Column,
        Self::Row,
        Self::RowReverse,
        Self::ColumnReverse,
    ];

    const fn native_direction(self) -> SLFlexDirection {
        match self {
            Self::Column => SL_FLEX_DIRECTION_COLUMN,
            Self::Row => SL_FLEX_DIRECTION_ROW,
            Self::RowReverse => SL_FLEX_DIRECTION_ROW_REVERSE,
            Self::ColumnReverse => SL_FLEX_DIRECTION_COLUMN_REVERSE,
        }
    }
}

fn standalone_public_flex_direction_layout_snapshots(
) -> Result<Vec<StandalonePublicFlexLayoutSnapshot>, CppBaselineError> {
    PublicFlexDirectionLayoutVariant::ALL
        .into_iter()
        .map(standalone_public_flex_direction_layout_snapshot)
        .collect()
}

fn standalone_public_flex_direction_layout_snapshot(
    variant: PublicFlexDirectionLayoutVariant,
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), variant.native_direction());
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 100.0);
        SLNodeStyleSetHeight(root.as_ptr(), 90.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 10.0);
        SLNodeStyleSetHeight(first.as_ptr(), 15.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 20.0);
        SLNodeStyleSetHeight(second.as_ptr(), 25.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 15.0);
        SLNodeStyleSetHeight(third.as_ptr(), 10.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            90.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_flex_alignment_order_layout_snapshot(
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // The public flex setters are applied before synchronous layout.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_SPACE_BETWEEN);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_CENTER);
        SLNodeStyleSetWidth(root.as_ptr(), 180.0);
        SLNodeStyleSetHeight(root.as_ptr(), 60.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlexBasis(first.as_ptr(), 30.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);
        SLNodeStyleSetOrder(first.as_ptr(), 2);
        SLNodeStyleSetAlignSelf(first.as_ptr(), SL_FLEX_ALIGN_FLEX_END);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlexBasisPercent(second.as_ptr(), 25.0);
        SLNodeStyleSetHeight(second.as_ptr(), 20.0);
        SLNodeStyleSetOrder(second.as_ptr(), -1);
        SLNodeStyleSetAlignSelf(second.as_ptr(), SL_FLEX_ALIGN_CENTER);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlexBasisCalc(third.as_ptr(), starlight_value_from_calc_length(20.0, 10.0));
        SLNodeStyleSetHeight(third.as_ptr(), 30.0);
        SLNodeStyleSetOrder(third.as_ptr(), 1);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            180.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            60.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_flex_grow_layout_snapshot(
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let fixed = probe_nodes.create()?;
    let grow = probe_nodes.create()?;
    let shorthand = probe_nodes.create()?;
    let nodes = [root, fixed, grow, shorthand];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters cover public flex grow and shorthand behavior.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 180.0);
        SLNodeStyleSetHeight(root.as_ptr(), 50.0);

        SLNodeStyleSetDisplay(fixed.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlexBasis(fixed.as_ptr(), 30.0);
        SLNodeStyleSetHeight(fixed.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(grow.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlexBasis(grow.as_ptr(), 20.0);
        SLNodeStyleSetFlexGrow(grow.as_ptr(), 1.0);
        SLNodeStyleSetHeight(grow.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(shorthand.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlex(shorthand.as_ptr(), 2.0);
        SLNodeStyleSetHeight(shorthand.as_ptr(), 30.0);

        SLNodeInsertChild(root.as_ptr(), fixed.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), grow.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), shorthand.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            180.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            50.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_flex_shrink_layout_snapshot(
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let inflexible = probe_nodes.create()?;
    let nodes = [root, first, second, inflexible];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters cover public flex shrink plus calc/percent basis resolution.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_NOWRAP);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 90.0);
        SLNodeStyleSetHeight(root.as_ptr(), 50.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlexBasis(first.as_ptr(), 60.0);
        SLNodeStyleSetFlexShrink(first.as_ptr(), 1.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlexBasisCalc(
            second.as_ptr(),
            starlight_value_from_calc_length(40.0, 20.0),
        );
        SLNodeStyleSetFlexShrink(second.as_ptr(), 2.0);
        SLNodeStyleSetHeight(second.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(inflexible.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetFlexBasisPercent(inflexible.as_ptr(), 30.0);
        SLNodeStyleSetFlexShrink(inflexible.as_ptr(), 0.0);
        SLNodeStyleSetHeight(inflexible.as_ptr(), 30.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), inflexible.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            90.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            50.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_flex_wrap_layout_snapshots(
) -> Result<Vec<StandalonePublicFlexLayoutSnapshot>, CppBaselineError> {
    [
        SL_FLEX_WRAP_WRAP,
        SL_FLEX_WRAP_NOWRAP,
        SL_FLEX_WRAP_WRAP_REVERSE,
    ]
    .into_iter()
    .map(standalone_public_flex_wrap_layout_snapshot)
    .collect()
}

fn standalone_public_flex_wrap_layout_snapshot(
    flex_wrap: SLFlexWrap,
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), flex_wrap);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignContent(root.as_ptr(), SL_ALIGN_CONTENT_FLEX_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 55.0);
        SLNodeStyleSetHeight(root.as_ptr(), 80.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_ROW, 5.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 30.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 30.0);
        SLNodeStyleSetHeight(second.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 30.0);
        SLNodeStyleSetHeight(third.as_ptr(), 15.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            55.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            80.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

fn standalone_public_flex_align_content_layout_snapshot(
) -> Result<StandalonePublicFlexLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let nodes = [root, first, second, third];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Setters use public standalone values and layout is synchronous.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_WRAP);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignContent(root.as_ptr(), SL_ALIGN_CONTENT_SPACE_BETWEEN);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 55.0);
        SLNodeStyleSetHeight(root.as_ptr(), 95.0);

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 30.0);
        SLNodeStyleSetHeight(first.as_ptr(), 10.0);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 30.0);
        SLNodeStyleSetHeight(second.as_ptr(), 20.0);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 30.0);
        SLNodeStyleSetHeight(third.as_ptr(), 15.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            55.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            95.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_edge_layout_snapshots(
) -> Result<Vec<StandalonePublicEdgeLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = Vec::new();

    for variant in [
        PublicEdgeStyleVariant::Points,
        PublicEdgeStyleVariant::Percent,
        PublicEdgeStyleVariant::Calc,
        PublicEdgeStyleVariant::ValueFr,
        PublicEdgeStyleVariant::ValueMaxContent,
        PublicEdgeStyleVariant::ValueFitContent,
        PublicEdgeStyleVariant::Auto,
    ] {
        snapshots.push(standalone_public_edge_layout_snapshot(variant)?);
    }

    Ok(snapshots)
}

fn standalone_public_edge_layout_snapshot(
    variant: PublicEdgeStyleVariant,
) -> Result<StandalonePublicEdgeLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let primary = probe_nodes.create()?;
    let secondary = probe_nodes.create()?;
    let trailing = probe_nodes.create()?;
    let nodes = [root, primary, secondary, trailing];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Public edge/gap setters are applied before synchronous layout.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_FLEX);
        SLNodeStyleSetFlexDirection(root.as_ptr(), SL_FLEX_DIRECTION_ROW);
        SLNodeStyleSetFlexWrap(root.as_ptr(), SL_FLEX_WRAP_WRAP);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetWidth(root.as_ptr(), 140.0);
        SLNodeStyleSetHeight(root.as_ptr(), 120.0);
        SLNodeStyleSetPadding(root.as_ptr(), SL_EDGE_LEFT, 5.0);
        SLNodeStyleSetBorder(root.as_ptr(), SL_EDGE_ALL, 1.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_ROW, 9.0);
        apply_public_edge_layout_container_variant(root, variant);

        SLNodeStyleSetDisplay(primary.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetPositionType(primary.as_ptr(), SL_POSITION_TYPE_RELATIVE);
        SLNodeStyleSetWidth(primary.as_ptr(), 50.0);
        SLNodeStyleSetHeight(primary.as_ptr(), 20.0);
        apply_public_edge_style_variant(primary, variant);
        normalize_public_edge_layout_child_variant(primary, variant);

        SLNodeStyleSetDisplay(secondary.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(secondary.as_ptr(), 50.0);
        SLNodeStyleSetHeight(secondary.as_ptr(), 25.0);

        SLNodeStyleSetDisplay(trailing.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(trailing.as_ptr(), 50.0);
        SLNodeStyleSetHeight(trailing.as_ptr(), 30.0);

        SLNodeInsertChild(root.as_ptr(), primary.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), secondary.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), trailing.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            140.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicEdgeLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_grid_track_layout_snapshot(
) -> Result<StandalonePublicGridTrackLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let explicit_intrinsic = probe_nodes.create()?;
    let explicit_following = probe_nodes.create()?;
    let implicit_intrinsic = probe_nodes.create()?;
    let implicit_following = probe_nodes.create()?;
    let nodes = [
        root,
        explicit_intrinsic,
        explicit_following,
        implicit_intrinsic,
        implicit_following,
    ];

    let template_columns = [Length::points(20.0), Length::points(10.0)];
    let template_column_max = [
        Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        Length::points(10.0),
    ];
    let template_rows = [Length::points(20.0), Length::points(10.0)];
    let template_row_max = [
        Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        Length::points(10.0),
    ];
    let auto_columns = [Length::points(20.0), Length::points(10.0)];
    let auto_column_max = [
        Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        Length::points(50.0),
    ];
    let auto_rows = [Length::points(20.0), Length::points(10.0)];
    let auto_row_max = [
        Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        Length::points(50.0),
    ];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Track vectors stay alive for each synchronous public setter call.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_GRID);
        SLNodeStyleSetWidth(root.as_ptr(), 120.0);
        SLNodeStyleSetHeight(root.as_ptr(), 120.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_COLUMN, 3.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_ROW, 2.0);
        SLNodeStyleSetJustifyItems(root.as_ptr(), SL_JUSTIFY_ITEM_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        SLNodeStyleSetGridAutoFlow(root.as_ptr(), SL_GRID_AUTO_FLOW_COLUMN_DENSE);
        set_grid_track_vector(
            root.as_ptr(),
            &template_columns,
            SLNodeStyleSetGridTemplateColumns,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_column_max,
            SLNodeStyleSetGridTemplateColumnsMax,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_rows,
            SLNodeStyleSetGridTemplateRows,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_row_max,
            SLNodeStyleSetGridTemplateRowsMax,
        )?;
        set_grid_track_vector(root.as_ptr(), &auto_columns, SLNodeStyleSetGridAutoColumns)?;
        set_grid_track_vector(
            root.as_ptr(),
            &auto_column_max,
            SLNodeStyleSetGridAutoColumnsMax,
        )?;
        set_grid_track_vector(root.as_ptr(), &auto_rows, SLNodeStyleSetGridAutoRows)?;
        set_grid_track_vector(root.as_ptr(), &auto_row_max, SLNodeStyleSetGridAutoRowsMax)?;

        SLNodeStyleSetDisplay(explicit_intrinsic.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(explicit_intrinsic.as_ptr(), 90.0);
        SLNodeStyleSetHeight(explicit_intrinsic.as_ptr(), 90.0);
        SLNodeStyleSetGridColumnStart(explicit_intrinsic.as_ptr(), 1);
        SLNodeStyleSetGridRowStart(explicit_intrinsic.as_ptr(), 1);

        SLNodeStyleSetDisplay(explicit_following.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(explicit_following.as_ptr(), 8.0);
        SLNodeStyleSetHeight(explicit_following.as_ptr(), 8.0);
        SLNodeStyleSetGridColumnStart(explicit_following.as_ptr(), 2);
        SLNodeStyleSetGridRowStart(explicit_following.as_ptr(), 2);

        SLNodeStyleSetDisplay(implicit_intrinsic.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(implicit_intrinsic.as_ptr(), 90.0);
        SLNodeStyleSetHeight(implicit_intrinsic.as_ptr(), 90.0);
        SLNodeStyleSetGridColumnStart(implicit_intrinsic.as_ptr(), 3);
        SLNodeStyleSetGridRowStart(implicit_intrinsic.as_ptr(), 3);

        SLNodeStyleSetDisplay(implicit_following.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(implicit_following.as_ptr(), 8.0);
        SLNodeStyleSetHeight(implicit_following.as_ptr(), 8.0);
        SLNodeStyleSetGridColumnStart(implicit_following.as_ptr(), 4);
        SLNodeStyleSetGridRowStart(implicit_following.as_ptr(), 4);

        SLNodeInsertChild(root.as_ptr(), explicit_intrinsic.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), explicit_following.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), implicit_intrinsic.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), implicit_following.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicGridTrackLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_grid_auto_flow_layout_snapshots(
) -> Result<Vec<StandalonePublicGridTrackLayoutSnapshot>, CppBaselineError> {
    [
        SL_GRID_AUTO_FLOW_ROW,
        SL_GRID_AUTO_FLOW_COLUMN,
        SL_GRID_AUTO_FLOW_DENSE,
        SL_GRID_AUTO_FLOW_ROW_DENSE,
        SL_GRID_AUTO_FLOW_COLUMN_DENSE,
    ]
    .into_iter()
    .map(standalone_public_grid_auto_flow_layout_snapshot)
    .collect()
}

fn standalone_public_grid_auto_flow_layout_snapshot(
    auto_flow: SLGridAutoFlow,
) -> Result<StandalonePublicGridTrackLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let third = probe_nodes.create()?;
    let fourth = probe_nodes.create()?;
    let fifth = probe_nodes.create()?;
    let nodes = [root, first, second, third, fourth, fifth];

    let fixed_tracks = [
        Length::points(10.0),
        Length::points(10.0),
        Length::points(10.0),
    ];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Track vectors stay alive for each synchronous public setter call.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_GRID);
        SLNodeStyleSetGridAutoFlow(root.as_ptr(), auto_flow);
        SLNodeStyleSetWidth(root.as_ptr(), 42.0);
        SLNodeStyleSetHeight(root.as_ptr(), 40.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_COLUMN, 2.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_ROW, 1.0);
        SLNodeStyleSetJustifyItems(root.as_ptr(), SL_JUSTIFY_ITEM_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        set_grid_track_vector(
            root.as_ptr(),
            &fixed_tracks,
            SLNodeStyleSetGridTemplateColumns,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &fixed_tracks,
            SLNodeStyleSetGridTemplateColumnsMax,
        )?;
        set_grid_track_vector(root.as_ptr(), &fixed_tracks, SLNodeStyleSetGridTemplateRows)?;
        set_grid_track_vector(
            root.as_ptr(),
            &fixed_tracks,
            SLNodeStyleSetGridTemplateRowsMax,
        )?;
        set_grid_track_vector(root.as_ptr(), &fixed_tracks, SLNodeStyleSetGridAutoColumns)?;
        set_grid_track_vector(
            root.as_ptr(),
            &fixed_tracks,
            SLNodeStyleSetGridAutoColumnsMax,
        )?;
        set_grid_track_vector(root.as_ptr(), &fixed_tracks, SLNodeStyleSetGridAutoRows)?;
        set_grid_track_vector(root.as_ptr(), &fixed_tracks, SLNodeStyleSetGridAutoRowsMax)?;

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(first.as_ptr(), 6.0);
        SLNodeStyleSetHeight(first.as_ptr(), 6.0);
        SLNodeStyleSetGridColumnSpan(first.as_ptr(), 2);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(second.as_ptr(), 7.0);
        SLNodeStyleSetHeight(second.as_ptr(), 7.0);
        SLNodeStyleSetGridRowSpan(second.as_ptr(), 2);

        SLNodeStyleSetDisplay(third.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(third.as_ptr(), 5.0);
        SLNodeStyleSetHeight(third.as_ptr(), 5.0);
        SLNodeStyleSetGridColumnSpan(third.as_ptr(), 2);

        SLNodeStyleSetDisplay(fourth.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(fourth.as_ptr(), 4.0);
        SLNodeStyleSetHeight(fourth.as_ptr(), 8.0);
        SLNodeStyleSetGridRowSpan(fourth.as_ptr(), 2);

        SLNodeStyleSetDisplay(fifth.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(fifth.as_ptr(), 3.0);
        SLNodeStyleSetHeight(fifth.as_ptr(), 3.0);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), third.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), fourth.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), fifth.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            42.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            40.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicGridTrackLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_grid_alignment_layout_snapshot(
) -> Result<StandalonePublicGridAlignmentLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let inherited = probe_nodes.create()?;
    let self_end = probe_nodes.create()?;
    let self_stretch = probe_nodes.create()?;
    let nodes = [root, inherited, self_end, self_stretch];
    let template_columns = [Length::points(20.0), Length::points(20.0)];
    let template_rows = [Length::points(20.0), Length::points(20.0)];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Track vectors stay alive for each synchronous public setter call.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_GRID);
        SLNodeStyleSetWidth(root.as_ptr(), 120.0);
        SLNodeStyleSetHeight(root.as_ptr(), 100.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_COLUMN, 2.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_ROW, 4.0);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_CENTER);
        SLNodeStyleSetAlignContent(root.as_ptr(), SL_ALIGN_CONTENT_CENTER);
        SLNodeStyleSetJustifyItems(root.as_ptr(), SL_JUSTIFY_ITEM_CENTER);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_CENTER);
        set_grid_track_vector(
            root.as_ptr(),
            &template_columns,
            SLNodeStyleSetGridTemplateColumns,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_columns,
            SLNodeStyleSetGridTemplateColumnsMax,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_rows,
            SLNodeStyleSetGridTemplateRows,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_rows,
            SLNodeStyleSetGridTemplateRowsMax,
        )?;

        SLNodeStyleSetDisplay(inherited.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(inherited.as_ptr(), 10.0);
        SLNodeStyleSetHeight(inherited.as_ptr(), 8.0);
        SLNodeStyleSetGridColumnStart(inherited.as_ptr(), 1);
        SLNodeStyleSetGridRowStart(inherited.as_ptr(), 1);

        SLNodeStyleSetDisplay(self_end.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetWidth(self_end.as_ptr(), 8.0);
        SLNodeStyleSetHeight(self_end.as_ptr(), 6.0);
        SLNodeStyleSetJustifySelf(self_end.as_ptr(), SL_JUSTIFY_ITEM_END);
        SLNodeStyleSetAlignSelf(self_end.as_ptr(), SL_FLEX_ALIGN_FLEX_END);
        SLNodeStyleSetGridColumnStart(self_end.as_ptr(), 2);
        SLNodeStyleSetGridRowStart(self_end.as_ptr(), 1);

        SLNodeStyleSetDisplay(self_stretch.as_ptr(), SL_DISPLAY_BLOCK);
        SLNodeStyleSetJustifySelf(self_stretch.as_ptr(), SL_JUSTIFY_ITEM_STRETCH);
        SLNodeStyleSetAlignSelf(self_stretch.as_ptr(), SL_FLEX_ALIGN_STRETCH);
        SLNodeStyleSetGridColumnStart(self_stretch.as_ptr(), 1);
        SLNodeStyleSetGridRowStart(self_stretch.as_ptr(), 2);

        SLNodeInsertChild(root.as_ptr(), inherited.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), self_end.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), self_stretch.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicGridAlignmentLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_grid_alignment_variant_layout_snapshots(
) -> Result<Vec<StandalonePublicGridAlignmentLayoutSnapshot>, CppBaselineError> {
    let mut snapshots = Vec::new();
    for value in [
        SL_JUSTIFY_CONTENT_FLEX_START,
        SL_JUSTIFY_CONTENT_CENTER,
        SL_JUSTIFY_CONTENT_FLEX_END,
        SL_JUSTIFY_CONTENT_SPACE_BETWEEN,
        SL_JUSTIFY_CONTENT_SPACE_AROUND,
        SL_JUSTIFY_CONTENT_SPACE_EVENLY,
        SL_JUSTIFY_CONTENT_STRETCH,
        SL_JUSTIFY_CONTENT_START,
        SL_JUSTIFY_CONTENT_END,
    ] {
        snapshots.push(standalone_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::JustifyContent(value),
        )?);
    }
    for value in [
        SL_ALIGN_CONTENT_FLEX_START,
        SL_ALIGN_CONTENT_CENTER,
        SL_ALIGN_CONTENT_FLEX_END,
        SL_ALIGN_CONTENT_STRETCH,
        SL_ALIGN_CONTENT_SPACE_BETWEEN,
        SL_ALIGN_CONTENT_SPACE_AROUND,
        SL_ALIGN_CONTENT_SPACE_EVENLY,
    ] {
        snapshots.push(standalone_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::AlignContent(value),
        )?);
    }
    for value in [
        SL_JUSTIFY_ITEM_AUTO,
        SL_JUSTIFY_ITEM_STRETCH,
        SL_JUSTIFY_ITEM_START,
        SL_JUSTIFY_ITEM_END,
        SL_JUSTIFY_ITEM_CENTER,
    ] {
        snapshots.push(standalone_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::JustifyItems(value),
        )?);
    }
    for value in [
        SL_FLEX_ALIGN_STRETCH,
        SL_FLEX_ALIGN_FLEX_START,
        SL_FLEX_ALIGN_FLEX_END,
        SL_FLEX_ALIGN_CENTER,
        SL_FLEX_ALIGN_BASELINE,
        SL_FLEX_ALIGN_START,
        SL_FLEX_ALIGN_END,
    ] {
        snapshots.push(standalone_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::AlignItems(value),
        )?);
    }
    for value in [
        SL_JUSTIFY_ITEM_AUTO,
        SL_JUSTIFY_ITEM_STRETCH,
        SL_JUSTIFY_ITEM_START,
        SL_JUSTIFY_ITEM_END,
        SL_JUSTIFY_ITEM_CENTER,
    ] {
        snapshots.push(standalone_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::JustifySelf(value),
        )?);
    }
    for value in [
        SL_FLEX_ALIGN_AUTO,
        SL_FLEX_ALIGN_STRETCH,
        SL_FLEX_ALIGN_FLEX_START,
        SL_FLEX_ALIGN_FLEX_END,
        SL_FLEX_ALIGN_CENTER,
        SL_FLEX_ALIGN_BASELINE,
        SL_FLEX_ALIGN_START,
        SL_FLEX_ALIGN_END,
    ] {
        snapshots.push(standalone_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::AlignSelf(value),
        )?);
    }
    Ok(snapshots)
}

#[derive(Clone, Copy)]
enum PublicGridAlignmentVariant {
    JustifyContent(SLJustifyContent),
    AlignContent(SLAlignContent),
    JustifyItems(SLJustifyItem),
    AlignItems(SLFlexAlign),
    JustifySelf(SLJustifyItem),
    AlignSelf(SLFlexAlign),
}

fn standalone_public_grid_alignment_variant_layout_snapshot(
    variant: PublicGridAlignmentVariant,
) -> Result<StandalonePublicGridAlignmentLayoutSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let root = probe_nodes.create()?;
    let first = probe_nodes.create()?;
    let second = probe_nodes.create()?;
    let nodes = [root, first, second];
    let template_columns = [Length::points(30.0), Length::points(30.0)];
    let template_rows = [Length::points(24.0), Length::points(24.0)];

    // SAFETY: all nodes are live standalone nodes owned by `probe_nodes`.
    // Track vectors stay alive for each synchronous public setter call.
    unsafe {
        SLNodeStyleSetDisplay(root.as_ptr(), SL_DISPLAY_GRID);
        SLNodeStyleSetWidth(root.as_ptr(), 120.0);
        SLNodeStyleSetHeight(root.as_ptr(), 100.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_COLUMN, 2.0);
        SLNodeStyleSetGap(root.as_ptr(), SL_GAP_ROW, 4.0);
        SLNodeStyleSetJustifyContent(root.as_ptr(), SL_JUSTIFY_CONTENT_FLEX_START);
        SLNodeStyleSetAlignContent(root.as_ptr(), SL_ALIGN_CONTENT_FLEX_START);
        SLNodeStyleSetJustifyItems(root.as_ptr(), SL_JUSTIFY_ITEM_START);
        SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_START);
        set_grid_track_vector(
            root.as_ptr(),
            &template_columns,
            SLNodeStyleSetGridTemplateColumns,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_columns,
            SLNodeStyleSetGridTemplateColumnsMax,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_rows,
            SLNodeStyleSetGridTemplateRows,
        )?;
        set_grid_track_vector(
            root.as_ptr(),
            &template_rows,
            SLNodeStyleSetGridTemplateRowsMax,
        )?;

        let mut first_has_width = true;
        let mut second_has_width = true;
        let mut first_has_height = true;
        let mut second_has_height = true;

        match variant {
            PublicGridAlignmentVariant::JustifyContent(value) => {
                SLNodeStyleSetJustifyContent(root.as_ptr(), value);
            }
            PublicGridAlignmentVariant::AlignContent(value) => {
                SLNodeStyleSetAlignContent(root.as_ptr(), value);
            }
            PublicGridAlignmentVariant::JustifyItems(value) => {
                SLNodeStyleSetJustifyItems(root.as_ptr(), value);
                first_has_width = value != SL_JUSTIFY_ITEM_AUTO && value != SL_JUSTIFY_ITEM_STRETCH;
                second_has_width = first_has_width;
            }
            PublicGridAlignmentVariant::AlignItems(value) => {
                SLNodeStyleSetAlignItems(root.as_ptr(), value);
                first_has_height = value != SL_FLEX_ALIGN_STRETCH;
                second_has_height = first_has_height;
            }
            PublicGridAlignmentVariant::JustifySelf(value) => {
                SLNodeStyleSetJustifyItems(root.as_ptr(), SL_JUSTIFY_ITEM_END);
                SLNodeStyleSetJustifySelf(first.as_ptr(), value);
                first_has_width = value != SL_JUSTIFY_ITEM_STRETCH;
            }
            PublicGridAlignmentVariant::AlignSelf(value) => {
                SLNodeStyleSetAlignItems(root.as_ptr(), SL_FLEX_ALIGN_FLEX_END);
                SLNodeStyleSetAlignSelf(first.as_ptr(), value);
                first_has_height = value != SL_FLEX_ALIGN_STRETCH;
            }
        }

        SLNodeStyleSetDisplay(first.as_ptr(), SL_DISPLAY_BLOCK);
        if first_has_width {
            SLNodeStyleSetWidth(first.as_ptr(), 10.0);
        }
        if first_has_height {
            SLNodeStyleSetHeight(first.as_ptr(), 8.0);
        }
        SLNodeStyleSetGridColumnStart(first.as_ptr(), 1);
        SLNodeStyleSetGridRowStart(first.as_ptr(), 1);

        SLNodeStyleSetDisplay(second.as_ptr(), SL_DISPLAY_BLOCK);
        if second_has_width {
            SLNodeStyleSetWidth(second.as_ptr(), 12.0);
        }
        if second_has_height {
            SLNodeStyleSetHeight(second.as_ptr(), 9.0);
        }
        SLNodeStyleSetGridColumnStart(second.as_ptr(), 2);
        SLNodeStyleSetGridRowStart(second.as_ptr(), 2);

        SLNodeInsertChild(root.as_ptr(), first.as_ptr(), -1);
        SLNodeInsertChild(root.as_ptr(), second.as_ptr(), -1);

        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            120.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }

    Ok(StandalonePublicGridAlignmentLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(*node))
            .collect(),
    })
}

pub fn standalone_public_layout_entrypoint_snapshot(
) -> Result<StandalonePublicLayoutEntrypointSnapshot, CppBaselineError> {
    Ok(StandalonePublicLayoutEntrypointSnapshot {
        finite_owner: standalone_public_layout_entrypoint_stage(|node| {
            // SAFETY: `node` is a live standalone node and this probes the
            // public finite-owner entry point.
            unsafe {
                SLNodeCalculateLayout(node.as_ptr(), 50.0, 40.0, SL_DIRECTION_LTR);
            }
        })?,
        sentinel_undefined_owner: standalone_public_layout_entrypoint_stage(|node| {
            // SAFETY: `node` is a live standalone node and `SL_UNDEFINED` is
            // the public sentinel that makes `SLNodeCalculateLayout` use
            // undefined owner constraints.
            unsafe {
                SLNodeCalculateLayout(node.as_ptr(), SL_UNDEFINED, SL_UNDEFINED, SL_DIRECTION_LTR);
            }
        })?,
        at_most_owner: standalone_public_layout_entrypoint_stage(|node| {
            // SAFETY: `node` is a live standalone node and the owner measure
            // modes are public standalone enum values.
            unsafe {
                SLNodeCalculateLayoutWithMode(
                    node.as_ptr(),
                    50.0,
                    SL_NODE_MEASURE_MODE_AT_MOST,
                    40.0,
                    SL_NODE_MEASURE_MODE_AT_MOST,
                    SL_DIRECTION_LTR,
                );
            }
        })?,
        undefined_owner: standalone_public_layout_entrypoint_stage(|node| {
            // SAFETY: `node` is a live standalone node. The undefined mode uses
            // the public standalone undefined size sentinel.
            unsafe {
                SLNodeCalculateLayoutWithMode(
                    node.as_ptr(),
                    SL_UNDEFINED,
                    SL_NODE_MEASURE_MODE_UNDEFINED,
                    SL_UNDEFINED,
                    SL_NODE_MEASURE_MODE_UNDEFINED,
                    SL_DIRECTION_LTR,
                );
            }
        })?,
    })
}

pub fn standalone_public_config_snapshot(
) -> Result<StandalonePublicConfigSnapshot, CppBaselineError> {
    let mut config = ProbeConfig::new()?;
    let default_config_physical_pixels_per_layout_unit = config.physical_pixels_per_layout_unit();
    config.set_physical_pixels_per_layout_unit(2.0);
    let updated_config_physical_pixels_per_layout_unit = config.physical_pixels_per_layout_unit();

    let mut probe_nodes = ProbeNodes::new();
    let default_node = probe_nodes.create()?;
    let configured_node = probe_nodes.create_with_config(2.0)?;
    let mut default_delegate = StarlightMeasureDelegate {
        measure_func: Some(standalone_public_config_fractional_measure),
        baseline_func: None,
        manager_node: std::ptr::null_mut(),
    };
    let mut configured_delegate = StarlightMeasureDelegate {
        measure_func: Some(standalone_public_config_fractional_measure),
        baseline_func: None,
        manager_node: std::ptr::null_mut(),
    };

    // SAFETY: nodes are live and delegates outlive the synchronous layout calls.
    unsafe {
        SLNodeSetMeasureDelegate(default_node.as_ptr(), &mut default_delegate);
        SLNodeSetMeasureDelegate(configured_node.as_ptr(), &mut configured_delegate);
        SLNodeCalculateLayoutWithMode(
            default_node.as_ptr(),
            SL_UNDEFINED,
            SL_NODE_MEASURE_MODE_UNDEFINED,
            SL_UNDEFINED,
            SL_NODE_MEASURE_MODE_UNDEFINED,
            SL_DIRECTION_LTR,
        );
        SLNodeCalculateLayoutWithMode(
            configured_node.as_ptr(),
            SL_UNDEFINED,
            SL_NODE_MEASURE_MODE_UNDEFINED,
            SL_UNDEFINED,
            SL_NODE_MEASURE_MODE_UNDEFINED,
            SL_DIRECTION_LTR,
        );
    }

    // SAFETY: nodes are live after layout.
    let (default_node_width, default_node_height, configured_node_width, configured_node_height) = unsafe {
        (
            SLNodeLayoutGetWidth(default_node.as_ptr()),
            SLNodeLayoutGetHeight(default_node.as_ptr()),
            SLNodeLayoutGetWidth(configured_node.as_ptr()),
            SLNodeLayoutGetHeight(configured_node.as_ptr()),
        )
    };

    Ok(StandalonePublicConfigSnapshot {
        default_config_physical_pixels_per_layout_unit,
        updated_config_physical_pixels_per_layout_unit,
        default_node_width,
        default_node_height,
        configured_node_width,
        configured_node_height,
    })
}

pub fn standalone_public_measure_delegate_snapshot(
) -> Result<StandalonePublicMeasureDelegateSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let node = probe_nodes.create()?;
    let mut state = StandalonePublicMeasureDelegateState::default();
    let mut delegate = StarlightMeasureDelegate {
        measure_func: Some(standalone_public_measure_delegate_measure),
        baseline_func: Some(standalone_public_measure_delegate_baseline),
        manager_node: (&mut state as *mut StandalonePublicMeasureDelegateState).cast::<c_void>(),
    };

    // SAFETY: `node` is live and owned by `probe_nodes`.
    let initial_delegate_is_null = unsafe { SLNodeGetMeasureDelegate(node.as_ptr()).is_null() };
    // SAFETY: `node` is live and owned by `probe_nodes`.
    let initial_has_measure_func = unsafe { SLNodeHasMeasureFunc(node.as_ptr()) };

    // SAFETY: `delegate` and `state` outlive the synchronous layout call.
    unsafe {
        SLNodeSetMeasureDelegate(node.as_ptr(), &mut delegate);
    }
    // SAFETY: `node` is live and the delegate has just been installed.
    let delegate_round_trips = std::ptr::eq(
        unsafe { SLNodeGetMeasureDelegate(node.as_ptr()) },
        &delegate,
    );
    // SAFETY: `node` is live and owned by `probe_nodes`.
    let after_set_has_measure_func = unsafe { SLNodeHasMeasureFunc(node.as_ptr()) };

    // SAFETY: `node` is live; the delegate and manager state are still alive.
    unsafe {
        SLNodeCalculateLayoutWithMode(
            node.as_ptr(),
            50.0,
            SL_NODE_MEASURE_MODE_AT_MOST,
            40.0,
            SL_NODE_MEASURE_MODE_AT_MOST,
            SL_DIRECTION_LTR,
        );
    }

    // SAFETY: `node` is live after layout.
    let (layout_width, layout_height, layout_baseline) = unsafe {
        (
            SLNodeLayoutGetWidth(node.as_ptr()),
            SLNodeLayoutGetHeight(node.as_ptr()),
            SLNodeLayoutGetBaseline(node.as_ptr()),
        )
    };

    // SAFETY: `node` is live and clearing a measure delegate accepts null.
    unsafe {
        SLNodeSetMeasureDelegate(node.as_ptr(), std::ptr::null_mut());
    }
    // SAFETY: `node` is live and owned by `probe_nodes`.
    let after_clear_delegate_is_null = unsafe { SLNodeGetMeasureDelegate(node.as_ptr()).is_null() };
    // SAFETY: `node` is live and owned by `probe_nodes`.
    let after_clear_has_measure_func = unsafe { SLNodeHasMeasureFunc(node.as_ptr()) };

    Ok(StandalonePublicMeasureDelegateSnapshot {
        initial_delegate_is_null,
        initial_has_measure_func,
        delegate_round_trips,
        after_set_has_measure_func,
        measure_call_count: state.measure_call_count,
        baseline_call_count: state.baseline_call_count,
        measure_width: state.measure_width,
        measure_width_mode: state.measure_width_mode,
        measure_height: state.measure_height,
        measure_height_mode: state.measure_height_mode,
        baseline_width: state.baseline_width,
        baseline_height: state.baseline_height,
        layout_width,
        layout_height,
        layout_baseline,
        after_clear_delegate_is_null,
        after_clear_has_measure_func,
    })
}

fn standalone_public_layout_entrypoint_stage(
    layout: impl FnOnce(NonNull<StarlightNode>),
) -> Result<StandalonePublicLayoutEntrypointStageSnapshot, CppBaselineError> {
    let mut probe_nodes = ProbeNodes::new();
    let node = probe_nodes.create()?;
    let mut state = StandalonePublicLayoutEntrypointState::default();
    let mut delegate = StarlightMeasureDelegate {
        measure_func: Some(standalone_public_layout_entrypoint_measure),
        baseline_func: None,
        manager_node: (&mut state as *mut StandalonePublicLayoutEntrypointState).cast::<c_void>(),
    };

    // SAFETY: `node` is live and `delegate`/`state` outlive the synchronous
    // layout call below.
    unsafe {
        SLNodeSetMeasureDelegate(node.as_ptr(), &mut delegate);
    }
    layout(node);

    // SAFETY: `node` is live after layout.
    let (layout_width, layout_height) = unsafe {
        (
            SLNodeLayoutGetWidth(node.as_ptr()),
            SLNodeLayoutGetHeight(node.as_ptr()),
        )
    };

    Ok(StandalonePublicLayoutEntrypointStageSnapshot {
        measure_call_count: state.measure_call_count,
        measure_width: state.measure_width,
        measure_width_mode: state.measure_width_mode,
        measure_height: state.measure_height,
        measure_height_mode: state.measure_height_mode,
        layout_width,
        layout_height,
    })
}

extern "C" fn standalone_public_config_fractional_measure(
    _manager_node: *mut c_void,
    _width: f32,
    _width_mode: SLNodeMeasureMode,
    _height: f32,
    _height_mode: SLNodeMeasureMode,
) -> StarlightSize {
    StarlightSize {
        width: 10.2,
        height: 4.2,
    }
}

#[derive(Default)]
struct StandalonePublicLayoutEntrypointState {
    measure_call_count: usize,
    measure_width: f32,
    measure_width_mode: i32,
    measure_height: f32,
    measure_height_mode: i32,
}

extern "C" fn standalone_public_layout_entrypoint_measure(
    manager_node: *mut c_void,
    width: f32,
    width_mode: SLNodeMeasureMode,
    height: f32,
    height_mode: SLNodeMeasureMode,
) -> StarlightSize {
    if let Some(state) = NonNull::new(manager_node.cast::<StandalonePublicLayoutEntrypointState>())
    {
        // SAFETY: `manager_node` is created from a live
        // `StandalonePublicLayoutEntrypointState` before layout and remains
        // live until after layout returns.
        let state = unsafe { &mut *state.as_ptr() };
        state.measure_call_count += 1;
        state.measure_width = layout_entrypoint_observed_size(width, width_mode);
        state.measure_width_mode = width_mode;
        state.measure_height = layout_entrypoint_observed_size(height, height_mode);
        state.measure_height_mode = height_mode;
    }
    StarlightSize {
        width: layout_entrypoint_measured_axis(width, width_mode, 11.0, 3.0),
        height: layout_entrypoint_measured_axis(height, height_mode, 13.0, 4.0),
    }
}

fn layout_entrypoint_observed_size(size: f32, mode: SLNodeMeasureMode) -> f32 {
    if mode == SL_NODE_MEASURE_MODE_UNDEFINED {
        0.0
    } else {
        size
    }
}

fn layout_entrypoint_measured_axis(
    size: f32,
    mode: SLNodeMeasureMode,
    undefined_size: f32,
    at_most_delta: f32,
) -> f32 {
    match mode {
        SL_NODE_MEASURE_MODE_UNDEFINED => undefined_size,
        SL_NODE_MEASURE_MODE_EXACTLY => size,
        SL_NODE_MEASURE_MODE_AT_MOST => size - at_most_delta,
        _ => size,
    }
}

#[derive(Default)]
struct StandalonePublicMeasureDelegateState {
    measure_call_count: usize,
    baseline_call_count: usize,
    measure_width: f32,
    measure_width_mode: i32,
    measure_height: f32,
    measure_height_mode: i32,
    baseline_width: f32,
    baseline_height: f32,
}

extern "C" fn standalone_public_measure_delegate_measure(
    manager_node: *mut c_void,
    width: f32,
    width_mode: SLNodeMeasureMode,
    height: f32,
    height_mode: SLNodeMeasureMode,
) -> StarlightSize {
    if let Some(state) = NonNull::new(manager_node.cast::<StandalonePublicMeasureDelegateState>()) {
        // SAFETY: `manager_node` is created from a live
        // `StandalonePublicMeasureDelegateState` before layout and remains live
        // until after layout returns.
        let state = unsafe { &mut *state.as_ptr() };
        state.measure_call_count += 1;
        state.measure_width = width;
        state.measure_width_mode = width_mode;
        state.measure_height = height;
        state.measure_height_mode = height_mode;
    }
    StarlightSize { width, height }
}

extern "C" fn standalone_public_measure_delegate_baseline(
    manager_node: *mut c_void,
    width: f32,
    height: f32,
) -> f32 {
    if let Some(state) = NonNull::new(manager_node.cast::<StandalonePublicMeasureDelegateState>()) {
        // SAFETY: `manager_node` is created from a live
        // `StandalonePublicMeasureDelegateState` before layout and remains live
        // until after layout returns.
        let state = unsafe { &mut *state.as_ptr() };
        state.baseline_call_count += 1;
        state.baseline_width = width;
        state.baseline_height = height;
    }
    width / 10.0 + height / 20.0
}

fn snapshot_public_scalar_style(
    node: NonNull<StarlightNode>,
) -> StandalonePublicScalarStyleSnapshot {
    // SAFETY: `node` is a live standalone node owned by `ProbeNodes`.
    unsafe {
        StandalonePublicScalarStyleSnapshot {
            flex_direction: SLNodeStyleGetFlexDirection(node.as_ptr()),
            justify_content: SLNodeStyleGetJustifyContent(node.as_ptr()),
            align_content: SLNodeStyleGetAlignContent(node.as_ptr()),
            align_items: SLNodeStyleGetAlignItems(node.as_ptr()),
            align_self: SLNodeStyleGetAlignSelf(node.as_ptr()),
            position_type: SLNodeStyleGetPositionType(node.as_ptr()),
            flex_wrap: SLNodeStyleGetFlexWrap(node.as_ptr()),
            linear_orientation: SLNodeStyleGetLinearOrientation(node.as_ptr()),
            linear_gravity: SLNodeStyleGetLinearGravity(node.as_ptr()),
            linear_layout_gravity: SLNodeStyleGetLinearLayoutGravity(node.as_ptr()),
            linear_cross_gravity: SLNodeStyleGetLinearCrossGravity(node.as_ptr()),
            relative_center: SLNodeStyleGetRelativeCenter(node.as_ptr()),
            grid_auto_flow: SLNodeStyleGetGridAutoFlow(node.as_ptr()),
            justify_items: SLNodeStyleGetJustifyItems(node.as_ptr()),
            justify_self: SLNodeStyleGetJustifySelf(node.as_ptr()),
            display: SLNodeStyleGetDisplay(node.as_ptr()),
            box_sizing: SLNodeStyleGetBoxSizing(node.as_ptr()),
            aspect_ratio: SLNodeStyleGetAspectRatio(node.as_ptr()),
            order: SLNodeStyleGetOrder(node.as_ptr()),
            relative_id: SLNodeStyleGetRelativeId(node.as_ptr()),
            relative_align_top: SLNodeStyleGetRelativeAlignTop(node.as_ptr()),
            relative_align_right: SLNodeStyleGetRelativeAlignRight(node.as_ptr()),
            relative_align_bottom: SLNodeStyleGetRelativeAlignBottom(node.as_ptr()),
            relative_align_left: SLNodeStyleGetRelativeAlignLeft(node.as_ptr()),
            relative_top_of: SLNodeStyleGetRelativeTopOf(node.as_ptr()),
            relative_right_of: SLNodeStyleGetRelativeRightOf(node.as_ptr()),
            relative_bottom_of: SLNodeStyleGetRelativeBottomOf(node.as_ptr()),
            relative_left_of: SLNodeStyleGetRelativeLeftOf(node.as_ptr()),
            relative_layout_once: SLNodeStyleGetRelativeLayoutOnce(node.as_ptr()),
            grid_column_start: SLNodeStyleGetGridColumnStart(node.as_ptr()),
            grid_column_end: SLNodeStyleGetGridColumnEnd(node.as_ptr()),
            grid_row_start: SLNodeStyleGetGridRowStart(node.as_ptr()),
            grid_row_end: SLNodeStyleGetGridRowEnd(node.as_ptr()),
            grid_column_span: SLNodeStyleGetGridColumnSpan(node.as_ptr()),
            grid_row_span: SLNodeStyleGetGridRowSpan(node.as_ptr()),
            flex_grow: SLNodeStyleGetFlexGrow(node.as_ptr()),
            flex_shrink: SLNodeStyleGetFlexShrink(node.as_ptr()),
            linear_weight: SLNodeStyleGetLinearWeight(node.as_ptr()),
            linear_weight_sum: SLNodeStyleGetLinearWeightSum(node.as_ptr()),
            dirty: SLNodeIsDirty(node.as_ptr()),
        }
    }
}

fn snapshot_public_dimension_style(
    node: NonNull<StarlightNode>,
) -> StandalonePublicDimensionStyleSnapshot {
    // SAFETY: `node` is a live standalone node owned by `ProbeNodes`.
    unsafe {
        StandalonePublicDimensionStyleSnapshot {
            flex_basis: length_value_from_native(SLNodeStyleGetFlexBasis(node.as_ptr())),
            width: length_value_from_native(SLNodeStyleGetWidth(node.as_ptr())),
            height: length_value_from_native(SLNodeStyleGetHeight(node.as_ptr())),
            min_width: length_value_from_native(SLNodeStyleGetMinWidth(node.as_ptr())),
            max_width: length_value_from_native(SLNodeStyleGetMaxWidth(node.as_ptr())),
            min_height: length_value_from_native(SLNodeStyleGetMinHeight(node.as_ptr())),
            max_height: length_value_from_native(SLNodeStyleGetMaxHeight(node.as_ptr())),
            dirty: SLNodeIsDirty(node.as_ptr()),
        }
    }
}

unsafe fn apply_public_dimension_style_variant(
    node: NonNull<StarlightNode>,
    variant: PublicDimensionStyleVariant,
) {
    // SAFETY: The caller guarantees `node` is live. This function forwards
    // fixed public test values to matching standalone C API setters.
    unsafe {
        match variant {
            PublicDimensionStyleVariant::Points => {
                SLNodeStyleSetFlexBasis(node.as_ptr(), 11.0);
                SLNodeStyleSetWidth(node.as_ptr(), 21.0);
                SLNodeStyleSetHeight(node.as_ptr(), 31.0);
                SLNodeStyleSetMinWidth(node.as_ptr(), 4.0);
                SLNodeStyleSetMaxWidth(node.as_ptr(), 41.0);
                SLNodeStyleSetMinHeight(node.as_ptr(), 5.0);
                SLNodeStyleSetMaxHeight(node.as_ptr(), 51.0);
            }
            PublicDimensionStyleVariant::Percent => {
                SLNodeStyleSetFlexBasisPercent(node.as_ptr(), 12.0);
                SLNodeStyleSetWidthPercent(node.as_ptr(), 22.0);
                SLNodeStyleSetHeightPercent(node.as_ptr(), 32.0);
                SLNodeStyleSetMinWidthPercent(node.as_ptr(), 6.0);
                SLNodeStyleSetMaxWidthPercent(node.as_ptr(), 42.0);
                SLNodeStyleSetMinHeightPercent(node.as_ptr(), 7.0);
                SLNodeStyleSetMaxHeightPercent(node.as_ptr(), 52.0);
            }
            PublicDimensionStyleVariant::Calc => {
                SLNodeStyleSetFlexBasisCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(13.0, 14.0),
                );
                SLNodeStyleSetWidthCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(23.0, 24.0),
                );
                SLNodeStyleSetHeightCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(33.0, 34.0),
                );
                SLNodeStyleSetMinWidthCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(8.0, 9.0),
                );
                SLNodeStyleSetMaxWidthCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(43.0, 44.0),
                );
                SLNodeStyleSetMinHeightCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(10.0, 11.0),
                );
                SLNodeStyleSetMaxHeightCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(53.0, 54.0),
                );
            }
            PublicDimensionStyleVariant::ValueFr => {
                SLNodeStyleSetFlexBasisValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(1.25)),
                );
                SLNodeStyleSetWidthValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(2.25)),
                );
                SLNodeStyleSetHeightValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(3.25)),
                );
                SLNodeStyleSetMinWidthValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(4.25)),
                );
                SLNodeStyleSetMaxWidthValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(5.25)),
                );
                SLNodeStyleSetMinHeightValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(6.25)),
                );
                SLNodeStyleSetMaxHeightValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(7.25)),
                );
            }
            PublicDimensionStyleVariant::Auto => {
                SLNodeStyleSetFlexBasisAuto(node.as_ptr());
                SLNodeStyleSetWidthAuto(node.as_ptr());
                SLNodeStyleSetHeightAuto(node.as_ptr());
                SLNodeStyleSetMinWidth(node.as_ptr(), 6.0);
                SLNodeStyleSetMaxWidth(node.as_ptr(), 46.0);
                SLNodeStyleSetMinHeight(node.as_ptr(), 7.0);
                SLNodeStyleSetMaxHeight(node.as_ptr(), 57.0);
            }
            PublicDimensionStyleVariant::MaxContent => {
                SLNodeStyleSetFlexBasisMaxContent(node.as_ptr());
                SLNodeStyleSetWidthMaxContent(node.as_ptr());
                SLNodeStyleSetHeightMaxContent(node.as_ptr());
                SLNodeStyleSetMinWidthMaxContent(node.as_ptr());
                SLNodeStyleSetMaxWidthMaxContent(node.as_ptr());
                SLNodeStyleSetMinHeightMaxContent(node.as_ptr());
                SLNodeStyleSetMaxHeightMaxContent(node.as_ptr());
            }
            PublicDimensionStyleVariant::FitContent => {
                SLNodeStyleSetFlexBasisFitContent(node.as_ptr());
                SLNodeStyleSetWidthFitContent(node.as_ptr());
                SLNodeStyleSetHeightFitContent(node.as_ptr());
                SLNodeStyleSetMinWidthFitContent(node.as_ptr());
                SLNodeStyleSetMaxWidthFitContent(node.as_ptr());
                SLNodeStyleSetMinHeightFitContent(node.as_ptr());
                SLNodeStyleSetMaxHeightFitContent(node.as_ptr());
            }
            PublicDimensionStyleVariant::FitContentValue => {
                SLNodeStyleSetFlexBasisFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(
                        15.0, 16.0,
                    )),
                );
                SLNodeStyleSetWidthFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(
                        25.0, 26.0,
                    )),
                );
                SLNodeStyleSetHeightFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(
                        35.0, 36.0,
                    )),
                );
                SLNodeStyleSetMinWidthFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(
                        17.0, 18.0,
                    )),
                );
                SLNodeStyleSetMaxWidthFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(
                        45.0, 46.0,
                    )),
                );
                SLNodeStyleSetMinHeightFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(
                        19.0, 20.0,
                    )),
                );
                SLNodeStyleSetMaxHeightFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(
                        55.0, 56.0,
                    )),
                );
            }
        }
    }
}

fn standalone_public_dimension_layout_delegate() -> StarlightMeasureDelegate {
    StarlightMeasureDelegate {
        measure_func: Some(standalone_public_dimension_layout_measure),
        baseline_func: None,
        manager_node: std::ptr::null_mut(),
    }
}

extern "C" fn standalone_public_dimension_layout_measure(
    _manager_node: *mut c_void,
    _width: f32,
    _width_mode: SLNodeMeasureMode,
    _height: f32,
    _height_mode: SLNodeMeasureMode,
) -> StarlightSize {
    StarlightSize {
        width: 58.0,
        height: 17.0,
    }
}

unsafe fn apply_public_dimension_layout_basis_variant(
    node: NonNull<StarlightNode>,
    variant: PublicDimensionStyleVariant,
) {
    // SAFETY: The caller guarantees `node` is live. This helper forwards
    // fixed public flex-basis setter values for layout-effect coverage.
    unsafe {
        match variant {
            PublicDimensionStyleVariant::Points => SLNodeStyleSetFlexBasis(node.as_ptr(), 11.0),
            PublicDimensionStyleVariant::Percent => {
                SLNodeStyleSetFlexBasisPercent(node.as_ptr(), 25.0);
            }
            PublicDimensionStyleVariant::Calc => {
                SLNodeStyleSetFlexBasisCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(13.0, 10.0),
                );
            }
            PublicDimensionStyleVariant::ValueFr => {
                SLNodeStyleSetFlexBasisValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(30.0)),
                );
            }
            PublicDimensionStyleVariant::Auto => SLNodeStyleSetFlexBasisAuto(node.as_ptr()),
            PublicDimensionStyleVariant::MaxContent => {
                SLNodeStyleSetFlexBasisMaxContent(node.as_ptr());
            }
            PublicDimensionStyleVariant::FitContent => {
                SLNodeStyleSetFlexBasisFitContent(node.as_ptr());
            }
            PublicDimensionStyleVariant::FitContentValue => {
                SLNodeStyleSetFlexBasisFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(45.0, 0.0)),
                );
            }
        }
    }
}

unsafe fn apply_public_dimension_layout_size_variant(
    node: NonNull<StarlightNode>,
    variant: PublicDimensionStyleVariant,
) {
    // SAFETY: The caller guarantees `node` is live. This helper forwards fixed
    // public width/height setter values for layout-effect coverage.
    unsafe {
        match variant {
            PublicDimensionStyleVariant::Points => {
                SLNodeStyleSetWidth(node.as_ptr(), 21.0);
                SLNodeStyleSetHeight(node.as_ptr(), 31.0);
            }
            PublicDimensionStyleVariant::Percent => {
                SLNodeStyleSetWidthPercent(node.as_ptr(), 25.0);
                SLNodeStyleSetHeightPercent(node.as_ptr(), 20.0);
            }
            PublicDimensionStyleVariant::Calc => {
                SLNodeStyleSetWidthCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(23.0, 10.0),
                );
                SLNodeStyleSetHeightCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(13.0, 10.0),
                );
            }
            PublicDimensionStyleVariant::ValueFr => {
                SLNodeStyleSetWidthValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(22.0)),
                );
                SLNodeStyleSetHeightValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(18.0)),
                );
            }
            PublicDimensionStyleVariant::Auto => {
                SLNodeStyleSetWidthAuto(node.as_ptr());
                SLNodeStyleSetHeightAuto(node.as_ptr());
            }
            PublicDimensionStyleVariant::MaxContent => {
                SLNodeStyleSetWidthMaxContent(node.as_ptr());
                SLNodeStyleSetHeightMaxContent(node.as_ptr());
            }
            PublicDimensionStyleVariant::FitContent => {
                SLNodeStyleSetWidthFitContent(node.as_ptr());
                SLNodeStyleSetHeightFitContent(node.as_ptr());
            }
            PublicDimensionStyleVariant::FitContentValue => {
                SLNodeStyleSetWidthFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(45.0, 0.0)),
                );
                SLNodeStyleSetHeightFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(32.0, 0.0)),
                );
            }
        }
    }
}

unsafe fn apply_public_dimension_layout_clamp_variant(
    node: NonNull<StarlightNode>,
    variant: PublicDimensionStyleVariant,
) {
    // SAFETY: The caller guarantees `node` is live. This helper forwards fixed
    // public min/max size setter values that visibly clamp layout.
    unsafe {
        match variant {
            PublicDimensionStyleVariant::Points => {
                SLNodeStyleSetMinWidth(node.as_ptr(), 45.0);
                SLNodeStyleSetMaxWidth(node.as_ptr(), 60.0);
                SLNodeStyleSetMinHeight(node.as_ptr(), 18.0);
                SLNodeStyleSetMaxHeight(node.as_ptr(), 24.0);
            }
            PublicDimensionStyleVariant::Percent => {
                SLNodeStyleSetMinWidthPercent(node.as_ptr(), 20.0);
                SLNodeStyleSetMaxWidthPercent(node.as_ptr(), 60.0);
                SLNodeStyleSetMinHeightPercent(node.as_ptr(), 20.0);
                SLNodeStyleSetMaxHeightPercent(node.as_ptr(), 50.0);
            }
            PublicDimensionStyleVariant::Calc => {
                SLNodeStyleSetMinWidthCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(35.0, 5.0),
                );
                SLNodeStyleSetMaxWidthCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(70.0, 0.0),
                );
                SLNodeStyleSetMinHeightCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(15.0, 5.0),
                );
                SLNodeStyleSetMaxHeightCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(30.0, 0.0),
                );
            }
            PublicDimensionStyleVariant::ValueFr => {
                SLNodeStyleSetMinWidthValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(44.0)),
                );
                SLNodeStyleSetMaxWidthValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(60.0)),
                );
                SLNodeStyleSetMinHeightValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(18.0)),
                );
                SLNodeStyleSetMaxHeightValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(25.0)),
                );
            }
            PublicDimensionStyleVariant::Auto => {
                SLNodeStyleSetMinWidth(node.as_ptr(), 6.0);
                SLNodeStyleSetMaxWidth(node.as_ptr(), 46.0);
                SLNodeStyleSetMinHeight(node.as_ptr(), 7.0);
                SLNodeStyleSetMaxHeight(node.as_ptr(), 57.0);
            }
            PublicDimensionStyleVariant::MaxContent => {
                SLNodeStyleSetMinWidthMaxContent(node.as_ptr());
                SLNodeStyleSetMaxWidthMaxContent(node.as_ptr());
                SLNodeStyleSetMinHeightMaxContent(node.as_ptr());
                SLNodeStyleSetMaxHeightMaxContent(node.as_ptr());
            }
            PublicDimensionStyleVariant::FitContent => {
                SLNodeStyleSetMinWidthFitContent(node.as_ptr());
                SLNodeStyleSetMaxWidthFitContent(node.as_ptr());
                SLNodeStyleSetMinHeightFitContent(node.as_ptr());
                SLNodeStyleSetMaxHeightFitContent(node.as_ptr());
            }
            PublicDimensionStyleVariant::FitContentValue => {
                SLNodeStyleSetMinWidthFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(45.0, 0.0)),
                );
                SLNodeStyleSetMaxWidthFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(55.0, 0.0)),
                );
                SLNodeStyleSetMinHeightFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(18.0, 0.0)),
                );
                SLNodeStyleSetMaxHeightFitContentValue(
                    node.as_ptr(),
                    starlight_value_from_fit_content_base(BaseLength::fixed_and_percent(22.0, 0.0)),
                );
            }
        }
    }
}

fn snapshot_public_layout_node(node: NonNull<StarlightNode>) -> StandalonePublicLayoutNodeSnapshot {
    const EDGES: [SLEdge; 6] = [
        SL_EDGE_LEFT,
        SL_EDGE_RIGHT,
        SL_EDGE_TOP,
        SL_EDGE_BOTTOM,
        SL_EDGE_START,
        SL_EDGE_END,
    ];

    // SAFETY: `node` is a live standalone node after layout.
    unsafe {
        StandalonePublicLayoutNodeSnapshot {
            left: SLNodeLayoutGetLeft(node.as_ptr()),
            top: SLNodeLayoutGetTop(node.as_ptr()),
            width: SLNodeLayoutGetWidth(node.as_ptr()),
            height: SLNodeLayoutGetHeight(node.as_ptr()),
            baseline: SLNodeLayoutGetBaseline(node.as_ptr()),
            margin: EDGES
                .iter()
                .map(|edge| SLNodeLayoutGetMargin(node.as_ptr(), *edge))
                .collect(),
            padding: EDGES
                .iter()
                .map(|edge| SLNodeLayoutGetPadding(node.as_ptr(), *edge))
                .collect(),
            border: EDGES
                .iter()
                .map(|edge| SLNodeLayoutGetBorder(node.as_ptr(), *edge))
                .collect(),
            sticky_position: EDGES
                .iter()
                .map(|edge| SLNodeLayoutGetStickyPosition(node.as_ptr(), *edge))
                .collect(),
        }
    }
}

fn snapshot_public_edge_style_stage(
    stage: StandalonePublicStyleStage,
    node: NonNull<StarlightNode>,
) -> StandalonePublicEdgeStyleSnapshot {
    const EDGES: [SLEdge; 9] = [
        SL_EDGE_LEFT,
        SL_EDGE_RIGHT,
        SL_EDGE_TOP,
        SL_EDGE_BOTTOM,
        SL_EDGE_START,
        SL_EDGE_END,
        SL_EDGE_HORIZONTAL,
        SL_EDGE_VERTICAL,
        SL_EDGE_ALL,
    ];
    const GAPS: [SLGap; 3] = [SL_GAP_COLUMN, SL_GAP_ROW, SL_GAP_ALL];

    StandalonePublicEdgeStyleSnapshot {
        stage,
        position: EDGES
            .iter()
            .map(|edge| {
                // SAFETY: `node` is live and `edge` is from the public enum.
                length_value_from_native(unsafe { SLNodeStyleGetPosition(node.as_ptr(), *edge) })
            })
            .collect(),
        margin: EDGES
            .iter()
            .map(|edge| {
                // SAFETY: `node` is live and `edge` is from the public enum.
                length_value_from_native(unsafe { SLNodeStyleGetMargin(node.as_ptr(), *edge) })
            })
            .collect(),
        padding: EDGES
            .iter()
            .map(|edge| {
                // SAFETY: `node` is live and `edge` is from the public enum.
                length_value_from_native(unsafe { SLNodeStyleGetPadding(node.as_ptr(), *edge) })
            })
            .collect(),
        border: EDGES
            .iter()
            .map(|edge| {
                // SAFETY: `node` is live and `edge` is from the public enum.
                unsafe { SLNodeStyleGetBorder(node.as_ptr(), *edge) }
            })
            .collect(),
        gap: GAPS
            .iter()
            .map(|gap| {
                // SAFETY: `node` is live and `gap` is from the public enum.
                length_value_from_native(unsafe { SLNodeStyleGetGap(node.as_ptr(), *gap) })
            })
            .collect(),
        // SAFETY: `node` is live.
        dirty: unsafe { SLNodeIsDirty(node.as_ptr()) },
    }
}

fn snapshot_public_edge_style_variant(
    node: NonNull<StarlightNode>,
) -> StandalonePublicEdgeStyleVariantSnapshot {
    // SAFETY: `node` is a live standalone node owned by `ProbeNodes`.
    unsafe {
        StandalonePublicEdgeStyleVariantSnapshot {
            position: length_value_from_native(SLNodeStyleGetPosition(node.as_ptr(), SL_EDGE_LEFT)),
            margin: length_value_from_native(SLNodeStyleGetMargin(node.as_ptr(), SL_EDGE_RIGHT)),
            padding: length_value_from_native(SLNodeStyleGetPadding(node.as_ptr(), SL_EDGE_TOP)),
            gap: length_value_from_native(SLNodeStyleGetGap(node.as_ptr(), SL_GAP_COLUMN)),
            dirty: SLNodeIsDirty(node.as_ptr()),
        }
    }
}

unsafe fn apply_public_edge_style_variant(
    node: NonNull<StarlightNode>,
    variant: PublicEdgeStyleVariant,
) {
    // SAFETY: The caller guarantees `node` is live. This function forwards
    // fixed public test values to matching standalone C API setters.
    unsafe {
        match variant {
            PublicEdgeStyleVariant::Points => {
                SLNodeStyleSetPosition(node.as_ptr(), SL_EDGE_LEFT, 1.0);
                SLNodeStyleSetMargin(node.as_ptr(), SL_EDGE_RIGHT, 2.0);
                SLNodeStyleSetPadding(node.as_ptr(), SL_EDGE_TOP, 3.0);
                SLNodeStyleSetGap(node.as_ptr(), SL_GAP_COLUMN, 4.0);
            }
            PublicEdgeStyleVariant::Percent => {
                SLNodeStyleSetPositionPercent(node.as_ptr(), SL_EDGE_LEFT, 11.0);
                SLNodeStyleSetMarginPercent(node.as_ptr(), SL_EDGE_RIGHT, 12.0);
                SLNodeStyleSetPaddingPercent(node.as_ptr(), SL_EDGE_TOP, 13.0);
                SLNodeStyleSetGapPercent(node.as_ptr(), SL_GAP_COLUMN, 14.0);
            }
            PublicEdgeStyleVariant::Calc => {
                SLNodeStyleSetPositionCalc(
                    node.as_ptr(),
                    SL_EDGE_LEFT,
                    starlight_value_from_calc_length(21.0, 22.0),
                );
                SLNodeStyleSetMarginCalc(
                    node.as_ptr(),
                    SL_EDGE_RIGHT,
                    starlight_value_from_calc_length(23.0, 24.0),
                );
                SLNodeStyleSetPaddingCalc(
                    node.as_ptr(),
                    SL_EDGE_TOP,
                    starlight_value_from_calc_length(25.0, 26.0),
                );
                SLNodeStyleSetGapCalc(
                    node.as_ptr(),
                    SL_GAP_COLUMN,
                    starlight_value_from_calc_length(27.0, 28.0),
                );
            }
            PublicEdgeStyleVariant::ValueFr => {
                SLNodeStyleSetPositionValue(
                    node.as_ptr(),
                    SL_EDGE_LEFT,
                    starlight_value_from_length(Length::fr(1.25)),
                );
                SLNodeStyleSetMarginValue(
                    node.as_ptr(),
                    SL_EDGE_RIGHT,
                    starlight_value_from_length(Length::fr(2.25)),
                );
                SLNodeStyleSetPaddingValue(
                    node.as_ptr(),
                    SL_EDGE_TOP,
                    starlight_value_from_length(Length::fr(3.25)),
                );
                SLNodeStyleSetGapValue(
                    node.as_ptr(),
                    SL_GAP_COLUMN,
                    starlight_value_from_length(Length::fr(4.25)),
                );
            }
            PublicEdgeStyleVariant::ValueMaxContent => {
                SLNodeStyleSetPositionValue(
                    node.as_ptr(),
                    SL_EDGE_LEFT,
                    starlight_value_from_length(Length::max_content()),
                );
                SLNodeStyleSetMarginValue(
                    node.as_ptr(),
                    SL_EDGE_RIGHT,
                    starlight_value_from_length(Length::max_content()),
                );
                SLNodeStyleSetPaddingValue(
                    node.as_ptr(),
                    SL_EDGE_TOP,
                    starlight_value_from_length(Length::max_content()),
                );
                SLNodeStyleSetGapValue(
                    node.as_ptr(),
                    SL_GAP_COLUMN,
                    starlight_value_from_length(Length::max_content()),
                );
            }
            PublicEdgeStyleVariant::ValueFitContent => {
                SLNodeStyleSetPositionValue(
                    node.as_ptr(),
                    SL_EDGE_LEFT,
                    starlight_value_from_length(Length::fit_content(Some(
                        BaseLength::fixed_and_percent(31.0, 32.0),
                    ))),
                );
                SLNodeStyleSetMarginValue(
                    node.as_ptr(),
                    SL_EDGE_RIGHT,
                    starlight_value_from_length(Length::fit_content(Some(
                        BaseLength::fixed_and_percent(33.0, 34.0),
                    ))),
                );
                SLNodeStyleSetPaddingValue(
                    node.as_ptr(),
                    SL_EDGE_TOP,
                    starlight_value_from_length(Length::fit_content(Some(
                        BaseLength::fixed_and_percent(35.0, 36.0),
                    ))),
                );
                SLNodeStyleSetGapValue(
                    node.as_ptr(),
                    SL_GAP_COLUMN,
                    starlight_value_from_length(Length::fit_content(Some(
                        BaseLength::fixed_and_percent(37.0, 38.0),
                    ))),
                );
            }
            PublicEdgeStyleVariant::Auto => {
                SLNodeStyleSetPositionAuto(node.as_ptr(), SL_EDGE_LEFT);
                SLNodeStyleSetMarginAuto(node.as_ptr(), SL_EDGE_RIGHT);
                SLNodeStyleSetPadding(node.as_ptr(), SL_EDGE_TOP, 41.0);
                SLNodeStyleSetGap(node.as_ptr(), SL_GAP_COLUMN, 42.0);
            }
        }
    }
}

unsafe fn apply_public_edge_layout_container_variant(
    node: NonNull<StarlightNode>,
    variant: PublicEdgeStyleVariant,
) {
    // SAFETY: The caller guarantees `node` is live. This helper applies only
    // container-side public edge values that affect child layout without mixing
    // root-position behavior into the transcript.
    unsafe {
        match variant {
            PublicEdgeStyleVariant::Points => {
                SLNodeStyleSetPadding(node.as_ptr(), SL_EDGE_TOP, 3.0);
                SLNodeStyleSetGap(node.as_ptr(), SL_GAP_COLUMN, 4.0);
            }
            PublicEdgeStyleVariant::Percent => {
                SLNodeStyleSetPaddingPercent(node.as_ptr(), SL_EDGE_TOP, 13.0);
                SLNodeStyleSetGapPercent(node.as_ptr(), SL_GAP_COLUMN, 14.0);
            }
            PublicEdgeStyleVariant::Calc => {
                SLNodeStyleSetPaddingCalc(
                    node.as_ptr(),
                    SL_EDGE_TOP,
                    starlight_value_from_calc_length(25.0, 26.0),
                );
                SLNodeStyleSetGapCalc(
                    node.as_ptr(),
                    SL_GAP_COLUMN,
                    starlight_value_from_calc_length(27.0, 28.0),
                );
            }
            PublicEdgeStyleVariant::ValueFr => {
                SLNodeStyleSetPaddingValue(
                    node.as_ptr(),
                    SL_EDGE_TOP,
                    starlight_value_from_length(Length::fr(3.25)),
                );
                SLNodeStyleSetGapValue(
                    node.as_ptr(),
                    SL_GAP_COLUMN,
                    starlight_value_from_length(Length::fr(4.25)),
                );
            }
            PublicEdgeStyleVariant::ValueMaxContent => {
                SLNodeStyleSetPaddingValue(
                    node.as_ptr(),
                    SL_EDGE_TOP,
                    starlight_value_from_length(Length::max_content()),
                );
                SLNodeStyleSetGapValue(
                    node.as_ptr(),
                    SL_GAP_COLUMN,
                    starlight_value_from_length(Length::max_content()),
                );
            }
            PublicEdgeStyleVariant::ValueFitContent => {
                SLNodeStyleSetPaddingValue(
                    node.as_ptr(),
                    SL_EDGE_TOP,
                    starlight_value_from_length(Length::fit_content(Some(
                        BaseLength::fixed_and_percent(35.0, 0.0),
                    ))),
                );
                SLNodeStyleSetGapValue(
                    node.as_ptr(),
                    SL_GAP_COLUMN,
                    starlight_value_from_length(Length::fit_content(Some(
                        BaseLength::fixed_and_percent(37.0, 0.0),
                    ))),
                );
            }
            PublicEdgeStyleVariant::Auto => {
                SLNodeStyleSetPadding(node.as_ptr(), SL_EDGE_TOP, 41.0);
                SLNodeStyleSetGap(node.as_ptr(), SL_GAP_COLUMN, 42.0);
            }
        }
    }
}

unsafe fn normalize_public_edge_layout_child_variant(
    node: NonNull<StarlightNode>,
    variant: PublicEdgeStyleVariant,
) {
    if !matches!(variant, PublicEdgeStyleVariant::ValueFitContent) {
        return;
    }

    // SAFETY: The caller guarantees `node` is live. Fixed-only fit-content
    // values keep this exact-equality transcript focused on public setter
    // layout effects instead of platform-specific float formatting noise.
    unsafe {
        SLNodeStyleSetPositionValue(
            node.as_ptr(),
            SL_EDGE_LEFT,
            starlight_value_from_length(Length::fit_content(Some(BaseLength::fixed_and_percent(
                31.0, 0.0,
            )))),
        );
        SLNodeStyleSetMarginValue(
            node.as_ptr(),
            SL_EDGE_RIGHT,
            starlight_value_from_length(Length::fit_content(Some(BaseLength::fixed_and_percent(
                33.0, 0.0,
            )))),
        );
        SLNodeStyleSetPaddingValue(
            node.as_ptr(),
            SL_EDGE_TOP,
            starlight_value_from_length(Length::fit_content(Some(BaseLength::fixed_and_percent(
                35.0, 0.0,
            )))),
        );
        SLNodeStyleSetGapValue(
            node.as_ptr(),
            SL_GAP_COLUMN,
            starlight_value_from_length(Length::fit_content(Some(BaseLength::fixed_and_percent(
                37.0, 0.0,
            )))),
        );
    }
}

fn length_value_from_native(value: StarlightValue) -> StandalonePublicLengthValue {
    StandalonePublicLengthValue {
        value: value.value,
        unit: value.unit,
        percentage: value.percentage,
        flags: value.flags,
    }
}

fn calculate_probe_layout(root: NonNull<StarlightNode>) {
    // SAFETY: `root` is a live standalone node. The owner constraints are
    // definite and use the public LTR direction enum.
    unsafe {
        SLNodeCalculateLayoutWithMode(
            root.as_ptr(),
            100.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            50.0,
            SL_NODE_MEASURE_MODE_EXACTLY,
            SL_DIRECTION_LTR,
        );
    }
}

fn snapshot_public_tree_stage(
    stage: StandalonePublicTreeStage,
    nodes: &[NonNull<StarlightNode>],
    root: NonNull<StarlightNode>,
    staging: NonNull<StarlightNode>,
) -> StandalonePublicTreeSnapshot {
    StandalonePublicTreeSnapshot {
        stage,
        root_children: native_child_ids(nodes, root),
        staging_children: native_child_ids(nodes, staging),
        parents: nodes
            .iter()
            .map(|node| native_parent_id(nodes, *node))
            .collect(),
        dirty: nodes
            .iter()
            .map(|node| {
                // SAFETY: `node` is a live standalone node owned by `ProbeNodes`.
                unsafe { SLNodeIsDirty(node.as_ptr()) }
            })
            .collect(),
    }
}

fn snapshot_public_dirty_nodes(nodes: &[NonNull<StarlightNode>]) -> Vec<bool> {
    nodes
        .iter()
        .map(|node| {
            // SAFETY: each node is live and owned by `ProbeNodes`.
            unsafe { SLNodeIsDirty(node.as_ptr()) }
        })
        .collect()
}

fn native_child_ids(
    nodes: &[NonNull<StarlightNode>],
    parent: NonNull<StarlightNode>,
) -> Vec<usize> {
    // SAFETY: `parent` is a live standalone node owned by `ProbeNodes`.
    let count = unsafe { SLNodeGetChildCount(parent.as_ptr()) };
    let mut children = Vec::new();
    for index in 0..count {
        // SAFETY: `index` is in range according to `SLNodeGetChildCount`.
        let child = unsafe { SLNodeGetChild(parent.as_ptr(), index) };
        if let Some(id) = native_node_id(nodes, child) {
            children.push(id);
        }
    }
    children
}

fn native_parent_id(
    nodes: &[NonNull<StarlightNode>],
    node: NonNull<StarlightNode>,
) -> Option<usize> {
    // SAFETY: `node` is a live standalone node owned by `ProbeNodes`.
    native_node_id(nodes, unsafe { SLNodeGetParent(node.as_ptr()) })
}

fn native_node_id(nodes: &[NonNull<StarlightNode>], node: SLNodeRef) -> Option<usize> {
    if node.is_null() {
        return None;
    }
    nodes
        .iter()
        .position(|candidate| std::ptr::eq(candidate.as_ptr(), node))
}

fn owner_constraint_to_native(
    constraint: starlight_layout::SideConstraint,
) -> (f32, SLNodeMeasureMode) {
    match constraint.mode {
        MeasureMode::Indefinite => (SL_UNDEFINED, SL_NODE_MEASURE_MODE_UNDEFINED),
        MeasureMode::Definite => (constraint.size, SL_NODE_MEASURE_MODE_EXACTLY),
        MeasureMode::AtMost => (constraint.size, SL_NODE_MEASURE_MODE_AT_MOST),
    }
}

fn build_native_subtree<T: LayoutTree>(
    tree: &T,
    id: T::NodeId,
    nodes: &mut Vec<NativeNode<T::NodeId>>,
    state: &mut NativeBuildState<T>,
) -> Result<NonNull<StarlightNode>, CppBaselineError> {
    let ptr = create_native_node(tree.physical_pixels_per_layout_unit(id))?;
    let result = (|| {
        apply_style(ptr, tree.style(id), tree.has_explicit_direction_style(id))?;
        if tree.has_measure(id) {
            install_measure_delegate(ptr, id, state);
        }
        for child in tree.children(id) {
            let child_ptr = build_native_subtree(tree, child, nodes, state)?;
            // SAFETY: both pointers are live native nodes; index -1 appends.
            unsafe { SLNodeInsertChild(ptr.as_ptr(), child_ptr.as_ptr(), -1) };
        }
        nodes.push(NativeNode { id, ptr });
        Ok(ptr)
    })();

    if result.is_err() {
        // SAFETY: `ptr` has not been returned to a caller; it owns any children
        // inserted before the error occurred.
        unsafe { SLNodeFreeRecursive(ptr.as_ptr()) };
    }

    result
}

fn create_native_node(
    physical_pixels_per_layout_unit: f32,
) -> Result<NonNull<StarlightNode>, CppBaselineError> {
    let node = if uses_default_physical_pixels_per_layout_unit(physical_pixels_per_layout_unit) {
        // SAFETY: `SLNodeNew` returns a newly allocated native node or null on
        // allocation failure.
        unsafe { SLNodeNew() }
    } else {
        // SAFETY: the config is allocated through the public standalone API,
        // initialized before node creation, and freed immediately after
        // `SLNodeNewWithConfig` copies the config value into the node style.
        unsafe {
            let config = NonNull::new(SLConfigCreate()).ok_or(
                CppBaselineError::NativeLayoutUnavailable("SLConfigCreate returned null"),
            )?;
            SLConfigSetPhysicalPixelsPerLayoutUnit(
                config.as_ptr(),
                physical_pixels_per_layout_unit,
            );
            let node = SLNodeNewWithConfig(config.as_ptr());
            SLConfigFree(config.as_ptr());
            node
        }
    };
    NonNull::new(node).ok_or(CppBaselineError::NativeLayoutUnavailable(
        "SLNodeNew returned null",
    ))
}

fn uses_default_physical_pixels_per_layout_unit(value: f32) -> bool {
    !value.is_finite() || value <= 0.0 || (value - 1.0).abs() <= f32::EPSILON
}

fn count_measure_nodes<T: LayoutTree>(tree: &T, id: T::NodeId) -> usize {
    let self_count = usize::from(tree.has_measure(id));
    self_count
        + tree
            .children(id)
            .map(|child| count_measure_nodes(tree, child))
            .sum::<usize>()
}

fn install_measure_delegate<T: LayoutTree>(
    ptr: NonNull<StarlightNode>,
    id: T::NodeId,
    state: &mut NativeBuildState<T>,
) {
    debug_assert!(state.measure_contexts.len() < state.measure_contexts.capacity());
    debug_assert!(state.measure_delegates.len() < state.measure_delegates.capacity());
    state.measure_contexts.push(MeasureContext {
        tree: state.tree,
        node: id,
        measured_content_size: Cell::new(None),
    });
    let context_ptr = state
        .measure_contexts
        .last_mut()
        .expect("measure context was just pushed") as *mut MeasureContext<T>
        as *mut c_void;
    state.measure_delegates.push(StarlightMeasureDelegate {
        measure_func: Some(measure_callback::<T>),
        baseline_func: Some(baseline_callback::<T>),
        manager_node: context_ptr,
    });
    let delegate_ptr = state
        .measure_delegates
        .last_mut()
        .expect("measure delegate was just pushed")
        as *mut StarlightMeasureDelegate;
    // SAFETY: `ptr` is a live native node. `state` preallocates context and
    // delegate vectors, so these pointers stay stable until native layout and
    // node teardown finish.
    unsafe { SLNodeSetMeasureDelegate(ptr.as_ptr(), delegate_ptr) };
}

extern "C" fn measure_callback<T: LayoutTree>(
    manager_node: *mut c_void,
    width: f32,
    width_mode: SLNodeMeasureMode,
    height: f32,
    height_mode: SLNodeMeasureMode,
) -> StarlightSize {
    let size = catch_unwind(AssertUnwindSafe(|| {
        if manager_node.is_null() {
            return Size::ZERO;
        }
        // SAFETY: `manager_node` points into `NativeBuildState` storage created
        // by `install_measure_delegate` and kept alive for the whole native
        // layout call.
        let context = unsafe { &mut *(manager_node as *mut MeasureContext<T>) };
        let constraints = Constraints::new(
            constraint_from_native(width, width_mode),
            constraint_from_native(height, height_mode),
        );
        // SAFETY: `context.tree` points to the `&mut T` passed to
        // `layout_standalone`. No Rust references to the tree are used while
        // C++ invokes this callback.
        let tree = unsafe { context.tree.as_mut() };
        let size = tree
            .measure(context.node, constraints)
            .unwrap_or(Size::ZERO);
        context.measured_content_size.set(Some(size));
        size
    }))
    .unwrap_or(Size::ZERO);

    StarlightSize {
        width: size.width,
        height: size.height,
    }
}

extern "C" fn baseline_callback<T: LayoutTree>(
    manager_node: *mut c_void,
    width: f32,
    height: f32,
) -> f32 {
    catch_unwind(AssertUnwindSafe(|| {
        if manager_node.is_null() {
            return 0.0;
        }
        // SAFETY: `manager_node` points into `NativeBuildState` storage created
        // by `install_measure_delegate` and kept alive for the whole native
        // layout call.
        let context = unsafe { &*(manager_node as *mut MeasureContext<T>) };
        // SAFETY: `context.tree` points to the `&mut T` passed to
        // `layout_standalone`. The callback only needs immutable access.
        let tree = unsafe { context.tree.as_ref() };
        let content_size = context
            .measured_content_size
            .get()
            .unwrap_or_else(|| Size::new(width, height));
        tree.baseline(context.node, content_size).unwrap_or(0.0)
    }))
    .unwrap_or(0.0)
}

fn constraint_from_native(size: f32, mode: SLNodeMeasureMode) -> starlight_layout::SideConstraint {
    match mode {
        SL_NODE_MEASURE_MODE_EXACTLY => starlight_layout::SideConstraint::definite(size),
        SL_NODE_MEASURE_MODE_AT_MOST => starlight_layout::SideConstraint::at_most(size),
        SL_NODE_MEASURE_MODE_UNDEFINED => starlight_layout::SideConstraint::indefinite(),
        _ => starlight_layout::SideConstraint::indefinite(),
    }
}

fn apply_style(
    ptr: NonNull<StarlightNode>,
    style: &Style,
    has_explicit_direction: bool,
) -> Result<(), CppBaselineError> {
    ensure_standalone_supported(style)?;
    let node = ptr.as_ptr();

    // SAFETY: `node` is live for the duration of tree construction.
    unsafe {
        let grid_template_columns_max = effective_grid_max_tracks(
            &style.grid_template_columns,
            &style.grid_template_columns_max,
        );
        let grid_template_rows_max =
            effective_grid_max_tracks(&style.grid_template_rows, &style.grid_template_rows_max);
        let grid_auto_columns_max =
            effective_grid_max_tracks(&style.grid_auto_columns, &style.grid_auto_columns_max);
        let grid_auto_rows_max =
            effective_grid_max_tracks(&style.grid_auto_rows, &style.grid_auto_rows_max);

        SLNodeStyleSetDisplay(node, map_display(style.display)?);
        if has_explicit_direction {
            SLNodeStyleSetDirection(node, map_direction(style.direction));
        }
        SLNodeStyleSetFlexDirection(node, map_flex_direction(style.flex_direction));
        SLNodeStyleSetJustifyContent(node, map_justify_content(style.justify_content));
        SLNodeStyleSetAlignContent(node, map_align_content(style.align_content)?);
        SLNodeStyleSetAlignItems(node, map_align_items(style.align_items)?);
        SLNodeStyleSetAlignSelf(
            node,
            match style.align_self {
                Some(align_self) => map_align_items(align_self)?,
                None => SL_FLEX_ALIGN_AUTO,
            },
        );
        SLNodeStyleSetPositionType(node, map_position(style)?);
        SLNodeStyleSetFlexWrap(node, map_flex_wrap(style.flex_wrap));
        SLNodeStyleSetLinearOrientation(node, map_linear_orientation(style.linear_orientation));
        SLNodeStyleSetLinearGravity(node, map_linear_gravity(style.linear_gravity));
        SLNodeStyleSetLinearLayoutGravity(
            node,
            map_linear_layout_gravity(style.linear_layout_gravity),
        );
        SLNodeStyleSetLinearCrossGravity(
            node,
            map_linear_cross_gravity(style.linear_cross_gravity),
        );
        if let Some(column_count) = style.linear_column_count.filter(|count| *count > 0) {
            let column_count = i32::try_from(column_count).map_err(|_| {
                CppBaselineError::UnsupportedStyle(
                    "linear column count exceeds standalone C API i32 range",
                )
            })?;
            SLNodeStyleSetLinearColumnCount(node, column_count);
        }
        if let Some(component_type) = style.list_component_type {
            SLNodeStyleSetListComponentType(node, map_list_component_type(component_type));
        }
        set_list_gap(
            node,
            style.list_main_axis_gap,
            SLNodeStyleSetListMainAxisGap,
            SLNodeStyleSetListMainAxisGapPercent,
            SLNodeStyleSetListMainAxisGapCalc,
            SLNodeStyleSetListMainAxisGapValue,
        );
        set_list_gap(
            node,
            style.list_cross_axis_gap,
            SLNodeStyleSetListCrossAxisGap,
            SLNodeStyleSetListCrossAxisGapPercent,
            SLNodeStyleSetListCrossAxisGapCalc,
            SLNodeStyleSetListCrossAxisGapValue,
        );
        SLNodeStyleSetLinearWeight(node, style.linear_weight);
        SLNodeStyleSetLinearWeightSum(node, style.linear_weight_sum);
        SLNodeStyleSetRelativeId(node, style.relative_id);
        SLNodeStyleSetRelativeAlignTop(node, style.relative_align_top);
        SLNodeStyleSetRelativeAlignRight(node, style.relative_align_right);
        SLNodeStyleSetRelativeAlignBottom(node, style.relative_align_bottom);
        SLNodeStyleSetRelativeAlignLeft(node, style.relative_align_left);
        SLNodeStyleSetRelativeTopOf(node, style.relative_top_of);
        SLNodeStyleSetRelativeRightOf(node, style.relative_right_of);
        SLNodeStyleSetRelativeBottomOf(node, style.relative_bottom_of);
        SLNodeStyleSetRelativeLeftOf(node, style.relative_left_of);
        SLNodeStyleSetRelativeLayoutOnce(node, style.relative_layout_once);
        SLNodeStyleSetRelativeCenter(node, map_relative_center(style.relative_center));
        set_grid_track_vector(
            node,
            &style.grid_template_columns,
            SLNodeStyleSetGridTemplateColumns,
        )?;
        set_grid_track_vector(
            node,
            &grid_template_columns_max,
            SLNodeStyleSetGridTemplateColumnsMax,
        )?;
        set_grid_track_vector(
            node,
            &style.grid_template_rows,
            SLNodeStyleSetGridTemplateRows,
        )?;
        set_grid_track_vector(
            node,
            &grid_template_rows_max,
            SLNodeStyleSetGridTemplateRowsMax,
        )?;
        set_grid_track_vector(
            node,
            &style.grid_auto_columns,
            SLNodeStyleSetGridAutoColumns,
        )?;
        set_grid_track_vector(
            node,
            &grid_auto_columns_max,
            SLNodeStyleSetGridAutoColumnsMax,
        )?;
        set_grid_track_vector(node, &style.grid_auto_rows, SLNodeStyleSetGridAutoRows)?;
        set_grid_track_vector(node, &grid_auto_rows_max, SLNodeStyleSetGridAutoRowsMax)?;
        SLNodeStyleSetGridAutoFlow(node, map_grid_auto_flow(style.grid_auto_flow));
        SLNodeStyleSetJustifyItems(node, map_justify_item(style.justify_items));
        SLNodeStyleSetJustifySelf(node, map_justify_item(style.justify_self));
        SLNodeStyleSetGridColumnStart(node, style.grid_column_start.unwrap_or(0));
        SLNodeStyleSetGridColumnEnd(node, style.grid_column_end.unwrap_or(0));
        SLNodeStyleSetGridRowStart(node, style.grid_row_start.unwrap_or(0));
        SLNodeStyleSetGridRowEnd(node, style.grid_row_end.unwrap_or(0));
        SLNodeStyleSetGridColumnSpan(node, style.grid_column_span as i32);
        SLNodeStyleSetGridRowSpan(node, style.grid_row_span as i32);
        SLNodeStyleSetBoxSizing(node, map_box_sizing(style.box_sizing));
        if let Some(aspect_ratio) = style.aspect_ratio {
            SLNodeStyleSetAspectRatio(node, aspect_ratio);
        }
        SLNodeStyleSetOrder(node, style.order);
        if is_standalone_flex_shorthand(style) {
            SLNodeStyleSetFlex(node, style.flex_grow);
        } else {
            SLNodeStyleSetFlexGrow(node, style.flex_grow);
            SLNodeStyleSetFlexShrink(node, style.flex_shrink);
            set_flex_basis(node, style.flex_basis);
        }
        set_axis_length(
            node,
            style.width,
            AxisLengthSetters {
                points: SLNodeStyleSetWidth,
                percent: SLNodeStyleSetWidthPercent,
                calc: SLNodeStyleSetWidthCalc,
                value: SLNodeStyleSetWidthValue,
                auto: SLNodeStyleSetWidthAuto,
                max_content: SLNodeStyleSetWidthMaxContent,
                fit_content: SLNodeStyleSetWidthFitContent,
                fit_content_value: SLNodeStyleSetWidthFitContentValue,
            },
        );
        set_axis_length(
            node,
            style.height,
            AxisLengthSetters {
                points: SLNodeStyleSetHeight,
                percent: SLNodeStyleSetHeightPercent,
                calc: SLNodeStyleSetHeightCalc,
                value: SLNodeStyleSetHeightValue,
                auto: SLNodeStyleSetHeightAuto,
                max_content: SLNodeStyleSetHeightMaxContent,
                fit_content: SLNodeStyleSetHeightFitContent,
                fit_content_value: SLNodeStyleSetHeightFitContentValue,
            },
        );
        set_optional_min_length(
            node,
            style.min_width,
            OptionalLengthSetters {
                points: SLNodeStyleSetMinWidth,
                percent: SLNodeStyleSetMinWidthPercent,
                calc: SLNodeStyleSetMinWidthCalc,
                value: SLNodeStyleSetMinWidthValue,
                max_content: SLNodeStyleSetMinWidthMaxContent,
                fit_content: SLNodeStyleSetMinWidthFitContent,
                fit_content_value: SLNodeStyleSetMinWidthFitContentValue,
            },
        );
        set_optional_min_length(
            node,
            style.min_height,
            OptionalLengthSetters {
                points: SLNodeStyleSetMinHeight,
                percent: SLNodeStyleSetMinHeightPercent,
                calc: SLNodeStyleSetMinHeightCalc,
                value: SLNodeStyleSetMinHeightValue,
                max_content: SLNodeStyleSetMinHeightMaxContent,
                fit_content: SLNodeStyleSetMinHeightFitContent,
                fit_content_value: SLNodeStyleSetMinHeightFitContentValue,
            },
        );
        set_optional_max_length(
            node,
            style.max_width,
            OptionalLengthSetters {
                points: SLNodeStyleSetMaxWidth,
                percent: SLNodeStyleSetMaxWidthPercent,
                calc: SLNodeStyleSetMaxWidthCalc,
                value: SLNodeStyleSetMaxWidthValue,
                max_content: SLNodeStyleSetMaxWidthMaxContent,
                fit_content: SLNodeStyleSetMaxWidthFitContent,
                fit_content_value: SLNodeStyleSetMaxWidthFitContentValue,
            },
        );
        set_optional_max_length(
            node,
            style.max_height,
            OptionalLengthSetters {
                points: SLNodeStyleSetMaxHeight,
                percent: SLNodeStyleSetMaxHeightPercent,
                calc: SLNodeStyleSetMaxHeightCalc,
                value: SLNodeStyleSetMaxHeightValue,
                max_content: SLNodeStyleSetMaxHeightMaxContent,
                fit_content: SLNodeStyleSetMaxHeightFitContent,
                fit_content_value: SLNodeStyleSetMaxHeightFitContentValue,
            },
        );
        let (position_left, position_right, position_top, position_bottom) =
            if style.position == PositionType::Static {
                (Length::Auto, Length::Auto, Length::Auto, Length::Auto)
            } else {
                (style.left, style.right, style.top, style.bottom)
            };
        set_edge_lengths(
            node,
            position_left,
            position_right,
            position_top,
            position_bottom,
            EdgeLengthSetters {
                points: SLNodeStyleSetPosition,
                percent: SLNodeStyleSetPositionPercent,
                calc: SLNodeStyleSetPositionCalc,
                value: SLNodeStyleSetPositionValue,
                auto: Some(SLNodeStyleSetPositionAuto),
            },
        );
        set_edge_lengths(
            node,
            style.margin.left,
            style.margin.right,
            style.margin.top,
            style.margin.bottom,
            EdgeLengthSetters {
                points: SLNodeStyleSetMargin,
                percent: SLNodeStyleSetMarginPercent,
                calc: SLNodeStyleSetMarginCalc,
                value: SLNodeStyleSetMarginValue,
                auto: Some(SLNodeStyleSetMarginAuto),
            },
        );
        set_edge_lengths(
            node,
            style.padding.left,
            style.padding.right,
            style.padding.top,
            style.padding.bottom,
            EdgeLengthSetters {
                points: SLNodeStyleSetPadding,
                percent: SLNodeStyleSetPaddingPercent,
                calc: SLNodeStyleSetPaddingCalc,
                value: SLNodeStyleSetPaddingValue,
                auto: None,
            },
        );
        set_edge_values(node, style.border, SLNodeStyleSetBorder);
        set_gap(node, SL_GAP_ROW, style.row_gap);
        set_gap(node, SL_GAP_COLUMN, style.column_gap);
        SLNodeSetMeasureDelegate(node, std::ptr::null_mut());
    }

    Ok(())
}

fn ensure_standalone_supported(style: &Style) -> Result<(), CppBaselineError> {
    ensure_standalone_lengths_supported(style)?;
    if !matches!(
        style.display,
        Display::None
            | Display::Block
            | Display::Flex
            | Display::Linear
            | Display::Relative
            | Display::Grid
    ) {
        return Err(CppBaselineError::UnsupportedStyle(
            "display type outside standalone block/flex/linear/relative/grid C API surface",
        ));
    }
    if style.grid_column_span > i32::MAX as usize || style.grid_row_span > i32::MAX as usize {
        return Err(CppBaselineError::UnsupportedStyle(
            "grid span exceeds standalone C API i32 range",
        ));
    }
    if style
        .linear_column_count
        .is_some_and(|count| count > i32::MAX as usize)
    {
        return Err(CppBaselineError::UnsupportedStyle(
            "linear column count exceeds standalone C API i32 range",
        ));
    }
    if style.visibility != Visibility::Visible {
        return Err(CppBaselineError::UnsupportedStyle(
            "visibility outside standalone C API surface",
        ));
    }
    Ok(())
}

fn ensure_standalone_lengths_supported(style: &Style) -> Result<(), CppBaselineError> {
    if !axis_length_is_standalone_supported(style.width)
        || !axis_length_is_standalone_supported(style.height)
    {
        return Err(CppBaselineError::UnsupportedStyle(
            "fr axis lengths in standalone C API",
        ));
    }

    let min_max_lengths = [
        style.min_width,
        style.min_height,
        style.max_width,
        style.max_height,
    ];

    if min_max_lengths
        .into_iter()
        .any(|length| !min_max_length_is_standalone_supported(length))
    {
        return Err(CppBaselineError::UnsupportedStyle(
            "fr min/max lengths in standalone C API",
        ));
    }

    if !flex_basis_length_is_standalone_supported(style.flex_basis) {
        return Err(CppBaselineError::UnsupportedStyle(
            "fr flex-basis lengths in standalone C API",
        ));
    }

    let fixed_lengths = [
        style.left,
        style.right,
        style.top,
        style.bottom,
        style.margin.left,
        style.margin.right,
        style.margin.top,
        style.margin.bottom,
        style.padding.left,
        style.padding.right,
        style.padding.top,
        style.padding.bottom,
    ];

    if fixed_lengths
        .into_iter()
        .any(|length| !length_is_standalone_supported(length))
    {
        return Err(CppBaselineError::UnsupportedStyle(
            "unsupported edge length in standalone C API",
        ));
    }

    if !gap_length_is_standalone_supported(style.row_gap)
        || !gap_length_is_standalone_supported(style.column_gap)
    {
        return Err(CppBaselineError::UnsupportedStyle(
            "unsupported row/column gap length in standalone C API",
        ));
    }

    if !list_gap_length_is_standalone_supported(style.list_main_axis_gap)
        || !list_gap_length_is_standalone_supported(style.list_cross_axis_gap)
    {
        return Err(CppBaselineError::UnsupportedStyle(
            "fr/intrinsic list gaps in standalone C API",
        ));
    }

    if style
        .grid_template_columns
        .iter()
        .chain(style.grid_template_rows.iter())
        .chain(style.grid_template_columns_max.iter())
        .chain(style.grid_template_rows_max.iter())
        .chain(style.grid_auto_columns.iter())
        .chain(style.grid_auto_rows.iter())
        .chain(style.grid_auto_columns_max.iter())
        .chain(style.grid_auto_rows_max.iter())
        .any(|length| !grid_track_length_is_standalone_supported(*length))
    {
        return Err(CppBaselineError::UnsupportedStyle(
            "unsupported grid track length in standalone C API",
        ));
    }

    Ok(())
}

fn length_is_standalone_supported(length: Length) -> bool {
    matches!(
        length,
        Length::Auto
            | Length::Points(_)
            | Length::Percent(_)
            | Length::Calc { .. }
            | Length::MaxContent
            | Length::FitContent(_)
            | Length::Fr(_)
    )
}

fn min_max_length_is_standalone_supported(length: Length) -> bool {
    matches!(
        length,
        Length::Auto
            | Length::Points(_)
            | Length::Percent(_)
            | Length::Calc { .. }
            | Length::Fr(_)
            | Length::MaxContent
            | Length::FitContent(None)
            | Length::FitContent(Some(_))
    )
}

fn flex_basis_length_is_standalone_supported(length: Length) -> bool {
    matches!(
        length,
        Length::Auto
            | Length::Points(_)
            | Length::Percent(_)
            | Length::Calc { .. }
            | Length::Fr(_)
            | Length::MaxContent
            | Length::FitContent(_)
    )
}

fn axis_length_is_standalone_supported(length: Length) -> bool {
    matches!(
        length,
        Length::Auto
            | Length::Points(_)
            | Length::Percent(_)
            | Length::Calc { .. }
            | Length::Fr(_)
            | Length::MaxContent
            | Length::FitContent(_)
    )
}

fn list_gap_length_is_standalone_supported(length: Length) -> bool {
    matches!(
        length,
        Length::Auto
            | Length::Points(_)
            | Length::Percent(_)
            | Length::Calc { .. }
            | Length::MaxContent
            | Length::FitContent(_)
            | Length::Fr(_)
    )
}

fn gap_length_is_standalone_supported(length: Length) -> bool {
    matches!(
        length,
        Length::Auto
            | Length::Points(_)
            | Length::Percent(_)
            | Length::Calc { .. }
            | Length::MaxContent
            | Length::FitContent(_)
            | Length::Fr(_)
    )
}

fn grid_track_length_is_standalone_supported(length: Length) -> bool {
    match length {
        Length::Auto
        | Length::Points(_)
        | Length::Percent(_)
        | Length::Calc { .. }
        | Length::MaxContent
        | Length::Fr(_) => true,
        Length::FitContent(Some(base)) => base.has_value(),
        Length::FitContent(None) | Length::MinContent => false,
    }
}

fn map_display(display: Display) -> Result<SLDisplay, CppBaselineError> {
    match display {
        Display::None => Ok(SL_DISPLAY_NONE),
        Display::Flex => Ok(SL_DISPLAY_FLEX),
        Display::Grid => Ok(SL_DISPLAY_GRID),
        Display::Linear => Ok(SL_DISPLAY_LINEAR),
        Display::Relative => Ok(SL_DISPLAY_RELATIVE),
        Display::Block => Ok(SL_DISPLAY_BLOCK),
    }
}

fn map_direction(direction: Direction) -> SLDirection {
    match direction {
        Direction::Ltr => SL_DIRECTION_LTR,
        Direction::Rtl => SL_DIRECTION_RTL,
    }
}

fn map_position(style: &Style) -> Result<SLPositionType, CppBaselineError> {
    match style.position {
        PositionType::Static | PositionType::Relative => Ok(SL_POSITION_TYPE_RELATIVE),
        PositionType::Absolute => Ok(SL_POSITION_TYPE_ABSOLUTE),
        PositionType::Fixed => Ok(SL_POSITION_TYPE_FIXED),
        PositionType::Sticky => Ok(SL_POSITION_TYPE_STICKY),
    }
}

fn map_flex_direction(direction: FlexDirection) -> SLFlexDirection {
    match direction {
        FlexDirection::Row => SL_FLEX_DIRECTION_ROW,
        FlexDirection::RowReverse => SL_FLEX_DIRECTION_ROW_REVERSE,
        FlexDirection::Column => SL_FLEX_DIRECTION_COLUMN,
        FlexDirection::ColumnReverse => SL_FLEX_DIRECTION_COLUMN_REVERSE,
    }
}

fn map_flex_wrap(wrap: FlexWrap) -> SLFlexWrap {
    match wrap {
        FlexWrap::NoWrap => SL_FLEX_WRAP_NOWRAP,
        FlexWrap::Wrap => SL_FLEX_WRAP_WRAP,
        FlexWrap::WrapReverse => SL_FLEX_WRAP_WRAP_REVERSE,
    }
}

fn map_linear_orientation(orientation: LinearOrientation) -> SLLinearOrientation {
    match orientation {
        LinearOrientation::Horizontal => SL_LINEAR_ORIENTATION_HORIZONTAL,
        LinearOrientation::HorizontalReverse => SL_LINEAR_ORIENTATION_HORIZONTAL_REVERSE,
        LinearOrientation::Vertical => SL_LINEAR_ORIENTATION_VERTICAL,
        LinearOrientation::VerticalReverse => SL_LINEAR_ORIENTATION_VERTICAL_REVERSE,
        LinearOrientation::Row => SL_LINEAR_ORIENTATION_ROW,
        LinearOrientation::RowReverse => SL_LINEAR_ORIENTATION_ROW_REVERSE,
        LinearOrientation::Column => SL_LINEAR_ORIENTATION_COLUMN,
        LinearOrientation::ColumnReverse => SL_LINEAR_ORIENTATION_COLUMN_REVERSE,
    }
}

fn map_linear_gravity(gravity: LinearGravity) -> SLLinearGravity {
    match gravity {
        LinearGravity::None => SL_LINEAR_GRAVITY_NONE,
        LinearGravity::Top => SL_LINEAR_GRAVITY_TOP,
        LinearGravity::Bottom => SL_LINEAR_GRAVITY_BOTTOM,
        LinearGravity::Left => SL_LINEAR_GRAVITY_LEFT,
        LinearGravity::Right => SL_LINEAR_GRAVITY_RIGHT,
        LinearGravity::CenterVertical => SL_LINEAR_GRAVITY_CENTER_VERTICAL,
        LinearGravity::CenterHorizontal => SL_LINEAR_GRAVITY_CENTER_HORIZONTAL,
        LinearGravity::SpaceBetween => SL_LINEAR_GRAVITY_SPACE_BETWEEN,
        LinearGravity::Start => SL_LINEAR_GRAVITY_START,
        LinearGravity::End => SL_LINEAR_GRAVITY_END,
        LinearGravity::Center => SL_LINEAR_GRAVITY_CENTER,
    }
}

fn map_linear_layout_gravity(gravity: LinearLayoutGravity) -> SLLinearLayoutGravity {
    match gravity {
        LinearLayoutGravity::None => SL_LINEAR_LAYOUT_GRAVITY_NONE,
        LinearLayoutGravity::Top => SL_LINEAR_LAYOUT_GRAVITY_TOP,
        LinearLayoutGravity::Bottom => SL_LINEAR_LAYOUT_GRAVITY_BOTTOM,
        LinearLayoutGravity::Left => SL_LINEAR_LAYOUT_GRAVITY_LEFT,
        LinearLayoutGravity::Right => SL_LINEAR_LAYOUT_GRAVITY_RIGHT,
        LinearLayoutGravity::CenterVertical => SL_LINEAR_LAYOUT_GRAVITY_CENTER_VERTICAL,
        LinearLayoutGravity::CenterHorizontal => SL_LINEAR_LAYOUT_GRAVITY_CENTER_HORIZONTAL,
        LinearLayoutGravity::FillVertical => SL_LINEAR_LAYOUT_GRAVITY_FILL_VERTICAL,
        LinearLayoutGravity::FillHorizontal => SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL,
        LinearLayoutGravity::Center => SL_LINEAR_LAYOUT_GRAVITY_CENTER,
        LinearLayoutGravity::Stretch => SL_LINEAR_LAYOUT_GRAVITY_STRETCH,
        LinearLayoutGravity::Start => SL_LINEAR_LAYOUT_GRAVITY_START,
        LinearLayoutGravity::End => SL_LINEAR_LAYOUT_GRAVITY_END,
    }
}

fn map_linear_cross_gravity(gravity: LinearCrossGravity) -> SLLinearCrossGravity {
    match gravity {
        LinearCrossGravity::None => SL_LINEAR_CROSS_GRAVITY_NONE,
        LinearCrossGravity::Start => SL_LINEAR_CROSS_GRAVITY_START,
        LinearCrossGravity::End => SL_LINEAR_CROSS_GRAVITY_END,
        LinearCrossGravity::Center => SL_LINEAR_CROSS_GRAVITY_CENTER,
        LinearCrossGravity::Stretch => SL_LINEAR_CROSS_GRAVITY_STRETCH,
    }
}

fn map_list_component_type(component_type: ListComponentType) -> SLListComponentType {
    match component_type {
        ListComponentType::Default => SL_LIST_COMPONENT_TYPE_DEFAULT,
        ListComponentType::Header => SL_LIST_COMPONENT_TYPE_HEADER,
        ListComponentType::Footer => SL_LIST_COMPONENT_TYPE_FOOTER,
        ListComponentType::ListRow => SL_LIST_COMPONENT_TYPE_LIST_ROW,
    }
}

fn map_relative_center(center: RelativeCenter) -> SLRelativeCenter {
    match center {
        RelativeCenter::None => SL_RELATIVE_CENTER_NONE,
        RelativeCenter::Horizontal => SL_RELATIVE_CENTER_HORIZONTAL,
        RelativeCenter::Vertical => SL_RELATIVE_CENTER_VERTICAL,
        RelativeCenter::Both => SL_RELATIVE_CENTER_BOTH,
    }
}

fn map_grid_auto_flow(auto_flow: GridAutoFlow) -> SLGridAutoFlow {
    match auto_flow {
        GridAutoFlow::Row => SL_GRID_AUTO_FLOW_ROW,
        GridAutoFlow::Column => SL_GRID_AUTO_FLOW_COLUMN,
        GridAutoFlow::Dense => SL_GRID_AUTO_FLOW_DENSE,
        GridAutoFlow::RowDense => SL_GRID_AUTO_FLOW_ROW_DENSE,
        GridAutoFlow::ColumnDense => SL_GRID_AUTO_FLOW_COLUMN_DENSE,
    }
}

fn map_justify_item(justify_item: JustifyItems) -> SLJustifyItem {
    match justify_item {
        JustifyItems::Auto => SL_JUSTIFY_ITEM_AUTO,
        JustifyItems::Stretch => SL_JUSTIFY_ITEM_STRETCH,
        JustifyItems::Start => SL_JUSTIFY_ITEM_START,
        JustifyItems::End => SL_JUSTIFY_ITEM_END,
        JustifyItems::Center => SL_JUSTIFY_ITEM_CENTER,
    }
}

fn map_justify_content(justify_content: JustifyContent) -> SLJustifyContent {
    match justify_content {
        JustifyContent::Stretch => SL_JUSTIFY_CONTENT_STRETCH,
        JustifyContent::FlexStart => SL_JUSTIFY_CONTENT_FLEX_START,
        JustifyContent::Start => SL_JUSTIFY_CONTENT_START,
        JustifyContent::Center => SL_JUSTIFY_CONTENT_CENTER,
        JustifyContent::FlexEnd => SL_JUSTIFY_CONTENT_FLEX_END,
        JustifyContent::End => SL_JUSTIFY_CONTENT_END,
        JustifyContent::SpaceBetween => SL_JUSTIFY_CONTENT_SPACE_BETWEEN,
        JustifyContent::SpaceAround => SL_JUSTIFY_CONTENT_SPACE_AROUND,
        JustifyContent::SpaceEvenly => SL_JUSTIFY_CONTENT_SPACE_EVENLY,
    }
}

fn map_align_content(align_content: AlignContent) -> Result<SLAlignContent, CppBaselineError> {
    match align_content {
        AlignContent::FlexStart | AlignContent::Start => Ok(SL_ALIGN_CONTENT_FLEX_START),
        AlignContent::Center => Ok(SL_ALIGN_CONTENT_CENTER),
        AlignContent::FlexEnd | AlignContent::End => Ok(SL_ALIGN_CONTENT_FLEX_END),
        AlignContent::SpaceBetween => Ok(SL_ALIGN_CONTENT_SPACE_BETWEEN),
        AlignContent::SpaceAround => Ok(SL_ALIGN_CONTENT_SPACE_AROUND),
        AlignContent::SpaceEvenly => Ok(SL_ALIGN_CONTENT_SPACE_EVENLY),
        AlignContent::Stretch => Ok(SL_ALIGN_CONTENT_STRETCH),
    }
}

fn map_align_items(align_items: AlignItems) -> Result<SLFlexAlign, CppBaselineError> {
    match align_items {
        AlignItems::Stretch => Ok(SL_FLEX_ALIGN_STRETCH),
        AlignItems::FlexStart => Ok(SL_FLEX_ALIGN_FLEX_START),
        AlignItems::Start => Ok(SL_FLEX_ALIGN_START),
        AlignItems::Center => Ok(SL_FLEX_ALIGN_CENTER),
        AlignItems::FlexEnd => Ok(SL_FLEX_ALIGN_FLEX_END),
        AlignItems::End => Ok(SL_FLEX_ALIGN_END),
        AlignItems::Baseline => Ok(SL_FLEX_ALIGN_BASELINE),
    }
}

fn map_box_sizing(box_sizing: BoxSizing) -> SLBoxSizing {
    match box_sizing {
        BoxSizing::ContentBox => SL_BOX_SIZING_CONTENT_BOX,
        BoxSizing::BorderBox => SL_BOX_SIZING_BORDER_BOX,
    }
}

fn is_standalone_flex_shorthand(style: &Style) -> bool {
    style.flex_shrink == 1.0 && style.flex_basis == Length::ZERO
}

unsafe fn set_flex_basis(node: SLNodeRef, length: Length) {
    // SAFETY: The caller guarantees `node` is a live standalone node. This block
    // only forwards validated style values to the matching C API setter.
    unsafe {
        match length {
            Length::Auto => SLNodeStyleSetFlexBasisAuto(node),
            Length::Points(value) => SLNodeStyleSetFlexBasis(node, value),
            Length::Percent(value) => SLNodeStyleSetFlexBasisPercent(node, value),
            Length::Calc { fixed, percent } => {
                SLNodeStyleSetFlexBasisCalc(node, starlight_value_from_calc_length(fixed, percent));
            }
            Length::Fr(_) => {
                SLNodeStyleSetFlexBasisValue(node, starlight_value_from_length(length))
            }
            Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
            Length::MaxContent => SLNodeStyleSetFlexBasisMaxContent(node),
            Length::FitContent(None) => SLNodeStyleSetFlexBasisFitContent(node),
            Length::FitContent(Some(base)) => {
                SLNodeStyleSetFlexBasisFitContentValue(
                    node,
                    starlight_value_from_fit_content_base(base),
                );
            }
        }
    }
}

unsafe fn set_axis_length(node: SLNodeRef, length: Length, setters: AxisLengthSetters) {
    // SAFETY: The caller guarantees `node` is live and the function pointers
    // come from the standalone C API for the same style field.
    unsafe {
        match length {
            Length::Auto => (setters.auto)(node),
            Length::Points(value) => (setters.points)(node, value),
            Length::Percent(value) => (setters.percent)(node, value),
            Length::Calc { fixed, percent } => {
                (setters.calc)(node, starlight_value_from_calc_length(fixed, percent));
            }
            Length::Fr(_) => (setters.value)(node, starlight_value_from_length(length)),
            Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
            Length::MaxContent => (setters.max_content)(node),
            Length::FitContent(None) => (setters.fit_content)(node),
            Length::FitContent(Some(base)) => {
                (setters.fit_content_value)(node, starlight_value_from_fit_content_base(base));
            }
        }
    }
}

struct AxisLengthSetters {
    points: unsafe extern "C" fn(SLNodeRef, f32),
    percent: unsafe extern "C" fn(SLNodeRef, f32),
    calc: unsafe extern "C" fn(SLNodeRef, StarlightValue),
    value: unsafe extern "C" fn(SLNodeRef, StarlightValue),
    auto: unsafe extern "C" fn(SLNodeRef),
    max_content: unsafe extern "C" fn(SLNodeRef),
    fit_content: unsafe extern "C" fn(SLNodeRef),
    fit_content_value: unsafe extern "C" fn(SLNodeRef, StarlightValue),
}

unsafe fn set_optional_min_length(node: SLNodeRef, length: Length, setters: OptionalLengthSetters) {
    // SAFETY: The caller guarantees `node` is live and the function pointers
    // come from the standalone C API for the same min-size field.
    unsafe {
        match length {
            Length::Auto => {}
            Length::Points(value) => (setters.points)(node, value),
            Length::Percent(value) => (setters.percent)(node, value),
            Length::Calc { fixed, percent } => {
                (setters.calc)(node, starlight_value_from_calc_length(fixed, percent));
            }
            Length::Fr(_) => (setters.value)(node, starlight_value_from_length(length)),
            Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
            Length::MaxContent => (setters.max_content)(node),
            Length::FitContent(None) => (setters.fit_content)(node),
            Length::FitContent(Some(base)) => {
                (setters.fit_content_value)(node, starlight_value_from_fit_content_base(base));
            }
        }
    }
}

unsafe fn set_optional_max_length(node: SLNodeRef, length: Length, setters: OptionalLengthSetters) {
    // SAFETY: The caller guarantees `node` is live and the function pointers
    // come from the standalone C API for the same max-size field.
    unsafe {
        match length {
            Length::Auto => {}
            Length::Points(value) => (setters.points)(node, value),
            Length::Percent(value) => (setters.percent)(node, value),
            Length::Calc { fixed, percent } => {
                (setters.calc)(node, starlight_value_from_calc_length(fixed, percent));
            }
            Length::Fr(_) => (setters.value)(node, starlight_value_from_length(length)),
            Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
            Length::MaxContent => (setters.max_content)(node),
            Length::FitContent(None) => (setters.fit_content)(node),
            Length::FitContent(Some(base)) => {
                (setters.fit_content_value)(node, starlight_value_from_fit_content_base(base));
            }
        }
    }
}

struct OptionalLengthSetters {
    points: unsafe extern "C" fn(SLNodeRef, f32),
    percent: unsafe extern "C" fn(SLNodeRef, f32),
    calc: unsafe extern "C" fn(SLNodeRef, StarlightValue),
    value: unsafe extern "C" fn(SLNodeRef, StarlightValue),
    max_content: unsafe extern "C" fn(SLNodeRef),
    fit_content: unsafe extern "C" fn(SLNodeRef),
    fit_content_value: unsafe extern "C" fn(SLNodeRef, StarlightValue),
}

struct EdgeLengthSetters {
    points: unsafe extern "C" fn(SLNodeRef, SLEdge, f32),
    percent: unsafe extern "C" fn(SLNodeRef, SLEdge, f32),
    calc: unsafe extern "C" fn(SLNodeRef, SLEdge, StarlightValue),
    value: unsafe extern "C" fn(SLNodeRef, SLEdge, StarlightValue),
    auto: Option<unsafe extern "C" fn(SLNodeRef, SLEdge)>,
}

unsafe fn set_edge_lengths(
    node: SLNodeRef,
    left: Length,
    right: Length,
    top: Length,
    bottom: Length,
    setters: EdgeLengthSetters,
) {
    // SAFETY: The caller guarantees `node` is live; `setters` is reused for the
    // four physical edges of the same style field.
    unsafe {
        set_edge_length(node, SL_EDGE_LEFT, left, &setters);
        set_edge_length(node, SL_EDGE_RIGHT, right, &setters);
        set_edge_length(node, SL_EDGE_TOP, top, &setters);
        set_edge_length(node, SL_EDGE_BOTTOM, bottom, &setters);
    }
}

unsafe fn set_edge_length(
    node: SLNodeRef,
    edge: SLEdge,
    length: Length,
    setters: &EdgeLengthSetters,
) {
    // SAFETY: The caller guarantees `node` is live and the setter table matches
    // the style field currently being applied.
    unsafe {
        match length {
            Length::Auto => {
                if let Some(auto) = setters.auto {
                    auto(node, edge);
                }
            }
            Length::Points(value) => (setters.points)(node, edge, value),
            Length::Percent(value) => (setters.percent)(node, edge, value),
            Length::Calc { fixed, percent } => {
                (setters.calc)(node, edge, starlight_value_from_calc_length(fixed, percent));
            }
            Length::Fr(_) | Length::MaxContent | Length::FitContent(_) => {
                (setters.value)(node, edge, starlight_value_from_length(length));
            }
            Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
        }
    }
}

unsafe fn set_edge_values(
    node: SLNodeRef,
    edges: Edges,
    setter: unsafe extern "C" fn(SLNodeRef, SLEdge, f32),
) {
    // SAFETY: The caller guarantees `node` is live and `setter` is a standalone
    // C API edge setter for raw numeric edge values.
    unsafe {
        setter(node, SL_EDGE_LEFT, edges.left);
        setter(node, SL_EDGE_RIGHT, edges.right);
        setter(node, SL_EDGE_TOP, edges.top);
        setter(node, SL_EDGE_BOTTOM, edges.bottom);
    }
}

unsafe fn set_gap(node: SLNodeRef, gap: SLGap, length: Length) {
    // SAFETY: The caller guarantees `node` is live. Gap values are validated
    // before style application and then forwarded to the matching C API setter.
    unsafe {
        match length {
            Length::Auto => {}
            Length::Points(value) => SLNodeStyleSetGap(node, gap, value),
            Length::Percent(value) => SLNodeStyleSetGapPercent(node, gap, value),
            Length::Calc { fixed, percent } => {
                SLNodeStyleSetGapCalc(node, gap, starlight_value_from_calc_length(fixed, percent))
            }
            Length::Fr(_) | Length::MaxContent | Length::FitContent(_) => {
                SLNodeStyleSetGapValue(node, gap, starlight_value_from_length(length));
            }
            Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
        }
    }
}

unsafe fn set_list_gap(
    node: SLNodeRef,
    length: Length,
    setter: unsafe extern "C" fn(SLNodeRef, f32),
    percent_setter: unsafe extern "C" fn(SLNodeRef, f32),
    calc_setter: unsafe extern "C" fn(SLNodeRef, StarlightValue),
    value_setter: unsafe extern "C" fn(SLNodeRef, StarlightValue),
) {
    // SAFETY: The caller guarantees `node` is live and each function pointer is
    // the standalone C API setter for the same list gap field.
    unsafe {
        match length {
            Length::Points(value) => setter(node, value),
            Length::Percent(value) => percent_setter(node, value),
            Length::Calc { fixed, percent } => {
                calc_setter(node, starlight_value_from_calc_length(fixed, percent));
            }
            Length::Auto | Length::Fr(_) | Length::MaxContent | Length::FitContent(_) => {
                value_setter(node, starlight_value_from_length(length));
            }
            Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
        }
    }
}

unsafe fn apply_public_list_gap_variant(
    node: NonNull<StarlightNode>,
    variant: PublicListGapVariant,
) {
    // SAFETY: The caller guarantees `node` is live. Each branch calls matching
    // public list-gap setters on the same standalone node.
    unsafe {
        match variant {
            PublicListGapVariant::Points => {
                SLNodeStyleSetListMainAxisGap(node.as_ptr(), 4.0);
                SLNodeStyleSetListCrossAxisGap(node.as_ptr(), 12.0);
            }
            PublicListGapVariant::Percent => {
                SLNodeStyleSetListMainAxisGapPercent(node.as_ptr(), 5.0);
                SLNodeStyleSetListCrossAxisGapPercent(node.as_ptr(), 10.0);
            }
            PublicListGapVariant::Calc => {
                SLNodeStyleSetListMainAxisGapCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(6.0, 50.0),
                );
                SLNodeStyleSetListCrossAxisGapCalc(
                    node.as_ptr(),
                    starlight_value_from_calc_length(12.0, 50.0),
                );
            }
            PublicListGapVariant::ValueAuto => {
                SLNodeStyleSetListMainAxisGapValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::Auto),
                );
                SLNodeStyleSetListCrossAxisGapValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::Auto),
                );
            }
            PublicListGapVariant::ValueFr => {
                SLNodeStyleSetListMainAxisGapValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(3.0)),
                );
                SLNodeStyleSetListCrossAxisGapValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fr(12.0)),
                );
            }
            PublicListGapVariant::ValueMaxContent => {
                SLNodeStyleSetListMainAxisGapValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::max_content()),
                );
                SLNodeStyleSetListCrossAxisGapValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::max_content()),
                );
            }
            PublicListGapVariant::ValueFitContent => {
                SLNodeStyleSetListMainAxisGapValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fit_content(Some(
                        BaseLength::fixed_and_percent(7.0, 8.0),
                    ))),
                );
                SLNodeStyleSetListCrossAxisGapValue(
                    node.as_ptr(),
                    starlight_value_from_length(Length::fit_content(Some(
                        BaseLength::fixed_and_percent(14.0, 16.0),
                    ))),
                );
            }
        }
    }
}

unsafe fn set_grid_track_vector(
    node: SLNodeRef,
    lengths: &[Length],
    setter: unsafe extern "C" fn(SLNodeRef, *const StarlightValue, i32),
) -> Result<(), CppBaselineError> {
    let count = i32::try_from(lengths.len()).map_err(|_| {
        CppBaselineError::UnsupportedStyle("grid track count exceeds standalone C API i32 range")
    })?;
    let values = lengths
        .iter()
        .copied()
        .map(starlight_value_from_grid_track_length)
        .collect::<Vec<_>>();
    // SAFETY: The caller guarantees `node` is live. `values` remains alive for
    // the duration of this synchronous C API call.
    unsafe {
        setter(node, values.as_ptr(), count);
    }
    Ok(())
}

fn effective_grid_max_tracks(min_tracks: &[Length], max_tracks: &[Length]) -> Vec<Length> {
    min_tracks
        .iter()
        .copied()
        .enumerate()
        .map(|(idx, min_track)| max_tracks.get(idx).copied().unwrap_or(min_track))
        .collect()
}

fn starlight_value_from_grid_track_length(length: Length) -> StarlightValue {
    match length {
        Length::FitContent(Some(base)) => {
            assert!(
                base.has_value(),
                "grid track fit-content requires a <length-percentage> argument"
            );
            starlight_value_from_base_length(SL_UNIT_FIT_CONTENT, base)
        }
        Length::FitContent(None) => {
            unreachable!("grid track fit-content requires a <length-percentage> argument")
        }
        _ => starlight_value_from_length(length),
    }
}

fn starlight_value_from_length(length: Length) -> StarlightValue {
    match length {
        Length::Auto => StarlightValue {
            value: 0.0,
            unit: SL_UNIT_AUTO,
            percentage: 0.0,
            flags: 0,
        },
        Length::Points(value) => StarlightValue {
            value,
            unit: SL_UNIT_POINT,
            percentage: 0.0,
            flags: SL_VALUE_FLAG_HAS_VALUE,
        },
        Length::Percent(value) => StarlightValue {
            value,
            unit: SL_UNIT_PERCENT,
            percentage: value,
            flags: SL_VALUE_FLAG_HAS_VALUE | SL_VALUE_FLAG_HAS_PERCENTAGE,
        },
        Length::Calc { fixed, percent } => StarlightValue {
            value: fixed,
            unit: SL_UNIT_CALC,
            percentage: percent,
            flags: SL_VALUE_FLAG_HAS_VALUE | SL_VALUE_FLAG_HAS_PERCENTAGE,
        },
        Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
        Length::MaxContent => StarlightValue {
            value: 0.0,
            unit: SL_UNIT_MAX_CONTENT,
            percentage: 0.0,
            flags: 0,
        },
        Length::FitContent(None) => StarlightValue {
            value: 0.0,
            unit: SL_UNIT_FIT_CONTENT,
            percentage: 0.0,
            flags: 0,
        },
        Length::FitContent(Some(base)) => {
            starlight_value_from_base_length(SL_UNIT_FIT_CONTENT, base)
        }
        Length::Fr(value) => StarlightValue {
            value,
            unit: SL_UNIT_FR,
            percentage: 0.0,
            flags: SL_VALUE_FLAG_HAS_VALUE,
        },
    }
}

fn starlight_value_from_calc_length(fixed: f32, percent: f32) -> StarlightValue {
    starlight_value_from_base_length(SL_UNIT_CALC, BaseLength::fixed_and_percent(fixed, percent))
}

fn starlight_value_from_fit_content_base(base: BaseLength) -> StarlightValue {
    starlight_value_from_base_length(SL_UNIT_FIT_CONTENT, base)
}

fn starlight_value_from_base_length(unit: SLUnit, base: BaseLength) -> StarlightValue {
    let mut flags = 0;
    if base.has_value() {
        flags |= SL_VALUE_FLAG_HAS_VALUE;
    }
    if base.contains_percentage() {
        flags |= SL_VALUE_FLAG_HAS_PERCENTAGE;
    }
    StarlightValue {
        value: base.fixed_part(),
        unit,
        percentage: base.percentage_part(),
        flags,
    }
}

fn read_native_size(ptr: NonNull<StarlightNode>) -> Size {
    // SAFETY: `ptr` is a live native node after layout.
    unsafe {
        Size::new(
            SLNodeLayoutGetWidth(ptr.as_ptr()),
            SLNodeLayoutGetHeight(ptr.as_ptr()),
        )
    }
}

fn read_native_layout(ptr: NonNull<StarlightNode>) -> LayoutResult {
    // SAFETY: `ptr` is a live native node after layout.
    unsafe {
        LayoutResult {
            offset: Point::new(
                SLNodeLayoutGetLeft(ptr.as_ptr()),
                SLNodeLayoutGetTop(ptr.as_ptr()),
            ),
            size: read_native_size(ptr),
            baseline: read_native_baseline(ptr),
            padding: read_edges(ptr, SLNodeLayoutGetPadding),
            border: read_edges(ptr, SLNodeLayoutGetBorder),
            margin: read_edges(ptr, SLNodeLayoutGetMargin),
            sticky_pos: read_edges(ptr, SLNodeLayoutGetStickyPosition),
        }
    }
}

fn read_native_baseline(ptr: NonNull<StarlightNode>) -> Option<f32> {
    // SAFETY: `ptr` is a live native node after layout.
    let baseline = unsafe { SLNodeLayoutGetBaseline(ptr.as_ptr()) };
    let height = read_native_size(ptr).height;
    native_baseline_to_result(baseline, height)
}

fn native_baseline_to_result(baseline: f32, height: f32) -> Option<f32> {
    (baseline.abs() > f32::EPSILON && (baseline - height).abs() > f32::EPSILON).then_some(baseline)
}

unsafe fn read_edges(
    ptr: NonNull<StarlightNode>,
    getter: unsafe extern "C" fn(SLNodeRef, SLEdge) -> f32,
) -> Edges {
    // SAFETY: The caller guarantees `ptr` is a live native node after layout,
    // and `getter` is one of the standalone layout edge accessors.
    unsafe {
        Edges::new(
            getter(ptr.as_ptr(), SL_EDGE_LEFT),
            getter(ptr.as_ptr(), SL_EDGE_RIGHT),
            getter(ptr.as_ptr(), SL_EDGE_TOP),
            getter(ptr.as_ptr(), SL_EDGE_BOTTOM),
        )
    }
}

#[cfg(test)]
mod tests {
    use crate::CppBaselineError;
    use std::collections::{BTreeMap, BTreeSet};
    #[cfg(starlight_cpp_native_standalone)]
    use std::ffi::c_void;
    use std::fs;
    use std::path::Path;
    #[cfg(starlight_cpp_native_standalone)]
    use std::ptr::NonNull;
    #[cfg(starlight_cpp_native_standalone)]
    use std::sync::atomic::{AtomicUsize, Ordering};

    use super::{
        axis_length_is_standalone_supported, ensure_standalone_lengths_supported,
        ensure_standalone_supported, flex_basis_length_is_standalone_supported,
        gap_length_is_standalone_supported, grid_track_length_is_standalone_supported,
        length_is_standalone_supported, list_gap_length_is_standalone_supported, map_align_content,
        map_align_items, map_box_sizing, map_direction, map_display, map_flex_direction,
        map_flex_wrap, map_grid_auto_flow, map_justify_content, map_justify_item,
        map_linear_cross_gravity, map_linear_gravity, map_linear_layout_gravity,
        map_linear_orientation, map_list_component_type, map_position, map_relative_center,
        min_max_length_is_standalone_supported, native_baseline_to_result,
        owner_constraint_to_native, starlight_value_from_calc_length,
        starlight_value_from_grid_track_length, uses_default_physical_pixels_per_layout_unit,
        SL_ALIGN_CONTENT_FLEX_END, SL_ALIGN_CONTENT_FLEX_START, SL_ALIGN_CONTENT_SPACE_EVENLY,
        SL_BOX_SIZING_BORDER_BOX, SL_BOX_SIZING_CONTENT_BOX, SL_DIRECTION_LTR,
        SL_DIRECTION_LYNX_RTL, SL_DIRECTION_NORMAL, SL_DIRECTION_RTL, SL_DISPLAY_BLOCK,
        SL_DISPLAY_FLEX, SL_DISPLAY_GRID, SL_DISPLAY_LINEAR, SL_DISPLAY_NONE, SL_DISPLAY_RELATIVE,
        SL_FLEX_ALIGN_END, SL_FLEX_ALIGN_START, SL_FLEX_DIRECTION_COLUMN,
        SL_FLEX_DIRECTION_COLUMN_REVERSE, SL_FLEX_DIRECTION_ROW, SL_FLEX_DIRECTION_ROW_REVERSE,
        SL_FLEX_WRAP_NOWRAP, SL_FLEX_WRAP_WRAP, SL_FLEX_WRAP_WRAP_REVERSE,
        SL_GRID_AUTO_FLOW_COLUMN, SL_GRID_AUTO_FLOW_COLUMN_DENSE, SL_GRID_AUTO_FLOW_DENSE,
        SL_GRID_AUTO_FLOW_ROW, SL_GRID_AUTO_FLOW_ROW_DENSE, SL_JUSTIFY_CONTENT_END,
        SL_JUSTIFY_CONTENT_START, SL_JUSTIFY_ITEM_AUTO, SL_JUSTIFY_ITEM_CENTER,
        SL_JUSTIFY_ITEM_END, SL_JUSTIFY_ITEM_START, SL_JUSTIFY_ITEM_STRETCH,
        SL_LINEAR_CROSS_GRAVITY_CENTER, SL_LINEAR_CROSS_GRAVITY_END, SL_LINEAR_CROSS_GRAVITY_NONE,
        SL_LINEAR_CROSS_GRAVITY_START, SL_LINEAR_CROSS_GRAVITY_STRETCH, SL_LINEAR_GRAVITY_BOTTOM,
        SL_LINEAR_GRAVITY_CENTER, SL_LINEAR_GRAVITY_CENTER_HORIZONTAL,
        SL_LINEAR_GRAVITY_CENTER_VERTICAL, SL_LINEAR_GRAVITY_END, SL_LINEAR_GRAVITY_LEFT,
        SL_LINEAR_GRAVITY_NONE, SL_LINEAR_GRAVITY_RIGHT, SL_LINEAR_GRAVITY_SPACE_BETWEEN,
        SL_LINEAR_GRAVITY_START, SL_LINEAR_GRAVITY_TOP, SL_LINEAR_LAYOUT_GRAVITY_BOTTOM,
        SL_LINEAR_LAYOUT_GRAVITY_CENTER, SL_LINEAR_LAYOUT_GRAVITY_CENTER_HORIZONTAL,
        SL_LINEAR_LAYOUT_GRAVITY_CENTER_VERTICAL, SL_LINEAR_LAYOUT_GRAVITY_END,
        SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL, SL_LINEAR_LAYOUT_GRAVITY_FILL_VERTICAL,
        SL_LINEAR_LAYOUT_GRAVITY_LEFT, SL_LINEAR_LAYOUT_GRAVITY_NONE,
        SL_LINEAR_LAYOUT_GRAVITY_RIGHT, SL_LINEAR_LAYOUT_GRAVITY_START,
        SL_LINEAR_LAYOUT_GRAVITY_STRETCH, SL_LINEAR_LAYOUT_GRAVITY_TOP,
        SL_LINEAR_ORIENTATION_COLUMN, SL_LINEAR_ORIENTATION_COLUMN_REVERSE,
        SL_LINEAR_ORIENTATION_HORIZONTAL, SL_LINEAR_ORIENTATION_HORIZONTAL_REVERSE,
        SL_LINEAR_ORIENTATION_ROW, SL_LINEAR_ORIENTATION_ROW_REVERSE,
        SL_LINEAR_ORIENTATION_VERTICAL, SL_LINEAR_ORIENTATION_VERTICAL_REVERSE,
        SL_LIST_COMPONENT_TYPE_DEFAULT, SL_LIST_COMPONENT_TYPE_FOOTER,
        SL_LIST_COMPONENT_TYPE_HEADER, SL_LIST_COMPONENT_TYPE_LIST_ROW,
        SL_NODE_MEASURE_MODE_AT_MOST, SL_NODE_MEASURE_MODE_EXACTLY, SL_NODE_MEASURE_MODE_UNDEFINED,
        SL_POSITION_TYPE_ABSOLUTE, SL_POSITION_TYPE_FIXED, SL_POSITION_TYPE_RELATIVE,
        SL_POSITION_TYPE_STICKY, SL_RELATIVE_CENTER_BOTH, SL_RELATIVE_CENTER_HORIZONTAL,
        SL_RELATIVE_CENTER_NONE, SL_RELATIVE_CENTER_VERTICAL, SL_UNDEFINED, SL_UNIT_CALC,
        SL_UNIT_FIT_CONTENT, SL_VALUE_FLAG_HAS_PERCENTAGE, SL_VALUE_FLAG_HAS_VALUE,
    };
    use starlight_layout::{
        AlignContent, AlignItems, BaseLength, BoxSizing, Direction, Display, FlexDirection,
        FlexWrap, GridAutoFlow, JustifyContent, JustifyItems, Length, LinearCrossGravity,
        LinearGravity, LinearLayoutGravity, LinearOrientation, ListComponentType, PositionType,
        Rect, RelativeCenter, SideConstraint, Style, Visibility,
    };
    #[cfg(starlight_cpp_native_standalone)]
    use starlight_layout::{Constraints, LayoutEngine, MeasureMode, Size};

    const STYLE_SOURCE: &str = include_str!("../../starlight_layout/src/style.rs");
    const NATIVE_SOURCE: &str = include_str!("native.rs");
    const LATEST_QUIRKS_VERSION: &str = "kNegativePaddingFixedVersion";
    const EXEMPTED_STANDALONE_STYLE_SETTERS: &[(&str, &str)] = &[];
    const EXEMPTED_STANDALONE_ENUM_VARIANTS: &[(&str, &str)] = &[
        (
            "SLHorizontal",
            "dimension enum is not part of the imported layout/style call surface",
        ),
        (
            "SLVertical",
            "dimension enum is not part of the imported layout/style call surface",
        ),
        (
            "SLDimensionCount",
            "dimension enum is not part of the imported layout/style call surface",
        ),
    ];

    #[cfg(starlight_cpp_native_standalone)]
    struct NativeApiTree {
        nodes: Vec<NonNull<super::StarlightNode>>,
    }

    #[cfg(starlight_cpp_native_standalone)]
    struct NativeApiConfig {
        ptr: NonNull<super::StarlightConfig>,
    }

    #[cfg(starlight_cpp_native_standalone)]
    impl NativeApiConfig {
        fn new() -> Self {
            // SAFETY: `SLConfigCreate` returns a newly allocated standalone
            // config or null on allocation failure.
            let ptr = NonNull::new(unsafe { super::SLConfigCreate() })
                .expect("SLConfigCreate should allocate a standalone config");
            Self { ptr }
        }

        fn set_physical_pixels_per_layout_unit(&mut self, value: f32) {
            // SAFETY: `self.ptr` is a live config owned by this wrapper.
            unsafe { super::SLConfigSetPhysicalPixelsPerLayoutUnit(self.ptr.as_ptr(), value) };
        }

        fn physical_pixels_per_layout_unit(&self) -> f32 {
            // SAFETY: `self.ptr` is a live config owned by this wrapper.
            unsafe { super::SLConfigGetPhysicalPixelsPerLayoutUnit(self.ptr.as_ptr()) }
        }

        fn default_physical_pixels_per_layout_unit() -> f32 {
            // SAFETY: the public C API explicitly treats a null config as the
            // default standalone config.
            unsafe { super::SLConfigGetPhysicalPixelsPerLayoutUnit(std::ptr::null_mut()) }
        }
    }

    #[cfg(starlight_cpp_native_standalone)]
    impl Drop for NativeApiConfig {
        fn drop(&mut self) {
            // SAFETY: `self.ptr` is owned by this wrapper and freed exactly once.
            unsafe { super::SLConfigFree(self.ptr.as_ptr()) };
        }
    }

    #[cfg(starlight_cpp_native_standalone)]
    impl NativeApiTree {
        fn new() -> Self {
            Self { nodes: Vec::new() }
        }

        fn create_node(&mut self) -> NonNull<super::StarlightNode> {
            // SAFETY: `SLNodeNew` returns a newly allocated standalone node or
            // null on allocation failure. `NativeApiTree` owns successful
            // pointers until `Drop`.
            let node = NonNull::new(unsafe { super::SLNodeNew() })
                .expect("SLNodeNew should allocate a standalone node");
            self.nodes.push(node);
            node
        }

        fn create_node_with_config(
            &mut self,
            config: &NativeApiConfig,
        ) -> NonNull<super::StarlightNode> {
            // SAFETY: `config.ptr` is live for this call. The standalone API
            // copies config values into the newly allocated node style.
            let node = NonNull::new(unsafe { super::SLNodeNewWithConfig(config.ptr.as_ptr()) })
                .expect("SLNodeNewWithConfig should allocate a standalone node");
            self.nodes.push(node);
            node
        }

        fn insert_child(
            &self,
            parent: NonNull<super::StarlightNode>,
            child: NonNull<super::StarlightNode>,
            index: i32,
        ) {
            // SAFETY: both pointers are live nodes owned by this test tree.
            unsafe { super::SLNodeInsertChild(parent.as_ptr(), child.as_ptr(), index) };
        }

        fn insert_child_before(
            &self,
            parent: NonNull<super::StarlightNode>,
            child: NonNull<super::StarlightNode>,
            reference: NonNull<super::StarlightNode>,
        ) {
            self.insert_child_before_or_append(parent, child, Some(reference));
        }

        fn insert_child_before_or_append(
            &self,
            parent: NonNull<super::StarlightNode>,
            child: NonNull<super::StarlightNode>,
            reference: Option<NonNull<super::StarlightNode>>,
        ) {
            let reference_ptr = reference.map_or(std::ptr::null_mut(), NonNull::as_ptr);
            // SAFETY: parent/child are live nodes owned by this test tree. A
            // null reference is the public standalone append path.
            unsafe {
                super::SLNodeInsertChildBefore(parent.as_ptr(), child.as_ptr(), reference_ptr)
            };
        }

        fn remove_child(
            &self,
            parent: NonNull<super::StarlightNode>,
            child: NonNull<super::StarlightNode>,
        ) {
            // SAFETY: both pointers are live nodes owned by this test tree.
            unsafe { super::SLNodeRemoveChild(parent.as_ptr(), child.as_ptr()) };
        }

        fn remove_all_children(&self, parent: NonNull<super::StarlightNode>) {
            // SAFETY: `parent` is a live node owned by this test tree.
            unsafe { super::SLNodeRemoveAllChildren(parent.as_ptr()) };
        }

        fn reset(&self, node: NonNull<super::StarlightNode>) {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeReset(node.as_ptr()) };
        }

        fn set_measure_delegate(
            &self,
            node: NonNull<super::StarlightNode>,
            delegate: &mut super::StarlightMeasureDelegate,
        ) {
            // SAFETY: `node` is live and `delegate` is kept alive by the caller
            // until layout finishes.
            unsafe { super::SLNodeSetMeasureDelegate(node.as_ptr(), delegate) };
        }

        fn measure_delegate(
            &self,
            node: NonNull<super::StarlightNode>,
        ) -> *mut super::StarlightMeasureDelegate {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeGetMeasureDelegate(node.as_ptr()) }
        }

        fn has_measure_func(&self, node: NonNull<super::StarlightNode>) -> bool {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeHasMeasureFunc(node.as_ptr()) }
        }

        fn child_count(&self, node: NonNull<super::StarlightNode>) -> i32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeGetChildCount(node.as_ptr()) }
        }

        fn child_at(&self, node: NonNull<super::StarlightNode>, index: i32) -> super::SLNodeRef {
            // SAFETY: `node` is a live node owned by this test tree. The C API
            // returns null for an out-of-range child.
            unsafe { super::SLNodeGetChild(node.as_ptr(), index) }
        }

        fn parent(&self, node: NonNull<super::StarlightNode>) -> super::SLNodeRef {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeGetParent(node.as_ptr()) }
        }

        fn is_dirty(&self, node: NonNull<super::StarlightNode>) -> bool {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeIsDirty(node.as_ptr()) }
        }

        fn mark_dirty(&self, node: NonNull<super::StarlightNode>) {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeMarkDirty(node.as_ptr()) };
        }

        fn is_rtl(&self, node: NonNull<super::StarlightNode>) -> bool {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeIsRTL(node.as_ptr()) }
        }

        fn calculate_layout(
            &self,
            node: NonNull<super::StarlightNode>,
            width: f32,
            height: f32,
            direction: super::SLDirection,
        ) {
            // SAFETY: `node` is a live root node owned by this test tree.
            unsafe { super::SLNodeCalculateLayout(node.as_ptr(), width, height, direction) };
        }

        fn layout_width(&self, node: NonNull<super::StarlightNode>) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetWidth(node.as_ptr()) }
        }

        fn layout_height(&self, node: NonNull<super::StarlightNode>) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetHeight(node.as_ptr()) }
        }

        fn layout_left(&self, node: NonNull<super::StarlightNode>) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetLeft(node.as_ptr()) }
        }

        fn layout_top(&self, node: NonNull<super::StarlightNode>) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetTop(node.as_ptr()) }
        }

        fn layout_baseline(&self, node: NonNull<super::StarlightNode>) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetBaseline(node.as_ptr()) }
        }

        fn layout_margin(&self, node: NonNull<super::StarlightNode>, edge: super::SLEdge) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetMargin(node.as_ptr(), edge) }
        }

        fn layout_padding(&self, node: NonNull<super::StarlightNode>, edge: super::SLEdge) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetPadding(node.as_ptr(), edge) }
        }

        fn layout_border(&self, node: NonNull<super::StarlightNode>, edge: super::SLEdge) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetBorder(node.as_ptr(), edge) }
        }

        fn layout_sticky_position(
            &self,
            node: NonNull<super::StarlightNode>,
            edge: super::SLEdge,
        ) -> f32 {
            // SAFETY: `node` is a live node owned by this test tree.
            unsafe { super::SLNodeLayoutGetStickyPosition(node.as_ptr(), edge) }
        }
    }

    #[cfg(starlight_cpp_native_standalone)]
    impl Drop for NativeApiTree {
        fn drop(&mut self) {
            let roots = self
                .nodes
                .iter()
                .copied()
                .filter(|node| {
                    // SAFETY: all pointers in `self.nodes` are live until roots
                    // are collected and freed below.
                    unsafe { super::SLNodeGetParent(node.as_ptr()).is_null() }
                })
                .collect::<Vec<_>>();
            for root in roots {
                // SAFETY: each collected node has no parent, so recursive frees
                // cover disjoint native subtrees.
                unsafe { super::SLNodeFreeRecursive(root.as_ptr()) };
            }
        }
    }

    #[cfg(starlight_cpp_native_standalone)]
    extern "C" fn native_api_fixed_measure(
        _manager_node: *mut c_void,
        _width: f32,
        _width_mode: super::SLNodeMeasureMode,
        _height: f32,
        _height_mode: super::SLNodeMeasureMode,
    ) -> super::StarlightSize {
        super::StarlightSize {
            width: 11.0,
            height: 7.0,
        }
    }

    #[cfg(starlight_cpp_native_standalone)]
    extern "C" fn native_api_counting_measure(
        manager_node: *mut c_void,
        _width: f32,
        _width_mode: super::SLNodeMeasureMode,
        _height: f32,
        _height_mode: super::SLNodeMeasureMode,
    ) -> super::StarlightSize {
        let counter = manager_node.cast::<AtomicUsize>();
        // SAFETY: tests pass a valid `AtomicUsize` pointer through
        // `manager_node` for the lifetime of the native layout call.
        unsafe { counter.as_ref() }
            .expect("counting measure counter")
            .fetch_add(1, Ordering::Relaxed);
        super::StarlightSize {
            width: 11.0,
            height: 7.0,
        }
    }

    #[cfg(starlight_cpp_native_standalone)]
    extern "C" fn native_api_fractional_measure(
        _manager_node: *mut c_void,
        _width: f32,
        _width_mode: super::SLNodeMeasureMode,
        _height: f32,
        _height_mode: super::SLNodeMeasureMode,
    ) -> super::StarlightSize {
        super::StarlightSize {
            width: 10.2,
            height: 4.2,
        }
    }

    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_constraint_sensitive_measure(constraints: Constraints) -> Size {
        let width = match constraints.width.mode {
            MeasureMode::AtMost => constraints.width.size - 3.0,
            MeasureMode::Definite => constraints.width.size + 20.0,
            MeasureMode::Indefinite => 13.0,
        };
        let height = match constraints.height.mode {
            MeasureMode::AtMost => constraints.height.size - 5.0,
            MeasureMode::Definite => constraints.height.size + 30.0,
            MeasureMode::Indefinite => 11.0,
        };
        Size::new(width, height)
    }

    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_baseline_from_content_size(content_size: Size) -> f32 {
        content_size.height - 2.0
    }

    #[cfg(starlight_cpp_native_standalone)]
    fn assert_node_eq(actual: super::SLNodeRef, expected: NonNull<super::StarlightNode>) {
        assert_eq!(actual, expected.as_ptr());
    }

    #[cfg(starlight_cpp_native_standalone)]
    fn assert_value_unit(value: super::StarlightValue, unit: super::SLUnit) {
        assert_eq!(value.unit, unit);
    }

    #[cfg(starlight_cpp_native_standalone)]
    fn assert_value_scalar(value: super::StarlightValue, unit: super::SLUnit, expected: f32) {
        assert_eq!(value.unit, unit);
        assert_close(value.value, expected);
    }

    #[cfg(starlight_cpp_native_standalone)]
    fn assert_value_parts(
        value: super::StarlightValue,
        unit: super::SLUnit,
        expected_value: f32,
        expected_percentage: f32,
        expected_flags: i32,
    ) {
        assert_eq!(value.unit, unit);
        assert_eq!(value.flags, expected_flags);
        assert_close(value.value, expected_value);
        assert_close(value.percentage, expected_percentage);
    }

    #[cfg(starlight_cpp_native_standalone)]
    fn assert_value_eq(actual: super::StarlightValue, expected: super::StarlightValue) {
        assert_eq!(actual.unit, expected.unit);
        assert_eq!(actual.flags, expected.flags);
        assert_close(actual.value, expected.value);
        assert_close(actual.percentage, expected.percentage);
    }

    #[cfg(starlight_cpp_native_standalone)]
    fn assert_close(actual: f32, expected: f32) {
        assert!(
            (actual - expected).abs() <= 0.001,
            "expected {actual} to be close to {expected}"
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_node_free_api_is_callable_for_detached_node() {
        // SAFETY: `SLNodeNew` returns an owned detached standalone node. It is
        // freed exactly once by `SLNodeFree` below and is not touched again.
        let node = NonNull::new(unsafe { super::SLNodeNew() })
            .expect("SLNodeNew should allocate a detached standalone node");
        // SAFETY: `node` is detached and owned by this test.
        unsafe { super::SLNodeFree(node.as_ptr()) };
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_config_physical_pixels_per_layout_unit_matches_rust_standalone() {
        assert_close(
            NativeApiConfig::default_physical_pixels_per_layout_unit(),
            1.0,
        );
        let mut native_config = NativeApiConfig::new();
        assert_close(native_config.physical_pixels_per_layout_unit(), 1.0);
        native_config.set_physical_pixels_per_layout_unit(2.0);
        assert_close(native_config.physical_pixels_per_layout_unit(), 2.0);

        let mut native_tree = NativeApiTree::new();
        let native_default = native_tree.create_node();
        let native_configured = native_tree.create_node_with_config(&native_config);
        let mut default_delegate = super::StarlightMeasureDelegate {
            measure_func: Some(native_api_fractional_measure),
            baseline_func: None,
            manager_node: std::ptr::null_mut(),
        };
        let mut configured_delegate = super::StarlightMeasureDelegate {
            measure_func: Some(native_api_fractional_measure),
            baseline_func: None,
            manager_node: std::ptr::null_mut(),
        };
        // SAFETY: nodes and delegates stay live for the layout calls below.
        unsafe {
            super::SLNodeSetMeasureDelegate(native_default.as_ptr(), &mut default_delegate);
            super::SLNodeSetMeasureDelegate(native_configured.as_ptr(), &mut configured_delegate);
        }
        native_tree.calculate_layout(
            native_default,
            super::SL_UNDEFINED,
            super::SL_UNDEFINED,
            super::SL_DIRECTION_LTR,
        );
        native_tree.calculate_layout(
            native_configured,
            super::SL_UNDEFINED,
            super::SL_UNDEFINED,
            super::SL_DIRECTION_LTR,
        );
        assert_close(native_tree.layout_width(native_default), 11.0);
        assert_close(native_tree.layout_height(native_default), 5.0);
        assert_close(native_tree.layout_width(native_configured), 10.5);
        assert_close(native_tree.layout_height(native_configured), 4.5);

        let mut rust_default_tree = starlight_standalone::StandaloneTree::new();
        let rust_default = rust_default_tree.create_default_node();
        rust_default_tree
            .set_measured_size(rust_default, Some(Size::new(10.2, 4.2)))
            .expect("set rust default measured size");
        rust_default_tree
            .calculate_layout_with_mode(rust_default, Constraints::indefinite(), Direction::Ltr)
            .expect("layout rust default");
        assert_close(
            rust_default_tree
                .layout_width(rust_default)
                .expect("rust default width"),
            native_tree.layout_width(native_default),
        );
        assert_close(
            rust_default_tree
                .layout_height(rust_default)
                .expect("rust default height"),
            native_tree.layout_height(native_default),
        );

        let mut rust_configured_tree = starlight_standalone::StandaloneTree::new();
        let rust_configured = rust_configured_tree.create_default_node_with_config(
            starlight_standalone::StandaloneConfig::with_physical_pixels_per_layout_unit(2.0),
        );
        rust_configured_tree
            .set_measured_size(rust_configured, Some(Size::new(10.2, 4.2)))
            .expect("set rust configured measured size");
        rust_configured_tree
            .calculate_layout_with_mode(rust_configured, Constraints::indefinite(), Direction::Ltr)
            .expect("layout rust configured");
        assert_close(
            rust_configured_tree
                .layout_width(rust_configured)
                .expect("rust configured width"),
            native_tree.layout_width(native_configured),
        );
        assert_close(
            rust_configured_tree
                .layout_height(rust_configured)
                .expect("rust configured height"),
            native_tree.layout_height(native_configured),
        );
    }

    #[test]
    fn invalid_physical_pixels_per_layout_unit_uses_default_rounding() {
        assert!(uses_default_physical_pixels_per_layout_unit(0.0));
        assert!(uses_default_physical_pixels_per_layout_unit(-2.0));
        assert!(uses_default_physical_pixels_per_layout_unit(f32::NAN));
        assert!(uses_default_physical_pixels_per_layout_unit(f32::INFINITY));
        assert!(uses_default_physical_pixels_per_layout_unit(
            f32::NEG_INFINITY
        ));
        assert!(uses_default_physical_pixels_per_layout_unit(1.0));
        assert!(!uses_default_physical_pixels_per_layout_unit(2.0));
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn baseline_import_defaults_invalid_physical_pixels_per_layout_unit_like_rust_standalone() {
        for physical_pixels_per_layout_unit in
            [0.0, -2.0, f32::NAN, f32::INFINITY, f32::NEG_INFINITY]
        {
            let mut rust_tree = starlight_standalone::StandaloneTree::new();
            let rust_root = rust_tree.create_default_node_with_config(
                starlight_standalone::StandaloneConfig::with_physical_pixels_per_layout_unit(
                    physical_pixels_per_layout_unit,
                ),
            );
            rust_tree
                .set_measured_size(rust_root, Some(Size::new(10.2, 4.2)))
                .expect("set rust measured size");
            let rust_size =
                LayoutEngine::new().layout(&mut rust_tree, rust_root, Constraints::indefinite());

            let mut cpp_tree = starlight_standalone::StandaloneTree::new();
            let cpp_root = cpp_tree.create_default_node_with_config(
                starlight_standalone::StandaloneConfig::with_physical_pixels_per_layout_unit(
                    physical_pixels_per_layout_unit,
                ),
            );
            cpp_tree
                .set_measured_size(cpp_root, Some(Size::new(10.2, 4.2)))
                .expect("set C++ baseline measured size");
            let cpp_size =
                super::layout_standalone(&mut cpp_tree, cpp_root, Constraints::indefinite())
                    .expect("C++ baseline layout");

            assert_close(rust_size.width, 11.0);
            assert_close(rust_size.height, 5.0);
            assert_close(cpp_size.width, rust_size.width);
            assert_close(cpp_size.height, rust_size.height);
            assert_close(
                cpp_tree.layout_width(cpp_root).expect("C++ baseline width"),
                rust_tree.layout_width(rust_root).expect("Rust width"),
            );
            assert_close(
                cpp_tree
                    .layout_height(cpp_root)
                    .expect("C++ baseline height"),
                rust_tree.layout_height(rust_root).expect("Rust height"),
            );
        }
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_reset_preserves_physical_pixel_config_for_measured_rounding() {
        let mut native_config = NativeApiConfig::new();
        native_config.set_physical_pixels_per_layout_unit(2.0);
        let mut native_tree = NativeApiTree::new();
        let native_root = native_tree.create_node_with_config(&native_config);
        native_tree.reset(native_root);
        let mut native_delegate = super::StarlightMeasureDelegate {
            measure_func: Some(native_api_fractional_measure),
            baseline_func: None,
            manager_node: std::ptr::null_mut(),
        };
        native_tree.set_measure_delegate(native_root, &mut native_delegate);
        native_tree.calculate_layout(
            native_root,
            super::SL_UNDEFINED,
            super::SL_UNDEFINED,
            super::SL_DIRECTION_LTR,
        );
        assert_close(native_tree.layout_width(native_root), 10.5);
        assert_close(native_tree.layout_height(native_root), 4.5);

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node_with_config(
            starlight_standalone::StandaloneConfig::with_physical_pixels_per_layout_unit(2.0),
        );
        rust_tree.reset_node(rust_root).expect("reset rust root");
        rust_tree
            .set_measured_size(rust_root, Some(Size::new(10.2, 4.2)))
            .expect("set rust measured size after reset");
        rust_tree
            .calculate_layout_with_mode(rust_root, Constraints::indefinite(), Direction::Ltr)
            .expect("layout rust root");
        assert_close(
            rust_tree.layout_width(rust_root).expect("rust width"),
            native_tree.layout_width(native_root),
        );
        assert_close(
            rust_tree.layout_height(rust_root).expect("rust height"),
            native_tree.layout_height(native_root),
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn baseline_import_uses_layout_tree_physical_pixels_per_layout_unit() {
        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node_with_config(
            starlight_standalone::StandaloneConfig::with_physical_pixels_per_layout_unit(2.0),
        );
        rust_tree
            .set_measured_size(rust_root, Some(Size::new(10.2, 4.2)))
            .expect("set rust measured size");
        let rust_size =
            LayoutEngine::new().layout(&mut rust_tree, rust_root, Constraints::indefinite());

        let mut cpp_tree = starlight_standalone::StandaloneTree::new();
        let cpp_root = cpp_tree.create_default_node_with_config(
            starlight_standalone::StandaloneConfig::with_physical_pixels_per_layout_unit(2.0),
        );
        cpp_tree
            .set_measured_size(cpp_root, Some(Size::new(10.2, 4.2)))
            .expect("set C++ baseline measured size");
        let cpp_size = super::layout_standalone(&mut cpp_tree, cpp_root, Constraints::indefinite())
            .expect("C++ baseline layout");

        assert_close(rust_size.width, 10.5);
        assert_close(rust_size.height, 4.5);
        assert_close(cpp_size.width, rust_size.width);
        assert_close(cpp_size.height, rust_size.height);
        assert_close(
            cpp_tree.layout_width(cpp_root).expect("C++ baseline width"),
            rust_tree.layout_width(rust_root).expect("Rust width"),
        );
        assert_close(
            cpp_tree
                .layout_height(cpp_root)
                .expect("C++ baseline height"),
            rust_tree.layout_height(rust_root).expect("Rust height"),
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn baseline_import_uses_child_layout_tree_physical_pixels_per_layout_unit() {
        let root_style = Style {
            display: Display::Flex,
            flex_direction: FlexDirection::Row,
            align_items: AlignItems::FlexStart,
            ..starlight_standalone::standalone_default_style()
        };

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_node(root_style.clone());
        let rust_child = rust_tree.create_default_node_with_config(
            starlight_standalone::StandaloneConfig::with_physical_pixels_per_layout_unit(2.0),
        );
        rust_tree
            .set_measured_size(rust_child, Some(Size::new(10.2, 4.2)))
            .expect("set rust child measured size");
        rust_tree
            .append_child(rust_root, rust_child)
            .expect("append rust child");
        let rust_size =
            LayoutEngine::new().layout(&mut rust_tree, rust_root, Constraints::indefinite());

        let mut cpp_tree = starlight_standalone::StandaloneTree::new();
        let cpp_root = cpp_tree.create_node(root_style);
        let cpp_child = cpp_tree.create_default_node_with_config(
            starlight_standalone::StandaloneConfig::with_physical_pixels_per_layout_unit(2.0),
        );
        cpp_tree
            .set_measured_size(cpp_child, Some(Size::new(10.2, 4.2)))
            .expect("set C++ baseline child measured size");
        cpp_tree
            .append_child(cpp_root, cpp_child)
            .expect("append C++ baseline child");
        let cpp_size = super::layout_standalone(&mut cpp_tree, cpp_root, Constraints::indefinite())
            .expect("C++ baseline layout");

        assert_close(
            rust_tree
                .layout_width(rust_child)
                .expect("Rust child width"),
            10.5,
        );
        assert_close(
            rust_tree
                .layout_height(rust_child)
                .expect("Rust child height"),
            4.5,
        );
        assert_close(
            cpp_tree
                .layout_width(cpp_child)
                .expect("C++ baseline child width"),
            rust_tree
                .layout_width(rust_child)
                .expect("Rust child width"),
        );
        assert_close(
            cpp_tree
                .layout_height(cpp_child)
                .expect("C++ baseline child height"),
            rust_tree
                .layout_height(rust_child)
                .expect("Rust child height"),
        );
        assert_close(cpp_size.width, rust_size.width);
        assert_close(cpp_size.height, rust_size.height);
        assert_close(
            cpp_tree.layout_width(cpp_root).expect("C++ baseline width"),
            rust_tree.layout_width(rust_root).expect("Rust width"),
        );
        assert_close(
            cpp_tree
                .layout_height(cpp_root)
                .expect("C++ baseline height"),
            rust_tree.layout_height(rust_root).expect("Rust height"),
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_native_measure_delegate_skips_measure_func_for_exact_constraints() {
        let measure_calls = AtomicUsize::new(0);
        let mut delegate = super::StarlightMeasureDelegate {
            measure_func: Some(native_api_counting_measure),
            baseline_func: None,
            manager_node: (&measure_calls as *const AtomicUsize)
                .cast::<c_void>()
                .cast_mut(),
        };
        let mut tree = NativeApiTree::new();
        let fixed_root = tree.create_node();
        let auto_root = tree.create_node();
        tree.set_measure_delegate(fixed_root, &mut delegate);
        tree.set_measure_delegate(auto_root, &mut delegate);

        // SAFETY: `fixed_root` is a live standalone node owned by `tree`.
        unsafe {
            super::SLNodeStyleSetWidth(fixed_root.as_ptr(), 30.0);
            super::SLNodeStyleSetHeight(fixed_root.as_ptr(), 20.0);
        }

        tree.calculate_layout(
            fixed_root,
            super::SL_UNDEFINED,
            super::SL_UNDEFINED,
            super::SL_DIRECTION_LTR,
        );
        assert_eq!(measure_calls.load(Ordering::Relaxed), 0);
        assert_close(tree.layout_width(fixed_root), 30.0);
        assert_close(tree.layout_height(fixed_root), 20.0);

        tree.calculate_layout(
            auto_root,
            super::SL_UNDEFINED,
            super::SL_UNDEFINED,
            super::SL_DIRECTION_LTR,
        );
        assert_eq!(measure_calls.load(Ordering::Relaxed), 1);
        assert_close(tree.layout_width(auto_root), 11.0);
        assert_close(tree.layout_height(auto_root), 7.0);
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn baseline_import_uses_rust_standalone_measure_func_constraints() {
        let constraints =
            Constraints::new(SideConstraint::at_most(30.0), SideConstraint::indefinite());

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node();
        rust_tree
            .set_measure_func(rust_root, Some(standalone_constraint_sensitive_measure))
            .expect("set rust measure func");
        let rust_size = LayoutEngine::new().layout(&mut rust_tree, rust_root, constraints);

        let mut cpp_tree = starlight_standalone::StandaloneTree::new();
        let cpp_root = cpp_tree.create_default_node();
        cpp_tree
            .set_measure_func(cpp_root, Some(standalone_constraint_sensitive_measure))
            .expect("set C++ baseline measure func");
        let cpp_size = super::layout_standalone(&mut cpp_tree, cpp_root, constraints)
            .expect("C++ baseline layout");

        assert_close(rust_size.width, 27.0);
        assert_close(rust_size.height, 11.0);
        assert_close(cpp_size.width, rust_size.width);
        assert_close(cpp_size.height, rust_size.height);
        assert_close(
            cpp_tree.layout_width(cpp_root).expect("C++ baseline width"),
            rust_tree.layout_width(rust_root).expect("Rust width"),
        );
        assert_close(
            cpp_tree
                .layout_height(cpp_root)
                .expect("C++ baseline height"),
            rust_tree.layout_height(rust_root).expect("Rust height"),
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn baseline_import_uses_rust_standalone_baseline_func_content_size() {
        let constraints =
            Constraints::new(SideConstraint::at_most(30.0), SideConstraint::indefinite());

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node();
        rust_tree
            .set_measure_func(rust_root, Some(standalone_constraint_sensitive_measure))
            .expect("set rust measure func");
        rust_tree
            .set_baseline_func(rust_root, Some(standalone_baseline_from_content_size))
            .expect("set rust baseline func");
        LayoutEngine::new().layout(&mut rust_tree, rust_root, constraints);

        let mut cpp_tree = starlight_standalone::StandaloneTree::new();
        let cpp_root = cpp_tree.create_default_node();
        cpp_tree
            .set_measure_func(cpp_root, Some(standalone_constraint_sensitive_measure))
            .expect("set C++ baseline measure func");
        cpp_tree
            .set_baseline_func(cpp_root, Some(standalone_baseline_from_content_size))
            .expect("set C++ baseline func");
        super::layout_standalone(&mut cpp_tree, cpp_root, constraints)
            .expect("C++ baseline layout");

        assert_close(rust_tree.layout_width(rust_root).expect("Rust width"), 27.0);
        assert_close(
            rust_tree.layout_height(rust_root).expect("Rust height"),
            11.0,
        );
        assert_close(
            rust_tree.layout_baseline(rust_root).expect("Rust baseline"),
            9.0,
        );
        assert_close(
            cpp_tree.layout_width(cpp_root).expect("C++ baseline width"),
            rust_tree.layout_width(rust_root).expect("Rust width"),
        );
        assert_close(
            cpp_tree
                .layout_height(cpp_root)
                .expect("C++ baseline height"),
            rust_tree.layout_height(rust_root).expect("Rust height"),
        );
        assert_close(
            cpp_tree.layout_baseline(cpp_root).expect("C++ baseline"),
            rust_tree.layout_baseline(rust_root).expect("Rust baseline"),
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_native_tree_query_and_measure_delegate_apis_are_callable() {
        let mut delegate = super::StarlightMeasureDelegate {
            measure_func: Some(native_api_fixed_measure),
            baseline_func: None,
            manager_node: std::ptr::null_mut(),
        };
        let mut tree = NativeApiTree::new();
        let root = tree.create_node();
        let first = tree.create_node();
        let second = tree.create_node();
        let inserted = tree.create_node();

        // SAFETY: `root` and `first` are live standalone nodes owned by
        // `tree`, and `delegate` outlives the native tree.
        unsafe {
            super::SLNodeStyleSetDisplay(root.as_ptr(), super::SL_DISPLAY_FLEX);
            super::SLNodeStyleSetWidth(root.as_ptr(), 30.0);
            super::SLNodeStyleSetHeight(root.as_ptr(), 20.0);
            super::SLNodeSetMeasureDelegate(first.as_ptr(), &mut delegate);
        }
        // SAFETY: `first` is live and the delegate was just installed.
        unsafe {
            assert_eq!(
                super::SLNodeGetMeasureDelegate(first.as_ptr()),
                &mut delegate as *mut super::StarlightMeasureDelegate
            );
            assert!(super::SLNodeHasMeasureFunc(first.as_ptr()));
        }

        tree.insert_child(root, first, -1);
        tree.insert_child(root, second, -1);
        tree.insert_child_before(root, inserted, second);
        assert_eq!(tree.child_count(root), 3);
        assert_node_eq(tree.child_at(root, 0), first);
        assert_node_eq(tree.child_at(root, 1), inserted);
        assert_node_eq(tree.child_at(root, 2), second);
        assert!(tree.child_at(root, 3).is_null());
        assert_node_eq(tree.parent(first), root);
        assert_node_eq(tree.parent(inserted), root);

        tree.remove_child(root, inserted);
        assert_eq!(tree.child_count(root), 2);
        assert!(tree.parent(inserted).is_null());

        tree.insert_child(root, inserted, 0);
        assert_node_eq(tree.child_at(root, 0), inserted);
        tree.remove_all_children(root);
        assert_eq!(tree.child_count(root), 0);
        assert!(tree.parent(first).is_null());
        assert!(tree.parent(second).is_null());
        assert!(tree.parent(inserted).is_null());

        tree.insert_child(root, first, -1);
        tree.calculate_layout(root, 30.0, 20.0, super::SL_DIRECTION_RTL);
        assert!(!tree.is_dirty(root));
        assert!(tree.is_rtl(root));

        tree.mark_dirty(first);
        assert!(tree.is_dirty(first));
        assert!(tree.is_dirty(root));

        tree.reset(root);
        assert_eq!(tree.child_count(root), 0);
        assert!(tree.parent(first).is_null());
        assert!(!tree.is_dirty(root));
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_native_owner_direction_reaches_unset_descendants() {
        let mut tree = NativeApiTree::new();
        let root = tree.create_node();
        let inherited_direction_container = tree.create_node();
        let inherited_direction_leaf = tree.create_node();
        let explicit_ltr_container = tree.create_node();
        let explicit_ltr_leaf = tree.create_node();

        // SAFETY: all nodes are live standalone nodes owned by `tree`.
        unsafe {
            super::SLNodeStyleSetDisplay(root.as_ptr(), super::SL_DISPLAY_FLEX);
            super::SLNodeStyleSetFlexDirection(root.as_ptr(), super::SL_FLEX_DIRECTION_COLUMN);
            super::SLNodeStyleSetAlignItems(root.as_ptr(), super::SL_FLEX_ALIGN_FLEX_START);
            super::SLNodeStyleSetWidth(root.as_ptr(), 30.0);
            super::SLNodeStyleSetHeight(root.as_ptr(), 20.0);

            for container in [inherited_direction_container, explicit_ltr_container] {
                super::SLNodeStyleSetDisplay(container.as_ptr(), super::SL_DISPLAY_FLEX);
                super::SLNodeStyleSetFlexDirection(
                    container.as_ptr(),
                    super::SL_FLEX_DIRECTION_ROW,
                );
                super::SLNodeStyleSetAlignItems(
                    container.as_ptr(),
                    super::SL_FLEX_ALIGN_FLEX_START,
                );
                super::SLNodeStyleSetWidth(container.as_ptr(), 30.0);
                super::SLNodeStyleSetHeight(container.as_ptr(), 10.0);
            }

            for leaf in [inherited_direction_leaf, explicit_ltr_leaf] {
                super::SLNodeStyleSetWidth(leaf.as_ptr(), 10.0);
                super::SLNodeStyleSetHeight(leaf.as_ptr(), 5.0);
            }

            super::SLNodeStyleSetDirection(
                explicit_ltr_container.as_ptr(),
                super::SL_DIRECTION_LTR,
            );
        }

        tree.insert_child(root, inherited_direction_container, -1);
        tree.insert_child(inherited_direction_container, inherited_direction_leaf, -1);
        tree.insert_child(root, explicit_ltr_container, -1);
        tree.insert_child(explicit_ltr_container, explicit_ltr_leaf, -1);
        tree.calculate_layout(root, 30.0, 20.0, super::SL_DIRECTION_RTL);

        // SAFETY: both leaf nodes are live and have just been laid out.
        unsafe {
            assert_close(
                super::SLNodeLayoutGetLeft(inherited_direction_leaf.as_ptr()),
                20.0,
            );
            assert_close(super::SLNodeLayoutGetLeft(explicit_ltr_leaf.as_ptr()), 0.0);
        }
        assert!(tree.is_rtl(root));
        assert!(tree.is_rtl(inherited_direction_container));
        assert!(!tree.is_rtl(explicit_ltr_container));
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_native_lynx_rtl_owner_direction_reaches_unset_node() {
        let mut tree = NativeApiTree::new();
        let node = tree.create_node();

        tree.calculate_layout(node, 10.0, 10.0, super::SL_DIRECTION_LYNX_RTL);

        assert!(tree.is_rtl(node));
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_reset_dirty_and_detach_semantics_match_rust_standalone() {
        let mut native_delegate = super::StarlightMeasureDelegate {
            measure_func: Some(native_api_fixed_measure),
            baseline_func: None,
            manager_node: std::ptr::null_mut(),
        };
        let mut native_tree = NativeApiTree::new();
        let native_root = native_tree.create_node();
        let native_child = native_tree.create_node();
        native_tree.insert_child(native_root, native_child, -1);
        native_tree.set_measure_delegate(native_root, &mut native_delegate);
        assert_eq!(native_tree.child_count(native_root), 1);
        assert_eq!(
            native_tree.measure_delegate(native_root),
            &mut native_delegate as *mut super::StarlightMeasureDelegate
        );
        assert!(native_tree.has_measure_func(native_root));
        assert!(native_tree.is_dirty(native_root));

        native_tree.reset(native_root);
        assert_eq!(native_tree.child_count(native_root), 0);
        assert!(native_tree.parent(native_child).is_null());
        assert!(native_tree.measure_delegate(native_root).is_null());
        assert!(!native_tree.has_measure_func(native_root));
        assert!(!native_tree.is_dirty(native_root));

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node();
        let rust_child = rust_tree.create_default_node();
        rust_tree
            .append_child(rust_root, rust_child)
            .expect("append rust child");
        rust_tree
            .set_measured_size(rust_root, Some(Size::new(11.0, 7.0)))
            .expect("set rust measured size");
        rust_tree
            .set_baseline(rust_root, Some(4.0))
            .expect("set rust baseline");
        assert_eq!(rust_tree.child_count(rust_root).expect("child count"), 1);
        assert!(rust_tree
            .has_measure_func(rust_root)
            .expect("rust measure flag"));
        assert!(rust_tree.is_dirty(rust_root).expect("rust root dirty"));

        rust_tree.reset_node(rust_root).expect("reset rust root");
        assert_eq!(rust_tree.child_count(rust_root).expect("child count"), 0);
        assert_eq!(
            rust_tree.parent(rust_child).expect("rust child parent"),
            None
        );
        assert!(!rust_tree
            .has_measure_func(rust_root)
            .expect("rust measure flag"));
        assert_eq!(rust_tree.baseline(rust_root).expect("rust baseline"), None);
        assert!(!rust_tree.is_dirty(rust_root).expect("rust root clean"));
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_reset_attached_child_keeps_parent_clean_like_rust_standalone() {
        let mut native_delegate = super::StarlightMeasureDelegate {
            measure_func: Some(native_api_fixed_measure),
            baseline_func: None,
            manager_node: std::ptr::null_mut(),
        };
        let mut native_tree = NativeApiTree::new();
        let native_root = native_tree.create_node();
        let native_child = native_tree.create_node();
        let native_grandchild = native_tree.create_node();
        native_tree.insert_child(native_root, native_child, -1);
        native_tree.insert_child(native_child, native_grandchild, -1);
        native_tree.set_measure_delegate(native_child, &mut native_delegate);
        native_tree.calculate_layout(native_root, 80.0, 30.0, super::SL_DIRECTION_LTR);
        assert!(!native_tree.is_dirty(native_root));
        assert!(!native_tree.is_dirty(native_child));

        native_tree.reset(native_child);

        assert_eq!(native_tree.child_count(native_root), 1);
        assert_node_eq(native_tree.child_at(native_root, 0), native_child);
        assert_node_eq(native_tree.parent(native_child), native_root);
        assert_eq!(native_tree.child_count(native_child), 0);
        assert!(native_tree.parent(native_grandchild).is_null());
        assert!(native_tree.measure_delegate(native_child).is_null());
        assert!(!native_tree.has_measure_func(native_child));
        assert!(!native_tree.is_dirty(native_child));
        assert!(!native_tree.is_dirty(native_root));

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node();
        let rust_child = rust_tree.create_default_node();
        let rust_grandchild = rust_tree.create_default_node();
        rust_tree
            .append_child(rust_root, rust_child)
            .expect("append rust child");
        rust_tree
            .append_child(rust_child, rust_grandchild)
            .expect("append rust grandchild");
        rust_tree
            .set_measured_size(rust_child, Some(Size::new(11.0, 7.0)))
            .expect("set rust measured size");
        rust_tree
            .calculate_layout(rust_root, Size::new(80.0, 30.0), Direction::Ltr)
            .expect("layout rust root");
        assert!(!rust_tree.is_dirty(rust_root).expect("rust root clean"));
        assert!(!rust_tree.is_dirty(rust_child).expect("rust child clean"));

        rust_tree.reset_node(rust_child).expect("reset rust child");

        assert_eq!(rust_tree.child_count(rust_root).expect("rust count"), 1);
        assert_eq!(
            rust_tree.child_at(rust_root, 0).expect("rust first child"),
            Some(rust_child)
        );
        assert_eq!(
            rust_tree.parent(rust_child).expect("rust child parent"),
            Some(rust_root)
        );
        assert_eq!(
            rust_tree.child_count(rust_child).expect("rust child count"),
            0
        );
        assert_eq!(
            rust_tree
                .parent(rust_grandchild)
                .expect("rust grandchild parent"),
            None
        );
        assert!(!rust_tree
            .has_measure_func(rust_child)
            .expect("rust measure flag"));
        assert!(!rust_tree.is_dirty(rust_child).expect("rust child clean"));
        assert!(!rust_tree.is_dirty(rust_root).expect("rust root clean"));
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_layout_getters_match_rust_standalone_after_layout() {
        let mut native_delegate = super::StarlightMeasureDelegate {
            measure_func: Some(native_api_fixed_measure),
            baseline_func: None,
            manager_node: std::ptr::null_mut(),
        };
        let mut native_tree = NativeApiTree::new();
        let native_root = native_tree.create_node();
        let native_child = native_tree.create_node();

        // SAFETY: both nodes are live standalone nodes owned by `native_tree`,
        // and `native_delegate` outlives the layout call below.
        unsafe {
            super::SLNodeStyleSetDisplay(native_root.as_ptr(), super::SL_DISPLAY_FLEX);
            super::SLNodeStyleSetFlexDirection(native_root.as_ptr(), super::SL_FLEX_DIRECTION_ROW);
            super::SLNodeStyleSetAlignItems(native_root.as_ptr(), super::SL_FLEX_ALIGN_FLEX_START);
            super::SLNodeStyleSetWidth(native_root.as_ptr(), 80.0);
            super::SLNodeStyleSetHeight(native_root.as_ptr(), 50.0);

            super::SLNodeStyleSetPositionType(
                native_child.as_ptr(),
                super::SL_POSITION_TYPE_STICKY,
            );
            super::SLNodeStyleSetPosition(native_child.as_ptr(), super::SL_EDGE_LEFT, 3.0);
            super::SLNodeStyleSetPosition(native_child.as_ptr(), super::SL_EDGE_RIGHT, 4.0);
            super::SLNodeStyleSetPosition(native_child.as_ptr(), super::SL_EDGE_TOP, 5.0);
            super::SLNodeStyleSetPosition(native_child.as_ptr(), super::SL_EDGE_BOTTOM, 6.0);
            super::SLNodeStyleSetMargin(native_child.as_ptr(), super::SL_EDGE_LEFT, 1.0);
            super::SLNodeStyleSetMargin(native_child.as_ptr(), super::SL_EDGE_RIGHT, 2.0);
            super::SLNodeStyleSetMargin(native_child.as_ptr(), super::SL_EDGE_TOP, 3.0);
            super::SLNodeStyleSetMargin(native_child.as_ptr(), super::SL_EDGE_BOTTOM, 4.0);
            super::SLNodeStyleSetPadding(native_child.as_ptr(), super::SL_EDGE_LEFT, 5.0);
            super::SLNodeStyleSetPadding(native_child.as_ptr(), super::SL_EDGE_RIGHT, 6.0);
            super::SLNodeStyleSetPadding(native_child.as_ptr(), super::SL_EDGE_TOP, 7.0);
            super::SLNodeStyleSetPadding(native_child.as_ptr(), super::SL_EDGE_BOTTOM, 8.0);
            super::SLNodeStyleSetBorder(native_child.as_ptr(), super::SL_EDGE_LEFT, 9.0);
            super::SLNodeStyleSetBorder(native_child.as_ptr(), super::SL_EDGE_RIGHT, 10.0);
            super::SLNodeStyleSetBorder(native_child.as_ptr(), super::SL_EDGE_TOP, 11.0);
            super::SLNodeStyleSetBorder(native_child.as_ptr(), super::SL_EDGE_BOTTOM, 12.0);
            super::SLNodeSetMeasureDelegate(native_child.as_ptr(), &mut native_delegate);
        }
        native_tree.insert_child(native_root, native_child, -1);
        native_tree.calculate_layout(native_root, 80.0, 50.0, super::SL_DIRECTION_LTR);

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_node(Style {
            display: Display::Flex,
            flex_direction: FlexDirection::Row,
            align_items: AlignItems::FlexStart,
            width: Length::points(80.0),
            height: Length::points(50.0),
            ..starlight_standalone::standalone_default_style()
        });
        let rust_child = rust_tree.create_measured_node(
            Style {
                position: PositionType::Sticky,
                left: Length::points(3.0),
                right: Length::points(4.0),
                top: Length::points(5.0),
                bottom: Length::points(6.0),
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(3.0),
                    Length::points(4.0),
                ),
                padding: Rect::new(
                    Length::points(5.0),
                    Length::points(6.0),
                    Length::points(7.0),
                    Length::points(8.0),
                ),
                border: Rect::new(9.0, 10.0, 11.0, 12.0),
                ..starlight_standalone::standalone_default_style()
            },
            Size::new(11.0, 7.0),
        );
        rust_tree
            .append_child(rust_root, rust_child)
            .expect("append rust child");
        rust_tree
            .calculate_layout(rust_root, Size::new(80.0, 50.0), Direction::Ltr)
            .expect("layout rust root");

        assert_close(
            native_tree.layout_left(native_child),
            rust_tree.layout_left(rust_child).expect("rust left"),
        );
        assert_close(
            native_tree.layout_top(native_child),
            rust_tree.layout_top(rust_child).expect("rust top"),
        );
        assert_close(
            native_tree.layout_width(native_child),
            rust_tree.layout_width(rust_child).expect("rust width"),
        );
        assert_close(
            native_tree.layout_height(native_child),
            rust_tree.layout_height(rust_child).expect("rust height"),
        );
        assert_close(
            native_tree.layout_baseline(native_child),
            rust_tree
                .layout_baseline(rust_child)
                .expect("rust baseline"),
        );

        for (edge, standalone_edge) in [
            (
                super::SL_EDGE_LEFT,
                starlight_standalone::StandaloneEdge::Left,
            ),
            (
                super::SL_EDGE_RIGHT,
                starlight_standalone::StandaloneEdge::Right,
            ),
            (
                super::SL_EDGE_TOP,
                starlight_standalone::StandaloneEdge::Top,
            ),
            (
                super::SL_EDGE_BOTTOM,
                starlight_standalone::StandaloneEdge::Bottom,
            ),
        ] {
            assert_close(
                native_tree.layout_margin(native_child, edge),
                rust_tree
                    .layout_margin(rust_child, standalone_edge)
                    .expect("rust margin"),
            );
            assert_close(
                native_tree.layout_padding(native_child, edge),
                rust_tree
                    .layout_padding(rust_child, standalone_edge)
                    .expect("rust padding"),
            );
            assert_close(
                native_tree.layout_border(native_child, edge),
                rust_tree
                    .layout_border(rust_child, standalone_edge)
                    .expect("rust border"),
            );
            assert_close(
                native_tree.layout_sticky_position(native_child, edge),
                rust_tree
                    .layout_sticky_position(rust_child, standalone_edge)
                    .expect("rust sticky"),
            );
        }
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_insert_signed_index_semantics_match_rust_standalone() {
        let mut native_tree = NativeApiTree::new();
        let native_root = native_tree.create_node();
        let native_first = native_tree.create_node();
        let native_second = native_tree.create_node();
        let native_middle = native_tree.create_node();
        let native_tail = native_tree.create_node();
        let native_zero = native_tree.create_node();

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node();
        let rust_first = rust_tree.create_default_node();
        let rust_second = rust_tree.create_default_node();
        let rust_middle = rust_tree.create_default_node();
        let rust_tail = rust_tree.create_default_node();
        let rust_zero = rust_tree.create_default_node();

        native_tree.insert_child(native_root, native_first, -1);
        rust_tree
            .insert_child_at_standalone_index(rust_root, rust_first, -1)
            .expect("rust append with -1");

        native_tree.insert_child(native_root, native_second, -2);
        rust_tree
            .insert_child_at_standalone_index(rust_root, rust_second, -2)
            .expect("rust append with any negative index");

        native_tree.insert_child(native_root, native_middle, 1);
        rust_tree
            .insert_child_at_standalone_index(rust_root, rust_middle, 1)
            .expect("rust insert at middle");

        native_tree.insert_child(native_root, native_tail, 99);
        rust_tree
            .insert_child_at_standalone_index(rust_root, rust_tail, 99)
            .expect("rust append with out-of-range index");

        native_tree.insert_child(native_root, native_zero, 0);
        rust_tree
            .insert_child_at_standalone_index(rust_root, rust_zero, 0)
            .expect("rust insert at zero");

        assert_eq!(native_tree.child_count(native_root), 5);
        assert_eq!(rust_tree.child_count(rust_root).expect("rust count"), 5);
        assert_node_eq(native_tree.child_at(native_root, 0), native_zero);
        assert_node_eq(native_tree.child_at(native_root, 1), native_first);
        assert_node_eq(native_tree.child_at(native_root, 2), native_middle);
        assert_node_eq(native_tree.child_at(native_root, 3), native_second);
        assert_node_eq(native_tree.child_at(native_root, 4), native_tail);
        assert_eq!(
            rust_tree.children(rust_root).expect("rust children"),
            [rust_zero, rust_first, rust_middle, rust_second, rust_tail]
        );

        native_tree.insert_child(native_root, native_first, 3);
        rust_tree
            .insert_child_at_standalone_index(rust_root, rust_first, 3)
            .expect("rust same-parent reinsert");

        assert_eq!(native_tree.child_count(native_root), 5);
        assert_eq!(rust_tree.child_count(rust_root).expect("rust count"), 5);
        assert_node_eq(native_tree.child_at(native_root, 0), native_zero);
        assert_node_eq(native_tree.child_at(native_root, 1), native_middle);
        assert_node_eq(native_tree.child_at(native_root, 2), native_second);
        assert_node_eq(native_tree.child_at(native_root, 3), native_first);
        assert_node_eq(native_tree.child_at(native_root, 4), native_tail);
        assert_eq!(
            rust_tree.children(rust_root).expect("rust children"),
            [rust_zero, rust_middle, rust_second, rust_first, rust_tail]
        );
        assert_node_eq(native_tree.parent(native_first), native_root);
        assert_eq!(
            rust_tree.parent(rust_first).expect("rust first parent"),
            Some(rust_root)
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_insert_before_optional_reference_semantics_match_rust_standalone() {
        let mut native_tree = NativeApiTree::new();
        let native_root = native_tree.create_node();
        let native_other = native_tree.create_node();
        let native_first = native_tree.create_node();
        let native_second = native_tree.create_node();
        let native_inserted = native_tree.create_node();

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node();
        let rust_other = rust_tree.create_default_node();
        let rust_first = rust_tree.create_default_node();
        let rust_second = rust_tree.create_default_node();
        let rust_inserted = rust_tree.create_default_node();

        native_tree.insert_child_before_or_append(native_root, native_first, None);
        rust_tree
            .insert_child_before_or_append(rust_root, rust_first, None)
            .expect("append rust first with null reference");
        native_tree.insert_child_before_or_append(native_root, native_second, None);
        rust_tree
            .insert_child_before_or_append(rust_root, rust_second, None)
            .expect("append rust second with null reference");
        native_tree.insert_child_before_or_append(
            native_root,
            native_inserted,
            Some(native_second),
        );
        rust_tree
            .insert_child_before_or_append(rust_root, rust_inserted, Some(rust_second))
            .expect("insert rust child before reference");

        assert_eq!(native_tree.child_count(native_root), 3);
        assert_eq!(rust_tree.child_count(rust_root).expect("rust count"), 3);
        assert_node_eq(native_tree.child_at(native_root, 0), native_first);
        assert_node_eq(native_tree.child_at(native_root, 1), native_inserted);
        assert_node_eq(native_tree.child_at(native_root, 2), native_second);
        assert_eq!(
            rust_tree.children(rust_root).expect("rust children"),
            [rust_first, rust_inserted, rust_second]
        );

        native_tree.insert_child_before_or_append(native_root, native_first, Some(native_second));
        rust_tree
            .insert_child_before_or_append(rust_root, rust_first, Some(rust_second))
            .expect("reorder rust child before reference");
        assert_node_eq(native_tree.child_at(native_root, 0), native_inserted);
        assert_node_eq(native_tree.child_at(native_root, 1), native_first);
        assert_node_eq(native_tree.child_at(native_root, 2), native_second);
        assert_eq!(
            rust_tree
                .children(rust_root)
                .expect("rust reordered children"),
            [rust_inserted, rust_first, rust_second]
        );

        native_tree.insert_child_before_or_append(native_other, native_inserted, None);
        rust_tree
            .insert_child_before_or_append(rust_other, rust_inserted, None)
            .expect("move rust child to other parent with null reference");
        assert_eq!(native_tree.child_count(native_root), 2);
        assert_eq!(
            rust_tree.child_count(rust_root).expect("rust root count"),
            2
        );
        assert_node_eq(native_tree.child_at(native_root, 0), native_first);
        assert_node_eq(native_tree.child_at(native_root, 1), native_second);
        assert_eq!(
            rust_tree.children(rust_root).expect("rust root children"),
            [rust_first, rust_second]
        );
        assert_eq!(native_tree.child_count(native_other), 1);
        assert_eq!(
            rust_tree.child_count(rust_other).expect("rust other count"),
            1
        );
        assert_node_eq(native_tree.child_at(native_other, 0), native_inserted);
        assert_eq!(
            rust_tree.children(rust_other).expect("rust other children"),
            [rust_inserted]
        );
        assert_node_eq(native_tree.parent(native_inserted), native_other);
        assert_eq!(
            rust_tree
                .parent(rust_inserted)
                .expect("rust inserted parent"),
            Some(rust_other)
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_child_signed_index_query_semantics_match_rust_standalone() {
        let mut native_tree = NativeApiTree::new();
        let native_root = native_tree.create_node();
        let native_first = native_tree.create_node();
        let native_second = native_tree.create_node();
        native_tree.insert_child(native_root, native_first, -1);
        native_tree.insert_child(native_root, native_second, -1);

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node();
        let rust_first = rust_tree.create_default_node();
        let rust_second = rust_tree.create_default_node();
        rust_tree
            .insert_child_at_standalone_index(rust_root, rust_first, -1)
            .expect("append rust first");
        rust_tree
            .insert_child_at_standalone_index(rust_root, rust_second, -1)
            .expect("append rust second");

        assert!(native_tree.child_at(native_root, -1).is_null());
        assert_eq!(
            rust_tree
                .child_at_standalone_index(rust_root, -1)
                .expect("rust negative child index"),
            None
        );
        assert_node_eq(native_tree.child_at(native_root, 0), native_first);
        assert_eq!(
            rust_tree
                .child_at_standalone_index(rust_root, 0)
                .expect("rust first child"),
            Some(rust_first)
        );
        assert_node_eq(native_tree.child_at(native_root, 1), native_second);
        assert_eq!(
            rust_tree
                .child_at_standalone_index(rust_root, 1)
                .expect("rust second child"),
            Some(rust_second)
        );
        assert!(native_tree.child_at(native_root, 2).is_null());
        assert_eq!(
            rust_tree
                .child_at_standalone_index(rust_root, 2)
                .expect("rust out-of-range child"),
            None
        );
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_remove_noop_and_remove_all_semantics_match_rust_standalone() {
        let mut native_tree = NativeApiTree::new();
        let native_root = native_tree.create_node();
        let native_child = native_tree.create_node();
        let native_wrong_parent = native_tree.create_node();
        let native_outsider = native_tree.create_node();
        native_tree.insert_child(native_root, native_child, -1);
        native_tree.calculate_layout(native_root, 20.0, 10.0, super::SL_DIRECTION_LTR);
        assert!(!native_tree.is_dirty(native_root));

        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_root = rust_tree.create_default_node();
        let rust_child = rust_tree.create_default_node();
        let rust_wrong_parent = rust_tree.create_default_node();
        let rust_outsider = rust_tree.create_default_node();
        rust_tree
            .append_child(rust_root, rust_child)
            .expect("append rust child");
        rust_tree
            .calculate_layout(
                rust_root,
                starlight_layout::Size::new(20.0, 10.0),
                Direction::Ltr,
            )
            .expect("layout rust root");
        assert!(!rust_tree.is_dirty(rust_root).expect("rust root clean"));

        native_tree.remove_child(native_root, native_outsider);
        rust_tree
            .remove_child(rust_root, rust_outsider)
            .expect("rust remove unattached child");
        assert_eq!(native_tree.child_count(native_root), 1);
        assert_eq!(rust_tree.child_count(rust_root).expect("rust count"), 1);
        assert_node_eq(native_tree.child_at(native_root, 0), native_child);
        assert_eq!(
            rust_tree.child_at(rust_root, 0).expect("rust first child"),
            Some(rust_child)
        );
        assert!(native_tree.parent(native_outsider).is_null());
        assert_eq!(
            rust_tree
                .parent(rust_outsider)
                .expect("rust outsider parent"),
            None
        );
        assert!(!native_tree.is_dirty(native_root));
        assert!(!rust_tree.is_dirty(rust_root).expect("rust root clean"));

        native_tree.remove_child(native_wrong_parent, native_child);
        rust_tree
            .remove_child(rust_wrong_parent, rust_child)
            .expect("rust remove child from wrong parent");
        assert_eq!(native_tree.child_count(native_root), 1);
        assert_eq!(rust_tree.child_count(rust_root).expect("rust count"), 1);
        assert_node_eq(native_tree.parent(native_child), native_root);
        assert_eq!(
            rust_tree.parent(rust_child).expect("rust child parent"),
            Some(rust_root)
        );
        assert!(!native_tree.is_dirty(native_root));
        assert!(!rust_tree.is_dirty(rust_root).expect("rust root clean"));

        native_tree.remove_all_children(native_root);
        rust_tree
            .remove_all_children(rust_root)
            .expect("rust remove all children");
        assert_eq!(native_tree.child_count(native_root), 0);
        assert_eq!(rust_tree.child_count(rust_root).expect("rust count"), 0);
        assert!(native_tree.parent(native_child).is_null());
        assert_eq!(
            rust_tree.parent(rust_child).expect("rust child parent"),
            None
        );
        assert!(native_tree.is_dirty(native_root));
        assert!(rust_tree.is_dirty(rust_root).expect("rust root dirty"));

        native_tree.calculate_layout(native_root, 20.0, 10.0, super::SL_DIRECTION_LTR);
        rust_tree
            .calculate_layout(
                rust_root,
                starlight_layout::Size::new(20.0, 10.0),
                Direction::Ltr,
            )
            .expect("layout rust root");
        assert!(!native_tree.is_dirty(native_root));
        assert!(!rust_tree.is_dirty(rust_root).expect("rust root clean"));

        native_tree.remove_all_children(native_root);
        rust_tree
            .remove_all_children(rust_root)
            .expect("rust remove all children from empty parent");
        assert_eq!(native_tree.child_count(native_root), 0);
        assert_eq!(rust_tree.child_count(rust_root).expect("rust count"), 0);
        assert!(native_tree.is_dirty(native_root));
        assert!(rust_tree.is_dirty(rust_root).expect("rust root dirty"));
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_style_getters_match_rust_standalone_after_public_setters() {
        let mut native_tree = NativeApiTree::new();
        let native_node = native_tree.create_node();
        let mut rust_tree = starlight_standalone::StandaloneTree::new();
        let rust_node = rust_tree.create_default_node();

        // SAFETY: `native_node` is live for the whole test and every call uses
        // the public standalone C API on that node.
        unsafe {
            super::SLNodeStyleSetFlexDirection(
                native_node.as_ptr(),
                super::SL_FLEX_DIRECTION_ROW_REVERSE,
            );
            super::SLNodeStyleSetJustifyContent(
                native_node.as_ptr(),
                super::SL_JUSTIFY_CONTENT_SPACE_BETWEEN,
            );
            super::SLNodeStyleSetAlignContent(
                native_node.as_ptr(),
                super::SL_ALIGN_CONTENT_SPACE_EVENLY,
            );
            super::SLNodeStyleSetAlignItems(native_node.as_ptr(), super::SL_FLEX_ALIGN_BASELINE);
            super::SLNodeStyleSetAlignSelf(native_node.as_ptr(), super::SL_FLEX_ALIGN_END);
            super::SLNodeStyleSetPositionType(native_node.as_ptr(), super::SL_POSITION_TYPE_STICKY);
            super::SLNodeStyleSetFlexWrap(native_node.as_ptr(), super::SL_FLEX_WRAP_WRAP_REVERSE);
            super::SLNodeStyleSetLinearOrientation(
                native_node.as_ptr(),
                super::SL_LINEAR_ORIENTATION_VERTICAL_REVERSE,
            );
            super::SLNodeStyleSetLinearGravity(
                native_node.as_ptr(),
                super::SL_LINEAR_GRAVITY_CENTER,
            );
            super::SLNodeStyleSetLinearLayoutGravity(
                native_node.as_ptr(),
                super::SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL,
            );
            super::SLNodeStyleSetLinearCrossGravity(
                native_node.as_ptr(),
                super::SL_LINEAR_CROSS_GRAVITY_STRETCH,
            );
            super::SLNodeStyleSetRelativeCenter(
                native_node.as_ptr(),
                super::SL_RELATIVE_CENTER_BOTH,
            );
            super::SLNodeStyleSetGridAutoFlow(
                native_node.as_ptr(),
                super::SL_GRID_AUTO_FLOW_COLUMN_DENSE,
            );
            super::SLNodeStyleSetJustifyItems(native_node.as_ptr(), super::SL_JUSTIFY_ITEM_CENTER);
            super::SLNodeStyleSetJustifySelf(native_node.as_ptr(), super::SL_JUSTIFY_ITEM_END);
            super::SLNodeStyleSetDisplay(native_node.as_ptr(), super::SL_DISPLAY_GRID);
            super::SLNodeStyleSetBoxSizing(native_node.as_ptr(), super::SL_BOX_SIZING_CONTENT_BOX);
            super::SLNodeStyleSetAspectRatio(native_node.as_ptr(), 1.5);
            super::SLNodeStyleSetOrder(native_node.as_ptr(), -2);
            super::SLNodeStyleSetRelativeId(native_node.as_ptr(), 17);
            super::SLNodeStyleSetRelativeAlignTop(native_node.as_ptr(), 1);
            super::SLNodeStyleSetRelativeAlignRight(native_node.as_ptr(), 2);
            super::SLNodeStyleSetRelativeAlignBottom(native_node.as_ptr(), 3);
            super::SLNodeStyleSetRelativeAlignLeft(native_node.as_ptr(), 4);
            super::SLNodeStyleSetRelativeTopOf(native_node.as_ptr(), 5);
            super::SLNodeStyleSetRelativeRightOf(native_node.as_ptr(), 6);
            super::SLNodeStyleSetRelativeBottomOf(native_node.as_ptr(), 7);
            super::SLNodeStyleSetRelativeLeftOf(native_node.as_ptr(), 8);
            super::SLNodeStyleSetRelativeLayoutOnce(native_node.as_ptr(), true);
            super::SLNodeStyleSetGridColumnStart(native_node.as_ptr(), 2);
            super::SLNodeStyleSetGridColumnEnd(native_node.as_ptr(), 4);
            super::SLNodeStyleSetGridRowStart(native_node.as_ptr(), 3);
            super::SLNodeStyleSetGridRowEnd(native_node.as_ptr(), 5);
            super::SLNodeStyleSetGridColumnSpan(native_node.as_ptr(), 6);
            super::SLNodeStyleSetGridRowSpan(native_node.as_ptr(), 7);
            super::SLNodeStyleSetFlex(native_node.as_ptr(), 2.5);
            super::SLNodeStyleSetLinearWeight(native_node.as_ptr(), 3.0);
            super::SLNodeStyleSetLinearWeightSum(native_node.as_ptr(), 9.0);
            super::SLNodeStyleSetBorder(native_node.as_ptr(), super::SL_EDGE_LEFT, 4.0);
            super::SLNodeStyleSetFlexBasisCalc(
                native_node.as_ptr(),
                super::starlight_value_from_calc_length(3.0, 20.0),
            );
            super::SLNodeStyleSetPositionAuto(native_node.as_ptr(), super::SL_EDGE_LEFT);
            super::SLNodeStyleSetMarginPercent(native_node.as_ptr(), super::SL_EDGE_RIGHT, 12.0);
            super::SLNodeStyleSetPaddingCalc(
                native_node.as_ptr(),
                super::SL_EDGE_TOP,
                super::starlight_value_from_calc_length(2.0, 5.0),
            );
            super::SLNodeStyleSetGap(native_node.as_ptr(), super::SL_GAP_ROW, 6.0);
            super::SLNodeStyleSetWidth(native_node.as_ptr(), 100.0);
            super::SLNodeStyleSetHeightPercent(native_node.as_ptr(), 50.0);
            super::SLNodeStyleSetMinWidthMaxContent(native_node.as_ptr());
            super::SLNodeStyleSetMaxWidthFitContentValue(
                native_node.as_ptr(),
                super::starlight_value_from_calc_length(10.0, 25.0),
            );
            super::SLNodeStyleSetMinHeight(native_node.as_ptr(), 7.0);
            super::SLNodeStyleSetMaxHeightFitContent(native_node.as_ptr());
        }

        rust_tree
            .set_flex_direction(rust_node, FlexDirection::RowReverse)
            .expect("set flex direction");
        rust_tree
            .set_justify_content(rust_node, JustifyContent::SpaceBetween)
            .expect("set justify content");
        rust_tree
            .set_align_content(rust_node, AlignContent::SpaceEvenly)
            .expect("set align content");
        rust_tree
            .set_align_items(rust_node, AlignItems::Baseline)
            .expect("set align items");
        rust_tree
            .set_align_self(rust_node, Some(AlignItems::End))
            .expect("set align self");
        rust_tree
            .set_position_type(rust_node, PositionType::Sticky)
            .expect("set position");
        rust_tree
            .set_flex_wrap(rust_node, FlexWrap::WrapReverse)
            .expect("set flex wrap");
        rust_tree
            .set_linear_orientation(rust_node, LinearOrientation::VerticalReverse)
            .expect("set linear orientation");
        rust_tree
            .set_linear_gravity(rust_node, LinearGravity::Center)
            .expect("set linear gravity");
        rust_tree
            .set_linear_layout_gravity(rust_node, LinearLayoutGravity::FillHorizontal)
            .expect("set linear layout gravity");
        rust_tree
            .set_linear_cross_gravity(rust_node, LinearCrossGravity::Stretch)
            .expect("set linear cross gravity");
        rust_tree
            .set_relative_center(rust_node, RelativeCenter::Both)
            .expect("set relative center");
        rust_tree
            .set_grid_auto_flow(rust_node, GridAutoFlow::ColumnDense)
            .expect("set grid auto flow");
        rust_tree
            .set_justify_items(rust_node, JustifyItems::Center)
            .expect("set justify items");
        rust_tree
            .set_justify_self(rust_node, JustifyItems::End)
            .expect("set justify self");
        rust_tree
            .set_display(rust_node, Display::Grid)
            .expect("set display");
        rust_tree
            .set_box_sizing(rust_node, BoxSizing::ContentBox)
            .expect("set box sizing");
        rust_tree
            .set_aspect_ratio(rust_node, Some(1.5))
            .expect("set aspect ratio");
        rust_tree.set_order(rust_node, -2).expect("set order");
        rust_tree
            .set_relative_id(rust_node, 17)
            .expect("set relative id");
        rust_tree
            .set_relative_align_top(rust_node, 1)
            .expect("set relative align top");
        rust_tree
            .set_relative_align_right(rust_node, 2)
            .expect("set relative align right");
        rust_tree
            .set_relative_align_bottom(rust_node, 3)
            .expect("set relative align bottom");
        rust_tree
            .set_relative_align_left(rust_node, 4)
            .expect("set relative align left");
        rust_tree
            .set_relative_top_of(rust_node, 5)
            .expect("set relative top of");
        rust_tree
            .set_relative_right_of(rust_node, 6)
            .expect("set relative right of");
        rust_tree
            .set_relative_bottom_of(rust_node, 7)
            .expect("set relative bottom of");
        rust_tree
            .set_relative_left_of(rust_node, 8)
            .expect("set relative left of");
        rust_tree
            .set_relative_layout_once(rust_node, true)
            .expect("set relative layout once");
        rust_tree
            .set_grid_column_start(rust_node, Some(2))
            .expect("set grid column start");
        rust_tree
            .set_grid_column_end(rust_node, Some(4))
            .expect("set grid column end");
        rust_tree
            .set_grid_row_start(rust_node, Some(3))
            .expect("set grid row start");
        rust_tree
            .set_grid_row_end(rust_node, Some(5))
            .expect("set grid row end");
        rust_tree
            .set_grid_column_span(rust_node, 6)
            .expect("set grid column span");
        rust_tree
            .set_grid_row_span(rust_node, 7)
            .expect("set grid row span");
        rust_tree
            .set_flex(rust_node, 2.5)
            .expect("set flex shorthand");
        rust_tree
            .set_linear_weight(rust_node, 3.0)
            .expect("set linear weight");
        rust_tree
            .set_linear_weight_sum(rust_node, 9.0)
            .expect("set linear weight sum");
        rust_tree
            .set_border(rust_node, starlight_standalone::StandaloneEdge::Left, 4.0)
            .expect("set border");
        rust_tree
            .set_flex_basis(rust_node, Length::calc(3.0, 20.0))
            .expect("set flex basis");
        rust_tree
            .set_position(
                rust_node,
                starlight_standalone::StandaloneEdge::Left,
                Length::Auto,
            )
            .expect("set position");
        rust_tree
            .set_margin(
                rust_node,
                starlight_standalone::StandaloneEdge::Right,
                Length::percent(12.0),
            )
            .expect("set margin");
        rust_tree
            .set_padding(
                rust_node,
                starlight_standalone::StandaloneEdge::Top,
                Length::calc(2.0, 5.0),
            )
            .expect("set padding");
        rust_tree
            .set_gap(
                rust_node,
                starlight_standalone::StandaloneGap::Row,
                Length::points(6.0),
            )
            .expect("set gap");
        rust_tree
            .set_width(rust_node, Length::points(100.0))
            .expect("set width");
        rust_tree
            .set_height(rust_node, Length::percent(50.0))
            .expect("set height");
        rust_tree
            .set_min_width(rust_node, Length::max_content())
            .expect("set min width");
        rust_tree
            .set_max_width(
                rust_node,
                Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 25.0))),
            )
            .expect("set max width");
        rust_tree
            .set_min_height(rust_node, Length::points(7.0))
            .expect("set min height");
        rust_tree
            .set_max_height(rust_node, Length::fit_content(None))
            .expect("set max height");

        let rust_style = rust_tree.style(rust_node).expect("rust style");

        // SAFETY: `native_node` remains live until `native_tree` drops.
        unsafe {
            assert_eq!(
                super::SLNodeStyleGetFlexDirection(native_node.as_ptr()),
                map_flex_direction(
                    rust_tree
                        .style_flex_direction(rust_node)
                        .expect("flex direction")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetJustifyContent(native_node.as_ptr()),
                map_justify_content(
                    rust_tree
                        .style_justify_content(rust_node)
                        .expect("justify content")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetAlignContent(native_node.as_ptr()),
                map_align_content(
                    rust_tree
                        .style_align_content(rust_node)
                        .expect("align content")
                )
                .expect("mapped align content")
            );
            assert_eq!(
                super::SLNodeStyleGetAlignItems(native_node.as_ptr()),
                map_align_items(rust_tree.style_align_items(rust_node).expect("align items"))
                    .expect("mapped align items")
            );
            assert_eq!(
                super::SLNodeStyleGetAlignSelf(native_node.as_ptr()),
                map_align_items(
                    rust_tree
                        .style_align_self(rust_node)
                        .expect("align self")
                        .expect("align self value")
                )
                .expect("mapped align self")
            );
            assert_eq!(
                super::SLNodeStyleGetPositionType(native_node.as_ptr()),
                map_position(rust_style).expect("mapped position")
            );
            assert_eq!(
                super::SLNodeStyleGetFlexWrap(native_node.as_ptr()),
                map_flex_wrap(rust_tree.style_flex_wrap(rust_node).expect("flex wrap"))
            );
            assert_eq!(
                super::SLNodeStyleGetLinearOrientation(native_node.as_ptr()),
                map_linear_orientation(
                    rust_tree
                        .style_linear_orientation(rust_node)
                        .expect("linear orientation")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetLinearGravity(native_node.as_ptr()),
                map_linear_gravity(
                    rust_tree
                        .style_linear_gravity(rust_node)
                        .expect("linear gravity")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetLinearLayoutGravity(native_node.as_ptr()),
                map_linear_layout_gravity(
                    rust_tree
                        .style_linear_layout_gravity(rust_node)
                        .expect("linear layout gravity")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetLinearCrossGravity(native_node.as_ptr()),
                map_linear_cross_gravity(
                    rust_tree
                        .style_linear_cross_gravity(rust_node)
                        .expect("linear cross gravity")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeCenter(native_node.as_ptr()),
                map_relative_center(
                    rust_tree
                        .style_relative_center(rust_node)
                        .expect("relative center")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetGridAutoFlow(native_node.as_ptr()),
                map_grid_auto_flow(
                    rust_tree
                        .style_grid_auto_flow(rust_node)
                        .expect("grid auto flow")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetJustifyItems(native_node.as_ptr()),
                map_justify_item(
                    rust_tree
                        .style_justify_items(rust_node)
                        .expect("justify items")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetJustifySelf(native_node.as_ptr()),
                map_justify_item(
                    rust_tree
                        .style_justify_self(rust_node)
                        .expect("justify self")
                )
            );
            assert_eq!(
                super::SLNodeStyleGetDisplay(native_node.as_ptr()),
                map_display(rust_tree.style_display(rust_node).expect("display"))
                    .expect("mapped display")
            );
            assert_eq!(
                super::SLNodeStyleGetBoxSizing(native_node.as_ptr()),
                map_box_sizing(rust_tree.style_box_sizing(rust_node).expect("box sizing"))
            );
            assert_close(
                super::SLNodeStyleGetAspectRatio(native_node.as_ptr()),
                rust_tree
                    .style_aspect_ratio(rust_node)
                    .expect("aspect ratio")
                    .expect("aspect ratio value"),
            );
            assert_eq!(
                super::SLNodeStyleGetOrder(native_node.as_ptr()),
                rust_tree.style_order(rust_node).expect("order")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeId(native_node.as_ptr()),
                rust_tree.style_relative_id(rust_node).expect("relative id")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeAlignTop(native_node.as_ptr()),
                rust_tree
                    .style_relative_align_top(rust_node)
                    .expect("relative align top")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeAlignRight(native_node.as_ptr()),
                rust_tree
                    .style_relative_align_right(rust_node)
                    .expect("relative align right")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeAlignBottom(native_node.as_ptr()),
                rust_tree
                    .style_relative_align_bottom(rust_node)
                    .expect("relative align bottom")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeAlignLeft(native_node.as_ptr()),
                rust_tree
                    .style_relative_align_left(rust_node)
                    .expect("relative align left")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeTopOf(native_node.as_ptr()),
                rust_tree
                    .style_relative_top_of(rust_node)
                    .expect("relative top of")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeRightOf(native_node.as_ptr()),
                rust_tree
                    .style_relative_right_of(rust_node)
                    .expect("relative right of")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeBottomOf(native_node.as_ptr()),
                rust_tree
                    .style_relative_bottom_of(rust_node)
                    .expect("relative bottom of")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeLeftOf(native_node.as_ptr()),
                rust_tree
                    .style_relative_left_of(rust_node)
                    .expect("relative left of")
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeLayoutOnce(native_node.as_ptr()),
                rust_tree
                    .style_relative_layout_once(rust_node)
                    .expect("relative layout once")
            );
            assert_eq!(
                super::SLNodeStyleGetGridColumnStart(native_node.as_ptr()),
                rust_tree
                    .style_grid_column_start(rust_node)
                    .expect("grid column start")
                    .expect("grid column start value")
            );
            assert_eq!(
                super::SLNodeStyleGetGridColumnEnd(native_node.as_ptr()),
                rust_tree
                    .style_grid_column_end(rust_node)
                    .expect("grid column end")
                    .expect("grid column end value")
            );
            assert_eq!(
                super::SLNodeStyleGetGridRowStart(native_node.as_ptr()),
                rust_tree
                    .style_grid_row_start(rust_node)
                    .expect("grid row start")
                    .expect("grid row start value")
            );
            assert_eq!(
                super::SLNodeStyleGetGridRowEnd(native_node.as_ptr()),
                rust_tree
                    .style_grid_row_end(rust_node)
                    .expect("grid row end")
                    .expect("grid row end value")
            );
            assert_eq!(
                super::SLNodeStyleGetGridColumnSpan(native_node.as_ptr()) as usize,
                rust_tree
                    .style_grid_column_span(rust_node)
                    .expect("grid column span")
            );
            assert_eq!(
                super::SLNodeStyleGetGridRowSpan(native_node.as_ptr()) as usize,
                rust_tree
                    .style_grid_row_span(rust_node)
                    .expect("grid row span")
            );
            assert_close(
                super::SLNodeStyleGetFlexGrow(native_node.as_ptr()),
                rust_tree.style_flex_grow(rust_node).expect("flex grow"),
            );
            assert_close(
                super::SLNodeStyleGetFlexShrink(native_node.as_ptr()),
                rust_tree.style_flex_shrink(rust_node).expect("flex shrink"),
            );
            assert_close(
                super::SLNodeStyleGetLinearWeight(native_node.as_ptr()),
                rust_tree
                    .style_linear_weight(rust_node)
                    .expect("linear weight"),
            );
            assert_close(
                super::SLNodeStyleGetLinearWeightSum(native_node.as_ptr()),
                rust_tree
                    .style_linear_weight_sum(rust_node)
                    .expect("linear weight sum"),
            );
            assert_close(
                super::SLNodeStyleGetBorder(native_node.as_ptr(), super::SL_EDGE_LEFT),
                rust_tree
                    .style_border(rust_node, starlight_standalone::StandaloneEdge::Left)
                    .expect("border"),
            );
            assert_value_eq(
                super::SLNodeStyleGetFlexBasis(native_node.as_ptr()),
                super::starlight_value_from_length(
                    rust_tree.style_flex_basis(rust_node).expect("flex basis"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetPosition(native_node.as_ptr(), super::SL_EDGE_LEFT),
                super::starlight_value_from_length(
                    rust_tree
                        .style_position(rust_node, starlight_standalone::StandaloneEdge::Left)
                        .expect("position"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetMargin(native_node.as_ptr(), super::SL_EDGE_RIGHT),
                super::starlight_value_from_length(
                    rust_tree
                        .style_margin(rust_node, starlight_standalone::StandaloneEdge::Right)
                        .expect("margin"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetPadding(native_node.as_ptr(), super::SL_EDGE_TOP),
                super::starlight_value_from_length(
                    rust_tree
                        .style_padding(rust_node, starlight_standalone::StandaloneEdge::Top)
                        .expect("padding"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetGap(native_node.as_ptr(), super::SL_GAP_ROW),
                super::starlight_value_from_length(
                    rust_tree
                        .style_gap(rust_node, starlight_standalone::StandaloneGap::Row)
                        .expect("gap"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetWidth(native_node.as_ptr()),
                super::starlight_value_from_length(
                    rust_tree.style_width(rust_node).expect("width"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetHeight(native_node.as_ptr()),
                super::starlight_value_from_length(
                    rust_tree.style_height(rust_node).expect("height"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetMinWidth(native_node.as_ptr()),
                super::starlight_value_from_length(
                    rust_tree.style_min_width(rust_node).expect("min width"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetMaxWidth(native_node.as_ptr()),
                super::starlight_value_from_length(
                    rust_tree.style_max_width(rust_node).expect("max width"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetMinHeight(native_node.as_ptr()),
                super::starlight_value_from_length(
                    rust_tree.style_min_height(rust_node).expect("min height"),
                ),
            );
            assert_value_eq(
                super::SLNodeStyleGetMaxHeight(native_node.as_ptr()),
                super::starlight_value_from_length(
                    rust_tree.style_max_height(rust_node).expect("max height"),
                ),
            );
        }
    }

    #[test]
    #[cfg(starlight_cpp_native_standalone)]
    fn standalone_native_style_getters_round_trip_public_setters() {
        let mut tree = NativeApiTree::new();
        let node = tree.create_node();

        // SAFETY: `node` is live for the whole test; every call targets the
        // standalone C API getter/setter pair for that node.
        unsafe {
            super::SLNodeStyleSetFlexDirection(node.as_ptr(), super::SL_FLEX_DIRECTION_ROW_REVERSE);
            super::SLNodeStyleSetJustifyContent(
                node.as_ptr(),
                super::SL_JUSTIFY_CONTENT_SPACE_BETWEEN,
            );
            super::SLNodeStyleSetAlignContent(node.as_ptr(), super::SL_ALIGN_CONTENT_SPACE_EVENLY);
            super::SLNodeStyleSetAlignItems(node.as_ptr(), super::SL_FLEX_ALIGN_BASELINE);
            super::SLNodeStyleSetAlignSelf(node.as_ptr(), super::SL_FLEX_ALIGN_END);
            super::SLNodeStyleSetPositionType(node.as_ptr(), super::SL_POSITION_TYPE_STICKY);
            super::SLNodeStyleSetFlexWrap(node.as_ptr(), super::SL_FLEX_WRAP_WRAP_REVERSE);
            super::SLNodeStyleSetLinearOrientation(
                node.as_ptr(),
                super::SL_LINEAR_ORIENTATION_VERTICAL_REVERSE,
            );
            super::SLNodeStyleSetLinearGravity(node.as_ptr(), super::SL_LINEAR_GRAVITY_CENTER);
            super::SLNodeStyleSetLinearLayoutGravity(
                node.as_ptr(),
                super::SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL,
            );
            super::SLNodeStyleSetLinearCrossGravity(
                node.as_ptr(),
                super::SL_LINEAR_CROSS_GRAVITY_STRETCH,
            );
            super::SLNodeStyleSetRelativeCenter(node.as_ptr(), super::SL_RELATIVE_CENTER_BOTH);
            super::SLNodeStyleSetGridAutoFlow(node.as_ptr(), super::SL_GRID_AUTO_FLOW_COLUMN_DENSE);
            super::SLNodeStyleSetJustifyItems(node.as_ptr(), super::SL_JUSTIFY_ITEM_CENTER);
            super::SLNodeStyleSetJustifySelf(node.as_ptr(), super::SL_JUSTIFY_ITEM_END);
            super::SLNodeStyleSetDisplay(node.as_ptr(), super::SL_DISPLAY_GRID);
            super::SLNodeStyleSetBoxSizing(node.as_ptr(), super::SL_BOX_SIZING_CONTENT_BOX);
            super::SLNodeStyleSetAspectRatio(node.as_ptr(), 1.5);
            super::SLNodeStyleSetOrder(node.as_ptr(), -2);
            super::SLNodeStyleSetRelativeId(node.as_ptr(), 17);
            super::SLNodeStyleSetRelativeAlignTop(node.as_ptr(), 1);
            super::SLNodeStyleSetRelativeAlignRight(node.as_ptr(), 2);
            super::SLNodeStyleSetRelativeAlignBottom(node.as_ptr(), 3);
            super::SLNodeStyleSetRelativeAlignLeft(node.as_ptr(), 4);
            super::SLNodeStyleSetRelativeTopOf(node.as_ptr(), 5);
            super::SLNodeStyleSetRelativeRightOf(node.as_ptr(), 6);
            super::SLNodeStyleSetRelativeBottomOf(node.as_ptr(), 7);
            super::SLNodeStyleSetRelativeLeftOf(node.as_ptr(), 8);
            super::SLNodeStyleSetRelativeLayoutOnce(node.as_ptr(), true);
            super::SLNodeStyleSetGridColumnStart(node.as_ptr(), 2);
            super::SLNodeStyleSetGridColumnEnd(node.as_ptr(), 4);
            super::SLNodeStyleSetGridRowStart(node.as_ptr(), 3);
            super::SLNodeStyleSetGridRowEnd(node.as_ptr(), 5);
            super::SLNodeStyleSetGridColumnSpan(node.as_ptr(), 6);
            super::SLNodeStyleSetGridRowSpan(node.as_ptr(), 7);
            super::SLNodeStyleSetFlex(node.as_ptr(), 2.5);
            super::SLNodeStyleSetLinearWeight(node.as_ptr(), 3.0);
            super::SLNodeStyleSetLinearWeightSum(node.as_ptr(), 9.0);
            super::SLNodeStyleSetBorder(node.as_ptr(), super::SL_EDGE_LEFT, 4.0);
            super::SLNodeStyleSetFlexBasisCalc(
                node.as_ptr(),
                super::starlight_value_from_calc_length(3.0, 20.0),
            );
            super::SLNodeStyleSetPositionAuto(node.as_ptr(), super::SL_EDGE_LEFT);
            super::SLNodeStyleSetMarginPercent(node.as_ptr(), super::SL_EDGE_RIGHT, 12.0);
            super::SLNodeStyleSetPaddingCalc(
                node.as_ptr(),
                super::SL_EDGE_TOP,
                super::starlight_value_from_calc_length(2.0, 5.0),
            );
            super::SLNodeStyleSetGap(node.as_ptr(), super::SL_GAP_ROW, 6.0);
            super::SLNodeStyleSetWidth(node.as_ptr(), 100.0);
            super::SLNodeStyleSetHeightPercent(node.as_ptr(), 50.0);
            super::SLNodeStyleSetMinWidthMaxContent(node.as_ptr());
            super::SLNodeStyleSetMaxWidthFitContentValue(
                node.as_ptr(),
                super::starlight_value_from_calc_length(10.0, 25.0),
            );
            super::SLNodeStyleSetMinHeight(node.as_ptr(), 7.0);
            super::SLNodeStyleSetMaxHeightFitContent(node.as_ptr());

            assert_eq!(
                super::SLNodeStyleGetFlexDirection(node.as_ptr()),
                super::SL_FLEX_DIRECTION_ROW_REVERSE
            );
            assert_eq!(
                super::SLNodeStyleGetJustifyContent(node.as_ptr()),
                super::SL_JUSTIFY_CONTENT_SPACE_BETWEEN
            );
            assert_eq!(
                super::SLNodeStyleGetAlignContent(node.as_ptr()),
                super::SL_ALIGN_CONTENT_SPACE_EVENLY
            );
            assert_eq!(
                super::SLNodeStyleGetAlignItems(node.as_ptr()),
                super::SL_FLEX_ALIGN_BASELINE
            );
            assert_eq!(
                super::SLNodeStyleGetAlignSelf(node.as_ptr()),
                super::SL_FLEX_ALIGN_END
            );
            assert_eq!(
                super::SLNodeStyleGetPositionType(node.as_ptr()),
                super::SL_POSITION_TYPE_STICKY
            );
            assert_eq!(
                super::SLNodeStyleGetFlexWrap(node.as_ptr()),
                super::SL_FLEX_WRAP_WRAP_REVERSE
            );
            assert_eq!(
                super::SLNodeStyleGetLinearOrientation(node.as_ptr()),
                super::SL_LINEAR_ORIENTATION_VERTICAL_REVERSE
            );
            assert_eq!(
                super::SLNodeStyleGetLinearGravity(node.as_ptr()),
                super::SL_LINEAR_GRAVITY_CENTER
            );
            assert_eq!(
                super::SLNodeStyleGetLinearLayoutGravity(node.as_ptr()),
                super::SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL
            );
            assert_eq!(
                super::SLNodeStyleGetLinearCrossGravity(node.as_ptr()),
                super::SL_LINEAR_CROSS_GRAVITY_STRETCH
            );
            assert_eq!(
                super::SLNodeStyleGetRelativeCenter(node.as_ptr()),
                super::SL_RELATIVE_CENTER_BOTH
            );
            assert_eq!(
                super::SLNodeStyleGetGridAutoFlow(node.as_ptr()),
                super::SL_GRID_AUTO_FLOW_COLUMN_DENSE
            );
            assert_eq!(
                super::SLNodeStyleGetJustifyItems(node.as_ptr()),
                super::SL_JUSTIFY_ITEM_CENTER
            );
            assert_eq!(
                super::SLNodeStyleGetJustifySelf(node.as_ptr()),
                super::SL_JUSTIFY_ITEM_END
            );
            assert_eq!(
                super::SLNodeStyleGetDisplay(node.as_ptr()),
                super::SL_DISPLAY_GRID
            );
            assert_eq!(
                super::SLNodeStyleGetBoxSizing(node.as_ptr()),
                super::SL_BOX_SIZING_CONTENT_BOX
            );
            assert_close(super::SLNodeStyleGetAspectRatio(node.as_ptr()), 1.5);
            assert_eq!(super::SLNodeStyleGetOrder(node.as_ptr()), -2);
            assert_eq!(super::SLNodeStyleGetRelativeId(node.as_ptr()), 17);
            assert_eq!(super::SLNodeStyleGetRelativeAlignTop(node.as_ptr()), 1);
            assert_eq!(super::SLNodeStyleGetRelativeAlignRight(node.as_ptr()), 2);
            assert_eq!(super::SLNodeStyleGetRelativeAlignBottom(node.as_ptr()), 3);
            assert_eq!(super::SLNodeStyleGetRelativeAlignLeft(node.as_ptr()), 4);
            assert_eq!(super::SLNodeStyleGetRelativeTopOf(node.as_ptr()), 5);
            assert_eq!(super::SLNodeStyleGetRelativeRightOf(node.as_ptr()), 6);
            assert_eq!(super::SLNodeStyleGetRelativeBottomOf(node.as_ptr()), 7);
            assert_eq!(super::SLNodeStyleGetRelativeLeftOf(node.as_ptr()), 8);
            assert!(super::SLNodeStyleGetRelativeLayoutOnce(node.as_ptr()));
            assert_eq!(super::SLNodeStyleGetGridColumnStart(node.as_ptr()), 2);
            assert_eq!(super::SLNodeStyleGetGridColumnEnd(node.as_ptr()), 4);
            assert_eq!(super::SLNodeStyleGetGridRowStart(node.as_ptr()), 3);
            assert_eq!(super::SLNodeStyleGetGridRowEnd(node.as_ptr()), 5);
            assert_eq!(super::SLNodeStyleGetGridColumnSpan(node.as_ptr()), 6);
            assert_eq!(super::SLNodeStyleGetGridRowSpan(node.as_ptr()), 7);
            assert_close(super::SLNodeStyleGetFlexGrow(node.as_ptr()), 2.5);
            assert_close(super::SLNodeStyleGetFlexShrink(node.as_ptr()), 1.0);
            assert_close(super::SLNodeStyleGetLinearWeight(node.as_ptr()), 3.0);
            assert_close(super::SLNodeStyleGetLinearWeightSum(node.as_ptr()), 9.0);
            assert_close(
                super::SLNodeStyleGetBorder(node.as_ptr(), super::SL_EDGE_LEFT),
                4.0,
            );
            assert_value_parts(
                super::SLNodeStyleGetFlexBasis(node.as_ptr()),
                super::SL_UNIT_CALC,
                3.0,
                20.0,
                super::SL_VALUE_FLAG_HAS_VALUE | super::SL_VALUE_FLAG_HAS_PERCENTAGE,
            );
            assert_value_unit(
                super::SLNodeStyleGetPosition(node.as_ptr(), super::SL_EDGE_LEFT),
                super::SL_UNIT_AUTO,
            );
            assert_value_scalar(
                super::SLNodeStyleGetMargin(node.as_ptr(), super::SL_EDGE_RIGHT),
                super::SL_UNIT_PERCENT,
                12.0,
            );
            assert_value_parts(
                super::SLNodeStyleGetPadding(node.as_ptr(), super::SL_EDGE_TOP),
                super::SL_UNIT_CALC,
                2.0,
                5.0,
                super::SL_VALUE_FLAG_HAS_VALUE | super::SL_VALUE_FLAG_HAS_PERCENTAGE,
            );
            assert_value_scalar(
                super::SLNodeStyleGetGap(node.as_ptr(), super::SL_GAP_ROW),
                super::SL_UNIT_POINT,
                6.0,
            );
            assert_value_scalar(
                super::SLNodeStyleGetWidth(node.as_ptr()),
                super::SL_UNIT_POINT,
                100.0,
            );
            assert_value_scalar(
                super::SLNodeStyleGetHeight(node.as_ptr()),
                super::SL_UNIT_PERCENT,
                50.0,
            );
            assert_value_unit(
                super::SLNodeStyleGetMinWidth(node.as_ptr()),
                super::SL_UNIT_MAX_CONTENT,
            );
            assert_value_parts(
                super::SLNodeStyleGetMaxWidth(node.as_ptr()),
                super::SL_UNIT_FIT_CONTENT,
                10.0,
                25.0,
                super::SL_VALUE_FLAG_HAS_VALUE | super::SL_VALUE_FLAG_HAS_PERCENTAGE,
            );
            assert_value_scalar(
                super::SLNodeStyleGetMinHeight(node.as_ptr()),
                super::SL_UNIT_POINT,
                7.0,
            );
            assert_value_unit(
                super::SLNodeStyleGetMaxHeight(node.as_ptr()),
                super::SL_UNIT_FIT_CONTENT,
            );
        }
    }

    #[test]
    fn native_baseline_equal_to_height_maps_to_fallback_none() {
        assert_eq!(native_baseline_to_result(20.0, 20.0), None);
    }

    #[test]
    fn native_zero_baseline_maps_to_missing_exported_baseline() {
        assert_eq!(native_baseline_to_result(0.0, 20.0), None);
    }

    #[test]
    fn standalone_default_config_uses_latest_non_quirks_mode() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let standalone_source = fs::read_to_string(
            manifest_dir.join("../../../../../services/starlight_standalone/core/src/starlight.cc"),
        )
        .expect("standalone C++ source should be readable");
        let layout_configs_source =
            fs::read_to_string(manifest_dir.join("../../../types/layout_configs.h"))
                .expect("layout config header should be readable");

        let default_config_region = source_region(
            &standalone_source,
            "static lynx::starlight::LayoutConfigs CreateDefaultLayoutConfigs()",
            "static const lynx::starlight::LayoutConfigs &GetDefaultLayoutConfigs()",
        );
        assert!(
            default_config_region.contains(&format!(
                "config.SetQuirksMode(lynx::{LATEST_QUIRKS_VERSION});"
            )),
            "standalone C++ baseline must opt into the latest non-quirks layout mode"
        );
        assert!(
            default_config_region.contains("config.css_align_with_legacy_w3c_ = true;"),
            "standalone C++ baseline must keep W3C-aligned display/alignment semantics"
        );
        assert!(
            default_config_region.contains("config.enable_fixed_new_ = true;"),
            "standalone C++ baseline must keep fixed-node root propagation enabled"
        );
        assert!(
            default_config_region
                .contains("config.SetTargetSDKVersion(kStarlightDefaultTargetSDKVersion);"),
            "standalone C++ baseline must opt into the standalone target SDK gate"
        );

        let versions = layout_config_versions(&layout_configs_source);
        let latest = versions
            .iter()
            .find(|version| version.name == LATEST_QUIRKS_VERSION)
            .expect("latest quirks version constant exists");
        for version in versions
            .iter()
            .filter(|version| version.name != "kQuirksModeEnableVersion")
        {
            assert!(
                latest.major_minor() >= version.major_minor(),
                "{LATEST_QUIRKS_VERSION} must stay at or above {name} to keep standalone parity in latest mode",
                name = version.name
            );
        }

        let set_quirks_mode_region = source_region(
            &layout_configs_source,
            "void SetQuirksMode(const base::Version& version)",
            "const base::Version& GetQuirksMode() const",
        );
        for version in versions
            .iter()
            .filter(|version| version.name != "kQuirksModeEnableVersion")
        {
            assert!(
                set_quirks_mode_region.contains(&format!("IsVersionHigherOrEqual({})", version.name)),
                "SetQuirksMode must gate {name}; otherwise latest-mode parity can silently miss a compatibility flag",
                name = version.name
            );
        }
    }

    #[test]
    fn source_built_native_baseline_compiles_every_starlight_cc_source() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let build_rs = fs::read_to_string(manifest_dir.join("build.rs"))
            .expect("starlight_cpp build script should be readable");
        let starlight_build = fs::read_to_string(manifest_dir.join("../../../BUILD.gn"))
            .expect("Starlight BUILD.gn should be readable");

        let native_starlight_sources = rust_string_array_values(
            &build_rs,
            "const STARLIGHT_NATIVE_SOURCES: &[&str] = &[",
            "];",
        )
        .into_iter()
        .filter(|source| source.starts_with("core/renderer/starlight/") && source.ends_with(".cc"))
        .collect::<BTreeSet<_>>();
        let expected_sources = [
            "starlight_layout_sources",
            "starlight_style_sources",
            "starlight_types_sources",
        ]
        .into_iter()
        .flat_map(|array_name| gn_string_array_values(&starlight_build, array_name))
        .filter(|source| source.ends_with(".cc"))
        .map(|source| format!("core/renderer/starlight/{source}"))
        .collect::<BTreeSet<_>>();

        assert_eq!(
            native_starlight_sources, expected_sources,
            "source-built C++ baseline must compile exactly the Starlight .cc files from the production starlight GN target"
        );

        let native_sources = rust_string_array_values(
            &build_rs,
            "const STARLIGHT_NATIVE_SOURCES: &[&str] = &[",
            "];",
        );
        for required_source in [
            "core/services/starlight_standalone/core/src/starlight.cc",
            "core/services/starlight_standalone/core/src/starlight_config.cc",
            "base/src/value/base_string.cc",
        ] {
            assert!(
                native_sources
                    .iter()
                    .any(|source| source == required_source),
                "source-built C++ baseline must include {required_source}"
            );
        }
    }

    #[test]
    fn source_built_native_baseline_reruns_when_headers_change() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let repo_root = manifest_dir.join("../../../../../..");
        let build_rs = fs::read_to_string(manifest_dir.join("build.rs"))
            .expect("starlight_cpp build script should be readable");

        let rerun_dirs = rust_string_array_values(
            &build_rs,
            "const STARLIGHT_NATIVE_RERUN_DIRS: &[&str] = &[",
            "];",
        )
        .into_iter()
        .collect::<BTreeSet<_>>();
        let required_dirs = [
            "base/include",
            "base/src/log",
            "base/src/value",
            "core/base",
            "core/include/starlight_standalone",
            "core/renderer/starlight/layout",
            "core/renderer/starlight/style",
            "core/renderer/starlight/types",
            "core/services/starlight_standalone/core/src",
            "core/style",
            "third_party/rapidjson",
        ]
        .into_iter()
        .map(str::to_owned)
        .collect::<BTreeSet<_>>();
        assert_eq!(
            rerun_dirs, required_dirs,
            "source-built C++ baseline must rerun when native headers or header-only deps change"
        );

        for dir in &required_dirs {
            assert!(
                repo_root.join(dir).exists(),
                "source-built C++ baseline rerun directory must exist: {dir}"
            );
        }

        let rerun_loop = source_region(
            &build_rs,
            "for dir in STARLIGHT_NATIVE_RERUN_DIRS",
            "for source in STARLIGHT_NATIVE_SOURCES",
        );
        assert!(
            rerun_loop.contains("cargo:rerun-if-changed={}")
                && rerun_loop.contains("repo_root.join(dir).display()"),
            "build script must register every STARLIGHT_NATIVE_RERUN_DIRS entry with Cargo"
        );
    }

    #[test]
    fn standalone_header_style_setters_are_declared_or_explicitly_exempted() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let standalone_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight.h"),
        )
        .expect("standalone public header should be readable");

        let header_setters = standalone_header_style_setters(&standalone_header);
        let native_declarations = native_extern_function_names(NATIVE_SOURCE);
        let exempted_setters = explicitly_exempted_standalone_style_setters();

        let missing_setters = header_setters
            .difference(&native_declarations)
            .filter(|setter| !exempted_setters.contains(*setter))
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            missing_setters.is_empty(),
            "native adapter must declare every standalone style setter or explicitly exempt it: {}",
            missing_setters.join(", ")
        );

        let stale_exemptions = exempted_setters
            .difference(&header_setters)
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            stale_exemptions.is_empty(),
            "standalone style setter exemptions must name existing public header functions: {}",
            stale_exemptions.join(", ")
        );

        let declared_exemptions = exempted_setters
            .intersection(&native_declarations)
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            declared_exemptions.is_empty(),
            "remove exemptions once a standalone style setter is imported explicitly: {}",
            declared_exemptions.join(", ")
        );
    }

    #[test]
    fn standalone_header_layout_getters_are_declared_and_read() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let standalone_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight.h"),
        )
        .expect("standalone public header should be readable");

        let header_getters = standalone_header_layout_getters(&standalone_header);
        let native_declarations = native_extern_function_names(NATIVE_SOURCE);
        let missing_getters = header_getters
            .difference(&native_declarations)
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            missing_getters.is_empty(),
            "native adapter must declare every standalone layout result getter: {}",
            missing_getters.join(", ")
        );

        let readback_region = source_region(
            NATIVE_SOURCE,
            "fn read_native_size(",
            "fn native_baseline_to_result(",
        );
        let unread_getters = header_getters
            .iter()
            .filter(|getter| !readback_region.contains(getter.as_str()))
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            unread_getters.is_empty(),
            "native adapter must read every standalone layout result getter so head-to-head parity covers new result fields: {}",
            unread_getters.join(", ")
        );
    }

    #[test]
    fn standalone_header_style_getters_are_declared_and_read_by_public_snapshots() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let standalone_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight.h"),
        )
        .expect("standalone public header should be readable");

        let header_getters = standalone_header_style_getters(&standalone_header);
        let native_declarations = native_extern_function_names(NATIVE_SOURCE);
        let missing_getters = header_getters
            .difference(&native_declarations)
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            missing_getters.is_empty(),
            "native adapter must declare every standalone style getter: {}",
            missing_getters.join(", ")
        );

        let public_snapshot_readback = [
            source_region(
                NATIVE_SOURCE,
                "fn snapshot_public_scalar_style(",
                "unsafe fn apply_public_dimension_style_variant(",
            ),
            source_region(
                NATIVE_SOURCE,
                "fn snapshot_public_edge_style_stage(",
                "unsafe fn apply_public_edge_style_variant(",
            ),
        ]
        .join("\n");
        let unread_getters = header_getters
            .iter()
            .filter(|getter| !public_snapshot_readback.contains(getter.as_str()))
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            unread_getters.is_empty(),
            "public standalone style snapshots must read every style getter so Rust-vs-C++ parity covers new getter fields: {}",
            unread_getters.join(", ")
        );
    }

    #[test]
    fn native_extern_functions_are_declared_in_standalone_header() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let standalone_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight.h"),
        )
        .expect("standalone public header should be readable");
        let standalone_config_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight_config.h"),
        )
        .expect("standalone config public header should be readable");

        let header_functions = standalone_header_function_names(&standalone_header)
            .into_iter()
            .chain(standalone_header_function_names(&standalone_config_header))
            .collect::<BTreeSet<_>>();
        let missing_functions = native_extern_function_names(NATIVE_SOURCE)
            .difference(&header_functions)
            .cloned()
            .collect::<Vec<_>>();

        assert!(
            missing_functions.is_empty(),
            "native adapter extern block must only import functions declared by the public standalone header: {}",
            missing_functions.join(", ")
        );

        let unimported_functions = header_functions
            .difference(&native_extern_function_names(NATIVE_SOURCE))
            .cloned()
            .collect::<Vec<_>>();

        assert!(
            unimported_functions.is_empty(),
            "native adapter must import every public standalone header function so C++ baseline coverage can drive the full API surface: {}",
            unimported_functions.join(", ")
        );
    }

    #[test]
    fn standalone_header_non_style_functions_are_exercised_outside_extern_block() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let standalone_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight.h"),
        )
        .expect("standalone public header should be readable");
        let standalone_config_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight_config.h"),
        )
        .expect("standalone config public header should be readable");

        let header_functions = standalone_header_function_names(&standalone_header)
            .into_iter()
            .chain(standalone_header_function_names(&standalone_config_header))
            .filter(|name| !name.starts_with("SLNodeStyle"))
            .collect::<BTreeSet<_>>();
        let exercised_source = native_source_after_extern_block(NATIVE_SOURCE);
        let unexercised_functions = header_functions
            .iter()
            .filter(|function| !exercised_source.contains(&format!("{function}(")))
            .cloned()
            .collect::<Vec<_>>();

        assert!(
            unexercised_functions.is_empty(),
            "native adapter must exercise every public standalone non-style function outside the extern declarations: {}",
            unexercised_functions.join(", ")
        );
    }

    #[test]
    fn standalone_header_enum_values_match_native_constants_or_explicit_exemptions() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let enum_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight_enums.h"),
        )
        .expect("standalone enum header should be readable");

        let header_values = header_enum_values(&enum_header);
        let native_values = native_standalone_enum_constants(NATIVE_SOURCE);
        let exempted_variants = explicitly_exempted_standalone_enum_variants();
        let mut imported_variants = BTreeSet::new();

        for (native_name, native_value) in &native_values {
            let header_variant = header_variant_from_native_constant(native_name);
            let header_value = header_values.get(&header_variant).unwrap_or_else(|| {
                panic!("{native_name} must map to an existing standalone enum variant")
            });
            assert_eq!(
                native_value, header_value,
                "{native_name} must match standalone header value for {header_variant}"
            );
            imported_variants.insert(header_variant);
        }

        let missing_variants = header_values
            .keys()
            .filter(|variant| {
                !imported_variants.contains(*variant) && !exempted_variants.contains(*variant)
            })
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            missing_variants.is_empty(),
            "native adapter must import every standalone enum variant or explicitly exempt it: {}",
            missing_variants.join(", ")
        );

        let stale_exemptions = exempted_variants
            .difference(&header_values.keys().cloned().collect())
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            stale_exemptions.is_empty(),
            "standalone enum variant exemptions must name existing public header variants: {}",
            stale_exemptions.join(", ")
        );

        let imported_exemptions = exempted_variants
            .intersection(&imported_variants)
            .cloned()
            .collect::<Vec<_>>();
        assert!(
            imported_exemptions.is_empty(),
            "remove enum exemptions once a standalone variant is imported explicitly: {}",
            imported_exemptions.join(", ")
        );
    }

    #[test]
    fn standalone_value_header_matches_native_repr_and_constants() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let value_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight_value.h"),
        )
        .expect("standalone value header should be readable");

        assert_eq!(
            header_c_struct_fields(&value_header, "StarlightSize"),
            rust_repr_struct_fields(NATIVE_SOURCE, "StarlightSize"),
            "native StarlightSize repr(C) fields must match standalone header order and types"
        );
        assert_eq!(
            header_c_struct_fields(&value_header, "StarlightValue"),
            rust_repr_struct_fields(NATIVE_SOURCE, "StarlightValue"),
            "native StarlightValue repr(C) fields must match standalone header order and types"
        );
        assert_eq!(
            header_define_i32(&value_header, "SLValueFlagHasValue"),
            native_i32_const(NATIVE_SOURCE, "SL_VALUE_FLAG_HAS_VALUE"),
            "SLValueFlagHasValue must match the native adapter's hardcoded flag bit"
        );
        assert_eq!(
            header_define_i32(&value_header, "SLValueFlagHasPercentage"),
            native_i32_const(NATIVE_SOURCE, "SL_VALUE_FLAG_HAS_PERCENTAGE"),
            "SLValueFlagHasPercentage must match the native adapter's hardcoded flag bit"
        );
        assert_eq!(
            header_define_f32(&value_header, "SLUndefined"),
            native_f32_const(NATIVE_SOURCE, "SL_UNDEFINED"),
            "SLUndefined must match the native adapter's hardcoded undefined sentinel"
        );
    }

    #[test]
    fn native_baseline_different_from_height_is_exported() {
        assert_eq!(native_baseline_to_result(12.0, 20.0), Some(12.0));
    }

    #[test]
    fn standalone_axis_lengths_support_intrinsic_width_height_values() {
        assert!(axis_length_is_standalone_supported(Length::calc(1.0, 10.0)));
        assert!(axis_length_is_standalone_supported(Length::MaxContent));
        assert!(axis_length_is_standalone_supported(Length::FitContent(
            None
        )));
        assert!(axis_length_is_standalone_supported(Length::FitContent(
            Some(BaseLength::fixed(20.0))
        )));
    }

    #[test]
    fn owner_constraints_preserve_root_measure_modes() {
        assert_eq!(
            owner_constraint_to_native(SideConstraint::indefinite()),
            (SL_UNDEFINED, SL_NODE_MEASURE_MODE_UNDEFINED)
        );
        assert_eq!(
            owner_constraint_to_native(SideConstraint::definite(42.0)),
            (42.0, SL_NODE_MEASURE_MODE_EXACTLY)
        );
        assert_eq!(
            owner_constraint_to_native(SideConstraint::at_most(88.0)),
            (88.0, SL_NODE_MEASURE_MODE_AT_MOST)
        );
    }

    #[test]
    fn standalone_core_enums_map_all_public_variants() {
        assert_eq!(map_display(Display::None), Ok(SL_DISPLAY_NONE));
        assert_eq!(map_display(Display::Flex), Ok(SL_DISPLAY_FLEX));
        assert_eq!(map_display(Display::Grid), Ok(SL_DISPLAY_GRID));
        assert_eq!(map_display(Display::Linear), Ok(SL_DISPLAY_LINEAR));
        assert_eq!(map_display(Display::Relative), Ok(SL_DISPLAY_RELATIVE));
        assert_eq!(map_display(Display::Block), Ok(SL_DISPLAY_BLOCK));

        assert_eq!(map_direction(Direction::Ltr), SL_DIRECTION_LTR);
        assert_eq!(map_direction(Direction::Rtl), SL_DIRECTION_RTL);
        assert_eq!(SL_DIRECTION_NORMAL, 0);
        assert_eq!(SL_DIRECTION_LYNX_RTL, 1);

        assert_eq!(
            map_position(&Style::default()),
            Ok(SL_POSITION_TYPE_RELATIVE)
        );
        assert_eq!(
            map_position(&Style {
                position: PositionType::Relative,
                ..Style::default()
            }),
            Ok(SL_POSITION_TYPE_RELATIVE)
        );
        assert_eq!(
            map_position(&Style {
                position: PositionType::Absolute,
                ..Style::default()
            }),
            Ok(SL_POSITION_TYPE_ABSOLUTE)
        );
        assert_eq!(
            map_position(&Style {
                position: PositionType::Fixed,
                ..Style::default()
            }),
            Ok(SL_POSITION_TYPE_FIXED)
        );
        assert_eq!(
            map_position(&Style {
                position: PositionType::Sticky,
                ..Style::default()
            }),
            Ok(SL_POSITION_TYPE_STICKY)
        );

        assert_eq!(
            map_box_sizing(BoxSizing::BorderBox),
            SL_BOX_SIZING_BORDER_BOX
        );
        assert_eq!(
            map_box_sizing(BoxSizing::ContentBox),
            SL_BOX_SIZING_CONTENT_BOX
        );
    }

    #[test]
    fn standalone_visibility_is_rejected_until_public_c_api_maps_it() {
        assert!(matches!(
            ensure_standalone_supported(&Style {
                visibility: Visibility::Hidden,
                ..Style::default()
            }),
            Err(crate::CppBaselineError::UnsupportedStyle(reason))
                if reason.contains("visibility")
        ));
        assert!(matches!(
            ensure_standalone_supported(&Style {
                visibility: Visibility::Collapse,
                ..Style::default()
            }),
            Err(crate::CppBaselineError::UnsupportedStyle(reason))
                if reason.contains("visibility")
        ));
    }

    #[test]
    fn standalone_layout_mode_enums_map_all_public_variants() {
        assert_eq!(
            map_flex_direction(FlexDirection::Row),
            SL_FLEX_DIRECTION_ROW
        );
        assert_eq!(
            map_flex_direction(FlexDirection::RowReverse),
            SL_FLEX_DIRECTION_ROW_REVERSE
        );
        assert_eq!(
            map_flex_direction(FlexDirection::Column),
            SL_FLEX_DIRECTION_COLUMN
        );
        assert_eq!(
            map_flex_direction(FlexDirection::ColumnReverse),
            SL_FLEX_DIRECTION_COLUMN_REVERSE
        );

        assert_eq!(map_flex_wrap(FlexWrap::NoWrap), SL_FLEX_WRAP_NOWRAP);
        assert_eq!(map_flex_wrap(FlexWrap::Wrap), SL_FLEX_WRAP_WRAP);
        assert_eq!(
            map_flex_wrap(FlexWrap::WrapReverse),
            SL_FLEX_WRAP_WRAP_REVERSE
        );

        assert_eq!(
            map_linear_orientation(LinearOrientation::Horizontal),
            SL_LINEAR_ORIENTATION_HORIZONTAL
        );
        assert_eq!(
            map_linear_orientation(LinearOrientation::HorizontalReverse),
            SL_LINEAR_ORIENTATION_HORIZONTAL_REVERSE
        );
        assert_eq!(
            map_linear_orientation(LinearOrientation::Vertical),
            SL_LINEAR_ORIENTATION_VERTICAL
        );
        assert_eq!(
            map_linear_orientation(LinearOrientation::VerticalReverse),
            SL_LINEAR_ORIENTATION_VERTICAL_REVERSE
        );
        assert_eq!(
            map_linear_orientation(LinearOrientation::Row),
            SL_LINEAR_ORIENTATION_ROW
        );
        assert_eq!(
            map_linear_orientation(LinearOrientation::RowReverse),
            SL_LINEAR_ORIENTATION_ROW_REVERSE
        );
        assert_eq!(
            map_linear_orientation(LinearOrientation::Column),
            SL_LINEAR_ORIENTATION_COLUMN
        );
        assert_eq!(
            map_linear_orientation(LinearOrientation::ColumnReverse),
            SL_LINEAR_ORIENTATION_COLUMN_REVERSE
        );

        assert_eq!(
            map_linear_gravity(LinearGravity::None),
            SL_LINEAR_GRAVITY_NONE
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::Top),
            SL_LINEAR_GRAVITY_TOP
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::Bottom),
            SL_LINEAR_GRAVITY_BOTTOM
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::Left),
            SL_LINEAR_GRAVITY_LEFT
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::Right),
            SL_LINEAR_GRAVITY_RIGHT
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::CenterVertical),
            SL_LINEAR_GRAVITY_CENTER_VERTICAL
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::CenterHorizontal),
            SL_LINEAR_GRAVITY_CENTER_HORIZONTAL
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::SpaceBetween),
            SL_LINEAR_GRAVITY_SPACE_BETWEEN
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::Start),
            SL_LINEAR_GRAVITY_START
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::End),
            SL_LINEAR_GRAVITY_END
        );
        assert_eq!(
            map_linear_gravity(LinearGravity::Center),
            SL_LINEAR_GRAVITY_CENTER
        );

        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::None),
            SL_LINEAR_LAYOUT_GRAVITY_NONE
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::Top),
            SL_LINEAR_LAYOUT_GRAVITY_TOP
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::Bottom),
            SL_LINEAR_LAYOUT_GRAVITY_BOTTOM
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::Left),
            SL_LINEAR_LAYOUT_GRAVITY_LEFT
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::Right),
            SL_LINEAR_LAYOUT_GRAVITY_RIGHT
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::CenterVertical),
            SL_LINEAR_LAYOUT_GRAVITY_CENTER_VERTICAL
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::CenterHorizontal),
            SL_LINEAR_LAYOUT_GRAVITY_CENTER_HORIZONTAL
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::FillVertical),
            SL_LINEAR_LAYOUT_GRAVITY_FILL_VERTICAL
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::FillHorizontal),
            SL_LINEAR_LAYOUT_GRAVITY_FILL_HORIZONTAL
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::Center),
            SL_LINEAR_LAYOUT_GRAVITY_CENTER
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::Stretch),
            SL_LINEAR_LAYOUT_GRAVITY_STRETCH
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::Start),
            SL_LINEAR_LAYOUT_GRAVITY_START
        );
        assert_eq!(
            map_linear_layout_gravity(LinearLayoutGravity::End),
            SL_LINEAR_LAYOUT_GRAVITY_END
        );

        assert_eq!(
            map_linear_cross_gravity(LinearCrossGravity::None),
            SL_LINEAR_CROSS_GRAVITY_NONE
        );
        assert_eq!(
            map_linear_cross_gravity(LinearCrossGravity::Start),
            SL_LINEAR_CROSS_GRAVITY_START
        );
        assert_eq!(
            map_linear_cross_gravity(LinearCrossGravity::End),
            SL_LINEAR_CROSS_GRAVITY_END
        );
        assert_eq!(
            map_linear_cross_gravity(LinearCrossGravity::Center),
            SL_LINEAR_CROSS_GRAVITY_CENTER
        );
        assert_eq!(
            map_linear_cross_gravity(LinearCrossGravity::Stretch),
            SL_LINEAR_CROSS_GRAVITY_STRETCH
        );

        assert_eq!(
            map_relative_center(RelativeCenter::None),
            SL_RELATIVE_CENTER_NONE
        );
        assert_eq!(
            map_relative_center(RelativeCenter::Horizontal),
            SL_RELATIVE_CENTER_HORIZONTAL
        );
        assert_eq!(
            map_relative_center(RelativeCenter::Vertical),
            SL_RELATIVE_CENTER_VERTICAL
        );
        assert_eq!(
            map_relative_center(RelativeCenter::Both),
            SL_RELATIVE_CENTER_BOTH
        );

        assert_eq!(map_justify_item(JustifyItems::Auto), SL_JUSTIFY_ITEM_AUTO);
        assert_eq!(
            map_justify_item(JustifyItems::Stretch),
            SL_JUSTIFY_ITEM_STRETCH
        );
        assert_eq!(map_justify_item(JustifyItems::Start), SL_JUSTIFY_ITEM_START);
        assert_eq!(map_justify_item(JustifyItems::End), SL_JUSTIFY_ITEM_END);
        assert_eq!(
            map_justify_item(JustifyItems::Center),
            SL_JUSTIFY_ITEM_CENTER
        );
    }

    #[test]
    fn native_adapter_apply_and_validation_mentions_every_style_field() {
        let native_style_region = native_style_application_region(NATIVE_SOURCE);
        let missing_fields = style_fields(STYLE_SOURCE)
            .into_iter()
            .filter(|field| !native_style_region.contains(&format!("style.{field}")))
            .collect::<Vec<_>>();

        assert!(
            missing_fields.is_empty(),
            "native adapter apply/validation path must explicitly handle every Style field; missing: {}",
            missing_fields.join(", ")
        );
    }

    #[test]
    fn standalone_edge_lengths_support_full_starlight_value_units() {
        assert!(length_is_standalone_supported(Length::Auto));
        assert!(length_is_standalone_supported(Length::points(10.0)));
        assert!(length_is_standalone_supported(Length::percent(10.0)));
        assert!(length_is_standalone_supported(Length::calc(1.0, 10.0)));
        assert!(length_is_standalone_supported(Length::MaxContent));
        assert!(length_is_standalone_supported(Length::FitContent(None)));
        assert!(length_is_standalone_supported(Length::FitContent(Some(
            BaseLength::fixed_and_percent(1.0, 10.0)
        ))));
        assert!(length_is_standalone_supported(Length::fr(1.0)));
    }

    #[test]
    fn standalone_row_column_gap_lengths_support_full_starlight_value_units() {
        assert!(gap_length_is_standalone_supported(Length::Auto));
        assert!(gap_length_is_standalone_supported(Length::points(10.0)));
        assert!(gap_length_is_standalone_supported(Length::percent(10.0)));
        assert!(gap_length_is_standalone_supported(Length::calc(1.0, 10.0)));
        assert!(gap_length_is_standalone_supported(Length::MaxContent));
        assert!(gap_length_is_standalone_supported(Length::FitContent(None)));
        assert!(gap_length_is_standalone_supported(Length::FitContent(
            Some(BaseLength::fixed_and_percent(1.0, 10.0))
        )));
        assert!(gap_length_is_standalone_supported(Length::fr(1.0)));
    }

    #[test]
    fn standalone_edge_lengths_support_percent_and_calc_insets() {
        let style = Style {
            position: PositionType::Absolute,
            left: Length::percent(10.0),
            right: Length::calc(5.0, 20.0),
            top: Length::calc(2.0, 10.0),
            bottom: Length::percent(25.0),
            margin: Rect::new(
                Length::points(3.0),
                Length::percent(4.0),
                Length::calc(1.0, 5.0),
                Length::points(6.0),
            ),
            padding: Rect::new(
                Length::percent(2.0),
                Length::calc(3.0, 6.0),
                Length::points(1.0),
                Length::points(4.0),
            ),
            row_gap: Length::calc(1.0, 10.0),
            column_gap: Length::percent(5.0),
            ..Style::default()
        };
        assert!(ensure_standalone_lengths_supported(&style).is_ok());

        let calc = starlight_value_from_calc_length(5.0, 20.0);
        assert_eq!(calc.unit, SL_UNIT_CALC);
        assert_eq!(
            calc.flags,
            SL_VALUE_FLAG_HAS_VALUE | SL_VALUE_FLAG_HAS_PERCENTAGE
        );
        assert_eq!(calc.value, 5.0);
        assert_eq!(calc.percentage, 20.0);
    }

    #[test]
    fn standalone_min_max_lengths_support_fit_content_with_base() {
        assert!(min_max_length_is_standalone_supported(Length::points(10.0)));
        assert!(min_max_length_is_standalone_supported(Length::percent(
            10.0
        )));
        assert!(min_max_length_is_standalone_supported(Length::calc(
            1.0, 10.0
        )));
        assert!(min_max_length_is_standalone_supported(Length::FitContent(
            Some(BaseLength::fixed(20.0))
        )));
        assert!(min_max_length_is_standalone_supported(Length::FitContent(
            None
        )));
        assert!(min_max_length_is_standalone_supported(Length::MaxContent));
        assert!(min_max_length_is_standalone_supported(Length::fr(1.0)));
    }

    #[test]
    fn standalone_flex_basis_supports_intrinsic_values() {
        assert!(flex_basis_length_is_standalone_supported(Length::Auto));
        assert!(flex_basis_length_is_standalone_supported(Length::points(
            10.0
        )));
        assert!(flex_basis_length_is_standalone_supported(Length::percent(
            10.0
        )));
        assert!(flex_basis_length_is_standalone_supported(Length::calc(
            1.0, 10.0
        )));
        assert!(flex_basis_length_is_standalone_supported(
            Length::MaxContent
        ));
        assert!(flex_basis_length_is_standalone_supported(
            Length::FitContent(None)
        ));
        assert!(flex_basis_length_is_standalone_supported(
            Length::FitContent(Some(BaseLength::fixed(20.0)))
        ));
        assert!(flex_basis_length_is_standalone_supported(Length::fr(1.0)));
    }

    #[test]
    fn standalone_grid_track_lengths_support_w3c_track_units() {
        assert!(grid_track_length_is_standalone_supported(Length::Auto));
        assert!(grid_track_length_is_standalone_supported(Length::points(
            10.0
        )));
        assert!(grid_track_length_is_standalone_supported(Length::percent(
            10.0
        )));
        assert!(grid_track_length_is_standalone_supported(Length::calc(
            1.0, 10.0
        )));
        assert!(grid_track_length_is_standalone_supported(
            Length::MaxContent
        ));
        assert!(grid_track_length_is_standalone_supported(
            Length::FitContent(Some(BaseLength::fixed(20.0)))
        ));
        assert!(grid_track_length_is_standalone_supported(Length::fr(1.0)));
        assert!(!grid_track_length_is_standalone_supported(
            Length::FitContent(None)
        ));
        assert!(!grid_track_length_is_standalone_supported(
            Length::FitContent(Some(BaseLength::empty()))
        ));
    }

    #[test]
    fn standalone_length_validation_rejects_min_content_without_cpp_nlength_unit() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let cxx_nlength_header = fs::read_to_string(
            manifest_dir.join("../../../../../renderer/starlight/types/nlength.h"),
        )
        .expect("read C++ NLength header");
        let standalone_enum_header = fs::read_to_string(
            manifest_dir.join("../../../../../include/starlight_standalone/starlight_enums.h"),
        )
        .expect("read standalone enum header");

        assert!(
            !cxx_nlength_header.contains("kNLengthMinContent")
                && !cxx_nlength_header.contains("MakeMinContentNLength"),
            "current C++ NLength surface does not expose min-content"
        );
        assert!(
            !standalone_enum_header.contains("SLUnitMinContent"),
            "standalone public C API does not expose a min-content value unit"
        );
        assert!(!length_is_standalone_supported(Length::MinContent));
        assert!(!gap_length_is_standalone_supported(Length::MinContent));
        assert!(!list_gap_length_is_standalone_supported(Length::MinContent));
        assert!(!axis_length_is_standalone_supported(Length::MinContent));
        assert!(!min_max_length_is_standalone_supported(Length::MinContent));
        assert!(!flex_basis_length_is_standalone_supported(
            Length::MinContent
        ));
        assert!(!grid_track_length_is_standalone_supported(
            Length::MinContent
        ));

        let style = Style {
            grid_template_columns: vec![Length::MinContent],
            ..Style::default()
        };
        assert!(matches!(
            ensure_standalone_lengths_supported(&style),
            Err(CppBaselineError::UnsupportedStyle(
                "unsupported grid track length in standalone C API"
            ))
        ));
    }

    #[test]
    fn standalone_grid_track_fit_content_encoding_preserves_base_flags() {
        assert!(grid_track_length_is_standalone_supported(
            Length::FitContent(Some(BaseLength::fixed_and_percent(10.0, 50.0)))
        ));

        let fixed_base = starlight_value_from_grid_track_length(Length::FitContent(Some(
            BaseLength::fixed(40.0),
        )));
        assert_eq!(fixed_base.unit, SL_UNIT_FIT_CONTENT);
        assert_eq!(fixed_base.flags, SL_VALUE_FLAG_HAS_VALUE);
        assert_eq!(fixed_base.value, 40.0);
        assert_eq!(fixed_base.percentage, 0.0);

        let percent_base = starlight_value_from_grid_track_length(Length::FitContent(Some(
            BaseLength::fixed_and_percent(0.0, 50.0),
        )));
        assert_eq!(percent_base.unit, SL_UNIT_FIT_CONTENT);
        assert_eq!(
            percent_base.flags,
            SL_VALUE_FLAG_HAS_VALUE | SL_VALUE_FLAG_HAS_PERCENTAGE
        );
        assert_eq!(percent_base.value, 0.0);
        assert_eq!(percent_base.percentage, 50.0);

        let calc_base = starlight_value_from_grid_track_length(Length::FitContent(Some(
            BaseLength::fixed_and_percent(10.0, 50.0),
        )));
        assert_eq!(calc_base.unit, SL_UNIT_FIT_CONTENT);
        assert_eq!(
            calc_base.flags,
            SL_VALUE_FLAG_HAS_VALUE | SL_VALUE_FLAG_HAS_PERCENTAGE
        );
        assert_eq!(calc_base.value, 10.0);
        assert_eq!(calc_base.percentage, 50.0);
    }

    #[test]
    fn standalone_grid_auto_flow_maps_all_public_variants() {
        assert_eq!(map_grid_auto_flow(GridAutoFlow::Row), SL_GRID_AUTO_FLOW_ROW);
        assert_eq!(
            map_grid_auto_flow(GridAutoFlow::Column),
            SL_GRID_AUTO_FLOW_COLUMN
        );
        assert_eq!(
            map_grid_auto_flow(GridAutoFlow::Dense),
            SL_GRID_AUTO_FLOW_DENSE
        );
        assert_eq!(
            map_grid_auto_flow(GridAutoFlow::RowDense),
            SL_GRID_AUTO_FLOW_ROW_DENSE
        );
        assert_eq!(
            map_grid_auto_flow(GridAutoFlow::ColumnDense),
            SL_GRID_AUTO_FLOW_COLUMN_DENSE
        );
    }

    #[test]
    fn standalone_list_gap_lengths_support_full_starlight_value_units() {
        assert!(list_gap_length_is_standalone_supported(Length::Auto));
        assert!(list_gap_length_is_standalone_supported(Length::points(
            10.0
        )));
        assert!(list_gap_length_is_standalone_supported(Length::percent(
            10.0
        )));
        assert!(list_gap_length_is_standalone_supported(Length::calc(
            1.0, 10.0
        )));
        assert!(list_gap_length_is_standalone_supported(Length::MaxContent));
        assert!(list_gap_length_is_standalone_supported(Length::FitContent(
            None
        )));
        assert!(list_gap_length_is_standalone_supported(Length::FitContent(
            Some(BaseLength::fixed_and_percent(1.0, 10.0))
        )));
        assert!(list_gap_length_is_standalone_supported(Length::fr(1.0)));
    }

    #[test]
    fn standalone_align_content_maps_space_evenly_without_lossy_mapping() {
        assert_eq!(
            map_align_content(AlignContent::SpaceEvenly),
            Ok(SL_ALIGN_CONTENT_SPACE_EVENLY)
        );
    }

    #[test]
    fn standalone_align_content_start_end_alias_to_flex_edges() {
        assert_eq!(
            map_align_content(AlignContent::Start),
            Ok(SL_ALIGN_CONTENT_FLEX_START)
        );
        assert_eq!(
            map_align_content(AlignContent::End),
            Ok(SL_ALIGN_CONTENT_FLEX_END)
        );
    }

    #[test]
    fn standalone_alignment_maps_start_end_without_lossy_aliasing() {
        assert_eq!(map_align_items(AlignItems::Start), Ok(SL_FLEX_ALIGN_START));
        assert_eq!(map_align_items(AlignItems::End), Ok(SL_FLEX_ALIGN_END));
        assert_eq!(
            map_justify_content(JustifyContent::Start),
            SL_JUSTIFY_CONTENT_START
        );
        assert_eq!(
            map_justify_content(JustifyContent::End),
            SL_JUSTIFY_CONTENT_END
        );
    }

    #[test]
    fn standalone_full_value_validation_covers_fr_support() {
        let style = Style {
            row_gap: Length::fr(1.0),
            column_gap: Length::FitContent(Some(BaseLength::fixed(6.0))),
            ..Style::default()
        };
        assert!(ensure_standalone_lengths_supported(&style).is_ok());

        let style = Style {
            margin: Rect::new(Length::fr(1.0), Length::ZERO, Length::ZERO, Length::ZERO),
            ..Style::default()
        };
        assert!(ensure_standalone_lengths_supported(&style).is_ok());

        let style = Style {
            width: Length::fr(2.0),
            height: Length::fr(3.0),
            ..Style::default()
        };
        assert!(ensure_standalone_lengths_supported(&style).is_ok());
        assert!(axis_length_is_standalone_supported(Length::fr(1.0)));

        let style = Style {
            min_width: Length::fr(1.0),
            max_width: Length::fr(1.0),
            min_height: Length::fr(1.0),
            max_height: Length::fr(1.0),
            flex_basis: Length::fr(1.0),
            ..Style::default()
        };
        assert!(ensure_standalone_lengths_supported(&style).is_ok());
        assert!(min_max_length_is_standalone_supported(Length::fr(1.0)));
        assert!(flex_basis_length_is_standalone_supported(Length::fr(1.0)));
    }

    #[test]
    fn standalone_list_linear_fields_map_supported_values() {
        assert!(list_gap_length_is_standalone_supported(Length::ZERO));
        assert!(list_gap_length_is_standalone_supported(Length::points(2.0)));
        assert!(list_gap_length_is_standalone_supported(Length::percent(
            2.0
        )));
        assert!(list_gap_length_is_standalone_supported(Length::calc(
            1.0, 2.0
        )));
        assert!(list_gap_length_is_standalone_supported(Length::Auto));
        assert!(list_gap_length_is_standalone_supported(Length::MaxContent));
        assert!(list_gap_length_is_standalone_supported(Length::FitContent(
            None
        )));
        assert!(list_gap_length_is_standalone_supported(Length::fr(1.0)));
        assert_eq!(
            map_list_component_type(ListComponentType::Default),
            SL_LIST_COMPONENT_TYPE_DEFAULT
        );
        assert_eq!(
            map_list_component_type(ListComponentType::Header),
            SL_LIST_COMPONENT_TYPE_HEADER
        );
        assert_eq!(
            map_list_component_type(ListComponentType::Footer),
            SL_LIST_COMPONENT_TYPE_FOOTER
        );
        assert_eq!(
            map_list_component_type(ListComponentType::ListRow),
            SL_LIST_COMPONENT_TYPE_LIST_ROW
        );
    }

    fn native_style_application_region(source: &str) -> &str {
        let start = source
            .find("fn apply_style(")
            .expect("native adapter apply_style exists");
        let end = source[start..]
            .find("fn length_is_standalone_supported")
            .map(|offset| start + offset)
            .expect("native adapter length support helpers exist");
        &source[start..end]
    }

    fn source_region<'a>(source: &'a str, start_marker: &str, end_marker: &str) -> &'a str {
        let start = source.find(start_marker).expect("source region starts");
        let end = source[start..]
            .find(end_marker)
            .map(|offset| start + offset)
            .expect("source region ends");
        &source[start..end]
    }

    fn rust_string_array_values(source: &str, start_marker: &str, end_marker: &str) -> Vec<String> {
        source_region(source, start_marker, end_marker)
            .lines()
            .filter_map(quoted_value)
            .collect()
    }

    fn gn_string_array_values(source: &str, array_name: &str) -> Vec<String> {
        let start_marker = format!("{array_name} = [");
        source_region(source, &start_marker, "]")
            .lines()
            .filter_map(quoted_value)
            .collect()
    }

    fn quoted_value(line: &str) -> Option<String> {
        let line = line.trim();
        let line = line.strip_prefix('"')?;
        let (value, _) = line.split_once('"')?;
        Some(value.to_owned())
    }

    fn braced_body_after<'a>(source: &'a str, marker: &str) -> &'a str {
        let start = source.find(marker).expect("marked item exists");
        let body_start = source[start..]
            .find('{')
            .map(|offset| start + offset + 1)
            .expect("marked item body starts");
        let body_end = source[body_start..]
            .find('}')
            .map(|offset| body_start + offset)
            .expect("marked item body ends");
        &source[body_start..body_end]
    }

    fn standalone_header_style_setters(source: &str) -> BTreeSet<String> {
        source
            .lines()
            .map(str::trim)
            .filter_map(|line| {
                let rest = line.strip_prefix("void SLNodeStyleSet")?;
                let (suffix, _) = rest.split_once('(')?;
                Some(format!("SLNodeStyleSet{}", suffix.trim()))
            })
            .collect()
    }

    fn standalone_header_layout_getters(source: &str) -> BTreeSet<String> {
        source
            .lines()
            .map(str::trim)
            .filter_map(|line| {
                let rest = line.strip_prefix("float SLNodeLayoutGet")?;
                let (suffix, _) = rest.split_once('(')?;
                Some(format!("SLNodeLayoutGet{}", suffix.trim()))
            })
            .collect()
    }

    fn standalone_header_style_getters(source: &str) -> BTreeSet<String> {
        source
            .lines()
            .map(str::trim)
            .filter_map(|line| {
                let name_start = line.find("SLNodeStyleGet")?;
                let rest = &line[name_start..];
                let (name, _) = rest.split_once('(')?;
                Some(name.trim().to_owned())
            })
            .collect()
    }

    fn standalone_header_function_names(source: &str) -> BTreeSet<String> {
        source
            .split(';')
            .filter_map(|declaration| {
                let declaration = declaration
                    .lines()
                    .filter_map(|line| {
                        let line = line.split("//").next().unwrap_or("").trim();
                        (!line.is_empty()).then_some(line)
                    })
                    .collect::<Vec<_>>()
                    .join(" ");
                let declaration = declaration.trim();
                let open_paren = declaration.rfind('(')?;
                let before_paren = &declaration[..open_paren];
                let name = before_paren.split_whitespace().last()?;
                name.starts_with("SL").then(|| name.to_owned())
            })
            .collect()
    }

    fn native_extern_function_names(source: &str) -> BTreeSet<String> {
        source_region(source, "unsafe extern \"C\" {", "\n}\n\nstruct NativeRoot")
            .lines()
            .map(str::trim)
            .filter_map(|line| {
                let rest = line.strip_prefix("fn ")?;
                let (name, _) = rest.split_once('(')?;
                Some(name.trim().to_owned())
            })
            .collect()
    }

    fn native_source_after_extern_block(source: &str) -> &str {
        let marker = "\n}\n\nstruct NativeRoot";
        let (_, tail) = source
            .split_once(marker)
            .expect("native source contains the standalone extern block terminator");
        tail
    }

    fn explicitly_exempted_standalone_style_setters() -> BTreeSet<String> {
        EXEMPTED_STANDALONE_STYLE_SETTERS
            .iter()
            .map(|(name, reason)| {
                assert!(
                    !reason.trim().is_empty(),
                    "standalone style setter exemptions must state why they are not imported"
                );
                (*name).to_owned()
            })
            .collect()
    }

    #[derive(Debug, Eq, PartialEq)]
    struct CAbiField {
        name: String,
        type_name: String,
    }

    fn header_c_struct_fields(source: &str, struct_name: &str) -> Vec<CAbiField> {
        braced_body_after(source, &format!("typedef struct {struct_name}"))
            .lines()
            .map(str::trim)
            .filter(|line| !line.is_empty() && !line.starts_with("//"))
            .map(|line| {
                let declaration = line.trim_end_matches(';');
                let (type_name, name) = declaration
                    .rsplit_once(' ')
                    .expect("header struct field contains type and name");
                CAbiField {
                    name: name.trim().trim_end_matches('_').to_owned(),
                    type_name: normalize_c_abi_type(type_name.trim()),
                }
            })
            .collect()
    }

    fn rust_repr_struct_fields(source: &str, struct_name: &str) -> Vec<CAbiField> {
        braced_body_after(source, &format!("struct {struct_name}"))
            .lines()
            .map(str::trim)
            .filter(|line| !line.is_empty() && !line.starts_with("#["))
            .filter_map(|line| {
                let (name, type_name) = line.split_once(':')?;
                Some(CAbiField {
                    name: name.trim().to_owned(),
                    type_name: type_name.trim().trim_end_matches(',').to_owned(),
                })
            })
            .collect()
    }

    fn normalize_c_abi_type(type_name: &str) -> String {
        match type_name {
            "float" => "f32",
            "int32_t" => "i32",
            other => other,
        }
        .to_owned()
    }

    fn header_enum_values(source: &str) -> BTreeMap<String, i32> {
        let mut values = BTreeMap::new();
        let mut cursor = 0;
        while let Some(relative_start) = source[cursor..].find("typedef enum") {
            let start = cursor + relative_start;
            let body_start = start
                + source[start..]
                    .find('{')
                    .expect("standalone enum body starts")
                + 1;
            let body_end = body_start
                + source[body_start..]
                    .find('}')
                    .expect("standalone enum body ends");
            for entry in source[body_start..body_end].split(',') {
                let entry = entry
                    .lines()
                    .map(|line| line.split_once("//").map_or(line, |(prefix, _)| prefix))
                    .collect::<Vec<_>>()
                    .join(" ");
                let entry = entry.trim();
                if entry.is_empty() {
                    continue;
                }
                let (name, value) = entry
                    .split_once('=')
                    .expect("standalone enum values must be explicit");
                let name = name.trim().to_owned();
                let value = value
                    .trim()
                    .parse::<i32>()
                    .expect("standalone enum value must be an i32 literal");
                assert!(
                    values.insert(name.clone(), value).is_none(),
                    "duplicate standalone enum variant {name}"
                );
            }
            cursor = body_end + 1;
        }
        values
    }

    fn native_standalone_enum_constants(source: &str) -> BTreeMap<String, i32> {
        source
            .lines()
            .map(str::trim)
            .filter_map(|line| {
                let line = line.strip_prefix("const SL_")?;
                let (suffix, rest) = line.split_once(':')?;
                let (_, value) = rest.split_once('=')?;
                let value = value.trim().trim_end_matches(';').parse::<i32>().ok()?;
                Some((format!("SL_{suffix}"), value))
            })
            .collect()
    }

    fn header_variant_from_native_constant(native_name: &str) -> String {
        let words = native_name
            .strip_prefix("SL_")
            .expect("standalone native constants use the SL_ prefix")
            .split('_')
            .map(header_variant_word)
            .collect::<String>();
        format!("SL{words}")
    }

    fn header_variant_word(word: &str) -> String {
        match word {
            "LTR" | "RTL" => word.to_owned(),
            _ => {
                let mut chars = word.chars();
                let Some(first) = chars.next() else {
                    return String::new();
                };
                first
                    .to_uppercase()
                    .chain(chars.as_str().to_ascii_lowercase().chars())
                    .collect()
            }
        }
    }

    fn explicitly_exempted_standalone_enum_variants() -> BTreeSet<String> {
        EXEMPTED_STANDALONE_ENUM_VARIANTS
            .iter()
            .map(|(name, reason)| {
                assert!(
                    !reason.trim().is_empty(),
                    "standalone enum variant exemptions must state why they are not imported"
                );
                (*name).to_owned()
            })
            .collect()
    }

    fn header_define_i32(source: &str, name: &str) -> i32 {
        parse_i32_const_expr(header_define_expr(source, name))
    }

    fn header_define_f32(source: &str, name: &str) -> f32 {
        parse_f32_const_expr(header_define_expr(source, name))
    }

    fn header_define_expr<'a>(source: &'a str, name: &str) -> &'a str {
        source
            .lines()
            .map(str::trim)
            .find_map(|line| {
                let rest = line.strip_prefix("#define ")?;
                let rest = rest.trim_start();
                let expr = rest.strip_prefix(name)?;
                expr.starts_with(char::is_whitespace).then(|| expr.trim())
            })
            .unwrap_or_else(|| panic!("{name} define exists in standalone value header"))
    }

    fn native_i32_const(source: &str, name: &str) -> i32 {
        parse_i32_const_expr(native_const_expr(source, name))
    }

    fn native_f32_const(source: &str, name: &str) -> f32 {
        parse_f32_const_expr(native_const_expr(source, name))
    }

    fn native_const_expr<'a>(source: &'a str, name: &str) -> &'a str {
        source
            .lines()
            .map(str::trim)
            .find_map(|line| {
                let rest = line.strip_prefix("const ")?;
                let (const_name, expr) = rest.split_once('=')?;
                let (const_name, _) = const_name.split_once(':')?;
                (const_name.trim() == name).then(|| expr.trim().trim_end_matches(';').trim())
            })
            .unwrap_or_else(|| panic!("{name} native constant exists"))
    }

    fn parse_i32_const_expr(expr: &str) -> i32 {
        let expr = expr.trim().trim_start_matches('(').trim_end_matches(')');
        if let Some((value, shift)) = expr.split_once("<<") {
            return value.trim().parse::<i32>().expect("shift value")
                << shift.trim().parse::<u32>().expect("shift amount");
        }
        expr.parse::<i32>().expect("i32 literal")
    }

    fn parse_f32_const_expr(expr: &str) -> f32 {
        expr.trim()
            .trim_start_matches('(')
            .trim_end_matches(')')
            .trim_end_matches("_f32")
            .trim_end_matches('f')
            .trim_end_matches('F')
            .parse::<f32>()
            .expect("f32 literal")
    }

    #[derive(Clone, Debug, Eq, PartialEq)]
    struct LayoutConfigVersion {
        name: String,
        major: u32,
        minor: u32,
    }

    impl LayoutConfigVersion {
        fn major_minor(&self) -> (u32, u32) {
            (self.major, self.minor)
        }
    }

    fn layout_config_versions(source: &str) -> Vec<LayoutConfigVersion> {
        source
            .lines()
            .map(str::trim)
            .filter_map(|line| line.strip_prefix("constexpr base::Version "))
            .filter_map(|line| {
                let (name, rest) = line.split_once('(')?;
                let (args, _) = rest.split_once(')')?;
                let (major, minor) = args.split_once(',')?;
                Some(LayoutConfigVersion {
                    name: name.trim().to_owned(),
                    major: major.trim().parse().ok()?,
                    minor: minor.trim().parse().ok()?,
                })
            })
            .collect()
    }

    fn style_fields(source: &str) -> Vec<String> {
        let start = source.find("pub struct Style").expect("Style exists");
        let body_start = source[start..]
            .find('{')
            .map(|offset| start + offset + 1)
            .expect("Style body starts");
        let body_end = source[body_start..]
            .find('}')
            .map(|offset| body_start + offset)
            .expect("Style body ends");

        source[body_start..body_end]
            .lines()
            .map(str::trim)
            .filter_map(|line| line.strip_prefix("pub "))
            .filter_map(|line| line.split_once(':').map(|(field, _)| field.trim()))
            .filter(|field| !field.is_empty())
            .map(ToOwned::to_owned)
            .collect()
    }
}
