// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_element.h"

#include <memory>
#include <utility>

#include "core/renderer/css/layout_property.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_list_base.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/feature_count/global_feature_counter.h"

namespace lynx {
namespace tasm {

RadonElement::RadonElement(const base::String& tag,
                           const fml::RefPtr<AttributeHolder>& node,
                           ElementManager* manager, uint32_t node_index)
    : Element(tag, manager, node_index),
      styles_manager_(
          this,
          manager ? manager->GetDynamicCSSConfigs()
                  : DynamicCSSConfigs::GetDefaultDynamicCSSConfigs(),
          manager ? manager->GetLynxEnvConfig().DefaultFontSize() : 0) {
  if (node) {
    SetAttributeHolder(node);
  }

  if (tag_.IsEquals(kViewTag)) {
    is_view_ = true;
  } else if (tag_.IsEquals(kImageTag)) {
    is_image_ = true;
  } else if (tag_.IsEquals(kTextTag)) {
    is_text_ = true;
  } else if (tag_.IsEqual(kListTag)) {
    is_list_ = true;
  }

  if (manager == nullptr) {
    return;
  }

  const auto& env_config = manager->GetLynxEnvConfig();

  manager->CreateLayoutNode(impl_id(), tag_);

  if (Config::DefaultFontScale() != env_config.FontScale()) {
    computed_css_style()->SetFontScale(env_config.FontScale());
    SetComputedFontSize(tasm::CSSValue(), env_config.PageDefaultFontSize(),
                        env_config.PageDefaultFontSize(), true);
    manager->UpdateLayoutNodeFontSize(impl_id(),
                                      env_config.PageDefaultFontSize(),
                                      env_config.PageDefaultFontSize());
  }

  styles_manager_.SetInitialResolvingStatus(GenerateRootPropertyStatus());
  if (is_view_ || tag_ == "component") {
    computed_css_style()->SetOverflowDefaultVisible(
        manager->GetDefaultOverflowVisible());
    overflow_ =
        manager->GetDefaultOverflowVisible() ? OVERFLOW_XY : OVERFLOW_HIDDEN;
  }
  if (is_text_ || tag_ == "x-text") {
    computed_css_style()->SetOverflowDefaultVisible(
        manager->GetDefaultTextOverflow());
    overflow_ =
        manager->GetDefaultTextOverflow() ? OVERFLOW_XY : OVERFLOW_HIDDEN;
  }
}

RadonElement::~RadonElement() {
  if (element_manager() == nullptr) {
    return;
  }

  element_manager()->NotifyElementDestroy(this);
  element_manager()->EraseGlobalBindElementId(global_bind_event_map(),
                                              impl_id());
  element_manager()->node_manager()->Erase(impl_id());
  // remove this element from parent and children.
  auto parent = static_cast<RadonElement*>(parent_);
  if (parent) {
    parent->RemoveNode(this);
  }
  for (auto& child : children_) {
    auto* child_element = static_cast<RadonElement*>(child);
    if (child_element) {
      child_element->set_parent(nullptr);
      if (child_element->IsPseudoNode()) {
        delete child_element;
      }
    }
  }
  element_manager()->DestroyLayoutNode(impl_id());
}

void RadonElement::MarkAsLayoutRoot() {
  element_manager_->SetRootOnLayout(id_);
}

void RadonElement::AttachLayoutNode(const fml::RefPtr<PropBundle>& props) {
  bool allow_inline = false;
  if (parent_) {
    allow_inline = parent_->IsShadowNodeCustom();
  }
  element_manager()->AttachLayoutNodeType(impl_id(), tag_, allow_inline, props);
}

void RadonElement::UpdateLayoutNodeProps(const fml::RefPtr<PropBundle>& props) {
  element_manager()->UpdateLayoutNodeProps(id_, props);
}

void RadonElement::UpdateLayoutNodeStyle(CSSPropertyID css_id,
                                         const tasm::CSSValue& value) {
  element_manager()->UpdateLayoutNodeStyle(id_, css_id, value);
}

void RadonElement::ResetLayoutNodeStyle(tasm::CSSPropertyID css_id) {
  element_manager()->ResetLayoutNodeStyle(id_, css_id);
}

void RadonElement::UpdateLayoutNodeFontSize(double cur_node_font_size,
                                            double root_node_font_size) {
  element_manager()->UpdateLayoutNodeFontSize(id_, cur_node_font_size,
                                              root_node_font_size);
}

void RadonElement::UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                             const lepus::Value& value) {
  element_manager()->UpdateLayoutNodeAttribute(id_, key, value);
}

std::optional<CSSValue> RadonElement::GetElementStyle(
    tasm::CSSPropertyID css_id) {
  if (!data_model()) {
    return std::optional<CSSValue>();
  }
  const auto& cached_styles = data_model()->radon_node_ptr()->cached_styles_;
  auto iter = cached_styles.find(css_id);
  if (iter == cached_styles.end()) {
    return std::optional<CSSValue>();
  }
  return iter->second;
}

ListNode* RadonElement::GetListNode() {
  auto* node = data_model()->radon_node_ptr();
  if (node && node->NodeType() == RadonNodeType::kRadonListNode) {
    return static_cast<RadonListBase*>(node);
  }
  return nullptr;
}

bool RadonElement::InComponent() const {
  if (data_model()) {
    return data_model()->radon_node_ptr()->InComponent();
  }
  return false;
}

Element* RadonElement::GetParentComponentElement() const {
  auto* node = data_model()->radon_node_ptr();
  RadonComponent* comp = node->component();

  Element* element = const_cast<RadonElement*>(this);
  if (element->GetPageElementEnabled() && comp->IsRadonPage()) {
    return static_cast<RadonNode*>(comp->radon_children_[0].get())->element();
  } else {
    return comp->element();
  }
}

bool RadonElement::is_component() const {
  if (!data_model_ || !data_model_->radon_node_ptr()) {
    return false;
  }
  return data_model_->radon_node_ptr()->IsRadonComponent();
}

void RadonElement::SetNativeProps(
    const lepus::Value& args,
    std::shared_ptr<PipelineOptions>& pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_ELEMENT_SET_NATIVE_PROPS);
  if (!args.IsTable()) {
    LOGE("SetNativeProps's param must be a Table!");
    return;
  }

  auto args_table = args.Table();

  if (args_table->size() <= 0) {
    LOGE("SetNativeProps's param must not be empty!");
    return;
  }
  // It hard to calculate the precise count of final styles.
  // Just reserve according to table size.
  StyleMap styles(args_table->size() + kCSSStyleMapFuzzyAllocationSize);
  for (auto& arg : *args_table) {
    auto id = CSSProperty::GetPropertyID(arg.first);
    if (id != kPropertyEnd) {
      UnitHandler::Process(id, arg.second, styles,
                           element_manager_->GetCSSParserConfigs());
      EXEC_EXPR_FOR_INSPECTOR(element_manager()->OnSetNativeProps(
          this, arg.first.str(), arg.second, true));
    } else if (arg.first.IsEqual("text") &&
               (tag_.IsEqual("text") || tag_.IsEqual("x-text") ||
                tag_.IsEqual("x-inline-text") || tag_.IsEqual("inline-text")) &&
               children_.size() > 0) {
      // FIXME(linxs): use function to get rawText
      children_[0]->SetAttribute(arg.first, arg.second, false);
      EXEC_EXPR_FOR_INSPECTOR(element_manager()->OnSetNativeProps(
          children_[0], arg.first.str(), arg.second, false));
    } else {
      SetAttribute(arg.first, arg.second, false);
      EXEC_EXPR_FOR_INSPECTOR(element_manager()->OnSetNativeProps(
          this, arg.first.str(), arg.second, false));
    }
  }
  ConsumeStyle(styles);
  element_manager_->OnFinishUpdateProps(this, pipeline_options);
}

void RadonElement::ReserveForAttribute(size_t count) {
  attributes_.reserve(count);
}

void RadonElement::SetAttribute(const base::String& key,
                                const lepus::Value& value,
                                bool need_update_data_model) {
  WillConsumeAttribute(key, value);

  PreparePropBundleIfNeed();

  // Any attribute will cause has_layout_only_props_ = false
  has_layout_only_props_ = false;

  // record attributes, used for worklet
  attributes_.insert_or_assign(key, value);

  StyleMap attr_styles;
  // FIXME(liyanbo): Compatible with old logic.support: <text
  // text-overflow="ellipsis"></text>
  // remove when front change this style of writing.
  if (key.IsEqual(kTextOverFlow)) {
    tasm::UnitHandler::Process(kPropertyIDTextOverflow, value, attr_styles,
                               element_manager_->GetCSSParserConfigs());
  } else {
    if (OnAttributeSet(key, value)) {
      prop_bundle_->SetProps(key.c_str(), pub::ValueImplLepus(value));
    }

    const auto& value_str = value.StdString();
    if (key.IsEquals(kScrollX) && value_str == kTrue) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(
              static_cast<int>(starlight::LinearOrientationType::kHorizontal)));
      element_manager_->UpdateLayoutNodeAttribute(
          id_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals(kScrollY) && value_str == kTrue) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(
              static_cast<int>(starlight::LinearOrientationType::kVertical)));
      element_manager_->UpdateLayoutNodeAttribute(
          id_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals(kScrollOrientation)) {
      if (value_str == kHorizontal) {
        attr_styles.insert_or_assign(
            kPropertyIDLinearOrientation,
            CSSValue::MakeEnum(static_cast<int>(
                starlight::LinearOrientationType::kHorizontal)));
        element_manager_->UpdateLayoutNodeAttribute(
            id_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
      } else if (value_str == kVertical) {
        attr_styles.insert_or_assign(
            kPropertyIDLinearOrientation,
            CSSValue::MakeEnum(
                static_cast<int>(starlight::LinearOrientationType::kVertical)));
        element_manager_->UpdateLayoutNodeAttribute(
            id_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
      }
      //(TODO)fangzhou.fz: If it becomes necessary in the future, extend the
      //'both' mode.
    } else if (key.IsEquals(kScrollXReverse) && value_str == kTrue) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(static_cast<int>(
              starlight::LinearOrientationType::kHorizontalReverse)));
      element_manager_->UpdateLayoutNodeAttribute(
          id_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals(kScrollYReverse) && value_str == kTrue) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(static_cast<int>(
              starlight::LinearOrientationType::kVerticalReverse)));
      element_manager_->UpdateLayoutNodeAttribute(
          id_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEqual(kColumnCount) || key.IsEqual(kSpanCount)) {
      element_manager_->UpdateLayoutNodeAttribute(
          id_, starlight::LayoutAttribute::kColumnCount, value);
    } else if (key.IsEqual(ListComponentInfo::kListCompType)) {
      element_manager_->UpdateLayoutNodeAttribute(
          id_, starlight::LayoutAttribute::kListCompType, value);
    } else if (key.IsEquals(kVerticalOrientation)) {
      if (value_str == kTrue) {
        attr_styles.insert_or_assign(
            kPropertyIDLinearOrientation,
            CSSValue::MakeEnum(
                static_cast<int>(starlight::LinearOrientationType::kVertical)));
        UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                  lepus::Value(true));
      } else if (value_str == kFalse) {
        attr_styles.insert_or_assign(
            kPropertyIDLinearOrientation,
            CSSValue::MakeEnum(static_cast<int>(
                starlight::LinearOrientationType::kHorizontal)));
        UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                  lepus::Value(true));
      }
    }
  }
  ConsumeStyle(attr_styles);
}

void RadonElement::ResetAttribute(const base::String& key) {
  Element::ResetAttribute(key);
  attributes_.erase(key);
}

void RadonElement::ResetStyle(const base::Vector<CSSPropertyID>& style_names) {
  if (style_names.empty()) {
    return;
  }

  bool should_consume_trans_styles_in_advance =
      ShouldConsumeTransitionStylesInAdvance();
  // #1. Check whether we need to reset transition styles in advance.
  if (should_consume_trans_styles_in_advance) {
    ResetTransitionStylesInAdvance(style_names);
  }

  for (auto& css_id : style_names) {
    // TODO: zhixuan
    if (css_id == kPropertyIDFontSize) {
      auto empty = CSSValue::Empty();
      styles_manager_.UpdateFontSizeStyle(&empty);
      continue;
    } else if (css_id == kPropertyIDDirection) {
      styles_manager_.UpdateDirectionStyle(CSSValue::Empty());
    } else if (css_id == kPropertyIDPosition) {
      is_fixed_ = false;
      // #2. If these transition styles have been reset beforehand, skip them
      // here.
    } else if (should_consume_trans_styles_in_advance &&
               CSSProperty::IsTransitionProps(css_id)) {
      continue;
    }
    // #3. Review each property to determine whether the reset should be
    // intercepted.
    if (css_transition_manager_ && css_transition_manager_->ConsumeCSSProperty(
                                       css_id, CSSValue::Empty())) {
      continue;
    }
    // Since the previous element styles cannot be accessed in element, we
    // need to record some necessary styles which New Animator transition needs,
    // and it needs to be saved before rtl converted logic.
    ResetElementPreviousStyle(css_id);
    styles_manager_.AdoptStyle(css_id, CSSValue::Empty());
  }
  for (auto& css_id : style_names) {
    // record styles, used for worklet
    styles_.erase(css_id);
  }
}

StyleMap RadonElement::GetStylesForWorklet() { return styles_; }

const AttrMap& RadonElement::GetAttributesForWorklet() { return attributes_; }

void RadonElement::InsertNode(const fml::RefPtr<Element>& child) {
  InsertNode(child, static_cast<int32_t>(children_.size()));
}

void RadonElement::InsertNode(const fml::RefPtr<Element>& child,
                              int32_t index) {
  InsertNode(static_cast<RadonElement*>(child.get()), index);
}

void RadonElement::InsertNode(RadonElement* child, int32_t index) {
  if (index == -1) {
    index = static_cast<int32_t>(children_.size());
  }

  element_manager()->InsertLayoutNode(impl_id(), child->impl_id(),
                                      static_cast<int>(index));
  AddChildAt(child, index);

  if (element_container()) {
    element_container()->AttachChildToTargetContainer(GetChildAt(index));
  }
}

void RadonElement::RemoveNode(const fml::RefPtr<Element>& child, bool destroy) {
  RemoveNode(static_cast<RadonElement*>(child.get()),
             IndexOf(static_cast<RadonElement*>(child.get())), destroy);
}

void RadonElement::RemoveNode(RadonElement* child, bool destroy) {
  RemoveNode(child, IndexOf(child), destroy);
}

void RadonElement::RemoveNode(RadonElement* child, int32_t index,
                              bool destroy) {
  if (index >= static_cast<int32_t>(children_.size())) return;
  bool destroy_platform_node = destroy && child->HasPaintingNode();
  element_manager()->RemoveLayoutNodeAtIndex(impl_id(), index);
  RemoveChildAt(index);
  child->element_container()->RemoveSelf(destroy_platform_node);
  if (destroy_platform_node) child->MarkPlatformNodeDestroyedRecursively();
}

void RadonElement::MarkPlatformNodeDestroyedRecursively() {
  has_painting_node_ = false;
  // All descent UI will be deleted recursively in platform side, should mark it
  // recursively
  for (size_t i = 0; i < GetChildCount(); ++i) {
    auto* child = static_cast<RadonElement*>(GetChildAt(i));
    child->MarkPlatformNodeDestroyedRecursively();
    // The z-index child's parent may be different from ui parent
    // and not destroyed
    if (child->HasElementContainer() && child->ZIndex() != 0) {
      child->element_container()->Destroy();
    }
    if (child->parent() == this) {
      child->set_parent(nullptr);
    }
  }
  // clear element's children only in radon or radon compatible mode.
  children_.clear();
}

void RadonElement::UpdateDynamicElementStyle(uint32_t style,
                                             bool force_update) {
  DCHECK(!parent());
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_ELEMENT_UPDATE_DYNAMIC_STYLE);
  ClearDynamicCSSChildrenStatus();
  PreparePropsBundleForDynamicCSS();
  NotifyUnitValuesUpdatedToAnimation(style);
  FlushDynamicStyles();
}

void RadonElement::ClearDynamicCSSChildrenStatus() {
  styles_manager_.ClearChildrenStatus();
  for (auto& child : children_) {
    static_cast<RadonElement*>(child)->ClearDynamicCSSChildrenStatus();
  }
}

void RadonElement::FlushDynamicStyles() {
  // When the element is first created, we will consume the transition data
  // after all styles (including dynamic styles) have been resolved.
  // If the has_transition_props_changed_ is still true here, it means that this
  // element is first created and the transition props do not be consumed ahead.
  // We should consume them here.
  if (has_transition_props_changed_ && enable_new_animator()) {
    SetDataToNativeTransitionAnimator();
  }

  if (prop_bundle_) {
    FlushProps();
  }

  const size_t children_size = children_.size();
  for (size_t i = 0; i < children_size; ++i) {
    auto* node = static_cast<RadonElement*>(GetChildAt(i));
    node->FlushDynamicStyles();
  }
}

int RadonElement::ParentComponentId() const {
  if (data_model()) {
    return data_model()->radon_node_ptr()->ParentComponentId();
  }
  return 0;
}

std::string RadonElement::ParentComponentIdString() const {
  return std::to_string(ParentComponentId());
}

const std::string& RadonElement::ParentComponentEntryName() const {
  if (data_model()) {
    auto* parent_component = data_model()->radon_node_ptr()->component();
    if (parent_component) {
      return parent_component->GetEntryName();
    }
  }
  static std::string kDefaultEntryName(tasm::DEFAULT_ENTRY_NAME);
  return kDefaultEntryName;
}

Element* RadonElement::Sibling(int offset) const {
  if (!parent_) return nullptr;
  auto index = static_cast<RadonElement*>(parent_)->IndexOf(this);
  // We know the index can't be -1
  return parent_->GetChildAt(index + offset);
}

void RadonElement::AddChildAt(RadonElement* child, size_t index) {
  children_.insert(children_.begin() + index, child);
  child->set_parent(this);
  child->StylesManager().MarkNewlyInserted();
  if (is_list_) {
    child->MarkAsListItem();
  }
}

RadonElement* RadonElement::RemoveChildAt(size_t index) {
  auto* removed = static_cast<RadonElement*>(children_[index]);
  children_.erase(children_.begin() + index);
  removed->set_parent(nullptr);
  return removed;
}

int32_t RadonElement::IndexOf(const Element* child) const {
  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end()) {
    return static_cast<int>(std::distance(children_.begin(), it));
  } else {
    return -1;
  }
}

bool RadonElement::GetPageElementEnabled() {
  EXEC_EXPR_FOR_INSPECTOR({
    if (!data_model()) return false;
    return data_model()
        ->radon_node_ptr()
        ->page_proxy()
        ->GetPageElementEnabled();
  });
  return false;
}

bool RadonElement::GetRemoveCSSScopeEnabled() {
  EXEC_EXPR_FOR_INSPECTOR({
    if (!data_model()) return false;
    return data_model()
        ->radon_node_ptr()
        ->page_proxy()
        ->GetRemoveCSSScopeEnabled();
  });
  return false;
}

Element* RadonElement::GetChildAt(size_t index) {
  if (index >= children_.size()) {
    return nullptr;
  }
  return children_[index];
}

size_t RadonElement::GetUIIndexForChild(Element* child) {
  int index = 0;
  bool found = false;
  for (auto& it : children_) {
    auto* current = static_cast<RadonElement*>(it);
    if (child == current) {
      found = true;
      break;
    }
    if (current->ZIndex() != 0 || current->IsNewFixed()) {
      continue;
    }
    index += (current->IsLayoutOnly() ? current->GetUIChildrenCount() : 1);
  }
  if (!found) {
    LOGE("element can not found:" + tag_.str());
    // Child id was not a child of parent id
    DCHECK(false);
  }
  return index;
}

size_t RadonElement::GetUIChildrenCount() {
  size_t ret = 0;
  for (auto& it : children_) {
    auto* current = static_cast<RadonElement*>(it);
    if (current->IsLayoutOnly()) {
      ret += current->GetUIChildrenCount();
    } else if (current->ZIndex() == 0 && !current->IsNewFixed()) {
      ret++;
    }
  }
  return ret;
}

void RadonElement::SetComponentIDPropsIfNeeded() {
  if (!tag_.IsEquals("component")) {
    return;
  }
  // only used in radon

  RadonComponent* comp =
      static_cast<RadonComponent*>(data_model_->radon_node_ptr());
  prop_bundle_->SetProps(kComponentID, comp->ComponentId());
}

void RadonElement::FlushPropsFirstTimeWithParentElement(Element* parent) {
  CheckHasInlineContainer(parent);

  FlushProps();
}

void RadonElement::FlushProps() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_ELEMENT_FLUSH_PROPS);

  // Only view and component can be optimized as layout only node
  if (has_layout_only_props_ && !(is_view_ || tag_.IsEquals("component"))) {
    has_layout_only_props_ = false;
  }

  if (tag_.IsEquals("scroll-view") || tag_.IsEqual("list") ||
      tag_.IsEquals("x-overlay-ng")) {
    element_manager_->UpdateLayoutNodeAttribute(
        impl_id(), starlight::LayoutAttribute::kScroll, lepus::Value(true));
    can_has_layout_only_children_ = false;
  }
  // Report when enableNewAnimator is the default value.
  if ((has_transition_props_changed_ || has_keyframe_props_changed_) &&
      !enable_new_animator()) {
    report::GlobalFeatureCounter::Count(
        report::LynxFeature::CPP_ENABLE_NEW_ANIMATOR_DEFAULT,
        element_manager()->GetInstanceId());
  }

  if (has_transition_props_changed_) {
    if (!enable_new_animator()) {
      PushToBundle(kPropertyIDTransition);
      has_transition_props_changed_ = false;
    }
  }

  if (has_keyframe_props_changed_) {
    if (!enable_new_animator()) {
      ResolveAndFlushKeyframes();
      PushToBundle(kPropertyIDAnimation);
    } else {
      SetDataToNativeKeyframeAnimator();
    }
    has_keyframe_props_changed_ = false;
  }
  // Update The root if needed

  if (!has_painting_node_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CATALYZER_NO_PAINTING_NODE);
    PreparePropBundleIfNeed();
    SetComponentIDPropsIfNeeded();
    element_manager_->AttachLayoutNodeType(
        impl_id(), tag_, allow_layoutnode_inline_, prop_bundle_);
    is_virtual_ = IsShadowNodeVirtual();
    bool platform_is_flatten = true;
    base::MoveOnlyClosure<bool, bool> func =
        [radon_element = this, has_z_props = has_z_props_,
         is_fixed = is_fixed_](bool judge_by_props) {
          if (judge_by_props) {
            return !(has_z_props || is_fixed);
          } else {
            return radon_element->TendToFlatten();
          }
        };
    platform_is_flatten = painting_context()->IsFlatten(std::move(func));
    bool is_layout_only = CanBeLayoutOnly() || is_virtual_;
    set_is_layout_only(is_layout_only);
    // native layer don't flatten.
    CreateElementContainer(platform_is_flatten);
    has_painting_node_ = true;
  } else {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CATALYZER_HAS_PAINTING_NODE);
    PreparePropBundleIfNeed();
    SetComponentIDPropsIfNeeded();
    element_manager_->UpdateLayoutNodeProps(impl_id(), prop_bundle_);
    if (!is_virtual()) {
      UpdateElement();
    }
  }
  ResetPropBundle();
}

void RadonElement::RequestLayout() { element_manager()->SetNeedsLayout(); }

void RadonElement::RequestNextFrame() {
  element_manager()->RequestNextFrame(this);
}

Element* RadonElement::first_child() const {
  if (!data_model_ || !data_model_->radon_node_ptr()) {
    return nullptr;
  }
  auto first_node_child = data_model_->radon_node_ptr()->FirstNodeChild();
  if (!first_node_child) return nullptr;
  return first_node_child->element();
}

Element* RadonElement::last_child() const {
  if (!data_model_ || !data_model_->radon_node_ptr()) {
    return nullptr;
  }
  auto last_node_child = data_model_->radon_node_ptr()->LastNodeChild();
  if (!last_node_child) return nullptr;
  return last_node_child->element();
}

void RadonElement::OnPseudoStatusChanged(PseudoState prev_status,
                                         PseudoState current_status) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_ELEMENT_ON_PSEUDO_STATUS_CHANGED);

  // If data_model() is null or data_model() is not RadonNode, return.
  if (data_model() == nullptr) {
    return;
  }

  RadonNode* node = data_model()->radon_node_ptr();
  node->SetPseudoState(current_status);
}

// If new animator is enabled and this element has been created before, we
// should consume transition styles in advance. Also transition manager needs to
// verify every property to determine whether to intercept this update.
// Therefore, the operations related to Transition in the SetStyle process are
// divided into three steps:
// 1. Consume all transition styles in advance if needed.
// 2. Skip all transition styles in the later process if they have been consume
// in advance.
// 3. Check every property to determine whether to intercept this update.
void RadonElement::ConsumeStyle(const StyleMap& styles,
                                const StyleMap* inherit_styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_ELEMENT_CONSUME_STYLE);
  if (styles.empty()) {
    return;
  }
  // set font-size first.Other css may use this to calc rem or em.
  auto it = styles.find(CSSPropertyID::kPropertyIDFontSize);
  if (it != styles.end()) {
    SetFontSize(&(it->second));
  } else {
    SetFontSize(nullptr);
  }

  // Set rtl flag and lynx-rtl flag.
  it = styles.find(CSSPropertyID::kPropertyIDDirection);
  if (it != styles.end()) {
    SetDirection(it->second);
  }

  bool should_consume_trans_styles_in_advance =
      ShouldConsumeTransitionStylesInAdvance();
  // #1. Consume all transition styles in advance.
  if (should_consume_trans_styles_in_advance) {
    ConsumeTransitionStylesInAdvance(styles);
  }

  styles_.reserve(styles.size() +
                  4);     // extra space for styles from SetAttribute
  styles_.merge(styles);  // record styles, used for worklet
  for (const auto& style : styles) {
    // #2. Skip all transition styles in the later process if they have been
    // consume in advance.
    if (style.first == kPropertyIDFontSize ||
        style.first == kPropertyIDDirection ||
        (should_consume_trans_styles_in_advance &&
         CSSProperty::IsTransitionProps(style.first))) {
      continue;
    }
    // #3. Check every property to determine whether to intercept this update.
    if (css_transition_manager_ && css_transition_manager_->ConsumeCSSProperty(
                                       style.first, style.second)) {
      continue;
    }
    // Since the previous element styles cannot be accessed in element, we
    // need to record some necessary styles which New Animator transition needs,
    // and it needs to be saved before rtl converted logic.
    RecordElementPreviousStyle(style.first, style.second);
    styles_manager_.AdoptStyle(style.first, style.second);
  }
}

bool RadonElement::NeedFastFlushPath(
    const std::pair<CSSPropertyID, tasm::CSSValue>& style) {
  return style.second.IsEmpty() || LayoutProperty::IsLayoutOnly(style.first) ||
         LayoutProperty::IsLayoutWanted(style.first) ||
         style.first == kPropertyIDTransform ||
         style.first == kPropertyIDColor || style.first == kPropertyIDFilter;
}

void RadonElement::ConsumeTransitionStylesInAdvanceInternal(
    CSSPropertyID css_id, const tasm::CSSValue& value) {
  // record styles, used for worklet
  styles_[css_id] = value;
  styles_manager_.AdoptStyle(css_id, value);
}

void RadonElement::ResetTransitionStylesInAdvanceInternal(
    CSSPropertyID css_id) {
  // record styles, used for worklet
  styles_.erase(css_id);
  StylesManager().AdoptStyle(css_id, CSSValue::Empty());
}

bool RadonElement::ResolveStyleValue(CSSPropertyID id,
                                     const tasm::CSSValue& value,
                                     bool force_update) {
  bool resolve_success = false;
  if (computed_css_style()->SetValue(id, value) || force_update) {
    // The props of transition and keyframe no need to be pushed to bundle here.
    // Those props will be pushed to bundle separately later.
    if (!(CheckTransitionProps(id) || CheckKeyframeProps(id))) {
      PushToBundle(id);
    }
    resolve_success = true;
  }

  return resolve_success;
}

void RadonElement::OnPatchFinish(std::shared_ptr<PipelineOptions>& option) {
  element_manager_->OnPatchFinish(option);
}

void RadonElement::FlushAnimatedStyleInternal(tasm::CSSPropertyID id,
                                              const tasm::CSSValue& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_ELEMENT_FLUSH_ANIMATED_STYLE);
  styles_manager_.AdoptStyle(id, value);
}

CSSFragment* RadonElement::GetRelatedCSSFragment() {
  CSSFragment* style_sheet = nullptr;
  if (!data_model_ || !data_model_->radon_node_ptr()) {
    return style_sheet;
  }
  if (data_model_->radon_node_ptr()->GetRemoveCSSScopeEnabled()) {
    style_sheet = data_model_->radon_node_ptr()->GetPageStyleSheet();
  } else {
    style_sheet = data_model_->radon_node_ptr()->ParentStyleSheet();
  }
  return style_sheet;
}

int32_t RadonElement::GetCSSID() const {
  if (data_model() == nullptr) {
    return kInvalidCssId;
  }
  auto* node = data_model()->radon_node_ptr();
  if (node != nullptr && node->component() != nullptr) {
    return node->component()->tid();
  } else {
    return kInvalidCssId;
  }
}

size_t RadonElement::CountInlineStyles() {
  return data_model()->inline_styles().size();
}

void RadonElement::MergeInlineStyles(StyleMap& new_styles) {
  if (data_model()->IsSSRAttrHolder()) {
    for (const auto& style : data_model()->inline_styles()) {
      new_styles.insert_if_absent(style.first, style.second);
    }
  } else {
    new_styles.merge(data_model()->inline_styles());
  }
}

bool RadonElement::WillResolveStyle(StyleMap& merged_styles) {
  return GetTag() != kElementRawTextTag && merged_styles.empty();
}

const base::String& RadonElement::GetPlatformNodeTag() const {
  if (is_list()) {
    return platform_node_tag_;
  } else {
    return tag_;
  }
}

void RadonElement::UpdatePlatformNodeTag() {
  if (is_list()) {
    const auto& attrs = data_model_->attributes();
    const auto& iterator = attrs.find(BASE_STATIC_STRING(list::kCustomLisName));
    if (attrs.end() != iterator) {
      platform_node_tag_ = iterator->second.String();

      // add feature count for custom-list or list-container
      if (platform_node_tag_.IsEqual(
              BASE_STATIC_STRING(list::kListContainer))) {
        // list-container
        tasm::report::FeatureCounter::Instance()->Count(
            tasm::report::LynxFeature::CPP_LIST_CONTAINER);
      } else if (!platform_node_tag_.IsEqual(list::kList)) {
        // custom-list
        tasm::report::FeatureCounter::Instance()->Count(
            tasm::report::LynxFeature::CPP_CUSTOM_LIST);
      }

      return;
    }
    if (element_manager_->GetEnableNativeListFromPageConfig()) {
      // If not set "custom-list-name" and enableNativeList from page config is
      // true, we modify platform_node_tag_ to "list-container"
      platform_node_tag_ = BASE_STATIC_STRING(list::kListContainer);
      tasm::report::FeatureCounter::Instance()->Count(
          tasm::report::LynxFeature::CPP_LIST_CONTAINER);
    }
  }
}

bool RadonElement::CanBeLayoutOnly() const {
  return element_manager()->GetEnableLayoutOnly() && has_layout_only_props_ &&
         overflow_ == OVERFLOW_XY &&
         (!is_component() || enable_component_layout_only_);
}

void RadonElement::SetPlaceHolderStyles(const PseudoPlaceHolderStyles& styles) {
  report::GlobalFeatureCounter::Count(
      report::LynxFeature::CPP_ENABLE_PLACE_HOLDER_STYLE,
      element_manager_->GetInstanceId());
  styles_manager_.SetPlaceHolderStyle(styles);
}

void RadonElement::PreparePropsBundleForDynamicCSS() {
  if (!styles_manager_.UpdateWithParentStatus(
          static_cast<RadonElement*>(parent()))) {
    return;
  }
  for (auto& child : children_) {
    static_cast<RadonElement*>(child)->PreparePropsBundleForDynamicCSS();
  }
}

}  // namespace tasm
}  // namespace lynx
