// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/include/starlight_standalone/starlight.h"

#include <optional>
#include <vector>

#include "base/include/no_destructor.h"
#include "core/include/starlight_standalone/starlight_enums.h"
#include "core/include/starlight_standalone/starlight_value.h"
#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"
#include "core/renderer/starlight/style/layout_computed_style.h"
#include "core/renderer/starlight/types/layout_configs.h"

#define GET_INNER_LAYOUT_NODE(a) \
  reinterpret_cast<lynx::starlight::LayoutObject *>(a)
#define GET_OUTER_LAYOUT_NODE(a) reinterpret_cast<StarlightNode *>(a)

static bool SLDirectionIsAnyRtl(SLDirection direction) {
  return direction == SLDirection::SLDirectionRTL ||
         direction == SLDirection::SLDirectionLynxRTL;
}

static lynx::starlight::Direction ResolveEdgeToDirection(
    lynx::starlight::LayoutObject *const node, SLEdge edge) {
  const bool is_rtl = node->GetCSSStyle()->IsRtl();
  switch (edge) {
    case SLEdgeLeft:
      return lynx::starlight::Direction::kLeft;
    case SLEdgeRight:
      return lynx::starlight::Direction::kRight;
    case SLEdgeTop:
      return lynx::starlight::Direction::kTop;
    case SLEdgeBottom:
      return lynx::starlight::Direction::kBottom;
    case SLEdgeStart:
      return is_rtl ? lynx::starlight::Direction::kRight
                    : lynx::starlight::Direction::kLeft;
    case SLEdgeEnd:
      return is_rtl ? lynx::starlight::Direction::kLeft
                    : lynx::starlight::Direction::kRight;
    default:
      return lynx::starlight::Direction::kLeft;
  }
}

static lynx::starlight::LayoutObject *FindStandaloneRoot(
    lynx::starlight::LayoutObject *node) {
  while (auto *parent = node->ParentLayoutObject()) {
    node = parent;
  }
  return node;
}

static void SetStandaloneRootRecursive(lynx::starlight::LayoutObject *node,
                                       lynx::starlight::LayoutObject *root) {
  node->SetRoot(root);
  auto *child =
      static_cast<lynx::starlight::LayoutObject *>(node->FirstChild());
  while (child) {
    SetStandaloneRootRecursive(child, root);
    child = static_cast<lynx::starlight::LayoutObject *>(child->Next());
  }
}

static void CollectStandaloneFixedNodes(lynx::starlight::LayoutObject *node,
                                        lynx::starlight::LayoutObject *root,
                                        lynx::SLNodeSet *fixed_nodes) {
  node->SetRoot(root);
  if (node != root && node->IsFixed()) {
    fixed_nodes->insert(node);
  }
  auto *child =
      static_cast<lynx::starlight::LayoutObject *>(node->FirstChild());
  while (child) {
    CollectStandaloneFixedNodes(child, root, fixed_nodes);
    child = static_cast<lynx::starlight::LayoutObject *>(child->Next());
  }
}

static StarlightValue NLengthToStarlightValue(
    const lynx::starlight::NLength &length) {
  StarlightValue result{0.0f, SLUnitPoint, 0.0f, 0};
  const auto &numeric_length = length.NumericLength();
  if (numeric_length.HasValue()) {
    result.flags_ |= SLValueFlagHasValue;
    result.value_ = numeric_length.GetFixedPart();
    if (numeric_length.ContainsPercentage()) {
      result.flags_ |= SLValueFlagHasPercentage;
      result.percentage_ = numeric_length.GetPercentagePart();
    }
  }
  switch (length.GetType()) {
    case lynx::starlight::NLengthType::kNLengthAuto:
      result.unit_ = SLUnitAuto;
      break;
    case lynx::starlight::NLengthType::kNLengthUnit:
      result.unit_ = SLUnitPoint;
      break;
    case lynx::starlight::NLengthType::kNLengthPercentage:
      result.value_ = length.GetRawValue();
      result.unit_ = SLUnitPercent;
      break;
    case lynx::starlight::NLengthType::kNLengthCalc:
      result.unit_ = SLUnitCalc;
      break;
    case lynx::starlight::NLengthType::kNLengthMaxContent:
      result.unit_ = SLUnitMaxContent;
      break;
    case lynx::starlight::NLengthType::kNLengthFitContent:
      result.unit_ = SLUnitFitContent;
      break;
    case lynx::starlight::NLengthType::kNLengthFr:
      result.value_ = length.GetRawValue();
      result.unit_ = SLUnitFr;
      break;
    default:
      break;
  }
  return result;
}

static lynx::starlight::NLength::BaseLength StarlightValueToBaseLength(
    const StarlightValue &value) {
  if ((value.flags_ & SLValueFlagHasValue) == 0) {
    return lynx::starlight::NLength::BaseLength();
  }
  if ((value.flags_ & SLValueFlagHasPercentage) != 0) {
    return lynx::starlight::NLength::BaseLength(value.value_,
                                                value.percentage_);
  }
  return lynx::starlight::NLength::BaseLength(value.value_);
}

static lynx::starlight::NLength StarlightValueToNLength(
    const StarlightValue &value) {
  switch (value.unit_) {
    case SLUnitPoint:
      return lynx::starlight::NLength::MakeUnitNLength(value.value_);
    case SLUnitPercent:
      return lynx::starlight::NLength::MakePercentageNLength(value.value_);
    case SLUnitAuto:
      return lynx::starlight::NLength::MakeAutoNLength();
    case SLUnitCalc:
      if ((value.flags_ & SLValueFlagHasPercentage) != 0) {
        return lynx::starlight::NLength::MakeCalcNLength(value.value_,
                                                         value.percentage_);
      }
      return lynx::starlight::NLength::MakeCalcNLength(value.value_);
    case SLUnitMaxContent:
      return lynx::starlight::NLength::MakeMaxContentNLength();
    case SLUnitFitContent:
      if ((value.flags_ & SLValueFlagHasValue) != 0) {
        return lynx::starlight::NLength::MakeFitContentNLength(
            StarlightValueToBaseLength(value));
      }
      return lynx::starlight::NLength::MakeFitContentNLength();
    case SLUnitFr:
      return lynx::starlight::NLength::MakeFrNLength(value.value_);
    default:
      return lynx::starlight::NLength::MakeAutoNLength();
  }
}

static std::vector<lynx::starlight::NLength> StarlightValuesToNLengths(
    const StarlightValue *values, int32_t count) {
  std::vector<lynx::starlight::NLength> result;
  if (values == nullptr || count <= 0) {
    return result;
  }
  result.reserve(count);
  for (int32_t index = 0; index < count; ++index) {
    result.push_back(StarlightValueToNLength(values[index]));
  }
  return result;
}

static lynx::starlight::LayoutConfigs CreateDefaultLayoutConfigs() {
  lynx::starlight::LayoutConfigs config;
  config.SetQuirksMode(lynx::kNegativePaddingFixedVersion);
  config.css_align_with_legacy_w3c_ = true;
  config.enable_fixed_new_ = true;
  config.SetTargetSDKVersion(kStarlightDefaultTargetSDKVersion);
  return config;
}

static const lynx::starlight::LayoutConfigs &GetDefaultLayoutConfigs() {
  static const lynx::base::NoDestructor<lynx::starlight::LayoutConfigs>
      kDefaultLayoutConfigs(CreateDefaultLayoutConfigs());
  return *kDefaultLayoutConfigs;
}

static const lynx::starlight::LayoutComputedStyle &GetDefaultStyle() {
  static const lynx::base::NoDestructor<lynx::starlight::LayoutComputedStyle>
      kDefaultStyle(kDefaultPhysicalPixelsPerLayoutUnit);
  return *kDefaultStyle;
}

SLNodeRef SLNodeNew() {
  lynx::starlight::LayoutObject *node = new lynx::starlight::LayoutObject(
      GetDefaultLayoutConfigs(),
      new lynx::starlight::LayoutComputedStyle(GetDefaultStyle()));
  return GET_OUTER_LAYOUT_NODE(node);
}

SLNodeRef SLNodeNewWithConfig(StarlightConfig *config) {
  lynx::starlight::LayoutComputedStyle *const css_style =
      new lynx::starlight::LayoutComputedStyle(GetDefaultStyle());
  css_style->SetPhysicalPixelsPerLayoutUnit(
      SLConfigGetPhysicalPixelsPerLayoutUnit(config));
  lynx::starlight::LayoutObject *node =
      new lynx::starlight::LayoutObject(GetDefaultLayoutConfigs(), css_style);
  return GET_OUTER_LAYOUT_NODE(node);
}

void SLNodeInsertChild(const SLNodeRef parent, const SLNodeRef child,
                       int32_t index) {
  lynx::starlight::LayoutObject *parent_node = GET_INNER_LAYOUT_NODE(parent);
  lynx::starlight::LayoutObject *child_node = GET_INNER_LAYOUT_NODE(child);
  if (lynx::starlight::LayoutObject *original_parent =
          child_node->ParentLayoutObject()) {
    original_parent->RemoveChild(child_node);
  }
  if (index == -1) {
    parent_node->AppendChild(child_node);
  } else {
    parent_node->InsertChildBefore(
        child_node,
        static_cast<lynx::starlight::LayoutObject *>(parent_node->Find(index)));
  }
  SetStandaloneRootRecursive(child_node, FindStandaloneRoot(parent_node));
  parent_node->MarkDirty();
}

void SLNodeInsertChildBefore(const SLNodeRef parent, const SLNodeRef child,
                             const SLNodeRef reference) {
  lynx::starlight::LayoutObject *parent_node = GET_INNER_LAYOUT_NODE(parent);
  lynx::starlight::LayoutObject *child_node = GET_INNER_LAYOUT_NODE(child);
  lynx::starlight::LayoutObject *reference_node =
      GET_INNER_LAYOUT_NODE(reference);
  if (lynx::starlight::LayoutObject *original_parent =
          child_node->ParentLayoutObject()) {
    original_parent->RemoveChild(child_node);
  }
  parent_node->InsertChildBefore(child_node, reference_node);
  SetStandaloneRootRecursive(child_node, FindStandaloneRoot(parent_node));
  parent_node->MarkDirty();
}

void SLNodeRemoveChild(const SLNodeRef parent, const SLNodeRef child) {
  lynx::starlight::LayoutObject *parent_node = GET_INNER_LAYOUT_NODE(parent);
  lynx::starlight::LayoutObject *child_node = GET_INNER_LAYOUT_NODE(child);
  if (parent_node == child_node->ParentLayoutObject()) {
    parent_node->RemoveChild(child_node);
    SetStandaloneRootRecursive(child_node, child_node);
    parent_node->MarkDirty();
  }
}

void SLNodeRemoveAllChildren(const SLNodeRef parent) {
  lynx::starlight::LayoutObject *parent_node = GET_INNER_LAYOUT_NODE(parent);
  parent_node->MarkDirty();
  while (parent_node->GetChildCount() > 0) {
    lynx::starlight::LayoutObject *child =
        static_cast<lynx::starlight::LayoutObject *>(parent_node->FirstChild());
    parent_node->RemoveChild(child);
    SetStandaloneRootRecursive(child, child);
  }
}

void SLNodeReset(const SLNodeRef node) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  inner_node->Reset(inner_node);
}

SLNodeRef SLNodeGetChild(const SLNodeRef node, int32_t index) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  if (index < inner_node->GetChildCount()) {
    return GET_OUTER_LAYOUT_NODE(inner_node->Find(index));
  }
  return nullptr;
}

int32_t SLNodeGetChildCount(const SLNodeRef node) {
  return GET_INNER_LAYOUT_NODE(node)->GetChildCount();
}

SLNodeRef SLNodeGetParent(const SLNodeRef node) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  return GET_OUTER_LAYOUT_NODE(inner_node->ParentLayoutObject());
}

void SLNodeFree(const SLNodeRef node) {
  if (node == nullptr) {
    return;
  }
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);

  if (auto *css_style = inner_node->GetCSSMutableStyle()) {
    delete css_style;
  }
  delete inner_node;
}

void SLNodeFreeRecursive(const SLNodeRef node) {
  lynx::starlight::LayoutObject *inner_node = GET_INNER_LAYOUT_NODE(node);
  while (inner_node->GetChildCount() > 0) {
    lynx::starlight::LayoutObject *const child =
        static_cast<lynx::starlight::LayoutObject *>(inner_node->FirstChild());
    SLNodeFreeRecursive(GET_OUTER_LAYOUT_NODE(child));
  }
  SLNodeFree(GET_OUTER_LAYOUT_NODE(inner_node));
}

bool SLNodeIsDirty(const SLNodeRef node) {
  return GET_INNER_LAYOUT_NODE(node)->IsDirty();
}

void SLNodeMarkDirty(const SLNodeRef node) {
  GET_INNER_LAYOUT_NODE(node)->MarkDirty();
}

static void SLNodeMarkNotDirtyRecursive(lynx::starlight::LayoutObject *node) {
  node->MarkNotDirty();
  lynx::starlight::LayoutObject *child =
      static_cast<lynx::starlight::LayoutObject *>(node->FirstChild());
  while (child) {
    SLNodeMarkNotDirtyRecursive(child);
    child = static_cast<lynx::starlight::LayoutObject *>(child->Next());
  }
}

bool SLNodeIsRTL(const SLNodeRef node) {
  return GET_INNER_LAYOUT_NODE(node)->GetCSSStyle()->IsRtl();
}

static void SLNodeHandleRTLRecursive(lynx::starlight::LayoutObject *const node,
                                     bool is_rtl) {
  lynx::starlight::LayoutComputedStyle *const style =
      node->GetCSSMutableStyle();
  // only the node not setting direction explicitly need update, but no need to
  // update has_explicit_direction_style_.
  if (!node->HasExplicitDirectionStyle()) {
    auto direction = is_rtl ? lynx::starlight::DirectionType::kRtl
                            : lynx::starlight::DirectionType::kLtr;

    style->SetDirection(direction);
  }
  lynx::starlight::LayoutObject *child =
      static_cast<lynx::starlight::LayoutObject *>(node->FirstChild());
  while (child) {
    SLNodeHandleRTLRecursive(child, is_rtl);
    child = static_cast<lynx::starlight::LayoutObject *>(child->Next());
  }
}

static lynx::starlight::OneSideConstraint SLNodeOwnerConstraint(
    float owner_size, SLNodeMeasureMode owner_mode) {
  switch (owner_mode) {
    case SLNodeMeasureModeExactly:
      return lynx::starlight::OneSideConstraint::Definite(owner_size);
    case SLNodeMeasureModeAtMost:
      return lynx::starlight::OneSideConstraint::AtMost(owner_size);
    case SLNodeMeasureModeUndefined:
    default:
      return lynx::starlight::OneSideConstraint::Indefinite();
  }
}

void SLNodeCalculateLayout(const SLNodeRef node, float owner_width,
                           float owner_height, SLDirection owner_direction) {
  SLNodeCalculateLayoutWithMode(
      node, owner_width,
      owner_width == SLUndefined ? SLNodeMeasureModeUndefined
                                 : SLNodeMeasureModeExactly,
      owner_height,
      owner_height == SLUndefined ? SLNodeMeasureModeUndefined
                                  : SLNodeMeasureModeExactly,
      owner_direction);
}

void SLNodeCalculateLayoutWithMode(const SLNodeRef node, float owner_width,
                                   SLNodeMeasureMode owner_width_mode,
                                   float owner_height,
                                   SLNodeMeasureMode owner_height_mode,
                                   SLDirection owner_direction) {
  // containing block
  lynx::starlight::Constraints owner_constraints;
  owner_constraints[SLHorizontal] =
      SLNodeOwnerConstraint(owner_width, owner_width_mode);
  owner_constraints[SLVertical] =
      SLNodeOwnerConstraint(owner_height, owner_height_mode);

  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  inner_node->MarkDirty();
  inner_node->GetBoxInfo()->InitializeBoxInfo(owner_constraints, *inner_node,
                                              inner_node->GetLayoutConfigs());
  lynx::starlight::Constraints constraints =
      lynx::starlight::property_utils::GenerateDefaultConstraints(
          *inner_node, owner_constraints);

  // handle RTL, if the direction of node is not setting, set the
  // owner_direction to the node.
  SLNodeHandleRTLRecursive(inner_node, SLDirectionIsAnyRtl(owner_direction));

  lynx::SLNodeSet fixed_nodes;
  CollectStandaloneFixedNodes(inner_node, inner_node, &fixed_nodes);
  inner_node->ReLayoutWithConstraints(
      constraints, fixed_nodes.empty() ? nullptr : &fixed_nodes);
  SLNodeMarkNotDirtyRecursive(inner_node);
}

void SLNodeSetMeasureDelegate(const SLNodeRef node,
                              StarlightMeasureDelegate *const delegate) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  inner_node->SetContext(delegate);
  if (delegate) {
    inner_node->SetSLMeasureFunc(
        [](void *context, const lynx::starlight::Constraints &constraints,
           bool final_measure) {
          StarlightMeasureDelegate *const measure_delegate =
              static_cast<StarlightMeasureDelegate *>(context);
          const SLNodeMeasureMode width_mode =
              static_cast<SLNodeMeasureMode>(constraints[SLHorizontal].Mode());
          const float width =
              width_mode == SLNodeMeasureMode::SLNodeMeasureModeUndefined
                  ? 0.f
                  : constraints[SLHorizontal].Size();
          const SLNodeMeasureMode height_mode =
              static_cast<SLNodeMeasureMode>(constraints[SLVertical].Mode());
          const float height =
              height_mode == SLNodeMeasureMode::SLNodeMeasureModeUndefined
                  ? 0.f
                  : constraints[SLVertical].Size();
          StarlightSize size;
          float baseline = 0.f;
          if (measure_delegate->measure_func_) {
            // If width and height are both exact, use the width and height
            // from the constraints, and don't call the measure_func_.
            if (width_mode == SLNodeMeasureMode::SLNodeMeasureModeExactly &&
                height_mode == SLNodeMeasureMode::SLNodeMeasureModeExactly) {
              size.width_ = width;
              size.height_ = height;
            } else {
              size = measure_delegate->measure_func_(
                  measure_delegate->manager_node_, width, width_mode, height,
                  height_mode);
            }
          }
          if (measure_delegate->baseline_func_) {
            baseline = measure_delegate->baseline_func_(
                measure_delegate->manager_node_, width, height);
          }
          return FloatSize{size.width_, size.height_, baseline};
        });
  } else {
    inner_node->SetSLMeasureFunc(nullptr);
  }
}

StarlightMeasureDelegate *SLNodeGetMeasureDelegate(const SLNodeRef node) {
  return node == nullptr ? nullptr
                         : static_cast<StarlightMeasureDelegate *>(
                               GET_INNER_LAYOUT_NODE(node)->GetContext());
}

bool SLNodeHasMeasureFunc(const SLNodeRef node) {
  return GET_INNER_LAYOUT_NODE(node)->GetSLMeasureFunc() != nullptr;
}

// Styles
void SLNodeStyleSetDirection(const SLNodeRef node, SLDirection type) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  // If the direction of node is changed by SLNodeHandleRTLRecursive, and then
  // the direction actually set, should markdirty.
  if (inner_node->GetCSSMutableStyle()->SetDirection(
          static_cast<lynx::starlight::DirectionType>(type)) ||
      !inner_node->HasExplicitDirectionStyle()) {
    inner_node->SetHasExplicitDirectionStyle(true);
    inner_node->MarkDirty();
  }
}

#define SET_ENUM_STYLE(type_name, enum_type, inner_enum_type, set_method) \
  void SLNodeStyleSet##type_name(const SLNodeRef node, enum_type value) { \
    lynx::starlight::LayoutObject *const inner_node =                     \
        GET_INNER_LAYOUT_NODE(node);                                      \
    if (inner_node->GetCSSMutableStyle()->set_method(                     \
            static_cast<lynx::starlight::inner_enum_type>(value))) {      \
      inner_node->MarkDirty();                                            \
    }                                                                     \
  }

#define SUPPORTED_ENUM_STYLE_SETTER(V)                                   \
  V(FlexDirection, SLFlexDirection, FlexDirectionType, SetFlexDirection) \
  V(AlignContent, SLAlignContent, AlignContentType, SetAlignContent)     \
  V(AlignSelf, SLFlexAlign, FlexAlignType, SetAlignSelf)                 \
  V(PositionType, SLPositionType, PositionType, SetPosition)             \
  V(FlexWrap, SLFlexWrap, FlexWrapType, SetFlexWrap)                     \
  V(LinearOrientation, SLLinearOrientation, LinearOrientationType,        \
    SetLinearOrientation)                                                \
  V(LinearGravity, SLLinearGravity, LinearGravityType, SetLinearGravity) \
  V(LinearLayoutGravity, SLLinearLayoutGravity, LinearLayoutGravityType,  \
    SetLinearLayoutGravity)                                              \
  V(LinearCrossGravity, SLLinearCrossGravity, LinearCrossGravityType,     \
    SetLinearCrossGravity)                                               \
  V(Display, SLDisplay, DisplayType, SetDisplay)                         \
  V(BoxSizing, SLBoxSizing, BoxSizingType, SetBoxSizing)

SUPPORTED_ENUM_STYLE_SETTER(SET_ENUM_STYLE)

#undef SUPPORTED_ENUM_STYLE_SETTER
#undef SET_ENUM_STYLE

void SLNodeStyleSetLinearColumnCount(const SLNodeRef node, int32_t value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  std::optional<int> column_count;
  if (value > 0) {
    column_count = value;
  }
  if (inner_node->attr_map().setColumnCount(column_count)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetListComponentType(const SLNodeRef node,
                                     SLListComponentType value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const std::optional<int> component_type = static_cast<int>(value);
  if (inner_node->attr_map().setListCompType(component_type)) {
    inner_node->MarkDirty();
  }
}

static void SLNodeStyleSetListGap(
    const SLNodeRef node,
    lynx::starlight::NLength lynx::starlight::LinearData::*field,
    lynx::starlight::NLength length) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *linear_data = inner_node->GetCSSMutableStyle()->linear_data_.Access();
  if (linear_data->*field != length) {
    linear_data->*field = length;
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetListMainAxisGap(const SLNodeRef node, float value) {
  SLNodeStyleSetListGap(node,
                        &lynx::starlight::LinearData::list_main_axis_gap_,
                        lynx::starlight::NLength::MakeUnitNLength(value));
}

void SLNodeStyleSetListMainAxisGapPercent(const SLNodeRef node, float value) {
  SLNodeStyleSetListGap(node,
                        &lynx::starlight::LinearData::list_main_axis_gap_,
                        lynx::starlight::NLength::MakePercentageNLength(value));
}

void SLNodeStyleSetListMainAxisGapCalc(const SLNodeRef node,
                                       StarlightValue value) {
  value.unit_ = SLUnitCalc;
  SLNodeStyleSetListGap(node,
                        &lynx::starlight::LinearData::list_main_axis_gap_,
                        StarlightValueToNLength(value));
}

void SLNodeStyleSetListMainAxisGapValue(const SLNodeRef node,
                                        StarlightValue value) {
  SLNodeStyleSetListGap(node,
                        &lynx::starlight::LinearData::list_main_axis_gap_,
                        StarlightValueToNLength(value));
}

void SLNodeStyleSetListCrossAxisGap(const SLNodeRef node, float value) {
  SLNodeStyleSetListGap(node,
                        &lynx::starlight::LinearData::list_cross_axis_gap_,
                        lynx::starlight::NLength::MakeUnitNLength(value));
}

void SLNodeStyleSetListCrossAxisGapPercent(const SLNodeRef node, float value) {
  SLNodeStyleSetListGap(node,
                        &lynx::starlight::LinearData::list_cross_axis_gap_,
                        lynx::starlight::NLength::MakePercentageNLength(value));
}

void SLNodeStyleSetListCrossAxisGapCalc(const SLNodeRef node,
                                        StarlightValue value) {
  value.unit_ = SLUnitCalc;
  SLNodeStyleSetListGap(node,
                        &lynx::starlight::LinearData::list_cross_axis_gap_,
                        StarlightValueToNLength(value));
}

void SLNodeStyleSetListCrossAxisGapValue(const SLNodeRef node,
                                         StarlightValue value) {
  SLNodeStyleSetListGap(node,
                        &lynx::starlight::LinearData::list_cross_axis_gap_,
                        StarlightValueToNLength(value));
}

// alignment
void SLNodeStyleSetJustifyContent(const SLNodeRef node,
                                  SLJustifyContent value) {
  lynx::starlight::JustifyContentType type;
  switch (value) {
    case SLJustifyContent::SLJustifyContentFlexStart:
      type = lynx::starlight::JustifyContentType::kFlexStart;
      break;
    case SLJustifyContent::SLJustifyContentCenter:
      type = lynx::starlight::JustifyContentType::kCenter;
      break;
    case SLJustifyContent::SLJustifyContentFlexEnd:
      type = lynx::starlight::JustifyContentType::kFlexEnd;
      break;
    case SLJustifyContent::SLJustifyContentSpaceBetween:
      type = lynx::starlight::JustifyContentType::kSpaceBetween;
      break;
    case SLJustifyContent::SLJustifyContentSpaceAround:
      type = lynx::starlight::JustifyContentType::kSpaceAround;
      break;
    case SLJustifyContent::SLJustifyContentSpaceEvenly:
      type = lynx::starlight::JustifyContentType::kSpaceEvenly;
      break;
    case SLJustifyContent::SLJustifyContentStretch:
      type = lynx::starlight::JustifyContentType::kStretch;
      break;
    case SLJustifyContent::SLJustifyContentStart:
      type = lynx::starlight::JustifyContentType::kFlexStart;
      break;
    case SLJustifyContent::SLJustifyContentEnd:
      type = lynx::starlight::JustifyContentType::kFlexEnd;
      break;
    default:
      type = lynx::starlight::JustifyContentType::kFlexStart;
      break;
  }
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetJustifyContent(type)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetAlignItems(const SLNodeRef node, SLFlexAlign value) {
  // auto is not supported in align-items
  if (value == SLFlexAlign::SLFlexAlignAuto) {
    return;
  }
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetAlignItems(
          static_cast<lynx::starlight::FlexAlignType>(value))) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetAspectRatio(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetAspectRatio(value)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetOrder(const SLNodeRef node, int32_t value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetOrder(value)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetFlexGrow(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetFlexGrow(value)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetFlexShrink(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  if (inner_node->GetCSSMutableStyle()->SetFlexShrink(value)) {
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetLinearWeight(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *linear_data = inner_node->GetCSSMutableStyle()->linear_data_.Access();
  if (linear_data->linear_weight_ != value) {
    linear_data->linear_weight_ = value;
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetLinearWeightSum(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *linear_data = inner_node->GetCSSMutableStyle()->linear_data_.Access();
  if (linear_data->linear_weight_sum_ != value) {
    linear_data->linear_weight_sum_ = value;
    inner_node->MarkDirty();
  }
}

#define SET_RELATIVE_INT_STYLE(type_name, field_name)                      \
  void SLNodeStyleSet##type_name(const SLNodeRef node, int32_t value) {    \
    lynx::starlight::LayoutObject *const inner_node =                      \
        GET_INNER_LAYOUT_NODE(node);                                       \
    auto *relative_data = inner_node->GetCSSMutableStyle()                 \
                              ->relative_data_.Access();                  \
    if (relative_data->field_name != value) {                              \
      relative_data->field_name = value;                                   \
      inner_node->MarkDirty();                                             \
    }                                                                      \
  }

SET_RELATIVE_INT_STYLE(RelativeId, relative_id_)
SET_RELATIVE_INT_STYLE(RelativeAlignTop, relative_align_top_)
SET_RELATIVE_INT_STYLE(RelativeAlignRight, relative_align_right_)
SET_RELATIVE_INT_STYLE(RelativeAlignBottom, relative_align_bottom_)
SET_RELATIVE_INT_STYLE(RelativeAlignLeft, relative_align_left_)
SET_RELATIVE_INT_STYLE(RelativeTopOf, relative_top_of_)
SET_RELATIVE_INT_STYLE(RelativeRightOf, relative_right_of_)
SET_RELATIVE_INT_STYLE(RelativeBottomOf, relative_bottom_of_)
SET_RELATIVE_INT_STYLE(RelativeLeftOf, relative_left_of_)

#undef SET_RELATIVE_INT_STYLE

void SLNodeStyleSetRelativeLayoutOnce(const SLNodeRef node, bool value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *relative_data =
      inner_node->GetCSSMutableStyle()->relative_data_.Access();
  if (relative_data->relative_layout_once_ != value) {
    relative_data->relative_layout_once_ = value;
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetRelativeCenter(const SLNodeRef node,
                                  SLRelativeCenter value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *relative_data =
      inner_node->GetCSSMutableStyle()->relative_data_.Access();
  auto center = static_cast<lynx::starlight::RelativeCenterType>(value);
  if (relative_data->relative_center_ != center) {
    relative_data->relative_center_ = center;
    inner_node->MarkDirty();
  }
}

static void SLNodeStyleSetGridTrackVector(
    const SLNodeRef node,
    std::vector<lynx::starlight::NLength> lynx::starlight::GridData::*field,
    const StarlightValue *values, int32_t count) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *grid_data = inner_node->GetCSSMutableStyle()->grid_data_.Access();
  auto converted = StarlightValuesToNLengths(values, count);
  if (grid_data->*field != converted) {
    grid_data->*field = converted;
    inner_node->MarkDirty();
  }
}

#define SET_GRID_TRACK_VECTOR_STYLE(type_name, field_name)                 \
  void SLNodeStyleSetGrid##type_name(const SLNodeRef node,                 \
                                     const StarlightValue *values,         \
                                     int32_t count) {                      \
    SLNodeStyleSetGridTrackVector(node,                                    \
                                  &lynx::starlight::GridData::field_name, \
                                  values, count);                         \
  }

SET_GRID_TRACK_VECTOR_STYLE(TemplateColumns,
                            grid_template_columns_min_track_sizing_function_)
SET_GRID_TRACK_VECTOR_STYLE(TemplateColumnsMax,
                            grid_template_columns_max_track_sizing_function_)
SET_GRID_TRACK_VECTOR_STYLE(TemplateRows,
                            grid_template_rows_min_track_sizing_function_)
SET_GRID_TRACK_VECTOR_STYLE(TemplateRowsMax,
                            grid_template_rows_max_track_sizing_function_)
SET_GRID_TRACK_VECTOR_STYLE(AutoColumns,
                            grid_auto_columns_min_track_sizing_function_)
SET_GRID_TRACK_VECTOR_STYLE(AutoColumnsMax,
                            grid_auto_columns_max_track_sizing_function_)
SET_GRID_TRACK_VECTOR_STYLE(AutoRows,
                            grid_auto_rows_min_track_sizing_function_)
SET_GRID_TRACK_VECTOR_STYLE(AutoRowsMax,
                            grid_auto_rows_max_track_sizing_function_)

#undef SET_GRID_TRACK_VECTOR_STYLE

void SLNodeStyleSetGridAutoFlow(const SLNodeRef node, SLGridAutoFlow value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *grid_data = inner_node->GetCSSMutableStyle()->grid_data_.Access();
  auto auto_flow = static_cast<lynx::starlight::GridAutoFlowType>(value);
  if (grid_data->grid_auto_flow_ != auto_flow) {
    grid_data->grid_auto_flow_ = auto_flow;
    inner_node->MarkDirty();
  }
}

static void SLNodeStyleSetGridJustify(
    const SLNodeRef node, lynx::starlight::JustifyType
                              lynx::starlight::GridData::*field,
    SLJustifyItem value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *grid_data = inner_node->GetCSSMutableStyle()->grid_data_.Access();
  auto justify = static_cast<lynx::starlight::JustifyType>(value);
  if (grid_data->*field != justify) {
    grid_data->*field = justify;
    inner_node->MarkDirty();
  }
}

void SLNodeStyleSetJustifyItems(const SLNodeRef node, SLJustifyItem value) {
  SLNodeStyleSetGridJustify(node, &lynx::starlight::GridData::justify_items_,
                            value);
}

void SLNodeStyleSetJustifySelf(const SLNodeRef node, SLJustifyItem value) {
  SLNodeStyleSetGridJustify(node, &lynx::starlight::GridData::justify_self_,
                            value);
}

#define SET_GRID_INT_STYLE(type_name, field_name)                      \
  void SLNodeStyleSetGrid##type_name(const SLNodeRef node,             \
                                     int32_t value) {                  \
    lynx::starlight::LayoutObject *const inner_node =                  \
        GET_INNER_LAYOUT_NODE(node);                                   \
    auto *grid_data = inner_node->GetCSSMutableStyle()->grid_data_.Access(); \
    if (grid_data->field_name != value) {                              \
      grid_data->field_name = value;                                   \
      inner_node->MarkDirty();                                         \
    }                                                                  \
  }

SET_GRID_INT_STYLE(ColumnStart, grid_column_start_)
SET_GRID_INT_STYLE(ColumnEnd, grid_column_end_)
SET_GRID_INT_STYLE(RowStart, grid_row_start_)
SET_GRID_INT_STYLE(RowEnd, grid_row_end_)
SET_GRID_INT_STYLE(ColumnSpan, grid_column_span_)
SET_GRID_INT_STYLE(RowSpan, grid_row_span_)

#undef SET_GRID_INT_STYLE

void SLNodeStyleSetFlex(const SLNodeRef node, float value) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  bool need_mark_dirty = false;
  need_mark_dirty = inner_node->GetCSSMutableStyle()->SetFlexGrow(value);
  need_mark_dirty |= inner_node->GetCSSMutableStyle()->SetFlexShrink(1);
  need_mark_dirty |= inner_node->GetCSSMutableStyle()->SetFlexBasis(
      lynx::starlight::NLength::MakeUnitNLength(0));
  if (need_mark_dirty) {
    inner_node->MarkDirty();
  }
}

#define DEFINE_EDGE_STYLE_SETTER(func_suffix, css_method_prefix,          \
                                 nlength_maker)                           \
  void SLNodeStyleSet##func_suffix(const SLNodeRef node, SLEdge edge,     \
                                   float value) {                         \
    bool need_mark_dirty = false;                                         \
    lynx::starlight::LayoutObject *const inner_node =                     \
        GET_INNER_LAYOUT_NODE(node);                                      \
    auto *css_style = inner_node->GetCSSMutableStyle();                   \
    const bool is_rtl = inner_node->GetCSSStyle()->IsRtl();               \
                                                                          \
    switch (edge) {                                                       \
      case SLEdgeLeft:                                                    \
        need_mark_dirty = css_style->Set##css_method_prefix##Left(        \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeRight:                                                   \
        need_mark_dirty = css_style->Set##css_method_prefix##Right(       \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeTop:                                                     \
        need_mark_dirty = css_style->Set##css_method_prefix##Top(         \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeBottom:                                                  \
        need_mark_dirty = css_style->Set##css_method_prefix##Bottom(      \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeStart:                                                   \
        need_mark_dirty =                                                 \
            is_rtl ? css_style->Set##css_method_prefix##Right(            \
                         lynx::starlight::NLength::nlength_maker(value))  \
                   : css_style->Set##css_method_prefix##Left(             \
                         lynx::starlight::NLength::nlength_maker(value)); \
        break;                                                            \
      case SLEdgeEnd:                                                     \
        need_mark_dirty =                                                 \
            is_rtl ? css_style->Set##css_method_prefix##Left(             \
                         lynx::starlight::NLength::nlength_maker(value))  \
                   : css_style->Set##css_method_prefix##Right(            \
                         lynx::starlight::NLength::nlength_maker(value)); \
        break;                                                            \
      case SLEdgeHorizontal:                                              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Left(       \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Right(      \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeVertical:                                                \
        need_mark_dirty |= css_style->Set##css_method_prefix##Top(        \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Bottom(     \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      case SLEdgeAll:                                                     \
        need_mark_dirty |= css_style->Set##css_method_prefix##Left(       \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Right(      \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Top(        \
            lynx::starlight::NLength::nlength_maker(value));              \
        need_mark_dirty |= css_style->Set##css_method_prefix##Bottom(     \
            lynx::starlight::NLength::nlength_maker(value));              \
        break;                                                            \
      default:                                                            \
        break;                                                            \
    }                                                                     \
    if (need_mark_dirty) {                                                \
      inner_node->MarkDirty();                                            \
    }                                                                     \
  }

// top, bottom, left, right
// SLNodeStyleSetPosition
DEFINE_EDGE_STYLE_SETTER(Position, , MakeUnitNLength)
// SLNodeStyleSetPositionPercent
DEFINE_EDGE_STYLE_SETTER(PositionPercent, , MakePercentageNLength)
// SLNodeStyleSetMargin
DEFINE_EDGE_STYLE_SETTER(Margin, Margin, MakeUnitNLength)
// SLNodeStyleSetMarginPercent
DEFINE_EDGE_STYLE_SETTER(MarginPercent, Margin, MakePercentageNLength)
// SLNodeStyleSetPadding
DEFINE_EDGE_STYLE_SETTER(Padding, Padding, MakeUnitNLength)
// SLNodeStyleSetPaddingPercent
DEFINE_EDGE_STYLE_SETTER(PaddingPercent, Padding, MakePercentageNLength)

#undef DEFINE_EDGE_STYLE_SETTER

#define DEFINE_EDGE_STYLE_CALC_SETTER(func_suffix, css_method_prefix)       \
  void SLNodeStyleSet##func_suffix##Calc(const SLNodeRef node, SLEdge edge, \
                                         StarlightValue value) {            \
    value.unit_ = SLUnitCalc;                                               \
    const auto length = StarlightValueToNLength(value);                     \
    bool need_mark_dirty = false;                                           \
    lynx::starlight::LayoutObject *const inner_node =                       \
        GET_INNER_LAYOUT_NODE(node);                                        \
    auto *css_style = inner_node->GetCSSMutableStyle();                     \
    const bool is_rtl = inner_node->GetCSSStyle()->IsRtl();                 \
                                                                            \
    switch (edge) {                                                         \
      case SLEdgeLeft:                                                      \
        need_mark_dirty = css_style->Set##css_method_prefix##Left(length);  \
        break;                                                              \
      case SLEdgeRight:                                                     \
        need_mark_dirty = css_style->Set##css_method_prefix##Right(length); \
        break;                                                              \
      case SLEdgeTop:                                                       \
        need_mark_dirty = css_style->Set##css_method_prefix##Top(length);   \
        break;                                                              \
      case SLEdgeBottom:                                                    \
        need_mark_dirty = css_style->Set##css_method_prefix##Bottom(length);\
        break;                                                              \
      case SLEdgeStart:                                                     \
        need_mark_dirty =                                                   \
            is_rtl ? css_style->Set##css_method_prefix##Right(length)       \
                   : css_style->Set##css_method_prefix##Left(length);       \
        break;                                                              \
      case SLEdgeEnd:                                                       \
        need_mark_dirty =                                                   \
            is_rtl ? css_style->Set##css_method_prefix##Left(length)        \
                   : css_style->Set##css_method_prefix##Right(length);      \
        break;                                                              \
      case SLEdgeHorizontal:                                                \
        need_mark_dirty |= css_style->Set##css_method_prefix##Left(length); \
        need_mark_dirty |= css_style->Set##css_method_prefix##Right(length);\
        break;                                                              \
      case SLEdgeVertical:                                                  \
        need_mark_dirty |= css_style->Set##css_method_prefix##Top(length);  \
        need_mark_dirty |= css_style->Set##css_method_prefix##Bottom(length);\
        break;                                                              \
      case SLEdgeAll:                                                       \
        need_mark_dirty |= css_style->Set##css_method_prefix##Left(length); \
        need_mark_dirty |= css_style->Set##css_method_prefix##Right(length);\
        need_mark_dirty |= css_style->Set##css_method_prefix##Top(length);  \
        need_mark_dirty |= css_style->Set##css_method_prefix##Bottom(length);\
        break;                                                              \
      default:                                                              \
        break;                                                              \
    }                                                                       \
    if (need_mark_dirty) {                                                  \
      inner_node->MarkDirty();                                              \
    }                                                                       \
  }

DEFINE_EDGE_STYLE_CALC_SETTER(Position, )
DEFINE_EDGE_STYLE_CALC_SETTER(Margin, Margin)
DEFINE_EDGE_STYLE_CALC_SETTER(Padding, Padding)

#undef DEFINE_EDGE_STYLE_CALC_SETTER

#define DEFINE_EDGE_STYLE_VALUE_SETTER(func_suffix, css_method_prefix)       \
  void SLNodeStyleSet##func_suffix##Value(const SLNodeRef node, SLEdge edge, \
                                          StarlightValue value) {            \
    const auto length = StarlightValueToNLength(value);                      \
    bool need_mark_dirty = false;                                            \
    lynx::starlight::LayoutObject *const inner_node =                        \
        GET_INNER_LAYOUT_NODE(node);                                         \
    auto *css_style = inner_node->GetCSSMutableStyle();                      \
    const bool is_rtl = inner_node->GetCSSStyle()->IsRtl();                  \
                                                                             \
    switch (edge) {                                                          \
      case SLEdgeLeft:                                                       \
        need_mark_dirty = css_style->Set##css_method_prefix##Left(length);   \
        break;                                                               \
      case SLEdgeRight:                                                      \
        need_mark_dirty = css_style->Set##css_method_prefix##Right(length);  \
        break;                                                               \
      case SLEdgeTop:                                                        \
        need_mark_dirty = css_style->Set##css_method_prefix##Top(length);    \
        break;                                                               \
      case SLEdgeBottom:                                                     \
        need_mark_dirty = css_style->Set##css_method_prefix##Bottom(length); \
        break;                                                               \
      case SLEdgeStart:                                                      \
        need_mark_dirty =                                                    \
            is_rtl ? css_style->Set##css_method_prefix##Right(length)        \
                   : css_style->Set##css_method_prefix##Left(length);        \
        break;                                                               \
      case SLEdgeEnd:                                                        \
        need_mark_dirty =                                                    \
            is_rtl ? css_style->Set##css_method_prefix##Left(length)         \
                   : css_style->Set##css_method_prefix##Right(length);       \
        break;                                                               \
      case SLEdgeHorizontal:                                                 \
        need_mark_dirty |= css_style->Set##css_method_prefix##Left(length);  \
        need_mark_dirty |= css_style->Set##css_method_prefix##Right(length); \
        break;                                                               \
      case SLEdgeVertical:                                                   \
        need_mark_dirty |= css_style->Set##css_method_prefix##Top(length);   \
        need_mark_dirty |=                                                   \
            css_style->Set##css_method_prefix##Bottom(length);               \
        break;                                                               \
      case SLEdgeAll:                                                        \
        need_mark_dirty |= css_style->Set##css_method_prefix##Left(length);  \
        need_mark_dirty |= css_style->Set##css_method_prefix##Right(length); \
        need_mark_dirty |= css_style->Set##css_method_prefix##Top(length);   \
        need_mark_dirty |=                                                   \
            css_style->Set##css_method_prefix##Bottom(length);               \
        break;                                                               \
      default:                                                               \
        break;                                                               \
    }                                                                        \
    if (need_mark_dirty) {                                                   \
      inner_node->MarkDirty();                                               \
    }                                                                        \
  }

DEFINE_EDGE_STYLE_VALUE_SETTER(Position, )
DEFINE_EDGE_STYLE_VALUE_SETTER(Margin, Margin)
DEFINE_EDGE_STYLE_VALUE_SETTER(Padding, Padding)

#undef DEFINE_EDGE_STYLE_VALUE_SETTER

void SLNodeStyleSetMarginAuto(const SLNodeRef node, SLEdge edge) {
  bool need_mark_dirty = false;
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *css_style = inner_node->GetCSSMutableStyle();
  switch (edge) {
    case SLEdgeLeft:
      if (css_style->SetMarginLeft(
              lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeRight:
      if (css_style->SetMarginRight(
              lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeTop:
      if (css_style->SetMarginTop(
              lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeBottom:
      if (css_style->SetMarginBottom(
              lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeStart:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty = css_style->SetMarginRight(
            lynx::starlight::NLength::MakeAutoNLength());
      } else {
        need_mark_dirty = css_style->SetMarginLeft(
            lynx::starlight::NLength::MakeAutoNLength());
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeEnd:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty = css_style->SetMarginLeft(
            lynx::starlight::NLength::MakeAutoNLength());
      } else {
        need_mark_dirty = css_style->SetMarginRight(
            lynx::starlight::NLength::MakeAutoNLength());
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeHorizontal:
      need_mark_dirty =
          css_style->SetMarginLeft(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |= css_style->SetMarginRight(
          lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeVertical:
      need_mark_dirty =
          css_style->SetMarginTop(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |= css_style->SetMarginBottom(
          lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeAll:
      need_mark_dirty =
          css_style->SetMarginLeft(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |= css_style->SetMarginRight(
          lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetMarginTop(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |= css_style->SetMarginBottom(
          lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    default:
      break;
  }
}

void SLNodeStyleSetPositionAuto(const SLNodeRef node, SLEdge edge) {
  bool need_mark_dirty = false;
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *css_style = inner_node->GetCSSMutableStyle();
  switch (edge) {
    case SLEdgeLeft:
      if (css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeRight:
      if (css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeTop:
      if (css_style->SetTop(lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeBottom:
      if (css_style->SetBottom(lynx::starlight::NLength::MakeAutoNLength())) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeStart:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty =
            css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength());
      } else {
        need_mark_dirty =
            css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength());
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeEnd:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty =
            css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength());
      } else {
        need_mark_dirty =
            css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength());
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeHorizontal:
      need_mark_dirty =
          css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeVertical:
      need_mark_dirty =
          css_style->SetTop(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetBottom(lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeAll:
      need_mark_dirty =
          css_style->SetLeft(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetRight(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetTop(lynx::starlight::NLength::MakeAutoNLength());
      need_mark_dirty |=
          css_style->SetBottom(lynx::starlight::NLength::MakeAutoNLength());
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    default:
      break;
  }
}

// border width
void SLNodeStyleSetBorder(const SLNodeRef node, SLEdge edge, float value) {
  bool need_mark_dirty = false;
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  auto *css_style = inner_node->GetCSSMutableStyle();
  switch (edge) {
    case SLEdgeLeft:
      if (css_style->SetBorderLeftWidth(value)) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeRight:
      if (css_style->SetBorderRightWidth(value)) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeTop:
      if (css_style->SetBorderTopWidth(value)) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeBottom:
      if (css_style->SetBorderBottomWidth(value)) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeStart:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty = css_style->SetBorderRightWidth(value);
      } else {
        need_mark_dirty = css_style->SetBorderLeftWidth(value);
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeEnd:
      if (inner_node->GetCSSStyle()->IsRtl()) {
        need_mark_dirty = css_style->SetBorderLeftWidth(value);
      } else {
        need_mark_dirty = css_style->SetBorderRightWidth(value);
      }
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeHorizontal:
      need_mark_dirty = css_style->SetBorderLeftWidth(value);
      need_mark_dirty |= css_style->SetBorderRightWidth(value);
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeVertical:
      need_mark_dirty = css_style->SetBorderTopWidth(value);
      need_mark_dirty |= css_style->SetBorderBottomWidth(value);
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    case SLEdgeAll:
      need_mark_dirty = css_style->SetBorderTopWidth(value);
      need_mark_dirty |= css_style->SetBorderBottomWidth(value);
      need_mark_dirty |= css_style->SetBorderLeftWidth(value);
      need_mark_dirty |= css_style->SetBorderRightWidth(value);
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    default:
      break;
  }
}

static void SLNodeStyleSetGapLength(const SLNodeRef node, SLGap gap,
                                    lynx::starlight::NLength length) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  switch (gap) {
    case SLGapColumn:
      if (inner_node->GetCSSMutableStyle()->SetColumnGap(length)) {
        inner_node->MarkDirty();
      }
      break;
    case SLGapRow:
      if (inner_node->GetCSSMutableStyle()->SetRowGap(length)) {
        inner_node->MarkDirty();
      }
      break;
    case SLGapAll: {
      bool need_mark_dirty = false;
      need_mark_dirty = inner_node->GetCSSMutableStyle()->SetColumnGap(length);
      need_mark_dirty |= inner_node->GetCSSMutableStyle()->SetRowGap(length);
      if (need_mark_dirty) {
        inner_node->MarkDirty();
      }
      break;
    }
  }
}

void SLNodeStyleSetGap(const SLNodeRef node, SLGap gap, float value) {
  SLNodeStyleSetGapLength(node, gap,
                          lynx::starlight::NLength::MakeUnitNLength(value));
}

void SLNodeStyleSetGapPercent(const SLNodeRef node, SLGap gap, float value) {
  SLNodeStyleSetGapLength(
      node, gap, lynx::starlight::NLength::MakePercentageNLength(value));
}

void SLNodeStyleSetGapCalc(const SLNodeRef node, SLGap gap,
                           StarlightValue value) {
  value.unit_ = SLUnitCalc;
  SLNodeStyleSetGapLength(node, gap, StarlightValueToNLength(value));
}

void SLNodeStyleSetGapValue(const SLNodeRef node, SLGap gap,
                            StarlightValue value) {
  SLNodeStyleSetGapLength(node, gap, StarlightValueToNLength(value));
}

// set size styles with value, e.g., width: 100%, height: max-content,
// min-width: 10%.
#define SET_SIZE_STYLE_WITH_VALUE(type_name, type_pre_fix, length_type)  \
  void SLNodeStyleSet##type_pre_fix(const SLNodeRef node, float value) { \
    lynx::starlight::LayoutObject *const inner_node =                    \
        GET_INNER_LAYOUT_NODE(node);                                     \
    if (inner_node->GetCSSMutableStyle()->Set##type_name(                \
            lynx::starlight::NLength::Make##length_type(value))) {       \
      inner_node->MarkDirty();                                           \
    }                                                                    \
  }

#define SUPPORTED_SIZE_STYLE_WITH_VALUE_SETTER(V)   \
  V(Width, Width, UnitNLength)                      \
  V(Width, WidthPercent, PercentageNLength)         \
  V(MinWidth, MinWidth, UnitNLength)                \
  V(MinWidth, MinWidthPercent, PercentageNLength)   \
  V(MaxWidth, MaxWidth, UnitNLength)                \
  V(MaxWidth, MaxWidthPercent, PercentageNLength)   \
  V(Height, Height, UnitNLength)                    \
  V(Height, HeightPercent, PercentageNLength)       \
  V(MinHeight, MinHeight, UnitNLength)              \
  V(MinHeight, MinHeightPercent, PercentageNLength) \
  V(MaxHeight, MaxHeight, UnitNLength)              \
  V(MaxHeight, MaxHeightPercent, PercentageNLength) \
  V(FlexBasis, FlexBasis, UnitNLength)              \
  V(FlexBasis, FlexBasisPercent, PercentageNLength)

SUPPORTED_SIZE_STYLE_WITH_VALUE_SETTER(SET_SIZE_STYLE_WITH_VALUE)

#undef SUPPORTED_SIZE_STYLE_WITH_VALUE_SETTER
#undef SET_SIZE_STYLE_WITH_VALUE

#define SET_SIZE_STYLE_WITH_CALC_VALUE(type_name, type_pre_fix)      \
  void SLNodeStyleSet##type_pre_fix##Calc(const SLNodeRef node,      \
                                          StarlightValue value) {    \
    value.unit_ = SLUnitCalc;                                        \
    lynx::starlight::LayoutObject *const inner_node =                \
        GET_INNER_LAYOUT_NODE(node);                                 \
    if (inner_node->GetCSSMutableStyle()->Set##type_name(            \
            StarlightValueToNLength(value))) {                       \
      inner_node->MarkDirty();                                       \
    }                                                                \
  }

#define SUPPORTED_SIZE_STYLE_WITH_CALC_VALUE_SETTER(V) \
  V(Width, Width)                                      \
  V(MinWidth, MinWidth)                                \
  V(MaxWidth, MaxWidth)                                \
  V(Height, Height)                                    \
  V(MinHeight, MinHeight)                              \
  V(MaxHeight, MaxHeight)                              \
  V(FlexBasis, FlexBasis)

SUPPORTED_SIZE_STYLE_WITH_CALC_VALUE_SETTER(SET_SIZE_STYLE_WITH_CALC_VALUE)

#undef SUPPORTED_SIZE_STYLE_WITH_CALC_VALUE_SETTER
#undef SET_SIZE_STYLE_WITH_CALC_VALUE

#define SET_SIZE_STYLE_WITH_FULL_VALUE(type_name, type_pre_fix)    \
  void SLNodeStyleSet##type_pre_fix##Value(const SLNodeRef node,   \
                                           StarlightValue value) { \
    lynx::starlight::LayoutObject *const inner_node =              \
        GET_INNER_LAYOUT_NODE(node);                               \
    if (inner_node->GetCSSMutableStyle()->Set##type_name(          \
            StarlightValueToNLength(value))) {                     \
      inner_node->MarkDirty();                                     \
    }                                                              \
  }

#define SUPPORTED_SIZE_STYLE_WITH_FULL_VALUE_SETTER(V) \
  V(Width, Width)                                      \
  V(MinWidth, MinWidth)                                \
  V(MaxWidth, MaxWidth)                                \
  V(Height, Height)                                    \
  V(MinHeight, MinHeight)                              \
  V(MaxHeight, MaxHeight)                              \
  V(FlexBasis, FlexBasis)

SUPPORTED_SIZE_STYLE_WITH_FULL_VALUE_SETTER(SET_SIZE_STYLE_WITH_FULL_VALUE)

#undef SUPPORTED_SIZE_STYLE_WITH_FULL_VALUE_SETTER
#undef SET_SIZE_STYLE_WITH_FULL_VALUE

#define SET_SIZE_STYLE_WITH_FIT_CONTENT_VALUE(type_name, type_pre_fix) \
  void SLNodeStyleSet##type_pre_fix##FitContentValue(                  \
      const SLNodeRef node, StarlightValue value) {                    \
    value.unit_ = SLUnitFitContent;                                    \
    lynx::starlight::LayoutObject *const inner_node =                  \
        GET_INNER_LAYOUT_NODE(node);                                   \
    if (inner_node->GetCSSMutableStyle()->Set##type_name(              \
            StarlightValueToNLength(value))) {                         \
      inner_node->MarkDirty();                                         \
    }                                                                  \
  }

#define SUPPORTED_SIZE_STYLE_WITH_FIT_CONTENT_VALUE_SETTER(V) \
  V(Width, Width)                                             \
  V(MinWidth, MinWidth)                                       \
  V(MaxWidth, MaxWidth)                                       \
  V(Height, Height)                                           \
  V(MinHeight, MinHeight)                                     \
  V(MaxHeight, MaxHeight)                                     \
  V(FlexBasis, FlexBasis)

SUPPORTED_SIZE_STYLE_WITH_FIT_CONTENT_VALUE_SETTER(
    SET_SIZE_STYLE_WITH_FIT_CONTENT_VALUE)

#undef SUPPORTED_SIZE_STYLE_WITH_FIT_CONTENT_VALUE_SETTER
#undef SET_SIZE_STYLE_WITH_FIT_CONTENT_VALUE

// set size styles with no value param, e.g., width: auto, height: max-content,
// flex-basis: auto.
#define SET_SIZE_STYLE_WITH_NO_VALUE_PARAM(type_name, type_pre_fix, \
                                           length_type)             \
  void SLNodeStyleSet##type_pre_fix(const SLNodeRef node) {         \
    lynx::starlight::LayoutObject *const inner_node =               \
        GET_INNER_LAYOUT_NODE(node);                                \
    if (inner_node->GetCSSMutableStyle()->Set##type_name(           \
            lynx::starlight::NLength::Make##length_type())) {       \
      inner_node->MarkDirty();                                      \
    }                                                               \
  }

#define SUPPORTED_SIZE_STYLE_WITH_VALUE_WITH_NO_VALUE_PARAM_SETTER(V) \
  V(Width, WidthAuto, AutoNLength)                                    \
  V(Width, WidthMaxContent, MaxContentNLength)                        \
  V(Width, WidthFitContent, FitContentNLength)                        \
  V(MinWidth, MinWidthMaxContent, MaxContentNLength)                  \
  V(MinWidth, MinWidthFitContent, FitContentNLength)                  \
  V(MaxWidth, MaxWidthMaxContent, MaxContentNLength)                  \
  V(MaxWidth, MaxWidthFitContent, FitContentNLength)                  \
  V(Height, HeightAuto, AutoNLength)                                  \
  V(Height, HeightMaxContent, MaxContentNLength)                      \
  V(Height, HeightFitContent, FitContentNLength)                      \
  V(MinHeight, MinHeightMaxContent, MaxContentNLength)                \
  V(MinHeight, MinHeightFitContent, FitContentNLength)                \
  V(MaxHeight, MaxHeightMaxContent, MaxContentNLength)                \
  V(MaxHeight, MaxHeightFitContent, FitContentNLength)                \
  V(FlexBasis, FlexBasisAuto, AutoNLength)                            \
  V(FlexBasis, FlexBasisMaxContent, MaxContentNLength)                \
  V(FlexBasis, FlexBasisFitContent, FitContentNLength)

SUPPORTED_SIZE_STYLE_WITH_VALUE_WITH_NO_VALUE_PARAM_SETTER(
    SET_SIZE_STYLE_WITH_NO_VALUE_PARAM)

#undef SUPPORTED_SIZE_STYLE_WITH_VALUE_WITH_NO_VALUE_PARAM_SETTER
#undef SET_SIZE_STYLE_WITH_NO_VALUE_PARAM

// get styles with eunm type getter, e.g., flex-direction, justify-content.
#define GET_ENUM_STYLE(type_name, return_type, get_expr)                  \
  return_type SLNodeStyleGet##type_name(const SLNodeRef node) {           \
    lynx::starlight::LayoutObject *const inner_node =                     \
        GET_INNER_LAYOUT_NODE(node);                                      \
    return static_cast<return_type>(inner_node->GetCSSStyle()->get_expr); \
  }

#define SUPPORTED_ENUM_STYLE_GETTER(V)                     \
  V(FlexDirection, SLFlexDirection, GetFlexDirection())    \
  V(JustifyContent, SLJustifyContent, GetJustifyContent()) \
  V(AlignContent, SLAlignContent, GetAlignContent())       \
  V(AlignItems, SLFlexAlign, GetAlignItems())              \
  V(AlignSelf, SLFlexAlign, GetAlignSelf())                \
  V(PositionType, SLPositionType, GetPosition())           \
  V(FlexWrap, SLFlexWrap, GetFlexWrap())                   \
  V(LinearOrientation, SLLinearOrientation,                 \
    GetLinearOrientation())                                 \
  V(LinearGravity, SLLinearGravity, GetLinearGravity())     \
  V(LinearLayoutGravity, SLLinearLayoutGravity,             \
    GetLinearLayoutGravity())                               \
  V(LinearCrossGravity, SLLinearCrossGravity,               \
    GetLinearCrossGravity())                                \
  V(RelativeCenter, SLRelativeCenter, GetRelativeCenter())   \
  V(GridAutoFlow, SLGridAutoFlow, GetGridAutoFlow())         \
  V(JustifyItems, SLJustifyItem, GetJustifyItemsType())      \
  V(JustifySelf, SLJustifyItem, GetJustifySelfType())        \
  V(Display, SLDisplay, display_)                          \
  V(BoxSizing, SLBoxSizing, box_sizing_)

SUPPORTED_ENUM_STYLE_GETTER(GET_ENUM_STYLE)

#undef SUPPORTED_ENUM_STYLE_GETTER
#undef GET_ENUM_STYLE

// get styles with basic type getter, e.g., aspect-ratio, order.
#define GET_BASIC_TYPE_STYLE(type_name, return_type, get_expr)  \
  return_type SLNodeStyleGet##type_name(const SLNodeRef node) { \
    lynx::starlight::LayoutObject *const inner_node =           \
        GET_INNER_LAYOUT_NODE(node);                            \
    return inner_node->GetCSSStyle()->get_expr;                 \
  }

#define SUPPORTED_BASIC_TYPE_STYLE_GETTER(V)      \
  V(AspectRatio, float, box_data_->aspect_ratio_) \
  V(Order, int32_t, GetOrder())                   \
  V(RelativeId, int32_t, GetRelativeId())          \
  V(RelativeAlignTop, int32_t, GetRelativeAlignTop())       \
  V(RelativeAlignRight, int32_t, GetRelativeAlignRight())   \
  V(RelativeAlignBottom, int32_t, GetRelativeAlignBottom()) \
  V(RelativeAlignLeft, int32_t, GetRelativeAlignLeft())     \
  V(RelativeTopOf, int32_t, GetRelativeTopOf())             \
  V(RelativeRightOf, int32_t, GetRelativeRightOf())         \
  V(RelativeBottomOf, int32_t, GetRelativeBottomOf())       \
  V(RelativeLeftOf, int32_t, GetRelativeLeftOf())           \
  V(RelativeLayoutOnce, bool, GetRelativeLayoutOnce())      \
  V(GridColumnStart, int32_t, GetGridColumnStart())         \
  V(GridColumnEnd, int32_t, GetGridColumnEnd())             \
  V(GridRowStart, int32_t, GetGridRowStart())               \
  V(GridRowEnd, int32_t, GetGridRowEnd())                   \
  V(GridColumnSpan, int32_t, GetGridColumnSpan())           \
  V(GridRowSpan, int32_t, GetGridRowSpan())                 \
  V(FlexGrow, float, GetFlexGrow())               \
  V(FlexShrink, float, GetFlexShrink())           \
  V(LinearWeight, float, GetLinearWeight())        \
  V(LinearWeightSum, float, GetLinearWeightSum())

SUPPORTED_BASIC_TYPE_STYLE_GETTER(GET_BASIC_TYPE_STYLE)

#undef SUPPORTED_ENUM_STYLE_GETTER
#undef GET_BASIC_TYPE_STYLE

// get styles with length type getter, e.g., width, height.
#define GET_LENGTH_STYLE(type_name, get_expr)                            \
  StarlightValue SLNodeStyleGet##type_name(const SLNodeRef node) {       \
    lynx::starlight::LayoutObject *const inner_node =                    \
        GET_INNER_LAYOUT_NODE(node);                                     \
    return NLengthToStarlightValue(inner_node->GetCSSStyle()->get_expr); \
  }

#define SUPPORTED_LENGTH_STYLE_GETTER(V) \
  V(FlexBasis, GetFlexBasis())           \
  V(Width, GetWidth())                   \
  V(Height, GetHeight())                 \
  V(MinWidth, GetMinWidth())             \
  V(MaxWidth, GetMaxWidth())             \
  V(MinHeight, GetMinHeight())           \
  V(MaxHeight, GetMaxHeight())

SUPPORTED_LENGTH_STYLE_GETTER(GET_LENGTH_STYLE)

#undef SUPPORTED_LENGTH_STYLE_GETTER
#undef GET_LENGTH_STYLE

// get padding, margin, position styles with edge type getter.
#define DEFINE_EDGE_STYLE_GETTER(StyleType, GetterPrefix)                   \
  StarlightValue SLNodeStyleGet##StyleType(const SLNodeRef node,            \
                                           SLEdge edge) {                   \
    lynx::starlight::LayoutObject *const inner_node =                       \
        GET_INNER_LAYOUT_NODE(node);                                        \
    switch (edge) {                                                         \
      case SLEdgeLeft:                                                      \
        return NLengthToStarlightValue(                                     \
            inner_node->GetCSSStyle()->GetterPrefix##Left());               \
      case SLEdgeRight:                                                     \
        return NLengthToStarlightValue(                                     \
            inner_node->GetCSSStyle()->GetterPrefix##Right());              \
      case SLEdgeTop:                                                       \
        return NLengthToStarlightValue(                                     \
            inner_node->GetCSSStyle()->GetterPrefix##Top());                \
      case SLEdgeBottom:                                                    \
        return NLengthToStarlightValue(                                     \
            inner_node->GetCSSStyle()->GetterPrefix##Bottom());             \
      case SLEdgeStart:                                                     \
        return SLNodeIsRTL(node)                                            \
                   ? NLengthToStarlightValue(                               \
                         inner_node->GetCSSStyle()->GetterPrefix##Right())  \
                   : NLengthToStarlightValue(                               \
                         inner_node->GetCSSStyle()->GetterPrefix##Left());  \
      case SLEdgeEnd:                                                       \
        return SLNodeIsRTL(node)                                            \
                   ? NLengthToStarlightValue(                               \
                         inner_node->GetCSSStyle()->GetterPrefix##Left())   \
                   : NLengthToStarlightValue(                               \
                         inner_node->GetCSSStyle()->GetterPrefix##Right()); \
      default:                                                              \
        return StarlightValue{0.0f, SLUnitPoint};                           \
    }                                                                       \
  }

DEFINE_EDGE_STYLE_GETTER(Position, Get)
DEFINE_EDGE_STYLE_GETTER(Margin, GetMargin)
DEFINE_EDGE_STYLE_GETTER(Padding, GetPadding)

#undef DEFINE_EDGE_STYLE_GETTER

StarlightValue SLNodeStyleGetGap(const SLNodeRef node, SLGap gap) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  switch (gap) {
    case SLGapColumn:
      return NLengthToStarlightValue(
          inner_node->GetCSSStyle()->GetGridColumnGap());
    case SLGapRow:
      return NLengthToStarlightValue(
          inner_node->GetCSSStyle()->GetGridRowGap());
    case SLGapAll:
      return NLengthToStarlightValue(
          inner_node->GetCSSStyle()->GetGridRowGap());
    default:
      return StarlightValue{0.0f, SLUnitPoint};
  }
}

float SLNodeStyleGetBorder(const SLNodeRef node, SLEdge edge) {
  auto *style = GET_INNER_LAYOUT_NODE(node)->GetCSSStyle();
  switch (edge) {
    case SLEdgeLeft:
      return style->GetBorderLeftWidth();
    case SLEdgeRight:
      return style->GetBorderRightWidth();
    case SLEdgeTop:
      return style->GetBorderTopWidth();
    case SLEdgeBottom:
      return style->GetBorderBottomWidth();
    case SLEdgeStart:
      return style->IsRtl() ? style->GetBorderRightWidth()
                            : style->GetBorderLeftWidth();
    case SLEdgeEnd:
      return style->IsRtl() ? style->GetBorderLeftWidth()
                            : style->GetBorderRightWidth();
    default:
      return 0.0f;
  }
}

float SLNodeLayoutGetLeft(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.offset_.X();
}

float SLNodeLayoutGetTop(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.offset_.Y();
}

// TODO
// float SLNodeLayoutGetRight(lynx::starlight::LayoutObject * const node) {
//   const auto &result = node->GetLayoutResult();
//   return result.offset_.X();
// }

// TODO
// float SLNodeLayoutGetBottom(lynx::starlight::LayoutObject * const node) {
//   const auto &result = node->GetLayoutResult();
//   return result.offset_.Y();
// }

float SLNodeLayoutGetWidth(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.size_.width_;
}

float SLNodeLayoutGetHeight(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.size_.height_;
}

float SLNodeLayoutGetBaseline(const SLNodeRef node) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  return inner_node->GetBaseline();
}

float SLNodeLayoutGetMargin(const SLNodeRef node, SLEdge edge) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.margin_[ResolveEdgeToDirection(inner_node, edge)];
}

float SLNodeLayoutGetPadding(const SLNodeRef node, SLEdge edge) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.padding_[ResolveEdgeToDirection(inner_node, edge)];
}

float SLNodeLayoutGetBorder(const SLNodeRef node, SLEdge edge) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.border_[ResolveEdgeToDirection(inner_node, edge)];
}

float SLNodeLayoutGetStickyPosition(const SLNodeRef node, SLEdge edge) {
  lynx::starlight::LayoutObject *const inner_node = GET_INNER_LAYOUT_NODE(node);
  const auto &result = inner_node->GetLayoutResult();
  return result.sticky_pos_[ResolveEdgeToDirection(inner_node, edge)];
}
