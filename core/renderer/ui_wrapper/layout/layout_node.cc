// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/layout_node.h"

#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/lynx_env_config.h"

namespace lynx {
namespace tasm {

namespace {
void UpdateStyleWithEnvConfig(starlight::ComputedCSSStyle& css_style,
                              const tasm::LynxEnvConfig& envs) {
  css_style.SetScreenWidth(envs.ScreenWidth());
  css_style.SetFontScale(envs.FontScale());
  css_style.SetViewportWidth(envs.ViewportWidth());
  css_style.SetViewportHeight(envs.ViewportHeight());
}

}  // namespace

#define FOREACH_LAYOUT_PROPERTY(V)    \
  V(Top, LAYOUT_ONLY)                 \
  V(Left, LAYOUT_ONLY)                \
  V(Right, LAYOUT_ONLY)               \
  V(Bottom, LAYOUT_ONLY)              \
  V(Width, LAYOUT_ONLY)               \
  V(MaxWidth, LAYOUT_ONLY)            \
  V(MinWidth, LAYOUT_ONLY)            \
  V(Height, LAYOUT_ONLY)              \
  V(MaxHeight, LAYOUT_ONLY)           \
  V(MinHeight, LAYOUT_ONLY)           \
  V(PaddingLeft, LAYOUT_ONLY)         \
  V(PaddingRight, LAYOUT_ONLY)        \
  V(PaddingTop, LAYOUT_ONLY)          \
  V(PaddingBottom, LAYOUT_ONLY)       \
  V(MarginLeft, LAYOUT_ONLY)          \
  V(MarginRight, LAYOUT_ONLY)         \
  V(MarginTop, LAYOUT_ONLY)           \
  V(MarginBottom, LAYOUT_ONLY)        \
  V(BorderLeftWidth, LAYOUT_WANTED)   \
  V(BorderRightWidth, LAYOUT_WANTED)  \
  V(BorderTopWidth, LAYOUT_WANTED)    \
  V(BorderBottomWidth, LAYOUT_WANTED) \
  V(FlexBasis, LAYOUT_ONLY)           \
  V(FlexGrow, LAYOUT_ONLY)            \
  V(FlexShrink, LAYOUT_ONLY)          \
  V(LinearWeightSum, LAYOUT_ONLY)     \
  V(LinearWeight, LAYOUT_ONLY)        \
  V(AspectRatio, LAYOUT_ONLY)         \
  V(RelativeId, LAYOUT_ONLY)          \
  V(RelativeAlignTop, LAYOUT_ONLY)    \
  V(RelativeAlignRight, LAYOUT_ONLY)  \
  V(RelativeAlignBottom, LAYOUT_ONLY) \
  V(RelativeAlignLeft, LAYOUT_ONLY)   \
  V(RelativeTopOf, LAYOUT_ONLY)       \
  V(RelativeRightOf, LAYOUT_ONLY)     \
  V(RelativeBottomOf, LAYOUT_ONLY)    \
  V(RelativeLeftOf, LAYOUT_ONLY)      \
  V(RelativeLayoutOnce, LAYOUT_ONLY)  \
  V(Order, LAYOUT_ONLY)               \
  V(Flex, LAYOUT_ONLY)                \
  V(BorderWidth, LAYOUT_WANTED)       \
  V(Padding, LAYOUT_ONLY)             \
  V(Margin, LAYOUT_ONLY)              \
  V(Border, LAYOUT_WANTED)            \
  V(BorderRight, LAYOUT_WANTED)       \
  V(BorderLeft, LAYOUT_WANTED)        \
  V(BorderTop, LAYOUT_WANTED)         \
  V(BorderBottom, LAYOUT_WANTED)      \
  V(Flex, LAYOUT_ONLY)                \
  V(FlexDirection, LAYOUT_ONLY)       \
  V(FlexWrap, LAYOUT_ONLY)            \
  V(AlignItems, LAYOUT_ONLY)          \
  V(AlignSelf, LAYOUT_ONLY)           \
  V(AlignContent, LAYOUT_ONLY)        \
  V(JustifyContent, LAYOUT_ONLY)      \
  V(LinearOrientation, LAYOUT_ONLY)   \
  V(LinearLayoutGravity, LAYOUT_ONLY) \
  V(LinearGravity, LAYOUT_ONLY)       \
  V(LinearCrossGravity, LAYOUT_ONLY)  \
  V(RelativeCenter, LAYOUT_ONLY)      \
  V(Position, LAYOUT_ONLY)            \
  V(Display, LAYOUT_ONLY)             \
  V(BoxSizing, LAYOUT_ONLY)           \
  V(Content, LAYOUT_ONLY)             \
  V(Direction, LAYOUT_WANTED)         \
  V(GridTemplateColumns, LAYOUT_ONLY) \
  V(GridTemplateRows, LAYOUT_ONLY)    \
  V(GridAutoColumns, LAYOUT_ONLY)     \
  V(GridAutoRows, LAYOUT_ONLY)        \
  V(GridColumnSpan, LAYOUT_ONLY)      \
  V(GridRowSpan, LAYOUT_ONLY)         \
  V(GridColumnStart, LAYOUT_ONLY)     \
  V(GridColumnEnd, LAYOUT_ONLY)       \
  V(GridRowStart, LAYOUT_ONLY)        \
  V(GridRowEnd, LAYOUT_ONLY)          \
  V(GridColumnGap, LAYOUT_ONLY)       \
  V(GridRowGap, LAYOUT_ONLY)          \
  V(JustifyItems, LAYOUT_ONLY)        \
  V(JustifySelf, LAYOUT_ONLY)         \
  V(GridAutoFlow, LAYOUT_ONLY)        \
  V(ListCrossAxisGap, LAYOUT_WANTED)  \
  V(LinearDirection, LAYOUT_ONLY)     \
  V(VerticalAlign, LAYOUT_WANTED)     \
  V(Gap, LAYOUT_ONLY)                 \
  V(ColumnGap, LAYOUT_ONLY)           \
  V(RowGap, LAYOUT_ONLY)

LayoutNode::LayoutNode(int id, const starlight::LayoutConfigs& layout_configs,
                       const tasm::LynxEnvConfig& envs,
                       const starlight::ComputedCSSStyle& init_style)
    : type_(LayoutNodeType::COMMON),
      css_style_(std::make_unique<starlight::ComputedCSSStyle>(init_style)),
      id_(id) {
  sl_node_ = std::make_unique<SLNode>(layout_configs,
                                      css_style_->GetLayoutComputedStyle());
  css_style_->SetFontScaleOnlyEffectiveOnSp(layout_configs.font_scale_sp_only_);
  css_style_->SetCssAlignLegacyWithW3c(
      layout_configs.css_align_with_legacy_w3c_);
  UpdateStyleWithEnvConfig(*css_style_, envs);
}

LayoutNode::~LayoutNode() { sl_node_ = nullptr; }

void LayoutNode::ConsumeStyle(CSSPropertyID id, const tasm::CSSValue& value,
                              bool reset) {
  if (css_style_->SetValue(id, value, reset)) {
    sl_node_->MarkDirty();
  }
}

void LayoutNode::ConsumeAttribute(starlight::LayoutAttribute key,
                                  const lepus::Value& value, bool reset) {
  lepus::Value new_value = reset ? lepus::Value() : value;
  bool changed = false;
  if (key == starlight::LayoutAttribute::kScroll) {
    changed = sl_node_->attr_map().setScroll(
        new_value.IsBool() ? std::optional<bool>(new_value.Bool())
                           : std::nullopt);
  } else if (key == starlight::LayoutAttribute::kColumnCount) {
    changed = sl_node_->attr_map().setColumnCount(
        new_value.IsNumber() ? std::optional<int>(new_value.Number())
                             : std::nullopt);
  } else if (key == starlight::LayoutAttribute::kListCompType) {
    changed = sl_node_->attr_map().setListCompType(
        new_value.IsNumber() ? std::optional<int>(new_value.Number())
                             : std::nullopt);
  } else if (key == starlight::LayoutAttribute::kListContainer &&
             value.IsBool()) {
    is_list_container_ = value.Bool();
  }

  if (changed) {
    if (sl_node_->IsList()) {
      sl_node_->MarkChildrenDirtyWithoutTriggerLayout();
    }
    sl_node_->MarkDirty();
  }
}

void LayoutNode::ConsumeFontSize(double cur_node_font_size,
                                 double root_node_font_size,
                                 double font_scale) {
  if (css_style_->SetFontSize(cur_node_font_size, root_node_font_size) ||
      css_style_->SetFontScale(font_scale)) {
    sl_node_->MarkDirty();
  }
}

void LayoutNode::InsertNode(LayoutNode* child, int index) {
  // Inline views should be bind to non-virtual parent layoutobject.
  if (is_virtual() && !child->is_virtual()) {
    LayoutNode* parent = FindNonVirtualNode();
    parent->slnode()->AppendChild(child->slnode());
  }

  if (index == -1) {
    if (!child->is_virtual() && !is_virtual()) {
      sl_node_->AppendChild(child->slnode());
    }
    MarkDirty();
    children_.push_back(child);
  } else {
    if (!child->is_virtual() && !is_virtual()) {
      LayoutNode* previous_non_virtual_child = FindNextNonVirtualChild(index);
      sl_node_->InsertChildBefore(child->slnode(),
                                  previous_non_virtual_child
                                      ? previous_non_virtual_child->slnode()
                                      : nullptr);
    }
    MarkDirty();
    children_.insert(children_.begin() + index, child);
  }
  child->parent_ = this;
}

LayoutNode* LayoutNode::RemoveNodeAtIndex(unsigned int index) {
  if (index >= children_.size()) return nullptr;

  auto child = children_[index];
  // Remove inline views from non-virtual parent node.
  if (is_virtual() && !child->is_virtual()) {
    LayoutNode* parent = FindNonVirtualNode();
    parent->slnode()->RemoveChild(child->slnode());
  }

  if (!child->is_virtual() && !is_virtual()) {
    sl_node_->RemoveChild(children_[index]->slnode());
  }
  MarkDirty();
  children_.erase(children_.begin() + index);
  child->parent_ = nullptr;
  return child;
}

void LayoutNode::MoveNode(LayoutNode* child, int from_index,
                          unsigned int to_index) {
  RemoveNodeAtIndex(from_index);
  InsertNode(child, to_index);
}

void LayoutNode::CalculateLayout(const SLNodeSet* fixed_node_set) {
  sl_node_->ReLayout(fixed_node_set);
}

void LayoutNode::CalculateLayoutWithConstraints(
    starlight::Constraints& constraints, const SLNodeSet* fixed_node_set) {
  sl_node_->ReLayoutWithConstraints(constraints, fixed_node_set);
}

LayoutNode* LayoutNode::FindNonVirtualNode() {
  if (!is_virtual()) {
    return this;
  }
  LayoutNode* temp = parent_;
  while (temp && temp->is_virtual()) {
    temp = temp->parent_;
  }
  return temp;
}

LayoutNode* LayoutNode::FindNextNonVirtualChild(
    size_t equal_or_after_index) const {
  for (size_t current_index = equal_or_after_index;
       current_index < children_.size(); ++current_index) {
    if (!children_[current_index]->is_virtual()) {
      return children_[current_index];
    }
  }
  return nullptr;
}

FloatSize LayoutNode::UpdateMeasureByPlatform(
    const starlight::Constraints& constraints, bool final_measure) {
  if (!slnode()) {
    return FloatSize{0.f, 0.f, 0.f};
  }

  // Always assign final measure to true, because It maybe faster in most cases.
  // Platform measure is very likely to be a slow process,
  // but starlight measure is super fast.
  // Setting final measure to true will make the children layout is always in
  // sync with the platform layout, to avoid triggering platform layout because
  // the children of platform node is not in sync with current layout.
  // TODO(liting,wangzhixuan.0821): Maybe invent a fast mechanism to sync
  // children layout without triggering the layout of parent.
  return slnode()->UpdateMeasureByPlatform(constraints, true);
}

void LayoutNode::AlignmentByPlatform(float offset_top, float offset_left) {
  if (!slnode()) {
    return;
  }

  slnode()->AlignmentByPlatform(offset_top, offset_left);
}

void LayoutNode::SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func) {
  measure_func_ = std::move(measure_func);

  sl_node_->SetContext(this);
  sl_node_->SetSLMeasureFunc([](void* context,
                                const starlight::Constraints& constraints,
                                bool final_measure) {
    MeasureFunc* measure = (static_cast<LayoutNode*>(context))->measure_func();
    DCHECK(measure);
    SLMeasureMode width_mode = constraints[starlight::kHorizontal].Mode();
    SLMeasureMode height_mode = constraints[starlight::kVertical].Mode();
    float width = IsSLIndefiniteMode(width_mode)
                      ? 0.f
                      : constraints[starlight::kHorizontal].Size();
    float height = IsSLIndefiniteMode(height_mode)
                       ? 0.f
                       : constraints[starlight::kVertical].Size();

    LayoutResult result =
        measure->Measure(width, width_mode, height, height_mode, final_measure);

    return FloatSize(result.width_, result.height_, result.baseline_);
  });
  sl_node_->SetSLAlignmentFunc([](void* context) {
    MeasureFunc* measure = (static_cast<LayoutNode*>(context))->measure_func();
    DCHECK(measure);
    measure->Alignment();
  });
}

ConsumptionStatus LayoutNode::ConsumptionTest(CSSPropertyID id) {
  static int kWantedProperty[kPropertyEnd];
  static bool kIsInit = false;
  if (!kIsInit) {
    kIsInit = true;
    std::fill(kWantedProperty, kWantedProperty + kPropertyEnd,
              ConsumptionStatus::SKIP);
#define DECLARE_WANTED_PROPERTY(name, type) \
  kWantedProperty[kPropertyID##name] = type;
    FOREACH_LAYOUT_PROPERTY(DECLARE_WANTED_PROPERTY)
#undef DECLARE_WANTED_PROPERTY
  }
  return static_cast<ConsumptionStatus>(kWantedProperty[id]);
}

void LayoutNode::set_type(LayoutNodeType type) { type_ = type; }

bool LayoutNode::IsDirty() {
  return is_dirty_ || (slnode() && slnode()->IsDirty());
}

void LayoutNode::MarkDirty() { MarkDirtyInternal(false); }

void LayoutNode::MarkDirtyAndRequestLayout() { MarkDirtyInternal(true); }

void LayoutNode::MarkDirtyInternal(bool request_layout) {
  if (is_dirty_) {
    return;
  }
  if (!is_virtual()) {
    if (sl_node_) {
      if (request_layout) {
        sl_node_->MarkDirtyAndRequestLayout();
      } else {
        sl_node_->MarkDirty();
      }
    }
  } else {
    LayoutNode* node = FindNonVirtualNode();
    if (node && node->sl_node_) {
      if (request_layout) {
        node->sl_node_->MarkDirtyAndRequestLayout();
      } else {
        node->sl_node_->MarkDirty();
      }
    }
  }
  is_dirty_ = true;
}

void LayoutNode::MarkUpdated() {
  is_dirty_ = false;
  if (!is_virtual()) {
    sl_node_->MarkUpdated();
  }
}

void LayoutNode::UpdateLynxEnv(const tasm::LynxEnvConfig& config) {
  UpdateStyleWithEnvConfig(*css_style_, config);
  for (auto& child : children_) {
    child->UpdateLynxEnv(config);
  }
}

void LayoutNode::SetTag(const base::String& tag) {
  tag_ = tag;
  if (sl_node_) {
    sl_node_->SetTag(tag.str());
  }
}

#undef FOREACH_LAYOUT_PROPERTY
}  // namespace tasm
}  // namespace lynx
