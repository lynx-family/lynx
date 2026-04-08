// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/no_destructor.h"
#include "base/include/path_utils.h"
#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/animation_delegate.h"
#include "core/renderer/css/computed_css_style_css_text_helper.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element_context_delegate.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/dom/element_property.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"
#include "core/renderer/dom/fiber/platform_layout_function_wrapper.h"
#include "core/renderer/dom/fiber/pseudo_element.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/layout_bundle.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/js/bindings/java_script_element.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/feature_count/global_feature_counter.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

const uint32_t Element::kDirtyCreated = 0x01 << 0;
const uint32_t Element::kDirtyTree = 0x01 << 1;
const uint32_t Element::kDirtyStyle = 0x01 << 2;
const uint32_t Element::kDirtyAttr = 0x01 << 3;
const uint32_t Element::kDirtyForceUpdate = 0x01 << 4;
const uint32_t Element::kDirtyEvent = 0x01 << 5;
const uint32_t Element::kDirtyReAttachContainer = 0x01 << 6;
const uint32_t Element::kDirtyPropagateInherited = 0x01 << 7;
const uint32_t Element::kDirtyDataset = 0x01 << 8;
const uint32_t Element::kDirtyGesture = 0x01 << 9;
const uint32_t Element::kDirtyFontSize = 0x01 << 11;
const uint32_t Element::kDirtyRefreshCSSVariables = 0x01 << 12;
const uint32_t Element::kDirtyStyleObjects = 0x01 << 13;

namespace {
constexpr std::array<starlight::Direction,
                     starlight::Direction::kDirectionCount>
    kDefaultDirectionValueOrder =
        std::array<starlight::Direction, starlight::Direction::kDirectionCount>{
            starlight::Direction::kLeft, starlight::Direction::kTop,
            starlight::Direction::kRight, starlight::Direction::kBottom};
starlight::DirectionValue<float> ConvertToDirectionValue(
    const std::array<float, starlight::Direction::kDirectionCount>& values) {
  std::array<float, starlight::Direction::kDirectionCount> result_values = {
      0.0f, 0.0f, 0.0f, 0.0f};

  for (size_t i = 0; i < starlight::Direction::kDirectionCount; ++i) {
    result_values[kDefaultDirectionValueOrder[i]] = values[i];
  }

  return starlight::DirectionValue<float>(result_values);
}
}  // namespace

#define FOREACH_EXTENDED_LAYOUT_ONLY_PROPERTY(V) \
  V(Direction, true)                             \
  V(TextAlign, true)

InspectorAttribute::InspectorAttribute()
    : style_root_(nullptr), doc_(nullptr), style_value_(nullptr) {}

InspectorAttribute::~InspectorAttribute() {
  if (doc_) {
    doc_->set_parent(nullptr);
  }
  if (style_value_) {
    style_value_->set_parent(nullptr);
  }
}

Element::Element(const base::String& tag, ElementManager* manager,
                 uint32_t node_index)
    : tag_(tag),
      id_(manager ? manager->GenerateElementID() : -1),
      node_index_(node_index),
      element_manager_(manager) {
  if (manager == nullptr) {
    return;
  }
  target_type_ = EventTarget::EventTargetType::kElement;
  arch_type_ = manager->GetEnableFiberArch() ? FiberArch : RadonArch;
  enable_new_animator_ = IsFiberArch()
                             ? manager->GetEnableNewAnimatorForFiber()
                             : manager->GetEnableNewAnimatorForRadon();
  manager->node_manager()->Record(id_, this);

  catalyzer_ = manager->catalyzer();
  config_flatten_ = manager->GetPageFlatten();

  const auto& env_config = manager->GetLynxEnvConfig();

  platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
      *manager->platform_computed_css());
  platform_css_style_->SetEnableZIndex(manager->GetEnableZIndex());
  platform_css_style_->SetScreenWidth(env_config.ScreenWidth());
  platform_css_style_->SetViewportHeight(env_config.ViewportHeight());
  platform_css_style_->SetViewportWidth(env_config.ViewportWidth());
  platform_css_style_->SetCssAlignLegacyWithW3c(
      manager->GetLayoutConfigs().css_align_with_legacy_w3c_);
  platform_css_style_->SetFontScaleOnlyEffectiveOnSp(
      env_config.FontScaleSpOnly());
  platform_css_style_->SetFontSize(env_config.DefaultFontSize(),
                                   env_config.DefaultFontSize());
  platform_css_style_->SetLayoutUnit(env_config.PhysicalPixelsPerLayoutUnit(),
                                     env_config.LayoutsUnitPerPx());
  if (IsRadonArch()) {
    enable_extended_layout_only_opt_ =
        manager->GetEnableExtendedLayoutOnlyOpt();
    enable_component_layout_only_ = manager->GetEnableComponentLayoutOnly();
  }

  record_parent_font_size_ = manager->GetLynxEnvConfig().PageDefaultFontSize();

  if (EnableFragmentLayerRender()) {
    element_container_ = std::make_unique<Fragment>(this);
  } else {
    element_container_ = std::make_unique<ElementContainer>(this);
  }
}

Element::~Element() = default;

// The copy constructor of the element is now only used for copying fiber
// elements. If you want to use it to copy radon elements, you need to check the
// copy constructor to determine if there are other additional member variables
// that need to be copied.
Element::Element(const Element& element, bool clone_resolved_props)
    : tag_(element.tag_),
      id_(element.id_),
      node_index_(element.node_index_),
      arch_type_(element.arch_type_),
      is_fixed_(element.is_fixed_),
      is_sticky_(element.is_sticky_),
      // Because is_fixed_ is false by default, if is_fixed_ is true, it means
      // that this element has the position:fixed attribute. In this case,
      // fixed_changed_ should be true, so that the final UI hierarchy can be
      // correct.
      fixed_changed_(element.is_fixed_),
      has_event_listener_(element.has_event_listener_),
      has_non_flatten_attrs_(element.has_non_flatten_attrs_),
      is_virtual_(element.is_virtual_),
      subtree_need_update_(element.subtree_need_update_),
      frame_changed_(element.frame_changed_),
      is_layout_only_(element.is_layout_only_),
      is_text_(element.is_text_),
      is_inline_element_(element.is_inline_element_),
      is_list_item_(element.is_list_item_),
      has_placeholder_(element.has_placeholder_),
      trigger_global_event_(element.trigger_global_event_),
      enable_new_animator_(element.enable_new_animator_),
      has_layout_only_props_(element.has_layout_only_props_),
      can_has_layout_only_children_(element.can_has_layout_only_children_),
      need_process_direction_(element.need_process_direction_),
      enable_extended_layout_only_opt_(
          element.enable_extended_layout_only_opt_),
      enable_component_layout_only_(element.enable_component_layout_only_),
      width_(element.width_),
      height_(element.height_),
      top_(element.top_),
      left_(element.left_),
      borders_(element.borders_),
      margins_(element.margins_),
      paddings_(element.paddings_),
      sticky_positions_(element.sticky_positions_),
      max_height_(element.max_height_),
      record_parent_font_size_(element.record_parent_font_size_),
      global_bind_target_set_(element.global_bind_target_set_),
      animation_previous_styles_(element.animation_previous_styles_) {
  platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
      *(element.computed_css_style()));
}

void Element::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  element_manager_ = manager;
  arch_type_ = manager->GetEnableFiberArch() ? FiberArch : RadonArch;
  if (style_manager) {
    style_manager->SetEnableCSSLazyImport(
        element_manager_->GetEnableCSSLazyImport());
  }
  config_flatten_ = manager->GetPageFlatten();
  catalyzer_ = manager->catalyzer();

  if (keep_element_id) {
    manager->ReuseElementID(id_);
  } else {
    id_ = manager->GenerateElementID();
  }
  manager->node_manager()->Record(id_, this);

  arch_type_ = manager->GetEnableFiberArch() ? FiberArch : RadonArch;
  enable_new_animator_ = IsFiberArch()
                             ? manager->GetEnableNewAnimatorForFiber()
                             : manager->GetEnableNewAnimatorForRadon();

  if (IsRadonArch()) {
    enable_extended_layout_only_opt_ =
        manager->GetEnableExtendedLayoutOnlyOpt();
    enable_component_layout_only_ = manager->GetEnableComponentLayoutOnly();
  }

  if (EnableFragmentLayerRender()) {
    element_container_ = std::make_unique<Fragment>(this);
  } else {
    element_container_ = std::make_unique<ElementContainer>(this);
  }
}

void Element::PushStyleToBundle() {
  if (EnableFragmentLayerRender() && !IsShadowNodeCustom()) {
    // TODO(renzhongyue): After EnableFragmentLayerRender(), style changes do
    // not need to be written to the PropBundle. computed_css_style() remains
    // dirty, and the Fragment determines whether to repaint based on whether
    // computed_css_style() is dirty.
    return;
  }

  if (computed_css_style() && computed_css_style()->IsDirty()) {
    PreparePropBundleIfNeed();
    PropBundleStyleWriter::PushStyleToBundle(prop_bundle_.get(),
                                             computed_css_style());
  }
}

std::vector<float> Element::ScrollBy(float width, float height) {
  return catalyzer_->ScrollBy(impl_id(), width, height);
}

// Sets the state of a gesture detector for the Element.
// Parameters:
//   gesture_id: The ID of the gesture to set the state for.
//   state: The state to set for the gesture  (state: 1 - active, 2 - fail, 3 -
//   end)
void Element::SetGestureDetectorState(int32_t gesture_id, int32_t state) {
  catalyzer_->SetGestureDetectorState(impl_id(), gesture_id, state);
}

void Element::ConsumeGesture(int32_t gesture_id, const lepus::Value& params) {
  catalyzer_->ConsumeGesture(impl_id(), gesture_id,
                             pub::ValueImplLepus(params));
}

// Returns the GestureMap associated with the Element, if available.
// If the data model is available, it returns the map of gesture detectors.
// If the data model is not available, it returns an empty GestureMap.
// Returns:
//   Reference to the GestureMap associated with the Element.
const GestureMap& Element::gesture_map() {
  if (data_model()) {
    return data_model()->gesture_detectors();
  }
  return AttributeHolder::DefaultEmptyGestureMap();
}

// Sets a GestureDetector for the Element.
// This prepares the property bundle and sets the GestureDetector.
// Parameters:
//   key: The identifier for the GestureDetector.
//   detector: Pointer to the GestureDetector to set.
void Element::SetGestureDetector(const uint32_t key,
                                 GestureDetectorImpl* detector) {
  // Prepare the property bundle if needed before setting the GestureDetector.
  PreparePropBundleIfNeed();
  prop_bundle_->SetGestureDetector(*detector);
}

std::vector<float> Element::GetRectToLynxView() {
  return catalyzer_->GetRectToLynxView(this);
}

void Element::set_will_destroy(bool destroy) {
  will_destroy_ = destroy;
  if (destroy && data_model_ && element_manager_ &&
      element_manager_->FixListCallbackLeakFlag()) {
    data_model_->Destroy();
  }
}

void Element::Invoke(
    const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  return catalyzer_->Invoke(impl_id(), method, params, callback);
}

const EventMap& Element::event_map() const {
  if (data_model()) {
    return data_model()->static_events();
  }
  static base::NoDestructor<EventMap> kEmptyEventMap;
  return *kEmptyEventMap;
}

const EventMap& Element::lepus_event_map() {
  if (data_model()) {
    return data_model()->lepus_events();
  }
  static base::NoDestructor<EventMap> kEmptyLepusEventMap;
  return *kEmptyLepusEventMap;
}

const EventMap& Element::global_bind_event_map() {
  if (data_model()) {
    return data_model()->global_bind_events();
  }
  static base::NoDestructor<EventMap> kEmptyGlobalBindEventMap;
  return *kEmptyGlobalBindEventMap.get();
}

void Element::UpdateLayout(float left, float top, float width, float height,
                           const std::array<float, 4>& paddings,
                           const std::array<float, 4>& margins,
                           const std::array<float, 4>& borders,
                           const std::array<float, 4>* sticky_positions,
                           float max_height) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_UPDATE_LAYOUT);
  // TODO: only leaf node need to update border padding
  frame_changed_ = true;
  top_ = top;
  left_ = left;
  width_ = width;
  height_ = height;
  paddings_ = paddings;
  margins_ = margins;
  borders_ = borders;
  if (sticky_positions != nullptr) {
    *sticky_positions_ = *sticky_positions;
  }
  MarkSubtreeNeedUpdate();
  NotifyElementSizeUpdated();
}

void Element::UpdateLayout(float left, float top) {
  top_ = top;
  left_ = left;
}

bool Element::ConsumeTransitionStylesInAdvance(const StyleMap& styles,
                                               bool force_reset) {
  bool has_transition_prop = false;
  for (unsigned int id =
           static_cast<unsigned int>(CSSPropertyID::kPropertyIDTransition);
       id <= static_cast<unsigned int>(
                 CSSPropertyID::kPropertyIDTransitionTimingFunction);
       ++id) {
    auto style = styles.find(static_cast<CSSPropertyID>(id));
    if (style == styles.end()) {
      continue;
    }
    has_transition_prop = true;
    if (force_reset) {
      ResetTransitionStylesInAdvanceInternal(style->first);
    } else {
      ConsumeTransitionStylesInAdvanceInternal(style->first, style->second);
    }
  }
  SetDataToNativeTransitionAnimator();
  return has_transition_prop;
}

void Element::SetStyleInternal(CSSPropertyID css_id,
                               const tasm::CSSValue& value) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, ELEMENT_SET_STYLE_INTERNAL,
      [css_id](lynx::perfetto::EventContext ctx) {
        auto* css_info = ctx.event()->add_debug_annotations();
        css_info->set_name("PropertyName");
        css_info->set_string_value(CSSProperty::GetPropertyNameCStr(css_id));
      });
  CheckDynamicUnit(css_id, value, false);

  // font-size has be handled, just ignore it.
  if (css_id == kPropertyIDFontSize) {
    return;
  }

  // check layout only related styles
  bool is_layout_only = LayoutProperty::IsLayoutOnly(css_id);

  bool need_layout = is_layout_only || LayoutProperty::IsLayoutWanted(css_id);
  if (need_layout) {
    // Check fixed&sticky before layout only
    CheckFixedSticky(css_id, value);

    UpdateLayoutNodeStyle(css_id, value);

    if (element_manager_->GetEnableDumpElementTree()) {
      (*layout_styles_)[css_id] = value;
    }
  }

  if (is_layout_only) {
    if (EnableLayoutInElementMode() &&
        computed_css_style()->SetValue(css_id, value)) {
      RequestLayout();
    }
    element_container()->InvalidateForRedraw();
    return;
  }

  if (css_id == kPropertyIDOpacity || css_id == kPropertyIDTransform ||
      css_id == kPropertyIDVisibility) {
    element_container()->InvalidateForSubtreeProperty();
  } else {
    element_container()->InvalidateForRedraw();
  }
  // resolve style and push to prop_bundle
  ResolveStyleValue(css_id, value);

  // overflow is special: if overflow is visible can be treated as layout only
  // prop!
  if (css_id == kPropertyIDOverflow || css_id == kPropertyIDOverflowX ||
      css_id == kPropertyIDOverflowY) {
    // take care: overflow:visible is allowed to be layout only
    if (!computed_css_style()->IsOverflowXY()) {
      has_layout_only_props_ = false;
    }
  } else {
    // if the style is not layout only, it shall be resolved to prop_bundle
    // such style is not layout only
    if (!enable_extended_layout_only_opt_ ||
        !IsExtendedLayoutOnlyProps(css_id)) {
      // currently, "text-align,direction" shall not make the layout only
      // optimization invalid!
      has_layout_only_props_ = false;
    }

    // do special check for transition, keyframe, z-index,etc.
    if (!(CheckTransitionProps(css_id) || CheckKeyframeProps(css_id))) {
      // Check flatten-related CSS props eagerly in C++.
      // This is shared by current platform-rendering flatten logic, and is
      // cheaper than deferring the same decision to platform-specific code.
      CheckHasNonFlattenCSSProps(css_id);
    }
  }
}

bool Element::ResolveStyleValue(CSSPropertyID id, const tasm::CSSValue& value) {
  bool resolve_success = false;
  if (computed_css_style()->SetValue(id, value)) {
    // The properties of transition and keyframe no need to be pushed to bundle
    // separately here. Those properties will be pushed to bundle together
    // later.
    CheckTransitionProps(id);
    CheckKeyframeProps(id);
    resolve_success = true;
  }

  if (EnableLayoutInElementMode()) {
    if (LayoutProperty::IsLayoutWanted(id)) {
      MarkLayoutDirtyLite();
    }
  }

  return resolve_success;
}

bool Element::HasUIPrimitive() const {
  return element_container()->HasUIPrimitive();
}

void Element::CheckHasInlineContainer(Element* parent) {
  if (parent) {
    allow_layoutnode_inline_ = parent->IsShadowNodeCustom();
  }
  if (parent && (parent->is_text_ ||
                 (parent->is_inline_element_ && !parent->is_view()))) {
    is_inline_element_ = true;
    has_layout_only_props_ = false;
  }
}

bool Element::EnableLayoutInElementMode() const {
  return element_manager() && element_manager()->IsLayoutInElementModeOn();
}

void Element::EnsureLayoutBundle() {
  if (EnableLayoutInElementMode()) {
    return;
  }

  if (layout_bundle_ == nullptr) {
    layout_bundle_ = std::make_unique<LayoutBundle>();
  }
}

void Element::InitLayoutBundle() {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->tag = tag_;
  layout_bundle_->is_create_bundle = true;
}

void Element::UpdateTagToLayoutBundle() {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->tag = tag_;
}

bool Element::UsingTextService() const {
  return element_manager() && element_manager()->IsUsingTextService();
}

bool Element::EnableFragmentLayerRender() const {
  return element_manager() && element_manager()->IsFragmentLayerRenderModeOn();
}

void Element::ResetStyleInternal(CSSPropertyID css_id) {
  // Since the previous element styles cannot be accessed in element, we
  // need to record some necessary styles which New Animator transition needs.
  // TODO(wujintian): We only need to record layout-only properties, while other
  // properties can be accessed through ComputedCSSStyle.

  WillResetCSSValue(css_id);

  ResetCSSValue(css_id);
}

bool Element::ResetCSSValue(CSSPropertyID css_id) {
  CheckDynamicUnit(css_id, CSSValue(), true);

  if (css_id == kPropertyIDFontSize) {
    // font-size has been reset to default value in WillResetCSSValue
    return false;
  }

  bool is_layout_only = LayoutProperty::IsLayoutOnly(css_id);
  bool need_layout = is_layout_only || LayoutProperty::IsLayoutWanted(css_id);
  bool processed = false;
  if (need_layout) {
    ResetLayoutNodeStyle(css_id);
    if (element_manager_->GetEnableDumpElementTree()) {
      if (layout_styles_.has_value()) {
        layout_styles_->erase(css_id);
      }
    }
    if (is_layout_only && EnableLayoutInElementMode() &&
        computed_css_style()->ResetValue(css_id)) {
      RequestLayout();
      processed = true;
    }
  }
  if (css_id == kPropertyIDPosition) {
    if (is_fixed_) {
      fixed_changed_ = true;
    }
    is_sticky_ = is_fixed_ = false;
  }
  if (is_layout_only) {
    return processed;
  }
  has_layout_only_props_ = false;
  processed = computed_css_style()->ResetValue(css_id);

  // The properties of transition and keyframe no need to be pushed to bundle
  // separately here. Those properties will be pushed to bundle together
  // later.
  CheckTransitionProps(css_id);
  CheckKeyframeProps(css_id);

  return processed;
}

// If the new animator is activated and this element has been created before,
// we need to reset the transition styles in advance. Additionally, the
// transition manager should verify each property to decide whether to
// intercept the reset. Therefore, we break down the operations related to the
// transition reset process into three steps:
// 1. We check whether we need to reset transition styles in advance.
// 2. If these styles have been reset beforehand, we can skip the transition
// styles in the later steps.
// 3. We review each property to determine whether the reset should be
// intercepted.
void Element::ResetStyle(const base::Vector<CSSPropertyID>& css_names) {
  if (css_names.empty()) {
    return;
  }

  bool should_consume_trans_styles_in_advance =
      ShouldConsumeTransitionStylesInAdvance();
  // #1. Check whether we need to reset transition styles in advance.
  if (should_consume_trans_styles_in_advance) {
    ResetTransitionStylesInAdvance(css_names);
  }

  for (auto& css_id : css_names) {
    // TODO: zhixuan
    if (css_id == kPropertyIDFontSize) {
      element_manager()->SetNeedsLayout();
      continue;
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
    if (css_transition_manager_ &&
        css_transition_manager_->ConsumeCSSProperty(css_id, CSSValue())) {
      continue;
    }
    // Since the previous element styles cannot be accessed in element, we
    // need to record some necessary styles which New Animator transition needs,
    // and it needs to be saved before rtl converted logic.
    ResetElementPreviousStyle(css_id);
    if (element_manager() && (LayoutProperty::IsLayoutOnly(css_id) ||
                              LayoutProperty::IsLayoutWanted(css_id))) {
      element_manager()->SetNeedsLayout();
    }
    ResetStyleInternal(DynamicCSSStylesManager::ResolveDirectionAwarePropertyID(
        css_id, computed_css_style()->GetDirection()));
  }
}

bool Element::ResetTransitionStylesInAdvance(
    const base::Vector<CSSPropertyID>& css_names) {
  bool has_transition_prop = false;
  for (auto& css_id : css_names) {
    if (CSSProperty::IsTransitionProps(css_id)) {
      ResetTransitionStylesInAdvanceInternal(css_id);
      has_transition_prop = true;
    }
  }
  SetDataToNativeTransitionAnimator();
  return has_transition_prop;
}

void Element::ResetAttribute(const base::String& key) {
  CheckGlobalBindTarget(key);
  has_layout_only_props_ = false;

  PreparePropBundleIfNeed();
  prop_bundle_->SetNullProps(key.c_str());

  if (EnableFragmentLayerRender()) {
    if (auto name = PlatformEventPropNameFromString(key.str());
        name != PlatformEventPropName::kUnknown) {
      if (auto fragment = fragment_impl()) {
        fragment->SetEventProp(name, lepus::Value(0));
      }
    }
  }
}

void Element::WillConsumeAttribute(const base::String& key,
                                   const lepus::Value& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_CONSUME_ATTRIBUTE);

  // Flatten realted.
  // TODO(songshourui.null): Currently, the Flatten information is only consumed
  // by Android native rendering. Theoretically, this logic could be executed
  // only on Android native rendering. However, for the sake of unit testing, we
  // are not optimizing it for now. In the long term, it should be executed as
  // needed.
  CheckFlattenRelatedProp(key, value);

  // Styling related.
  CheckHasPlaceholder(key, value);
  CheckHasTextSelection(key, value);

  // Event related.
  CheckTriggerGlobalEvent(key, value);
  CheckGlobalBindTarget(key, value);

  // Animation related.
  CheckNewAnimatorAttr(key, value);

  // Timing related.
  CheckTimingAttribute(key, value);
}

void Element::MarkStyleDirty(bool recursive) {
  MarkDirty(kDirtyStyle);
  if (recursive) {
    for (const auto& child : scoped_children_) {
      static_cast<FiberElement*>(child.get())->MarkStyleDirty(recursive);
    }
  }
}

// TODO(wujintian) : Perhaps we can provide an rvalue version of the API to
// achieve better performance. However, this would result in the need to
// maintain two versions of the code: one for lvalues and one for rvalues.
void Element::SetClass(const base::String& clazz) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_CLASS);
  data_model_->SetClass(clazz);
  MarkStyleDirty(NeedForceClassChangeTransmit());
}

void Element::SetClasses(ClassList&& classes) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_CLASSES);
  data_model_->SetClasses(std::move(classes));
  MarkStyleDirty(NeedForceClassChangeTransmit());

  // clear ssr parsed style
  if (has_extreme_parsed_styles_) {
    extreme_parsed_styles_.reset();
    has_extreme_parsed_styles_ = false;
  }
}

void Element::RemoveAllClass() {
  data_model_->RemoveAllClass();
  MarkStyleDirty(NeedForceClassChangeTransmit());
}

void Element::SetAttribute(const base::String& key, const lepus::Value& value,
                           bool need_update_data_model) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_ATTRIBUTE);

  CheckClassChangeTransmitAttribute(key, value);

  if (!value.IsEmpty()) {
    updated_attr_map_[key] = value;
    // In the RadonNode-driven Fiber architecture, the attribute
    // used for diffing is already stored in the data_model,
    //  so there is no need to update this attribute in the data_model again.
    if (need_update_data_model) {
      data_model_->SetStaticAttribute(key, value);
    }
  } else {
    reset_attr_vec_->emplace_back(key);
    if (need_update_data_model) {
      data_model_->RemoveAttribute(key);
    }
  }
  MarkDirty(kDirtyAttr);
}

void Element::SetBuiltinAttribute(ElementBuiltInAttributeEnum key,
                                  const lepus::Value& value) {
  bool key_is_legal = true;
  switch (key) {
    case ElementBuiltInAttributeEnum::NODE_INDEX:
      node_index_ = static_cast<uint32_t>(value.Number());
      break;
    case ElementBuiltInAttributeEnum::CSS_ID:
      css_id_ = static_cast<int32_t>(value.Number());
      break;
    case ElementBuiltInAttributeEnum::DIRTY_ID:
      MarkPartElement(value.String());
      break;
    case ElementBuiltInAttributeEnum::CONFIG:
      if (value.IsTable()) {
        config_ = value.Table();
      } else if (value.IsJSTable()) {
        config_ = value.ToLepusValue().Table();
      } else {
        DCHECK(false);
      }
      break;
    case ElementBuiltInAttributeEnum::IS_TEMPLATE_PART:
      if (value.Bool()) {
        MarkTemplateElement();
      }
      break;
    default:
      key_is_legal = false;
      break;
  }
  if (key_is_legal) {
    builtin_attr_map_->try_emplace(static_cast<uint32_t>(key), value);
  }
}

void Element::SetIdSelector(const base::String& idSelector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_ID_SELECTOR);
  if (element_manager() && element_manager()->GetEnableStandardCSSSelector()) {
    if (element_manager()->CSSFragmentParsingOnTASMWorkerMTSRender()) {
      element_manager()->GetTasmWorkerTaskRunner()->PostTask(
          [this, old_id = data_model_->idSelector().str(),
           new_id = idSelector.str()]() {
            CheckHasInvalidationForId(old_id, new_id);
          });
    } else {
      CheckHasInvalidationForId(data_model_->idSelector().str(),
                                idSelector.str());
    }
  }

  updated_attr_map_[BASE_STATIC_STRING(AttributeHolder::kIdSelectorAttrName)]
      .SetString(idSelector);
  data_model_->SetIdSelector(idSelector);
  if (element_manager() && element_manager()->EnableSimpleStyle()) {
    MarkDirty(kDirtyAttr);
  } else {
    MarkDirty(kDirtyStyle | kDirtyAttr);
  }
}

void Element::SetDataSet(const tasm::DataMap& data) {
  PreparePropBundleIfNeed();
  lepus::Value datas_val(lepus::Dictionary::Create());
  for (const auto& pair : data) {
    datas_val.SetProperty(pair.first, pair.second);
  }
  prop_bundle_->SetProps("dataset", pub::ValueImplLepus(datas_val));
}

void Element::AddDataset(const base::String& key, const lepus::Value& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_ADD_DATA_SET);

  data_model_->SetDataSet(key, value);
  MarkDirty(kDirtyDataset);
}

void Element::SetDataset(const lepus::Value& data_set) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_DATA_SET);

  data_model_->SetDataSet(data_set);
  MarkDirty(kDirtyDataset);
}

void Element::SetJSEventHandler(const base::String& name,
                                const base::String& type,
                                const base::String& callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_JS_EVENT_HANDLER);

  data_model_->SetStaticEvent(type, name, callback);
  MarkDirty(kDirtyEvent);
}

void Element::SetLepusEventHandler(const base::String& name,
                                   const base::String& type,
                                   const lepus::Value& script,
                                   const lepus::Value& callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_LEPUS_EVENT_HANDLER);

  data_model_->SetLepusEvent(type, name, script, callback);
  MarkDirty(kDirtyEvent);
}

void Element::SetWorkletEventHandler(const base::String& name,
                                     const base::String& type,
                                     const lepus::Value& worklet_info,
                                     runtime::MTSRuntime* ctx) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_WORKLET_EVENT_HANDLER);

  data_model_->SetWorkletEvent(type, name, worklet_info, ctx);
  MarkDirty(kDirtyEvent);
}

void Element::RemoveEvent(const base::String& name, const base::String& type) {
  data_model_->RemoveEvent(name, type);
  MarkDirty(kDirtyEvent);
}

void Element::RemoveAllEvents() {
  data_model_->RemoveAllEvents();
  MarkDirty(kDirtyEvent);
}

void Element::AddConfig(const base::String& key, const lepus::Value& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_ADD_CONFIG);
  if (config_ == nullptr) {
    config_ = lepus::Dictionary::Create();
  } else if (config_->IsConst()) {
    config_ = lepus::Value::ShallowCopy(lepus::Value(config_)).Table();
  }
  config_->SetValue(key, value);
}

void Element::SetConfig(const lepus::Value& config) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_CONFIG);

  // To improve performance, ensure that the isObject check is performed before
  // calling SetConfig, and the check and LOGW in SetConfig are no longer
  // performed.
  if (config.IsTable()) {
    config_ = config.Table();
  } else if (config.IsJSTable()) {
    config_ = config.ToLepusValue().Table();
  } else {
    DCHECK(false);
  }
}

const lepus::Value Element::config() const {
  return lepus::Value(
      config_ ? config_
              : fml::RefPtr<lepus::Dictionary>(lepus::Value::DummyTable()));
}

void Element::SetKeyframesByNames(const lepus::Value& names,
                                  const CSSKeyframesTokenMap& keyframes,
                                  bool force_flush) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_SET_KEYFRAMES_BY_NAMES);
  auto lepus_keyframes = ResolveCSSKeyframesByNames(
      names, keyframes, computed_css_style()->GetMeasureContext(),
      element_manager()->GetCSSParserConfigs(), force_flush);
  if (!lepus_keyframes.IsTable() || lepus_keyframes.Table()->size() == 0) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_PUSH_KEYFRAMES_TO_BUNDLE);
  auto bundle = element_manager()->GetPropBundleCreator()->CreatePropBundle();
  bundle->SetProps("keyframes", pub::ValueImplLepus(lepus_keyframes));

  element_container()->SetKeyframes(std::move(bundle));
}

lepus::Value Element::ResolveCSSKeyframesByNames(
    const lepus::Value& names, const tasm::CSSKeyframesTokenMap& frames,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs, bool force_flush) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_RESOLVE_KEYFRAMES_BY_NAMES);
  DCHECK(names.IsString() || names.IsArray());
  auto dict = lepus::Dictionary::Create();
  ForEachLepusValue(
      names, [&dict, &context, &configs, &frames, this, &force_flush](
                 const lepus::Value& key, const lepus::Value& val) {
        if (val.IsString()) {
          auto val_str = val.String();
          auto keyframes_token_iter = frames.find(val_str);
          if (keyframes_token_iter != frames.end() &&
              keyframes_token_iter->second) {
            auto unique_id = "__lynx_unique_" + std::to_string(GetCSSID()) +
                             "_" + val_str.str();
            if (!element_manager()->CheckResolvedKeyframes(unique_id) ||
                force_flush) {
              dict->SetValue(
                  val_str,
                  starlight::CSSStyleUtils::ResolveCSSKeyframesToken(
                      keyframes_token_iter->second.get(), context, configs));
              element_manager()->SetResolvedKeyframes(unique_id);
            }
          }
        }
      });
  return lepus::Value(std::move(dict));
}

void Element::SetFontFaces(const CSSFontFaceRuleMap& fontFaces) {
  element_manager_->SetFontFaces(fontFaces);
}

void Element::SetProp(const char* key, const lepus::Value& value) {
  PreparePropBundleIfNeed();
  prop_bundle_->SetProps(key, pub::ValueImplLepus(value));
}

// TODO: just so easy?
void Element::SetEventHandler(const base::String& name, EventHandler* handler) {
  PreparePropBundleIfNeed();
  prop_bundle_->SetEventHandler(handler->ToPubLepusValue());
  if (handler->name().IsEquals("attach") ||
      handler->name().IsEquals("detach")) {
    has_event_listener_ = true;
  }
  if (EnableFragmentLayerRender()) {
    // TODO(hexionghui): This also needs to be set when the event is cleared.
    if (auto fragment = fragment_impl();
        fragment && !handler->IsGlobalBindEvent()) {
      auto event_name = PlatformEventNameFromString(name.str());
      if (event_name != PlatformEventName::kUnknown) {
        fragment->AddEventName(event_name);
      }
    }
  }
  has_layout_only_props_ = false;
}

void Element::ResetEventHandlers() {
  if (prop_bundle_ != nullptr) {
    prop_bundle_->ResetEventHandler();
  }
  has_event_listener_ = false;
}

ElementContainer* Element::element_container_impl() {
  return static_cast<ElementContainer*>(element_container());
}
Fragment* Element::fragment_impl() {
  return static_cast<Fragment*>(element_container());
}

void Element::CreateElementContainer(bool platform_is_flatten) {
  element_container()->CreatePaintingNode(platform_is_flatten, prop_bundle_);

  element_manager_->IncreaseElementCount();
  if (IsLayoutOnly()) {
    element_manager_->IncreaseLayoutOnlyElementCount();
  }
}

void Element::UpdateElement() {
  if (!IsLayoutOnly()) {
    element_container()->UpdatePaintingNode(TendToFlatten(), prop_bundle_);
  } else if (!CanBeLayoutOnly()) {
    // Is layout only and can not be layout only
    TransitionToNativeView();
  }
  element_container()->StyleChanged();
}

void Element::OnNodeReady() {
  if (element_container() == nullptr) {
    return;
  }
  element_container()->OnNodeReady();
}

void Element::onNodeReload() {
  if (element_container() == nullptr) {
    return;
  }
  element_container()->OnNodeReload();
}

void Element::Animate(const lepus::Value& args,
                      std::shared_ptr<PipelineOptions>& pipeline_option) {
  // animate's args: operation, js_name, keyframes, animation_data.
  if (!args.IsArrayOrJSArray()) {
    LOGE("Element::Animate's para must be array");
    return;
  }
  if (args.GetLength() < 2) {
    LOGE("Element::Animate's para size must >= 2");
    return;
  }
  const auto& op =
      static_cast<runtime::js::JavaScriptElement::AnimationOperation>(
          args.GetProperty(0).Int32());
  StyleMap styles;
  auto& parser_configs = element_manager()->GetCSSParserConfigs();
  switch (op) {
    case runtime::js::JavaScriptElement::AnimationOperation::START: {
      if (args.GetLength() != 4) {
        LOGE("When start Element::Animate, the para size must be 4");
        return;
      }
      lepus::Value lepus_name;
      base::String animate_name;
      const auto& table = args.GetProperty(3).Table();
      // Since autoincrement keys causes the keyframes_map to overflow, we
      // remove them from keyframes_map when the last animation was overwritten.
      if (!will_removed_keyframe_name_.empty()) {
        if (enable_new_animator()) {
          if (keyframes_map_.has_value()) {
            keyframes_map_->erase(will_removed_keyframe_name_);
          }
        } else {
          auto remove_name = lepus::Value(will_removed_keyframe_name_);
          auto bundle =
              element_manager()->GetPropBundleCreator()->CreatePropBundle();
          bundle->SetProps("removeKeyframe", pub::ValueImplLepus(remove_name));
          element_container()->SetKeyframes(std::move(bundle));
        }
        will_removed_keyframe_name_ = base::String();
      }
      BASE_STATIC_STRING_DECL(kName, "name");
      auto iter = table->find(kName);
      if (iter == table->end()) {
        // If the user has not set animation_name, the system-generated
        // autoincrement key animation_name is used, and it is logged and
        // removed when overridden.
        animate_name = args.GetProperty(1).String();
        will_removed_keyframe_name_ = animate_name;
      } else {
        // If the user has set animation_name, it is used.
        animate_name = iter->second.String();
      }

      starlight::CSSStyleUtils::UpdateCSSKeyframes(
          *keyframes_map_, animate_name, args.GetProperty(2), parser_configs);
      lepus_name = lepus::Value(std::move(animate_name));
      if (!enable_new_animator()) {
        // the unique_id may be the same but the keyframes content is different
        // when Animate trigger each time.
        SetKeyframesByNames(lepus_name, *keyframes_map_, true);
      }
      UnitHandler::Process(kPropertyIDAnimationName, lepus_name, styles,
                           parser_configs);
      for (auto& [key, value] : *table) {
        const auto& id = CSSProperty::GetTimingOptionsPropertyID(key);
        if (id == kPropertyEnd) {
          continue;
        }
        if (id == kPropertyIDAnimationIterationCount && value.IsNumber()) {
          if (isinf(value.Number()) == 1) {
            BASE_STATIC_STRING_DECL(kInf, "infinite");
            value = lepus::Value(kInf);
          } else {
            value = lepus::Value(std::to_string(value.Number()));
          }
        }
        UnitHandler::Process(id, value, styles, parser_configs);
      }
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::PAUSE: {
      BASE_STATIC_STRING_DECL(kPaused, "paused");
      UnitHandler::Process(kPropertyIDAnimationPlayState, lepus::Value(kPaused),
                           styles, parser_configs);
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::PLAY: {
      BASE_STATIC_STRING_DECL(kRunning, "running");
      UnitHandler::Process(kPropertyIDAnimationPlayState,
                           lepus::Value(kRunning), styles, parser_configs);
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::CANCEL: {
      BASE_STATIC_STRING_DECL(kRunning, "running");
      UnitHandler::Process(kPropertyIDAnimationPlayState,
                           lepus::Value(kRunning), styles, parser_configs);
      base::InlineVector<CSSPropertyID, 8> reset_names{
          kPropertyIDAnimationDuration,       kPropertyIDAnimationDelay,
          kPropertyIDAnimationIterationCount, kPropertyIDAnimationFillMode,
          kPropertyIDAnimationTimingFunction, kPropertyIDAnimationDirection,
          kPropertyIDAnimationName,           kPropertyIDAnimationPlayState,
      };
      DCHECK(reset_names.is_static_buffer());
      ResetStyle(reset_names);
      break;
    }
    default:
      break;
  }
  ConsumeStyle(styles);
  element_manager_->OnFinishUpdateProps(this, pipeline_option);
}

void Element::AnimateV2(const lepus::Value& args,
                        std::shared_ptr<PipelineOptions>& pipeline_option) {
  // AnimateV2 only work on NewAnimator.
  if (!enable_new_animator()) {
    return;
  }
  // animate's args: operation, js_name, keyframes, animation_data.
  if (!args.IsArrayOrJSArray()) {
    LOGE("Element::Animate's para must be array");
    return;
  }
  if (args.GetLength() < 2) {
    LOGE("Element::Animate's para size must >= 2");
    return;
  }
  const auto& op =
      static_cast<runtime::js::JavaScriptElement::AnimationOperation>(
          args.GetProperty(0).Int32());
  StyleMap styles;
  auto& parser_configs = element_manager()->GetCSSParserConfigs();
  switch (op) {
    case runtime::js::JavaScriptElement::AnimationOperation::START: {
      if (args.GetLength() != 4) {
        LOGE("When start Element::Animate, the para size must be 4");
        return;
      }
      lepus::Value lepus_name;
      base::String animate_name;
      const auto& table = args.GetProperty(3).Table();
      BASE_STATIC_STRING_DECL(kName, "name");
      auto iter = table->find(kName);
      if (iter == table->end()) {
        // If the user has not set animation_name, the system-generated
        // autoincrement key animation_name is used, and it is logged and
        // removed when overridden.
        animate_name = args.GetProperty(1).String();
      } else {
        // If the user has set animation_name, it is used.
        animate_name = iter->second.String();
      }

      starlight::CSSStyleUtils::UpdateCSSKeyframes(
          *keyframes_map_, animate_name, args.GetProperty(2), parser_configs);
      lepus_name = lepus::Value(std::move(animate_name));
      UnitHandler::Process(kPropertyIDAnimationName, lepus_name, styles,
                           parser_configs);
      for (auto& [key, value] : *table) {
        const auto& id = CSSProperty::GetTimingOptionsPropertyID(key);
        if (id == kPropertyEnd) {
          continue;
        }
        if (id == kPropertyIDAnimationIterationCount && value.IsNumber()) {
          if (isinf(value.Number()) == 1) {
            BASE_STATIC_STRING_DECL(kInf, "infinite");
            value = lepus::Value(kInf);
          } else {
            value = lepus::Value(std::to_string(value.Number()));
          }
        }
        UnitHandler::Process(id, value, styles, parser_configs);
      }
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::PAUSE: {
      if (args.GetLength() != 2) {
        LOGE("Element::Animate Pause, unexpected param size.");
        return;
      }
      BASE_STATIC_STRING_DECL(kPaused, "paused");
      UnitHandler::Process(kPropertyIDAnimationPlayState, lepus::Value(kPaused),
                           styles, parser_configs);
      UnitHandler::Process(kPropertyIDAnimationName,
                           lepus::Value(args.GetProperty(1).StdString()),
                           styles, parser_configs);
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::PLAY: {
      if (args.GetLength() != 2) {
        LOGE("Element::Animate Play, unexpected param size.");
        return;
      }
      BASE_STATIC_STRING_DECL(kRunning, "running");
      UnitHandler::Process(kPropertyIDAnimationPlayState,
                           lepus::Value(kRunning), styles, parser_configs);
      UnitHandler::Process(kPropertyIDAnimationName,
                           lepus::Value(args.GetProperty(1).StdString()),
                           styles, parser_configs);
      break;
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::CANCEL: {
      BASE_STATIC_STRING_DECL(kRunning, "running");
      UnitHandler::Process(kPropertyIDAnimationPlayState,
                           lepus::Value(kRunning), styles, parser_configs);
      base::InlineVector<CSSPropertyID, 8> reset_names{
          kPropertyIDAnimationDuration,       kPropertyIDAnimationDelay,
          kPropertyIDAnimationIterationCount, kPropertyIDAnimationFillMode,
          kPropertyIDAnimationTimingFunction, kPropertyIDAnimationDirection,
          kPropertyIDAnimationName,           kPropertyIDAnimationPlayState,
      };
      DCHECK(reset_names.is_static_buffer());
      ResetStyle(reset_names);
      break;
    }
    default:
      break;
  }
  if (!styles.empty()) {
    computed_css_style()->AppendAnimatedAnimationValue(styles);
    has_keyframe_props_changed_ = true;
  }
  element_manager_->OnFinishUpdateProps(this, pipeline_option);
}

void Element::PreparePropBundleIfNeed() {
  if (!prop_bundle_) {
    bool use_map_buffer = element_manager_->GetEnableUseMapBuffer();
    prop_bundle_ = element_manager()->GetPropBundleCreator()->CreatePropBundle(
        use_map_buffer, EnableFragmentLayerRender());
  }
}

void Element::ResetPropBundle() {
  if (prop_bundle_) {
    // TODO(songshourui.null): Consider removing dependency on pre_prop_bundle_
    // in unit tests, so that the ENABLE_UNITTESTS macro can be removed.
#if ENABLE_UNITTESTS
    // Stores the previous PropBundle for unit test verification after a reset.
    pre_prop_bundle_ = prop_bundle_;
#endif

    prop_bundle_ = nullptr;
  }
}

void Element::PushToBundle(CSSPropertyID id) {
  PreparePropBundleIfNeed();
  PropBundleStyleWriter::PushStyleToBundle(prop_bundle_.get(), id,
                                           computed_css_style());
}

void Element::ResolveStyle(StyleMap& new_styles,
                           CSSVariableMap* changed_css_vars) {
  style_resolver_.ResolveStyle(new_styles, GetRelatedCSSFragment(),
                               changed_css_vars);
}

void Element::HandlePseudoElement() {
  style_resolver_.HandlePseudoElement(GetRelatedCSSFragment());
}

void Element::HandleCSSVariables(StyleMap& styles) {
  style_resolver_.HandleCSSVariables(styles);
}

bool Element::DisableFlattenWithOpacity() {
  return computed_css_style()->HasOpacity() && !is_text() && !is_image();
}

starlight::ComputedCSSStyle* Element::GetParentComputedCSSStyle() {
  auto temp = parent();
  while (temp != nullptr && temp->is_wrapper()) {
    temp = temp->parent();
  }

  if (temp == nullptr) {
    return nullptr;
  }

  return temp->computed_css_style();
}

bool Element::ShouldAvoidFlattenForView() {
  return is_view() && element_manager()->GetDefaultOverflowVisible() &&
         computed_css_style()->IsOverflowHidden() &&
         computed_css_style()->HasBorderRadius();
}

bool Element::TendToFlatten() {
  return config_flatten_ && !has_event_listener_ && !has_non_flatten_attrs_ &&
         !DisableFlattenWithOpacity() &&
         !(has_z_props() && !is_image() && !is_text()) && !is_inline_element_ &&
         !ShouldAvoidFlattenForView()
#if OS_IOS
         // On iOS, the current CUI platform-rendering flatten path does not
         // preserve clip/overflow scope the same way as Android's platform
         // renderer. To avoid clipping previously replayed
         // border/image/text contents, only allow flatten when overflow stays
         // visible here. Other non-flatten CSS props, including transform,
         // are filtered earlier by CheckHasNonFlattenCSSProps.
         //
         // TODO(songshourui.null): Revisit this once iOS supports both CUI
         // platform rendering and self rendering with separate clip semantics.
         // Nodes with non-visible overflow may become flattenable again once
         // clip is scoped to rendering content instead of the host view
         // subtree.
         && computed_css_style()->IsOverflowXY();
#endif
  ;
}

double Element::GetFontSize() { return computed_css_style()->GetFontSize(); }

double Element::GetParentFontSize() {
  if (IsCSSInheritanceEnabled() && !is_greedy_parallel_flush() &&
      parent() != nullptr) {
    record_parent_font_size_ = parent()->GetFontSize();
  }
  return record_parent_font_size_;
}

double Element::GetRecordedRootFontSize() {
  return computed_css_style()->GetRootFontSize();
}

double Element::GetCurrentRootFontSize() {
  return element_manager()->root()->GetFontSize();
}

// TODO(songshourui.null): This function is called during Element creation, and
// ResolveStyleValue marks computed_css_style() as dirty, causing font-size to
// be written to the bundle by default. To optimize, consider maintaining the
// default font scale and font size at both the platform and C++ layers. This
// would avoid the performance cost of passing the default font size. A similar
// optimization could be applied to other default style values.
void Element::SetComputedFontSize(double font_size, double root_font_size) {
  if (font_size != GetFontSize()) {
    NotifyUnitValuesUpdatedToAnimation(DynamicCSSStylesManager::kUpdateEm);
  }

  if (root_font_size != GetRecordedRootFontSize()) {
    NotifyUnitValuesUpdatedToAnimation(DynamicCSSStylesManager::kUpdateRem);
  }

  computed_css_style()->SetFontSize(font_size, root_font_size);
  UpdateLayoutNodeFontSize(font_size, root_font_size);
  ResolveStyleValue(kPropertyIDFontSize,
                    CSSValue(font_size, CSSValuePattern::NUMBER));
}

void Element::CheckFlattenRelatedProp(const base::String& key,
                                      const lepus::Value& value) {
  constexpr const static char* kFlatten = "flatten";

  constexpr const static char* kName = "name";
  constexpr const static char* kNativeInteractionEnabled =
      "native-interaction-enabled";

  // TODO(hexionghui): remove this latter.
  constexpr const static char* kUserInteractionEnabled =
      "user-interaction-enabled";

  constexpr const static char* kOverLap = "overlap";

  // TODO(hexionghui): remove this latter.
  constexpr const static char* kExposureScene = "exposure-scene";
  constexpr const static char* kExposureId = "exposure-id";
  // TODO(renzhongyue): remove this latter.
  constexpr const static char* kClipRadius = "clip-radius";

  if (key.IsEqual(kFlatten)) {
    if ((value.IsString() && value.String().IsEqual(kFalse)) ||
        (value.IsBool() && !value.Bool())) {
      config_flatten_ = false;
    } else {
      config_flatten_ = true;
    }
    return;
  }

  // If already have non flatten attributes or `config_flatten_ == false`, there
  // is no need to proceed with subsequent checks.
  if (has_non_flatten_attrs_ || !config_flatten_) return;

  const static auto check_key = [](const base::String& key) {
    return key.IsEqual(kName) || key.IsEqual(kNativeInteractionEnabled) ||
           key.IsEqual(kUserInteractionEnabled) || key.IsEqual(kOverLap);
  };

  const static auto check_key_and_value = [](const base::String& key,
                                             const lepus::Value& value) {
    return (key.IsEqual(kExposureScene) || key.IsEqual(kExposureId)) &&
           !value.IsEmpty();
  };

  const static auto check_clip_radius = [](const base::String& key,
                                           const lepus::Value& value) {
    if (key.IsEqual(kClipRadius)) {
      if (tasm::LynxEnv::GetInstance().GetBoolEnv(
              tasm::LynxEnv::Key::CLIP_RADIUS_FLATTEN, false)) {
        return true;
      }

      if ((value.IsString() && value.StdString() == kTrue) ||
          (value.IsBool() && value.Bool())) {
        return true;
      }

      return false;
    }

    return false;
  };

  if (check_key(key) ||
      (!EnableFragmentLayerRender() && check_key_and_value(key, value)) ||
      check_clip_radius(key, value)) {
    has_non_flatten_attrs_ = true;
  }
}

void Element::CheckHasPlaceholder(const base::String& key,
                                  const lepus::Value& value) {
  constexpr const static char* kPlaceholder = "placeholder";
  if (key.IsEqual(kPlaceholder) && value.IsString()) {
    has_placeholder_ = !value.StdString().empty();
  }
}

void Element::CheckHasTextSelection(const base::String& key,
                                    const lepus::Value& value) {
  static constexpr char kTextSelection[] = "text-selection";
  if (key.IsEqual(kTextSelection) && value.IsBool()) {
    has_text_selection_ = value.Bool();
  }
}

void Element::CheckTriggerGlobalEvent(const lynx::base::String& key,
                                      const lynx::lepus::Value& value) {
  constexpr char kTriggerGlobalEventAttribute[] = "trigger-global-event";
  if (key.str() == kTriggerGlobalEventAttribute && value.IsBool()) {
    trigger_global_event_ = value.Bool();
  }
}

void Element::CheckClassChangeTransmitAttribute(const base::String& key,
                                                const lepus::Value& value) {
  if (key.IsEquals(kTransmitClassDirty)) {
    enable_class_change_transmit_ = value.IsBool() && value.Bool();
  }
}

void Element::CheckNewAnimatorAttr(const base::String& key,
                                   const lepus::Value& value) {
#if OS_HARMONY
  // No need to switch back to platform animation on HarmonyOS
  return;
#endif

  if (key.IsEquals("enable-new-animator")) {
    if (IsFiberArch()) {
      // For FiberArch.
      if (value.IsBool()) {
        enable_new_animator_ = value.Bool();
      } else if (value.IsString()) {
        const std::string& val_str = value.StdString();
        if (val_str == "false") {
          enable_new_animator_ = false;
        } else if (val_str == "true") {
          enable_new_animator_ = true;
        }
      }
    } else {
      // For RadonArch.
      if (value.IsBool()) {
        enable_new_animator_ = value.Bool();
      } else if (value.IsString()) {
        if (value.StdString() == "false") {
          enable_new_animator_ = false;
        } else if (value.StdString() == "true") {
          enable_new_animator_ = true;
        } else if (value.StdString() == "iOS") {
          enable_new_animator_ = true;
#if !OS_IOS
          enable_new_animator_ = false;
#endif
        } else {
          enable_new_animator_ =
              element_manager_->GetEnableNewAnimatorForRadon();
        }
      } else {
        enable_new_animator_ = element_manager_->GetEnableNewAnimatorForRadon();
      }
    }
  }
}

void Element::CheckTimingAttribute(const lynx::base::String& key,
                                   const lynx::lepus::Value& value) {
  if (!key.IsEqual(timing::kTimingFlag)) {
    return;
  }
  if (!value.IsString()) {
    return;
  }
  if (value.String().empty()) {
    return;
  }

  element_manager()->AppendTimingFlag(value.String());
}

void Element::CheckGlobalBindTarget(const lynx::base::String& key,
                                    const lynx::lepus::Value& value) {
  // check global-target id attribute in order to global-bind event
  constexpr char kGlobalTarget[] = "global-target";
  if (!key.IsEqual(kGlobalTarget)) {
    return;
  }
  if (!value.IsString()) {
    return;
  }

  // clear target_set_ if set global-target attribute, no matter value is empty
  // or not
  auto value_str = value.StringView();
  global_bind_target_set_.reset();
  if (value_str.empty()) {
    return;
  }
  constexpr const static char kDelimiter = ',';
  std::vector<std::string> id_targets;
  // multiple id split by comma delimiter
  base::SplitString(base::TrimString(value_str), kDelimiter, id_targets);
  for (auto& s : id_targets) {
    global_bind_target_set_->insert(base::TrimString(s));
  }
}

bool Element::CheckTransitionProps(CSSPropertyID id) {
  if (CSSProperty::IsTransitionProps(id)) {
    has_transition_props_changed_ = true;
    has_non_flatten_attrs_ = true;
    return true;
  }
  return false;
}

bool Element::CheckKeyframeProps(CSSPropertyID id) {
  if (CSSProperty::IsKeyframeProps(id)) {
    has_keyframe_props_changed_ = true;
    has_non_flatten_attrs_ = true;
    return true;
  }
  return false;
}

void Element::CheckHasNonFlattenCSSProps(CSSPropertyID id) {
  if (has_non_flatten_attrs_) {
    // never change has_non_flatten_attrs_ to false again
    return;
  }
  if (id == CSSPropertyID::kPropertyIDFilter || id == kPropertyIDVisibility ||
      id == kPropertyIDClipPath || id == CSSPropertyID::kPropertyIDBoxShadow ||
      id == CSSPropertyID::kPropertyIDTransform ||
      id == CSSPropertyID::kPropertyIDTransformOrigin ||
      id == CSSPropertyID::kPropertyIDMaskImage ||
      (id >= CSSPropertyID::kPropertyIDOutline &&
       id <= CSSPropertyID::kPropertyIDOutlineWidth) ||
      (id >= CSSPropertyID::kPropertyIDLayoutAnimationCreateDuration &&
       id <= CSSPropertyID::kPropertyIDLayoutAnimationUpdateDelay)) {
    has_non_flatten_attrs_ = true;
  }
}

void Element::CheckFixedSticky(CSSPropertyID id, const tasm::CSSValue& value) {
  if (id == kPropertyIDPosition) {
    // Check fixed&sticky before layout only
    bool is_fixed_before = is_fixed_;
    auto type = value.GetEnum<starlight::PositionType>();
    is_fixed_ = type == starlight::PositionType::kFixed;
    is_sticky_ = type == starlight::PositionType::kSticky;
    fixed_changed_ |= (is_fixed_before != is_fixed_);
    if (this->IsNewFixed()) {
      // fixed node should not be layout only. We need it to locate the entire
      // subtree.
      has_layout_only_props_ = false;
    }
  }
}

bool Element::IsStackingContextNode() {
  if (!GetEnableZIndex()) return false;
  return element_manager()->root() == this || has_z_props() || is_fixed_ ||
         computed_css_style()->HasTransform() ||
         computed_css_style()->HasOpacity();
}

bool Element::IsCSSInheritanceEnabled() const {
  return element_manager_ && element_manager_->GetCSSInheritance();
}

bool Element::IsCSSInlineVariablesEnabled() const {
  return element_manager_ &&
         element_manager_->GetDynamicCSSConfigs().enable_css_inline_variables_;
}

PaintingContext* Element::painting_context() {
  return catalyzer_->painting_context();
}

void Element::MarkLayoutDirty() { element_manager_->MarkLayoutDirty(id_); }

void Element::RequireFlush() {
  if (flush_required_) {
    return;
  }
  MarkRequireFlush();
  if (parent_ && !parent_->flush_required_) {
    parent_->RequireFlush();
  }
}

PropertiesResolvingStatus Element::GenerateRootPropertyStatus() const {
  PropertiesResolvingStatus status;
  const auto& env_config = element_manager_->GetLynxEnvConfig();
  status.page_status_.root_font_size_ = env_config.PageDefaultFontSize();
  status.computed_font_size_ = env_config.PageDefaultFontSize();
  status.page_status_.font_scale_ = env_config.FontScale();
  status.page_status_.screen_width_ = env_config.ScreenWidth();
  status.page_status_.viewport_width_ = env_config.ViewportWidth();
  status.page_status_.viewport_height_ = env_config.ViewportHeight();
  return status;
}

void Element::MarkSubtreeNeedUpdate() {
  if (!subtree_need_update_) {
    subtree_need_update_ = true;
    if (parent_) {
      parent_->MarkSubtreeNeedUpdate();
    }
  }
}

void Element::NotifyElementSizeUpdated() {
  if (css_keyframe_manager_) {
    css_keyframe_manager_->NotifyElementSizeUpdated();
  }
  if (css_transition_manager_) {
    css_transition_manager_->NotifyElementSizeUpdated();
  }
  if (is_list_item() && parent_) {
    parent_->OnListItemLayoutUpdated(this);
  }
}

std::pair<CSSValuePattern, CSSValuePattern>
Element::ConvertDynamicStyleFlagToCSSValuePattern(uint32_t style) {
  switch (style) {
    case DynamicCSSStylesManager::kUpdateEm:
      return std::make_pair(CSSValuePattern::EM, CSSValuePattern::EMPTY);
    case DynamicCSSStylesManager::kUpdateRem:
      return std::make_pair(CSSValuePattern::REM, CSSValuePattern::EMPTY);
    case DynamicCSSStylesManager::kUpdateScreenMetrics:
      return std::make_pair(CSSValuePattern::RPX, CSSValuePattern::EMPTY);
    case DynamicCSSStylesManager::kUpdateViewport:
      return std::make_pair(CSSValuePattern::VW, CSSValuePattern::VH);
    case DynamicCSSStylesManager::kUpdateFontScale:
      return std::make_pair(CSSValuePattern::EM, CSSValuePattern::REM);
    default:
      return std::make_pair(CSSValuePattern::EMPTY, CSSValuePattern::EMPTY);
  }
}

void Element::NotifyUnitValuesUpdatedToAnimation(uint32_t style) {
  auto pattern_pair = ConvertDynamicStyleFlagToCSSValuePattern(style);
  if (pattern_pair.first != CSSValuePattern::EMPTY) {
    if (css_keyframe_manager_) {
      css_keyframe_manager_->NotifyUnitValuesUpdatedToAnimation(
          pattern_pair.first);
      if (pattern_pair.second != CSSValuePattern::EMPTY) {
        css_keyframe_manager_->NotifyUnitValuesUpdatedToAnimation(
            pattern_pair.second);
      }
    }
    if (css_transition_manager_) {
      css_transition_manager_->NotifyUnitValuesUpdatedToAnimation(
          pattern_pair.first);
      if (pattern_pair.second != CSSValuePattern::EMPTY) {
        css_transition_manager_->NotifyUnitValuesUpdatedToAnimation(
            pattern_pair.second);
      }
    }
  }
}

void Element::SetPlaceHolderStylesInternal(
    const PseudoPlaceHolderStyles& styles) {
  fml::RefPtr<lepus::Dictionary> dict = lepus::Dictionary::Create();
  if (styles.color_) {
    const auto& value = styles.color_->GetValue();
    if (value.IsNumber()) {
      dict->SetValue(BASE_STATIC_STRING(kPropertyNameColor), value);
    }
  }

  if (styles.font_size_) {
    const auto result = starlight::CSSStyleUtils::ResolveFontSize(
        *styles.font_size_, element_manager()->GetLynxEnvConfig(),
        element_manager()->GetLynxEnvConfig().ViewportWidth(),
        element_manager()->GetLynxEnvConfig().ViewportHeight(), GetFontSize(),
        GetRecordedRootFontSize(), element_manager()->GetCSSParserConfigs());
    if (result.has_value()) {
      dict->SetValue(BASE_STATIC_STRING(kPropertyNameFontSize), *result);
    }
  }
  if (styles.font_weight_) {
    const auto& value = styles.font_weight_->GetValue();
    if (value.IsNumber()) {
      dict->SetValue(BASE_STATIC_STRING(kPropertyNameFontWeight), value);
    }
  }
  if (styles.font_family_) {
    const auto& value = styles.font_family_->GetValue();
    if (value.IsString()) {
      dict->SetValue(BASE_STATIC_STRING(kPropertyNameFontFamily), value);
    }
  }
  SetProp("placeholder-style", lepus::Value(std::move(dict)));
}

bool Element::GetEnableZIndex() { return element_manager_->GetEnableZIndex(); }

void Element::SetDataToNativeKeyframeAnimator(bool from_resume) {
  if (element_manager_->IsPause()) {
    element_manager_->AddPausedAnimationElement(this);
    return;
  }
  // keyframe animation
  if (!has_keyframe_props_changed_ && !from_resume) {
    return;
  }

  if (!css_keyframe_manager_) {
    css_keyframe_manager_ =
        std::make_unique<animation::CSSKeyframeManager>(this);
  }
  css_keyframe_manager_->SetAnimationDataAndPlay(
      computed_css_style()->animation_data());
}

void Element::SetDataToNativeTransitionAnimator() {
  // transition animation
  if (!has_transition_props_changed_) {
    return;
  }

  if (!css_transition_manager_) {
    css_transition_manager_ =
        std::make_unique<animation::CSSTransitionManager>(this);
  }
  css_transition_manager_->setTransitionData(
      computed_css_style()->transition_data());
  has_transition_props_changed_ = false;
}

void Element::ClearTransitionPreviousEndValue(
    const base::String& transition_name) {
  auto css_id = CSSProperty::GetPropertyID(transition_name);
  if (css_transition_manager_) {
    css_transition_manager_->ClearPreviousEndValue(css_id);
  }
}

bool Element::TickAllAnimation(fml::TimePoint& frame_time,
                               std::shared_ptr<PipelineOptions>& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_TICK_ALL_ANIMATION);

  if (css_transition_manager_ != nullptr) {
    css_transition_manager_->TickAllAnimation(frame_time);
  }
  if (css_keyframe_manager_ != nullptr) {
    css_keyframe_manager_->TickAllAnimation(frame_time);
  }
  auto [need_layout, has_pending_bundle] = FlushAnimatedStyle();
  bool need_mark_props_dirty = need_layout;
  if (element_manager_->FixNewAnimatorFlushBug()) {
    // FIXME(linxs): remove this settings in next version
    need_mark_props_dirty = need_layout || has_pending_bundle;
  }
  if (need_mark_props_dirty) {
    if (tasm::LynxEnv::GetInstance().EnableNewAnimatorOnPatchFinishOpt()) {
      static_cast<FiberElement*>(this)->MarkPropsDirty();
    } else {
      element_manager_->OnFinishUpdateProps(this, options);
    }
  }
  return need_layout;
}

void Element::UpdateFinalStyleMap(const StyleMap& styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_UPDATE_FINAL_STYLE_MAP);
  if (!styles.empty()) {
    final_animator_map_->merge(styles);
  }
}

std::tuple<bool, bool> Element::FlushAnimatedStyle() {
  if (!final_animator_map_.has_value() || final_animator_map_->empty()) {
    return {false, false};
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_FLUSH_ANIMATED_STYLE);
  bool has_layout_style = false;
  for (const auto& style : *final_animator_map_) {
    if (NeedFullFlushPath(style.first, style.second)) {
      has_layout_style = true;
      break;
    }
  }

  // When prop_bundle_ == nullptr && computed_css_style()->IsClean() and
  // has_pending_bundle is true, a fast path is needed to dispatch a bundle to
  // the painting node.
  bool need_dispatch =
      prop_bundle_ == nullptr && computed_css_style()->IsClean();
  bool has_pending_bundle = false;
  bool should_do_full_flush =
      has_layout_style || !has_painting_node_ || EnableFragmentLayerRender();

  for (const auto& style : *final_animator_map_) {
    // Record previous before rtl-converter for transition.
    if (style.second != CSSValue()) {
      RecordElementPreviousStyle(style.first, style.second);
    } else {
      ResetElementPreviousStyle(style.first);
    }

    if (should_do_full_flush) {
      FlushAnimatedStyleInternal(style.first, style.second);
    } else {
      // If it's a render property, push it to the temporary bundle.
      has_pending_bundle |= WriteRenderStyleToBundle(style.first, style.second);
    }
  }
  if (has_pending_bundle && need_dispatch) {
    // if prop_bundle_ not null, it means the element is dirty,no need to
    // dispatch it here
    auto bundle = element_manager()->GetPropBundleCreator()->CreatePropBundle();
    PropBundleStyleWriter::PushStyleToBundle(bundle.get(),
                                             computed_css_style());
    DispatchBundleToPaintingNode(bundle);
    has_pending_bundle = false;
  }
  final_animator_map_.reset();
  return {should_do_full_flush, has_pending_bundle};
}

// Currently, this function is only called by the list for animation logic, and
// it only handles opacity animations. If other animation types are added in the
// future, or if it might affect the layout, be aware that changes need to be
// coordinated with the current animation logic of the list.
void Element::FlushAnimatedStyle(tasm::CSSPropertyID id, tasm::CSSValue value) {
  auto style = std::make_pair(id, std::move(value));

  if (computed_css_style()->IsClean() && prop_bundle_ == nullptr &&
      WriteRenderStyleToBundle(id, style.second)) {
    auto bundle = element_manager()->GetPropBundleCreator()->CreatePropBundle();
    PropBundleStyleWriter::PushStyleToBundle(bundle.get(),
                                             computed_css_style());
    DispatchBundleToPaintingNode(bundle);
  }
}

bool Element::WriteRenderStyleToBundle(tasm::CSSPropertyID id,
                                       const tasm::CSSValue& value) {
  switch (id) {
    case kPropertyIDTransform:
    case kPropertyIDColor:
    case kPropertyIDBackgroundColor:
    case kPropertyIDBorderLeftColor:
    case kPropertyIDBorderRightColor:
    case kPropertyIDBorderTopColor:
    case kPropertyIDBorderBottomColor:
    case kPropertyIDOpacity:
    case kPropertyIDOffsetDistance:
    case kPropertyIDTransformOrigin:
    case kPropertyIDVisibility:
      return computed_css_style()->SetValue(id, value);
    default:
      LOGE("[animation] unsupported animation value type for css:" << id);
      return false;
  }
}

void Element::DispatchBundleToPaintingNode(fml::RefPtr<PropBundle> bundle) {
  HandleDelayTask([this, impl_id = impl_id(), tend_to_flatten = TendToFlatten(),
                   bundle = bundle]() {
    element_container()->UpdatePaintingNode(tend_to_flatten, bundle);
    element_container()->OnNodeReady();
  });
}

bool Element::ShouldConsumeTransitionStylesInAdvance() {
  return (enable_new_animator() && HasPaintingNode());
}

// Since the previous element styles cannot be accessed in element, we
// need to record some necessary styles which New Animator transition needs.
// TODO(wujintian): We only need to record layout-only properties, while other
// properties can be accessed through ComputedCSSStyle.
void Element::RecordElementPreviousStyle(CSSPropertyID css_id,
                                         const tasm::CSSValue& value) {
  if (!enable_new_animator()) {
    return;
  }
  if (animation::IsAnimatableProperty(css_id)) {
    animation_previous_styles_[css_id] = value;
  }
}

void Element::ResetElementPreviousStyle(CSSPropertyID css_id) {
  if (!enable_new_animator()) {
    return;
  }
  if (animation::IsAnimatableProperty(css_id)) {
    animation_previous_styles_.erase(css_id);
  }
}

std::optional<CSSValue> Element::GetElementPreviousStyle(
    tasm::CSSPropertyID css_id) {
  auto iter = animation_previous_styles_.find(css_id);
  if (iter == animation_previous_styles_.end()) {
    return std::optional<CSSValue>();
  }
  return iter->second;
}

CSSKeyframesToken* Element::GetSimpleStyleKeyframesToken(
    const base::String& animation_name) {
  const auto& keyframes = element_manager()->GetSimpleStyleKeyframes();
  if (!keyframes) {
    return nullptr;
  }

  if (auto it = keyframes->find(animation_name); it != keyframes->end()) {
    return it->second.get();
  }
  return nullptr;
}

CSSKeyframesToken* Element::GetCSSKeyframesToken(
    const base::String& animation_name) {
  auto* manager = element_manager();
  if (manager && manager->EnableSimpleStyle()) {
    return GetSimpleStyleKeyframesToken(animation_name);
  }
  tasm::CSSFragment* style_sheet = GetRelatedCSSFragment();
  if (style_sheet) {
    return style_sheet->GetKeyframesRule(animation_name);
  }
  return nullptr;
}

void Element::ResolveAndFlushKeyframes() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_RESOLVE_AND_FLUSH_KEYFRAMES);

  auto animation_data = computed_css_style()->animation_data();
  auto animation_names = lepus::CArray::Create();
  for (auto data : animation_data) {
    if (data.name.empty()) {
      continue;
    }
    animation_names->emplace_back(data.name);
  }

  CSSFragment* css_fragment = GetRelatedCSSFragment();
  if (animation_names->size() != 0 && css_fragment &&
      !css_fragment->GetKeyframesRuleMap().empty()) {
    SetKeyframesByNames(lepus::Value(animation_names),
                        css_fragment->GetKeyframesRuleMap(), false);
  } else if (animation_names->size() != 0 &&
             element_manager_->GetSimpleStyleKeyframes()) {
    SetKeyframesByNames(lepus::Value(animation_names),
                        *(element_manager_->GetSimpleStyleKeyframes().get()),
                        false);
  }
}

void Element::EnsureTagInfo() {
  if (layout_node_type_ == kLayoutNodeTypeNotInit) {
    int32_t node_info = EnableLayoutInElementMode() ? GetBuiltInNodeInfo() : 0;
    if (node_info == 0) {
      node_info = element_manager()->GetNodeInfoByTag(tag_);
    }
    layout_node_type_ = (node_info & 0xFFFF);
    create_node_async_ = ((node_info & 0x10000) > 0);
    need_process_direction_ = ((node_info & 0x20000) > 0);
  }
}

void Element::TransitionToNativeView() {
  // If already layout only or is virtual, do not need create ui for this
  // element.
  if (!IsLayoutOnly() || is_virtual()) {
    return;
  }
  HandleDelayTask(
      [prop_bundle =
           prop_bundle_
               ? prop_bundle_
               : element_manager_->GetPropBundleCreator()->CreatePropBundle(),
       element_container = element_container()]() {
        element_container->TransitionToNativeView(std::move(prop_bundle));
      });
}

void Element::EnqueueLayoutTask(base::MoveOnlyClosure<void> operation) {
  operation();
}

bool Element::IsExtendedLayoutOnlyProps(CSSPropertyID css_id) {
  static const base::NoDestructor<std::array<bool, kPropertyEnd>>
      kWantedProperty([]() {
        std::array<bool, kPropertyEnd> property_array;
        std::fill(property_array.begin(), property_array.end(), false);
#define DECLARE_EXTENDED_PROPERTY(name, type) \
  property_array[kPropertyID##name] = type;
        FOREACH_EXTENDED_LAYOUT_ONLY_PROPERTY(DECLARE_EXTENDED_PROPERTY)
#undef DECLARE_WANTED_PROPERTY
        return property_array;
      }());

  return (*kWantedProperty)[css_id];
}

bool Element::IsNewFixed() const {
  return is_fixed_ && element_manager()->GetEnableFixedNew();
}

bool Element::GetEnableFixedNew() const {
  return element_manager()->GetEnableFixedNew();
}

bool Element::IsFixedUnified() const {
  return is_fixed_ && element_manager()->GetEnableUnifyFixedBehavior();
}

bool Element::IsFixedUnifiedEnabled() const {
  return element_manager()->GetEnableUnifyFixedBehavior();
}

bool Element::IsFixedNewOrUnifiedEnabled() const {
  return element_manager()->GetEnableFixedNew() ||
         element_manager()->GetEnableUnifyFixedBehavior();
}

bool Element::IsFixedNewOrUnified() const {
  return IsNewFixed() || IsFixedUnified();
}

bool Element::IsFixedUnifiedOnly() const {
  return IsFixedUnified() && !IsNewFixed();
}

bool Element::IsEventPathCatch(event::EventTarget* target,
                               event::Event* event) {
  if (event && event->from_frontend() && target != this) {
    auto root_component =
        static_cast<Element*>(target)->GetParentComponentElement();
    if (this == root_component && !event->composed()) {
      return true;
    }
  }

  // Compatible with the previous logic that position:fixed will modify
  // the structure of the element tree.
  if (IsRadonArch() && is_fixed()) {
    auto root = element_manager()->root();
    if (this != root) {
      LOGI("Element::IsEventPathCatch fixed target.")
      event->event_path().push_back(root->GetWeakTarget());
      return true;
    }
  }
  return false;
}

bool Element::IsEventPathSkip(event::EventTarget* target, event::Event* event) {
  if (event && event->from_frontend() && target != this) {
    auto root_component =
        static_cast<Element*>(target)->GetParentComponentElement();
    if (GetParentComponentElement() != root_component && !event->composed()) {
      return true;
    }
  }
  return false;
}

// TODO(hexionghui): move this to EventDispatcher
void Element::HandleGlobalEvent(fml::RefPtr<event::Event> event) {
  // handle the trigger-global-event attribute
  auto path = event->event_path();
  auto delegate = element_manager_->element_manager_delegate();
  event->set_event_phase(event::Event::PhaseType::kGlobal);
  for (const auto& item : path) {
    auto current_target = static_cast<Element*>(item.get());
    if (!current_target) {
      LOGE(
          "Element::HandleGlobalEvent trigger global error: the current_target "
          "is null.");
      continue;
    }
    if (current_target->EnableTriggerGlobalEvent()) {
      event->set_current_target(current_target->GetWeakTarget());
      event->HandleEventBaseDetail();
      delegate->SendGlobalEvent(event->type(), event->detail());
    }
  }

  // handle the global-bind event
  auto node_manager = element_manager_->node_manager();
  if (node_manager == nullptr) {
    LOGE("Element::HandleGlobalEvent error: the node_manager is null.");
    return;
  }
  const auto& global_bind_ids =
      element_manager_->GetGlobalBindElementIds(event->type());
  if (global_bind_ids.size() > 0) {
    for (const auto& id : global_bind_ids) {
      auto current_target = node_manager->Get(id);
      if (!current_target) {
        LOGE(
            "Element::HandleGlobalEvent global bind error: the current_target "
            "is null.");
        continue;
      }
      event->set_current_target(current_target->GetWeakTarget());
      const auto& global_bind_target_set = current_target->GlobalBindTarget();
      // If set is empty, means the target is all other elements.
      if (!global_bind_target_set.has_value() ||
          global_bind_target_set->empty()) {
        current_target->DispatchEvent(event);
      } else {
        // event can bubble
        if (event->bubbles()) {
          for (const auto& item : path) {
            Element* bubble_target = static_cast<Element*>(item.get());
            if (!bubble_target) {
              LOGE(
                  "Element::HandleGlobalEvent global bind error: the "
                  "bubble_target is null.");
              continue;
            }
            if (bubble_target->data_model() == nullptr ||
                bubble_target->data_model()->idSelector().empty()) {
              continue;
            }
            auto id_selector = static_cast<Element*>(item.get())
                                   ->data_model()
                                   ->idSelector()
                                   .str();
            if (global_bind_target_set->contains(id_selector)) {
              event->set_target(bubble_target->GetWeakTarget());
              current_target->DispatchEvent(event);
            }
          }
          // reset target for event
          event->set_target(GetWeakTarget());
        }
        // event can't bubble
        else {
          if (data_model() && !data_model()->idSelector().empty()) {
            auto id_selector = data_model()->idSelector().str();
            if (global_bind_target_set->contains(id_selector)) {
              current_target->DispatchEvent(event);
            }
          }
        }
      }
    }
  }
}

lepus::Value Element::GetEventTargetInfo(bool is_core_event) {
  auto dict = lepus::Dictionary::Create();
  if (data_model_ != nullptr) {
    BASE_STATIC_STRING_DECL(kId, "id");
    BASE_STATIC_STRING_DECL(kDataset, "dataset");
    BASE_STATIC_STRING_DECL(kUid, "uid");

    dict.get()->SetValue(kId, data_model_->idSelector());
    auto dataset = lepus::Dictionary::Create();
    for (const auto& [key, value] : data_model_->dataset()) {
      dataset.get()->SetValue(key, value);
    }
    dict.get()->SetValue(kDataset, std::move(dataset));
    dict.get()->SetValue(kUid, id_);
  }

  // element ref needed in fiber element worklet
  if (is_core_event) {
    BASE_STATIC_STRING_DECL(kElementRefptr, "elementRefptr");
    dict.get()->SetValue(kElementRefptr, fml::RefPtr<tasm::Element>(this));
  }

  return lepus::Value(std::move(dict));
}

lepus::Value Element::GetEventControlInfo(bool is_core_event) {
  auto array = lepus::CArray::Create();
  if (is_core_event) {
    array->emplace_back(ParentComponentIdString());
    array->emplace_back(ParentComponentEntryName());
    array->emplace_back(impl_id());
  } else {
    if (InComponent()) {
      array->emplace_back(false);
      array->emplace_back(ParentComponentIdString());
    } else {
      array->emplace_back(true);
      array->emplace_back("");
    }
  }
  return lepus::Value(std::move(array));
}

bool Element::GetEnableMultiTouchParamsCompatible() {
  return element_manager_->GetEnableMultiTouchParamsCompatible();
}

float Element::GetLayoutsUnitPerPx() {
  return element_manager_->GetLynxEnvConfig().LayoutsUnitPerPx();
}

starlight::LayoutResultForRendering Element::layout_result() {
  auto layout_result = starlight::LayoutResultForRendering();
  layout_result.size_ = FloatSize(width(), height());
  layout_result.offset_ = starlight::FloatPoint(left(), top());
  layout_result.padding_ = ConvertToDirectionValue(paddings_);
  layout_result.margin_ = ConvertToDirectionValue(margins_);
  layout_result.border_ = ConvertToDirectionValue(borders_);
  return layout_result;
}

void Element::UpdateGlobalInsertionOrder() {
  global_insertion_order_ = element_manager()->GenerateGlobalInsertionOrder();
}

Element* Element::root_virtual_parent() {
  Element* root_virtual = virtual_parent_;
  while (root_virtual && root_virtual->virtual_parent() != nullptr) {
    root_virtual = root_virtual->virtual_parent();
  }
  return root_virtual;
}

Element* Element::FindFirstNonVirtualRenderAncestor() {
  auto* current = this;
  while (current && current->is_virtual()) {
    current = current->render_parent();
  }
  return current;
}

Element* Element::FindFirstNonVirtualRenderSibling() {
  auto* current = this;
  while (current && current->is_virtual()) {
    current = current->next_render_sibling();
  }
  return current;
}

Element* Element::FindFirstNonWrapperRenderAncestor() {
  auto* current = this;
  while (current && current->is_wrapper()) {
    auto* parent = current->render_parent();
    if (!parent) {
      break;
    }
    current = parent;
  }
  return current;
}

Element* Element::FindFirstNonWrapperChildOrSibling() {
  auto* current = this;
  while (current) {
    if (!current->is_wrapper()) {
      return current;
    }

    auto* first_child = current->first_render_child();
    if (first_child) {
      if (!first_child->is_wrapper()) {
        return first_child;
      }
      auto* candidate = first_child->FindFirstNonWrapperChildOrSibling();
      if (candidate && !candidate->is_wrapper()) {
        return candidate;
      }
    }

    current = current->next_render_sibling();
  }
  return current;
}

// TODO: Place logic in Element for now. If other module need to apply
// same logic, move it to css_property
Element::DirectionMapping Element::CheckDirectionMapping(CSSPropertyID css_id) {
  static const base::NoDestructor<
      std::array<Element::DirectionMapping, kPropertyEnd>>
      kDirectionMappingProperty([]() {
        std::array<Element::DirectionMapping, kPropertyEnd>
            property_mapping_array;
        std::fill(property_mapping_array.begin(), property_mapping_array.end(),
                  Element::DirectionMapping());
#define DECLARE_DIRECTION_MAPPING(name, is_logic, ltr_value, rtl_value) \
  property_mapping_array[kPropertyID##name] =                           \
      Element::DirectionMapping(is_logic, ltr_value, rtl_value);
        FOREACH_DIRECTION_MAPPING_PROPERTY(DECLARE_DIRECTION_MAPPING)
#undef DECLARE_DIRECTION_MAPPING
        return property_mapping_array;
      }());

  return (*kDirectionMappingProperty)[css_id];
}

Element* Element::Sibling(int offset) const {
  if (!parent_) return nullptr;
  auto index = parent_->IndexOf(this);
  // We know the index can't be -1
  return parent_->GetChildAt(index + offset);
}

bool Element::InComponent() const {
  auto* p = GetParentComponentElement();
  if (p) {
    return !(p->is_page());
  }
  return false;
}

/**
 * A key function to get parent component's element
 */
Element* Element::GetParentComponentElement() const {
  if (IsDetached()) {
    // if the Element is not attached, it is meaningless to return parent
    // component, and more important, the parent component may be destroyed!
    return nullptr;
  }
  ResolveParentComponentElement();
  return parent_component_element_;
}

std::string Element::ParentComponentIdString() const {
  auto* p = GetParentComponentElement();
  if (p) {
    return static_cast<ComponentElement*>(p)->component_id().str();
  }
  return "";
}

const std::string& Element::ParentComponentEntryName() const {
  auto* p = GetParentComponentElement();
  if (p) {
    return static_cast<ComponentElement*>(p)->GetEntryName();
  }
  static const std::string kDefaultEntryName(DEFAULT_ENTRY_NAME);
  return kDefaultEntryName;
}

/**
 * A function to resolve parent component element CSSFragment
 */
void Element::ResolveParentComponentElement() const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_RESOLVE_PARENT_COMPONENT);
  // parent_component_unique_id_ less than page element element id is invalid.
  if (!parent_component_element_ &&
      parent_component_unique_id_ >= kInitialImplId) {
    if (element_manager()->GetPageElement() != nullptr &&
        parent_component_unique_id_ ==
            element_manager()->GetPageElement()->impl_id()) {
      // fast path: if parent_component_unique_id is page element's id, set
      // parent_component_element to page_element
      parent_component_element_ = element_manager()->GetPageElement();
    } else {
      ResolveParentComponentElementImpl();
    }
  }
}

void Element::ResolveParentComponentElementImpl() const {
  if (this->parent() == nullptr) {
    return;
  }

  Element* anchor = this->parent();
  while (anchor != nullptr) {
    if (anchor->parent_component_unique_id_ == parent_component_unique_id_ &&
        anchor->parent_component_element_ != nullptr) {
      // anchor element has identical parent_component_element with current
      // element, reuse anchor element's parent component element
      parent_component_element_ = anchor->parent_component_element_;
      return;
    }

    if (anchor->impl_id() == parent_component_unique_id_) {
      // anchor element is current element's parent component element
      parent_component_element_ = anchor;
      return;
    }

    anchor = anchor->parent();
  }
}

bool Element::IsInheritable(CSSPropertyID id) const {
  if (!IsCSSInheritanceEnabled()) {
    return false;
  }

  if (!element_manager_->GetDynamicCSSConfigs().custom_inherit_list_.empty()) {
    return element_manager_->GetDynamicCSSConfigs().custom_inherit_list_.count(
        id);
  }

  return DynamicCSSStylesManager::GetInheritableProps().count(id);
}

bool Element::IsDirectionChangedEnabled() const {
  // FIXME(linxs): we just use enable_css_inheritance_ to indicate is enable
  // direction temporarily
  // DirectionChange is enabled by default in RadonArch mode.
  // TODO(kechenglong): Avoid using IsRadonArch() & IsFiberArch() in Dom layer.
  return IsRadonArch() || element_manager_->GetCSSInheritance();
}

std::pair<bool, CSSPropertyID> Element::ConvertRtlCSSPropertyID(
    CSSPropertyID id) {
  auto direction_mapping = CheckDirectionMapping(id);
  bool is_logic_property = direction_mapping.is_logic_;

  // default ltr_property/rtl_property for CSSProperty is kPropertyStart.
  bool is_direction_aware_property =
      direction_mapping.ltr_property_ != kPropertyStart ||
      direction_mapping.rtl_property_ != kPropertyStart;
  if (is_direction_aware_property) {
    // When in LynxRTL mode or RTL mode with current property is a logic
    // property, use RTL CSSPropertyID, other wise use LTR CSSPropertyID
    auto current_direction = computed_css_style()->GetDirection();
    bool use_rtl_value = (IsRTL(current_direction) && is_logic_property) ||
                         IsLynxRTL(current_direction);
    return std::make_pair(true, use_rtl_value
                                    ? direction_mapping.rtl_property_
                                    : direction_mapping.ltr_property_);
  }
  return std::make_pair(false, id);
}

int32_t Element::IndexOf(const Element* child) const {
  for (auto it = scoped_children_.begin(); it != scoped_children_.end(); ++it) {
    if (it->get() == child) {
      return static_cast<int>(std::distance(scoped_children_.begin(), it));
    }
  }
  return -1;
}

Element* Element::GetChildAt(size_t index) {
  if (index >= scoped_children_.size()) {
    return nullptr;
  }
  return scoped_children_[index].get();
}

size_t Element::GetChildCount() { return scoped_children_.size(); }

ElementChildrenArray Element::GetChildren() {
  ElementChildrenArray ret;
  ret.reserve(scoped_children_.size());
  for (const auto& child : scoped_children_) {
    ret.push_back(child.get());
  }
  return ret;
}

Element* Element::first_child() const {
  return scoped_children_.empty() ? nullptr : scoped_children_.front().get();
}

Element* Element::last_child() const {
  return scoped_children_.empty() ? nullptr : scoped_children_.back().get();
}

void Element::LogNodeInfo() {
  LOGE("FiberElement node ,this:"
       << this << ", tag:" << tag_.str() << ",id:" << id_
       << (!data_model_->idSelector().empty() ? data_model_->idSelector().str()
                                              : "")
       << ", first class:"
       << (data_model_->classes().size() > 0 ? data_model_->classes()[0].str()
                                             : ""));
}

bool Element::NeedPropagateInheritedDirtyFlag(bool force_propagate) {
  // When level order traversing is enabled, mark kDirtyPropagateInherited is
  // performed before FlushActions.
  return children_propagate_inherited_styles_flag_ &&
         (force_propagate ||
          (!element_manager()->GetEnableParallelElement() ||
           !element_manager()->EnableLevelOrderTraversing()));
}

bool Element::CheckHasIdMapInCSSFragment() {
  auto* css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (css_fragment && css_fragment->HasIdSelector()) {
    return true;
  }
  return false;
}

void Element::HandleBeforeFlushActionsTask(
    base::MoveOnlyClosure<void> operation,
    int32_t predicate_parallel_flush_flag) {
  if ((this->parallel_flush_ & predicate_parallel_flush_flag) > 0) {
    parallel_before_flush_action_tasks_->emplace_back(std::move(operation));
  } else {
    operation();
  }
}

void Element::VerifyKeyframePropsChangedHandling() {
  if (has_keyframe_props_changed_) {
    // Throw exception on purpose in debug mode or UT to indicate that
    // keyframe_props is not handled properly in this flow
    DCHECK(!has_keyframe_props_changed_);
    has_keyframe_props_changed_ = false;
  }
}

bool Element::IfNeedsUpdateLayoutInfo() {
  if (sl_node_ == nullptr) {
    return false;
  }
  return sl_node_->GetHasNewLayout();
}

void Element::ResetStyleSheet() { style_sheet_ = nullptr; }

const base::String& Element::GetRawInlineStyles() {
  return full_raw_inline_style_;
}

void Element::SetRawInlineStyles(base::String value) {
  full_raw_inline_style_ = std::move(value);
  MarkDirty(kDirtyStyle);
}

void Element::SetDefaultOverflow(bool visible) {
  computed_css_style()->SetOverflowDefaultVisible(visible);
}

void Element::DestroyPlatformNode() {
  if (element_container() && has_painting_node_) {
    element_container()->Destroy();
  }
  has_painting_node_ = false;
  MarkPlatformNodeDestroyed();
}

void Element::MarkPlatformNodeDestroyed() {
  for (size_t i = 0; i < GetChildCount(); ++i) {
    auto* child = GetChildAt(i);
    // Element may be referenced by JS engine. Just clear the parent-child
    // relationship.
    if (child->parent_ == this) {
      child->parent_ = nullptr;
    }
    if (child->render_parent_ == this) {
      child->render_parent_ = nullptr;
    }
  }
  if (scoped_virtual_children_.has_value()) {
    for (size_t i = 0; i < scoped_virtual_children_->size(); ++i) {
      auto* virtual_child = (*scoped_virtual_children_)[i].get();
      if (virtual_child->parent_ == this) {
        virtual_child->parent_ = nullptr;
      }
    }
  }
  // clear element's children only in radon or radon compatible mode.
  scoped_children_.clear();
  scoped_virtual_children_.reset();
  logical_children_.clear();
}

void Element::ConvertToInlineElement() {
  MarkAsInline();
  for (auto& child : scoped_children_) {
    child->ConvertToInlineElement();
  }
}

void Element::SetStyle(CSSPropertyID id, const lepus::Value& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_SET_STYLE);

  // When the `SetStyle` API is called, the `SetRawInlineStyles` API might
  // already have been invoked. In this case, it is necessary to call
  // `ProcessFullRawInlineStyle` first to ensure that `full_raw_inline_style_`
  // is set into `current_raw_inline_styles_`. Otherwise, `SetRawInlineStyles`
  // might override the `SetStyle` call, leading to unexpected behavior.
  ProcessFullRawInlineStyle(nullptr);

  if (!value.IsEmpty()) {
    current_raw_inline_styles_->insert_or_assign(id, value);
  } else if (current_raw_inline_styles_.has_value()) {
    current_raw_inline_styles_->erase(id);
  }

  MarkDirty(kDirtyStyle);

  if (has_extreme_parsed_styles_ && !only_selector_extreme_parsed_styles_) {
    has_extreme_parsed_styles_ = false;
    extreme_parsed_styles_.reset();
  }

  // Only exec the following expr when ENABLE_INSPECTOR, such that devtool can
  // get element's inline style.
  EXEC_EXPR_FOR_INSPECTOR({
    if (element_manager_ && element_manager_->IsDomTreeEnabled()) {
      if (value.IsEmpty()) {
        data_model()->ResetInlineStyle(id);
      } else {
        data_model()->SetInlineStyle(id,
                                     value.IsNumber()
                                         ? std::to_string(value.Number())
                                         : value.ToString(),
                                     element_manager_->GetCSSParserConfigs());
      }
    }
  });
}

void Element::RemoveAllInlineStyles() {
  // Only exec the following expr when ENABLE_INSPECTOR, such that devtool can
  // get element's inline style.
  EXEC_EXPR_FOR_INSPECTOR({
    if (element_manager_->IsDomTreeEnabled() &&
        current_raw_inline_styles_.has_value()) {
      for (const auto& pair : *current_raw_inline_styles_) {
        const static base::String kNull;
        data_model()->SetInlineStyle(pair.first, kNull,
                                     element_manager_->GetCSSParserConfigs());
      }
    }
  });

  full_raw_inline_style_ = base::String();
  current_raw_inline_styles_.reset();

  MarkDirty(kDirtyStyle);
}

void Element::CacheStyleFromAttributes(CSSPropertyID id, CSSValue&& value) {
  styles_from_attributes_->insert_or_assign(id, std::move(value));
}

void Element::CacheStyleFromAttributes(CSSPropertyID id,
                                       const lepus::Value& value) {
  UnitHandler::Process(id, value, *styles_from_attributes_,
                       element_manager()->GetCSSParserConfigs());
}

void Element::DidConsumeStyle() {
  if (!styles_from_attributes_.has_value()) {
    return;
  }
  if (styles_from_attributes_->empty()) {
    return;
  }

  ConsumeStyleInternal(*styles_from_attributes_, nullptr,
                       [](auto id, const auto& value) {
                         // Do not skip any style here.
                         return false;
                       });
  styles_from_attributes_.reset();
}

void Element::SetParsedStyles(const ParsedStyles& parsed_styles,
                              const lepus::Value& config) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_SET_PARSED_STYLES);

  constexpr const static char kOnlySelector[] = "selectorParsedStyles";
  const auto& only_selector_prop =
      config.GetProperty(BASE_STATIC_STRING(kOnlySelector));
  if (only_selector_prop.IsBool()) {
    only_selector_extreme_parsed_styles_ = only_selector_prop.Bool();
  }

  has_extreme_parsed_styles_ = true;
  *extreme_parsed_styles_ = parsed_styles.first;
  data_model()->set_css_variables_map(parsed_styles.second);
  MarkDirty(kDirtyStyle);
}

void Element::SetParsedStyles(StyleMap&& parsed_styles,
                              CSSVariableMap&& css_var) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_SET_PARSED_STYLES);
  has_extreme_parsed_styles_ = true;
  only_selector_extreme_parsed_styles_ = false;
  *extreme_parsed_styles_ = std::move(parsed_styles);
  data_model()->set_css_variables_map(std::move(css_var));
  MarkDirty(kDirtyStyle);
}

void Element::SetGestureDetector(const uint32_t gesture_id,
                                 GestureDetectorImpl gesture_detector) {
  data_model_->SetGestureDetector(gesture_id, gesture_detector);
  MarkDirty(kDirtyGesture);
}

void Element::RemoveGestureDetector(const uint32_t gesture_id) {
  data_model_->RemoveGestureDetector(gesture_id);
  MarkDirty(kDirtyGesture);
}

lepus::Value Element::GetComputedStyleByKey(const base::String& key) {
  auto property_id = CSSProperty::GetPropertyID(key);
  if (property_id == tasm::CSSPropertyID::kPropertyEnd) {
    return lepus::Value("");
  }

  return lepus::Value(
      ComputedCSSStyleCssTextHelper().GetComputedStyleByPropertyID(
          property_id, computed_css_style(), layout_result()));
}

bool Element::NeedFullFlushPath(CSSPropertyID id, const CSSValue& value) {
  return value.IsEmpty() || LayoutProperty::IsLayoutOnly(id) ||
         LayoutProperty::IsLayoutWanted(id) ||
         starlight::CSSStyleUtils::IsLayoutRelatedTransform(id, value) ||
         id == kPropertyIDColor || id == kPropertyIDFilter ||
         id == kPropertyIDBackgroundPosition;
}

void Element::OnPatchFinish(std::shared_ptr<PipelineOptions>& option) {
  element_manager_->OnPatchFinish(option, this);
}

void Element::FlushAnimatedStyleInternal(tasm::CSSPropertyID id,
                                         const tasm::CSSValue& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_FLUSH_ANIMATED_STYLE);
  auto trans_id = ConvertRtlCSSPropertyID(id).second;
  if (value != CSSValue()) {
    SetStyleInternal(trans_id, value);
  } else {
    ResetStyleInternal(trans_id);
  }
}

std::optional<CSSValue> Element::GetElementStyle(tasm::CSSPropertyID css_id) {
  auto iter = parsed_styles_map_.find(css_id);
  if (iter != parsed_styles_map_.end()) {
    return iter->second;
  }
  if (updated_inherited_styles_.has_value()) {
    iter = updated_inherited_styles_->find(css_id);
    if (iter != updated_inherited_styles_->end()) {
      return iter->second;
    }
  }
  return {};
}

const AttrMap& Element::GetAttributesForWorklet() {
  if (data_model() == nullptr) {
    static base::NoDestructor<AttrMap> kEmptyMap =
        base::NoDestructor<AttrMap>{};
    return *kEmptyMap;
  }
  return data_model()->attributes();
}

void Element::SetCSSID(int32_t id) {
  if (css_id_ != id) {
    ResetStyleSheet();
    css_id_ = id;
  }
}

}  // namespace tasm
}  // namespace lynx
