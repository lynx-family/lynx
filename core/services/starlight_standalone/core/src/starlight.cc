// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/starlight_standalone/core/include/starlight.h"

#include <cmath>
#include <list>

#include "base/include/no_destructor.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/types/layout_configs.h"

using LayoutObject = lynx::starlight::LayoutObject;

#define GET_NODE(a) static_cast<StarlightLayoutNode *>(a)
#define GET_LAYOUT_NODE(a) \
  static_cast<StarlightLayoutNode *>(a)->GetLayoutObject()

namespace starlight {

namespace {
class StarlightLayoutNode {
 public:
  StarlightLayoutNode(const LayoutConfig &config)
      : css_style_(std::make_unique<lynx::starlight::ComputedCSSStyle>(
            config.Density(), config.Scale())) {
    sl_node_ = std::make_unique<LayoutObject>(
        lynx::starlight::LayoutConfigs(), css_style_->GetLayoutComputedStyle());
    auto envs =
        lynx::tasm::LynxEnvConfig(config.ScreenWidth(), config.ScreenHeight(),
                                  config.Density(), config.Scale());
    envs.UpdateViewport(
        config.ViewportWidth(),
        static_cast<SLMeasureMode>(config.ViewportWidthMode()),
        config.ViewportHeight(),
        static_cast<SLMeasureMode>(config.ViewportHeightMode()));
    css_style_->SetScreenWidth(envs.ScreenWidth());
    css_style_->SetViewportWidth(envs.ViewportWidth());
    css_style_->SetViewportHeight(envs.ViewportHeight());
    css_style_->SetLayoutUnit(config.Scale(), config.Density());
  }
  ~StarlightLayoutNode() = default;

  lynx::starlight::ComputedCSSStyle *GetCSSMutableStyle() {
    return css_style_.get();
  }
  LayoutObject *GetLayoutObject() { return sl_node_.get(); }

  std::list<StarlightLayoutNode *> children_;
  StarlightLayoutNode *parent_ = nullptr;

 private:
  std::unique_ptr<LayoutObject> sl_node_;
  std::unique_ptr<lynx::starlight::ComputedCSSStyle> css_style_;
};
}  // namespace

static const lynx::tasm::CSSParserConfigs &GetCSSParserConfigs() {
  static lynx::base::NoDestructor<lynx::tasm::CSSParserConfigs>
      kDefaultCSSConfigs(
          lynx::tasm::CSSParserConfigs{.enable_css_strict_mode = false,
                                       .remove_css_parser_log = true,
                                       .enable_legacy_parser = false,
                                       .enable_length_unit_check = false});
  return *kDefaultCSSConfigs;
}

static lynx::tasm::CSSValuePattern GetPatternForLengthType(SLLengthType type) {
  switch (type) {
    case kSLLengthPPX:
      return lynx::tasm::CSSValuePattern::NUMBER;
    case kSLLengthPX:
      return lynx::tasm::CSSValuePattern::PX;
    case kSLLengthRPX:
      return lynx::tasm::CSSValuePattern::RPX;
    case kSLLengthVW:
      return lynx::tasm::CSSValuePattern::VW;
    case kSLLengthVH:
      return lynx::tasm::CSSValuePattern::VH;
    case kSLLengthPercentage:
      return lynx::tasm::CSSValuePattern::PERCENT;
    case kSLLengthAuto:
      return lynx::tasm::CSSValuePattern::ENUM;
    default:
      return lynx::tasm::CSSValuePattern::EMPTY;
  }
}

static SLLengthType GetSLLengthTypeFromNLengthType(
    lynx::starlight::NLengthType type) {
  switch (type) {
    case lynx::starlight::NLengthType::kNLengthAuto:
      return kSLLengthAuto;
    case lynx::starlight::NLengthType::kNLengthUnit:
      return kSLLengthPPX;
    case lynx::starlight::NLengthType::kNLengthPercentage:
      return kSLLengthPercentage;
    default:
      return kSLLengthAuto;
  }
}

template <typename T>
static lynx::tasm::CSSValue GetNumberCSSValue(T value) {
  lynx::tasm::CSSValue css_value = lynx::tasm::CSSValue(
      lynx::lepus::Value(value), lynx::tasm::CSSValuePattern::NUMBER);
  return css_value;
}

static lynx::tasm::CSSValue GetLengthCSSValue(const SLLength value) {
  lynx::tasm::CSSValue css_value;
  css_value.GetValue().SetNumber(value.value_);
  css_value.SetPattern(GetPatternForLengthType(value.type_));
  return css_value;
}

template <typename T>
static void SetEnumTypeByPropertyID(const SLNodeRef node,
                                    lynx::tasm::CSSPropertyID id, T type) {
  if (GET_NODE(node)->GetCSSMutableStyle()->SetValue(
          id, lynx::tasm::CSSValue::MakeEnum(type))) {
    GET_LAYOUT_NODE(node)->MarkDirty();
  }
}

static SLLength NLengthToSLLength(const lynx::starlight::NLength &length) {
  return SLLength(length.GetRawValue(),
                  GetSLLengthTypeFromNLengthType(length.GetType()));
}

static void SetValueTypeByPropertyID(const SLNodeRef node,
                                     lynx::tasm::CSSPropertyID id,
                                     float value) {
  if (GET_NODE(node)->GetCSSMutableStyle()->SetValue(
          id, GetNumberCSSValue(value), std::isnan(value))) {
    GET_LAYOUT_NODE(node)->MarkDirty();
  }
}

static void SetLengthTypeByPropertyID(const SLNodeRef node,
                                      lynx::tasm::CSSPropertyID id,
                                      const SLLength value) {
  if (GET_NODE(node)->GetCSSMutableStyle()->SetValue(
          id, GetLengthCSSValue(value), std::isnan(value.value_))) {
    GET_LAYOUT_NODE(node)->MarkDirty();
  }
}

static lynx::starlight::Direction ResolveEdgeToDirection(SLNodeRef node,
                                                         SLEdge edge) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  bool isRtl = layout_object->GetCSSStyle()->IsRtl();
  switch (edge) {
    case kSLLeft:
      return lynx::starlight::Direction::kLeft;
    case kSLRight:
      return lynx::starlight::Direction::kRight;
    case kSLTop:
      return lynx::starlight::Direction::kTop;
    case kSLBottom:
      return lynx::starlight::Direction::kBottom;
    case kSLInlineStart:
      return isRtl ? lynx::starlight::Direction::kRight
                   : lynx::starlight::Direction::kLeft;
    case kSLInlineEnd:
      return isRtl ? lynx::starlight::Direction::kLeft
                   : lynx::starlight::Direction::kRight;
  }
}

SLNodeRef CreateWithConfig(const LayoutConfig &config) {
  StarlightLayoutNode *node = new StarlightLayoutNode(config);
  return static_cast<SLNodeRef>(node);
}

void UpdateConfig(const SLNodeRef node, const LayoutConfig &config) {
  for (const auto &child : GET_NODE(node)->children_) {
    GET_NODE(child)->GetCSSMutableStyle()->SetScreenWidth(config.ScreenWidth());
    UpdateConfig(child, config);
  }
}

void SetViewportSizeToRootNode(const SLNodeRef node, float width,
                               StarlightMeasureMode width_mode, float height,
                               StarlightMeasureMode height_mode) {
  switch (width_mode) {
    case kStarlightMeasureModeDefinite:
      SetWidth(node, SLLength(width, kSLLengthPPX));
      SetMaxWidth(node,
                  SLLength(lynx::starlight::DefaultLayoutStyle::kDefaultMaxSize,
                           kSLLengthPPX));
      break;
    case kStarlightMeasureModeAtMost:
      SetWidth(node, kSLAutoLength);
      SetMaxWidth(node, SLLength(width, kSLLengthPPX));
      break;
    case kStarlightMeasureModeIndefinite:
      SetWidth(node, kSLAutoLength);
      SetMaxWidth(node,
                  SLLength(lynx::starlight::DefaultLayoutStyle::kDefaultMaxSize,
                           kSLLengthPPX));
      break;
  }

  switch (height_mode) {
    case kStarlightMeasureModeDefinite:
      SetHeight(node, SLLength(height, kSLLengthPPX));
      SetMaxHeight(
          node, SLLength(lynx::starlight::DefaultLayoutStyle::kDefaultMaxSize,
                         kSLLengthPPX));
      break;
    case kStarlightMeasureModeAtMost:
      SetHeight(node, kSLAutoLength);
      SetMaxHeight(node, SLLength(height, kSLLengthPPX));
      break;
    case kStarlightMeasureModeIndefinite:
      SetHeight(node, kSLAutoLength);
      SetMaxHeight(
          node, SLLength(lynx::starlight::DefaultLayoutStyle::kDefaultMaxSize,
                         kSLLengthPPX));
      break;
  }
}

void UpdateViewport(const SLNodeRef node, float width,
                    StarlightMeasureMode width_mode, float height,
                    StarlightMeasureMode height_mode) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  lynx::starlight::ComputedCSSStyle *css_style =
      GET_NODE(node)->GetCSSMutableStyle();
  lynx::starlight::LayoutUnit viewport_width =
      width_mode == kStarlightMeasureModeDefinite
          ? lynx::starlight::LayoutUnit(width)
          : lynx::starlight::LayoutUnit();
  lynx::starlight::LayoutUnit viewport_height =
      height_mode == kStarlightMeasureModeDefinite
          ? lynx::starlight::LayoutUnit(height)
          : lynx::starlight::LayoutUnit();
  css_style->SetViewportWidth(viewport_width);
  css_style->SetViewportHeight(viewport_height);

  if (!layout_object->ParentLayoutObject()) {
    SetViewportSizeToRootNode(node, width, width_mode, height, height_mode);
  }

  for (const auto &child : GET_NODE(node)->children_) {
    UpdateViewport(child, width, width_mode, height, height_mode);
  }
}

void InsertChild(const SLNodeRef node, const SLNodeRef child, int32_t index) {
  StarlightLayoutNode *layout_node = GET_NODE(node);
  StarlightLayoutNode *child_node = GET_NODE(child);
  if (StarlightLayoutNode *original_parent = child_node->parent_) {
    GET_LAYOUT_NODE(original_parent)->RemoveChild(GET_LAYOUT_NODE(child_node));
    original_parent->children_.remove(child_node);
    child_node->parent_ = nullptr;
  }
  if (index == -1) {
    GET_LAYOUT_NODE(layout_node)->AppendChild(GET_LAYOUT_NODE(child_node));
    layout_node->children_.push_back(child_node);
  } else {
    GET_LAYOUT_NODE(layout_node)
        ->InsertChildBefore(GET_LAYOUT_NODE(child_node),
                            static_cast<LayoutObject *>(
                                GET_LAYOUT_NODE(layout_node)->Find(index)));
    auto iter = GET_NODE(layout_node)->children_.begin();
    std::advance(iter, index);
    GET_NODE(layout_node)->children_.insert(iter, child_node);
  }
  GET_LAYOUT_NODE(layout_node)->MarkDirty();
  child_node->parent_ = layout_node;
}

void RemoveAllChild(const SLNodeRef parent) {
  StarlightLayoutNode *parent_node = GET_NODE(parent);

  if (parent_node->children_.empty()) {
    return;
  }

  for (auto &child_node : parent_node->children_) {
    GET_LAYOUT_NODE(parent_node)->RemoveChild(GET_LAYOUT_NODE(child_node));
    GET_LAYOUT_NODE(parent_node)->MarkDirty();
    child_node->parent_ = nullptr;
  }

  parent_node->children_.clear();
}

void RemoveChild(const SLNodeRef parent, const SLNodeRef child) {
  StarlightLayoutNode *parent_node = GET_NODE(parent);
  StarlightLayoutNode *child_node = GET_NODE(child);
  if (parent_node == child_node->parent_) {
    GET_LAYOUT_NODE(parent_node)->RemoveChild(GET_LAYOUT_NODE(child_node));
    GET_LAYOUT_NODE(parent_node)->MarkDirty();
    GET_NODE(parent_node)->children_.remove(child_node);
    child_node->parent_ = nullptr;
  }
}

void RemoveChild(const SLNodeRef parent, int32_t index) {
  int count = GET_LAYOUT_NODE(parent)->GetChildCount();
  if (count == 0) {
    return;
  }

  auto iter = GET_NODE(parent)->children_.begin();
  std::advance(iter, index);
  if (LayoutObject *node =
          static_cast<LayoutObject *>(GET_LAYOUT_NODE(parent)->Find(index))) {
    GET_LAYOUT_NODE(parent)->RemoveChild(node);
    GET_LAYOUT_NODE(parent)->MarkDirty();
    (*iter)->parent_ = nullptr;
    GET_NODE(parent)->children_.erase(iter);
  }
}

void MoveChild(SLNodeRef node, SLNodeRef child, uint32_t from_index,
               uint32_t to_index) {
  RemoveChild(node, child);
  InsertChild(node, child, to_index);
}

uint32_t GetChildCount(SLNodeRef node) {
  return GET_NODE(node)->children_.size();
}

SLNodeRef GetParent(SLNodeRef node) { return GET_NODE(node)->parent_; }

SLNodeRef GetChild(SLNodeRef node, uint32_t index) {
  auto &children = GET_NODE(node)->children_;
  if (index >= children.size()) {
    return nullptr;
  }
  auto it = std::next(children.begin(), index);
  return *it;
}

void Free(const SLNodeRef node) {
  StarlightLayoutNode *child_node = GET_NODE(node);
  if (StarlightLayoutNode *parent_node = child_node->parent_) {
    RemoveChild(parent_node, node);
  }
  RemoveAllChild(node);
  delete GET_NODE(node);
}

void MarkDirty(const SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  layout_object->MarkDirty();
}

bool IsDirty(SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  return layout_object->IsDirty();
}

bool IsRTL(const SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  return layout_object->GetCSSStyle()->IsRtl();
}

bool SetStyle(SLNodeRef node, const std::string &name,
              const std::string &value) {
  lynx::tasm::CSSPropertyID id = lynx::tasm::CSSProperty::GetPropertyID(name);
  lynx::tasm::StyleMap styleMap = lynx::tasm::UnitHandler::Process(
      id, lynx::lepus::Value(value), GetCSSParserConfigs());
  bool result = false;
  for (auto &pair : styleMap) {
    result = result | GET_NODE(node)->GetCSSMutableStyle()->SetValue(
                          pair.first, pair.second);
  }
  if (result) {
    GET_LAYOUT_NODE(node)->MarkDirty();
  }
  return result;
}

bool SetMultiStyles(SLNodeRef node, const std::vector<std::string> &names,
                    const std::vector<std::string> &values) {
  if (names.size() != values.size()) {
    return false;
  }

  bool result = false;
  for (size_t i = 0; i < names.size(); ++i) {
    result |= SetStyle(GET_NODE(node), names[i], values[i]);
  }

  return result;
}

bool ResetStyle(SLNodeRef node, const std::string &name) {
  lynx::tasm::CSSPropertyID id = lynx::tasm::CSSProperty::GetPropertyID(name);
  bool ret = false;
  static lynx::base::NoDestructor<lynx::tasm::CSSValue> kEmpty(
      lynx::tasm::CSSValue::Empty());
  if ((ret = GET_NODE(node)->GetCSSMutableStyle()->SetValue(id, *kEmpty.get(),
                                                            true))) {
    GET_LAYOUT_NODE(node)->MarkDirty();
  }
  return ret;
}

void MarkUpdated(SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  layout_object->MarkUpdated();
  for (const auto &child : GET_NODE(node)->children_) {
    MarkUpdated(child);
  }
}

void CalculateLayout(SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  layout_object->ReLayout();
  MarkUpdated(node);
}

void SetMeasureDelegate(SLNodeRef node, MeasureDelegate *delegate) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);

  layout_object->SetContext(delegate);
  if (delegate) {
    layout_object->SetSLMeasureFunc(
        [](void *context, const lynx::starlight::Constraints &constraints,
           bool final_measure) {
          MeasureDelegate *measure_delegate =
              static_cast<MeasureDelegate *>(context);
          SLConstraints &constraints_ =
              *((SLConstraints *)(&const_cast<lynx::starlight::Constraints &>(
                  constraints)));
          SLSize size = measure_delegate->Measure(constraints_);
          float baseline = measure_delegate->Baseline(constraints_);
          return FloatSize(size.width_, size.height_, baseline);
        });
    layout_object->SetSLAlignmentFunc([](void *context) {
      MeasureDelegate *measure_delegate =
          static_cast<MeasureDelegate *>(context);
      measure_delegate->Alignment();
    });
  } else {
    layout_object->SetSLMeasureFunc(nullptr);
    layout_object->SetSLAlignmentFunc(nullptr);
  }
}

bool HasMeasureDelegate(SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  return layout_object->GetContext() != nullptr;
}

// flex
void SetFlexGrow(const SLNodeRef node, float value) {
  SetValueTypeByPropertyID(node, lynx::tasm::kPropertyIDFlexGrow, value);
}

void SetFlexShrink(const SLNodeRef node, float value) {
  SetValueTypeByPropertyID(node, lynx::tasm::kPropertyIDFlexShrink, value);
}

void SetFlexBasis(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDFlexBasis, value);
}

void SetFlexWrap(const SLNodeRef node, SLFlexWrapType type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDFlexWrap, type);
}

// align
void SetJustifyContent(const SLNodeRef node, SLJustifyContentType type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDJustifyContent, type);
}

void SetAlignContent(const SLNodeRef node, SLAlignContentType type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDAlignContent, type);
}

void SetAlignSelf(const SLNodeRef node, SLFlexAlignType type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDAlignSelf, type);
}

void SetAlignItems(const SLNodeRef node, SLFlexAlignType type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDAlignItems, type);
}

void SetDirection(const SLNodeRef node, SLDirectionType type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDDirection, type);
}

void SetFlexDirection(const SLNodeRef node, SLFlexDirection type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDFlexDirection, type);
}

void SetDisplay(const SLNodeRef node, SLDisplayType type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDDisplay, type);
}

void SetAspectRatio(const SLNodeRef node, float value) {
  SetValueTypeByPropertyID(node, lynx::tasm::kPropertyIDAspectRatio, value);
}

// length
SLSize GetLayoutSize(SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  const auto &result = layout_object->GetLayoutResult();
  return SLSize(result.size_.width_, result.size_.height_);
}

SLPoint GetLayoutOffset(SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  const auto &result = layout_object->GetLayoutResult();
  return SLPoint(result.offset_.X(), result.offset_.Y());
}

float GetLayoutMargin(SLNodeRef node, SLEdge edge) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  const auto &result = layout_object->GetLayoutResult();
  float margin_edge = result.margin_[ResolveEdgeToDirection(node, edge)];
  return margin_edge;
}

float GetLayoutPadding(SLNodeRef node, SLEdge edge) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  const auto &result = layout_object->GetLayoutResult();
  float padding_edge = result.padding_[ResolveEdgeToDirection(node, edge)];
  return padding_edge;
}

float GetLayoutBorder(SLNodeRef node, SLEdge edge) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  const auto &result = layout_object->GetLayoutResult();
  float border_edge = result.border_[ResolveEdgeToDirection(node, edge)];
  return border_edge;
}

void SetWidth(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDWidth, value);
}

SLLength GetWidth(const SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  const lynx::starlight::NLength &width =
      layout_object->GetCSSStyle()->GetWidth();
  return NLengthToSLLength(width);
}

void SetHeight(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDHeight, value);
}

SLLength GetHeight(const SLNodeRef node) {
  LayoutObject *layout_object = GET_LAYOUT_NODE(node);
  const lynx::starlight::NLength &width =
      layout_object->GetCSSStyle()->GetHeight();
  return NLengthToSLLength(width);
}

void SetMinWidth(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMinWidth, value);
}

void SetMinHeight(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMinHeight, value);
}

void SetMaxWidth(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMaxWidth, value);
}

void SetMaxHeight(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMaxHeight, value);
}

void SetLeft(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDLeft, value);
}

void SetRight(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDRight, value);
}

void SetBottom(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBottom, value);
}

void SetTop(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDTop, value);
}

void SetInlineStart(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDInsetInlineStart,
                            value);
}

void SetInlineEnd(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDInsetInlineEnd, value);
}

void SetMarginLeft(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginLeft, value);
}

void SetMarginRight(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginRight, value);
}

void SetMarginTop(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginTop, value);
}

void SetMarginBottom(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginBottom, value);
}

void SetMarginInlineStart(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginInlineStart,
                            value);
}

void SetMarginInlineEnd(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginInlineEnd,
                            value);
}

void SetMargin(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginLeft, value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginRight, value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginTop, value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDMarginBottom, value);
}

void SetPaddingLeft(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingLeft, value);
}

void SetPaddingRight(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingRight, value);
}

void SetPaddingTop(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingTop, value);
}

void SetPaddingBottom(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingBottom, value);
}

void SetPaddingInlineStart(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingInlineStart,
                            value);
}

void SetPaddingInlineEnd(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingInlineEnd,
                            value);
}

void SetPadding(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingLeft, value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingRight, value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingTop, value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDPaddingBottom, value);
}

void SetBorderLeft(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBorderLeftWidth,
                            value);
}

void SetBorderRight(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBorderRightWidth,
                            value);
}

void SetBorderTop(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBorderTopWidth, value);
}

void SetBorderBottom(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBorderBottomWidth,
                            value);
}

void SetBorder(const SLNodeRef node, const SLLength value) {
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBorderLeftWidth,
                            value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBorderRightWidth,
                            value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBorderTopWidth, value);
  SetLengthTypeByPropertyID(node, lynx::tasm::kPropertyIDBorderBottomWidth,
                            value);
}

void SetPosition(const SLNodeRef node, SLPositionType type) {
  SetEnumTypeByPropertyID(node, lynx::tasm::kPropertyIDPosition, type);
}

}  // namespace starlight
