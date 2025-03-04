// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element.h"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/no_destructor.h"
#include "base/include/path_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/animation_delegate.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/bindings/jsi/java_script_element.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/feature_count/global_feature_counter.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

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
      css_patching_(this, manager),
      styles_manager_(
          this,
          manager ? manager->GetDynamicCSSConfigs()
                  : DynamicCSSConfigs::GetDefaultDynamicCSSConfigs(),
          manager ? manager->GetLynxEnvConfig().DefaultFontSize() : 0),
      id_(manager ? manager->GenerateElementID() : -1),
      node_index_(node_index),
      element_manager_(manager) {
  if (manager == nullptr) {
    return;
  }
  arch_type_ = manager->GetEnableFiberArch() ? FiberArch : RadonArch;
  enable_new_animator_ = IsFiberArch()
                             ? manager->GetEnableNewAnimatorForFiber()
                             : manager->GetEnableNewAnimatorForRadon();
  manager->node_manager()->Record(id_, this);

  catalyzer_ = manager->catalyzer();
  config_flatten_ = manager->GetPageFlatten();

  config_enable_layout_only_ = manager->GetEnableLayoutOnly();
  enable_new_fixed_ = manager->GetEnableFixedNew();

  const auto& env_config = manager->GetLynxEnvConfig();

  platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
      *manager->platform_computed_css());
  platform_css_style_->SetScreenWidth(env_config.ScreenWidth());
  platform_css_style_->SetViewportHeight(env_config.ViewportHeight());
  platform_css_style_->SetViewportWidth(env_config.ViewportWidth());
  platform_css_style_->SetCssAlignLegacyWithW3c(
      manager->GetLayoutConfigs().css_align_with_legacy_w3c_);
  platform_css_style_->SetFontScaleOnlyEffectiveOnSp(
      env_config.FontScaleSpOnly());
  platform_css_style_->SetFontSize(env_config.DefaultFontSize(),
                                   env_config.DefaultFontSize());
  styles_manager_.SetViewportSizeWhenInitialize(env_config);
  if (IsRadonArch()) {
    enable_extended_layout_only_opt_ =
        manager->GetEnableExtendedLayoutOnlyOpt();
    enable_component_layout_only_ = manager->GetEnableComponentLayoutOnly();
  }
}

// The copy constructor of the element is now only used for copying fiber
// elements. If you want to use it to copy radon elements, you need to check the
// copy constructor to determine if there are other additional member variables
// that need to be copied.
Element::Element(const Element& element, bool clone_resolved_props)
    : arch_type_(element.arch_type_),
      is_fixed_(element.is_fixed_),
      is_sticky_(element.is_sticky_),
      // Because is_fixed_ is false by default, if is_fixed_ is true, it means
      // that this element has the position:fixed attribute. In this case,
      // fixed_changed_ should be true, so that the final UI hierarchy can be
      // correct.
      fixed_changed_(element.is_fixed_),
      has_event_listener_(element.has_event_listener_),
      has_non_flatten_attrs_(element.has_non_flatten_attrs_),
      has_opacity_(element.has_opacity_),
      has_z_props_(element.has_z_props_),
      can_has_layout_only_children_(element.can_has_layout_only_children_),
      is_virtual_(element.is_virtual_),
      tag_(element.tag_),
      css_patching_(this, nullptr),
      has_layout_only_props_(element.has_layout_only_props_),
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
      subtree_need_update_(element.subtree_need_update_),
      frame_changed_(element.frame_changed_),
      is_layout_only_(element.is_layout_only_),
      is_text_(element.is_text_),
      is_inline_element_(element.is_inline_element_),
      is_list_item_(element.is_list_item_),
      direction_(element.direction_),
      overflow_(element.overflow_),
      has_placeholder_(element.has_placeholder_),
      trigger_global_event_(element.trigger_global_event_),
      styles_manager_(
          this, DynamicCSSConfigs::GetDefaultDynamicCSSConfigs(),
          element.element_manager()
              ? element.element_manager()->GetLynxEnvConfig().DefaultFontSize()
              : 0),
      id_(element.id_),
      node_index_(element.node_index_),
      enable_new_animator_(element.enable_new_animator_),
      global_bind_target_set_(element.global_bind_target_set_),
      animation_previous_styles_(element.animation_previous_styles_) {
  platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
      *(element.computed_css_style()));
  if (element.element_manager()) {
    styles_manager_.SetViewportSizeWhenInitialize(
        element.element_manager()->GetLynxEnvConfig());
  }
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
  config_enable_layout_only_ = manager->GetEnableLayoutOnly();
  catalyzer_ = manager->catalyzer();
  enable_new_fixed_ = manager->GetEnableFixedNew();

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

  css_patching_.SetElementManager(manager);
  css_patching_.SetEnableFiberArch(is_fiber_element());
  styles_manager_.SetViewportSizeWhenInitialize(manager->GetLynxEnvConfig());
  if (IsRadonArch()) {
    enable_extended_layout_only_opt_ =
        manager->GetEnableExtendedLayoutOnlyOpt();
    enable_component_layout_only_ = manager->GetEnableComponentLayoutOnly();
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

  // Create a static, empty GestureMap if the data model is not available.
  static base::NoDestructor<GestureMap> kEmptyGestureMap;
  return *kEmptyGestureMap;
}

// Sets a GestureDetector for the Element.
// This prepares the property bundle and sets the GestureDetector.
// Parameters:
//   key: The identifier for the GestureDetector.
//   detector: Pointer to the GestureDetector to set.
void Element::SetGestureDetector(const uint32_t key,
                                 GestureDetector* detector) {
  // Prepare the property bundle if needed before setting the GestureDetector.
  PreparePropBundleIfNeed();
  prop_bundle_->SetGestureDetector(*detector);
}

std::vector<float> Element::GetRectToLynxView() {
  return catalyzer_->GetRectToLynxView(this);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::UpdateLayout");
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
    sticky_positions_ = *sticky_positions;
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
                               const tasm::CSSValue& value, bool force_update) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, "Element::SetStyleInternal",
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
  bool is_layout_only = LayoutNode::IsLayoutOnly(css_id);

  bool need_layout = is_layout_only || LayoutNode::IsLayoutWanted(css_id);
  if (need_layout) {
    // Check fixed&sticky before layout only
    CheckFixedSticky(css_id, value);

    UpdateLayoutNodeStyle(css_id, value);

    if (element_manager_->GetEnableDumpElementTree()) {
      layout_styles_[css_id] = value;
    }
  }

  if (is_layout_only) {
    return;
  }

  // if the style is not layout only, it shall be resolved to prop_bundle

  // overflow is special: if overflow is visible can be treated as layout only
  // prop!
  if (css_id == kPropertyIDOverflow || css_id == kPropertyIDOverflowX ||
      css_id == kPropertyIDOverflowY) {
    CheckOverflow(css_id, value);
    // take care: overflow:visible is allowed to be layout only
    if (overflow() != OVERFLOW_XY) {
      has_layout_only_props_ = false;
    }
  } else {
    // such style is not layout only
    if (!enable_extended_layout_only_opt_ ||
        !IsExtendedLayoutOnlyProps(css_id)) {
      // currently, "text-align,direction" shall not make the layout only
      // optimization invalid!
      has_layout_only_props_ = false;
    }

    // do special check for transition, keyframe, z-index,etc.
    if (!(CheckTransitionProps(css_id) || CheckKeyframeProps(css_id) ||
          CheckZIndexProps(css_id, false))) {
#if OS_ANDROID
      // check flatten flag for Android platform if needed
      // FIXME(linxs): only Android need to check below props for flatten.
      // Normally, it's better to move below checks to Android platform side,
      // but checking in C++ size has a better performance
      CheckHasOpacityProps(css_id, false);
      CheckHasNonFlattenCSSProps(css_id);
#endif
    }
  }

  // resolve style and push to prop_bundle
  ResolveStyleValue(css_id, value, force_update);
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

void Element::ResetStyleInternal(CSSPropertyID css_id) {
  // Since the previous element styles cannot be accessed in element, we
  // need to record some necessary styles which New Animator transition needs.
  // TODO(wujintian): We only need to record layout-only properties, while other
  // properties can be accessed through ComputedCSSStyle.

  WillResetCSSValue(css_id);

  ResetCSSValue(css_id);
}

void Element::ResetCSSValue(CSSPropertyID css_id) {
  CheckDynamicUnit(css_id, CSSValue::Empty(), true);

  bool is_layout_only = LayoutNode::IsLayoutOnly(css_id);
  bool need_layout = is_layout_only || LayoutNode::IsLayoutWanted(css_id);
  if (need_layout) {
    ResetLayoutNodeStyle(css_id);
    if (element_manager_->GetEnableDumpElementTree()) {
      layout_styles_.erase(css_id);
    }
  }
  if (css_id == kPropertyIDPosition) {
    if (is_fixed_) {
      fixed_changed_ = true;
    }
    is_sticky_ = is_fixed_ = false;
  }
  if (is_layout_only) {
    return;
  }
  has_layout_only_props_ = false;
  computed_css_style()->ResetValue(css_id);

  CheckZIndexProps(css_id, true);

  // The properties of transition and keyframe no need to be pushed to bundle
  // separately here. Those properties will be pushed to bundle together
  // later.
  if (!(CheckTransitionProps(css_id) || CheckKeyframeProps(css_id))) {
    ResetProp(CSSProperty::GetPropertyName(css_id).c_str());
  }
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
      ResetFontSize();
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
    StylesManager().AdoptStyle(css_id, CSSValue::Empty());
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
  ResetProp(key.c_str());
}

void Element::WillConsumeAttribute(const base::String& key,
                                   const lepus::Value& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::WillConsumeAttribute");

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

void Element::SetDataSet(const tasm::DataMap& data) {
  PreparePropBundleIfNeed();
  lepus::Value datas_val(lepus::Dictionary::Create());
  for (const auto& pair : data) {
    datas_val.SetProperty(pair.first, pair.second);
  }
  prop_bundle_->SetProps("dataset", pub::ValueImplLepus(datas_val));
}

void Element::SetKeyframesByNames(const lepus::Value& names,
                                  const CSSKeyframesTokenMap& keyframes,
                                  bool force_flush) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::SetKeyframesByNames");
  auto lepus_keyframes = ResolveCSSKeyframesByNames(
      names, keyframes, computed_css_style()->GetMeasureContext(),
      element_manager()->GetCSSParserConfigs(), force_flush);
  if (!lepus_keyframes.IsTable() || lepus_keyframes.Table()->size() == 0) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::PushKeyframesToBundle");
  auto bundle = element_manager()->GetPropBundleCreator()->CreatePropBundle();
  bundle->SetProps("keyframes", pub::ValueImplLepus(lepus_keyframes));

  SetKeyframesByNamesInner(std::move(bundle));
}

void Element::SetKeyframesByNamesInner(std::unique_ptr<PropBundle> bundle) {
  painting_context()->SetKeyframes(std::move(bundle));
}

lepus::Value Element::ResolveCSSKeyframesByNames(
    const lepus::Value& names, const tasm::CSSKeyframesTokenMap& frames,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs, bool force_flush) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::ResolveCSSKeyframesByNames");
  DCHECK(names.IsString() || names.IsArray());
  auto dict = lepus::Dictionary::Create();
  ForEachLepusValue(
      names, [&dict, &context, &configs, &frames, this, &force_flush](
                 const lepus::Value& key, const lepus::Value& val) {
        if (val.IsString()) {
          auto val_str = val.String();
          auto keyframes_token_iter = frames.find(val_str.str());
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

void Element::ResetProp(const char* key) {
  PreparePropBundleIfNeed();
  prop_bundle_->SetNullProps(key);
}

// TODO: just so easy?
void Element::SetEventHandler(const base::String& name, EventHandler* handler) {
  PreparePropBundleIfNeed();
  prop_bundle_->SetEventHandler(handler->ToPubLepusValue());
  if (handler->name().IsEquals("attach") ||
      handler->name().IsEquals("detach")) {
    has_event_listener_ = true;
  }
  has_layout_only_props_ = false;
}

void Element::ResetEventHandlers() {
  if (prop_bundle_ != nullptr) {
    prop_bundle_->ResetEventHandler();
  }
  has_event_listener_ = false;
}

void Element::CreateElementContainer(bool platform_is_flatten) {
  element_container_ = std::make_unique<ElementContainer>(this);
  element_manager_->IncreaseElementCount();
  if (IsLayoutOnly()) {
    element_manager_->IncreaseLayoutOnlyElementCount();
    return;
  }

  painting_context()->CreatePaintingNode(id_, GetPlatformNodeTag().str(),
                                         prop_bundle_, platform_is_flatten,
                                         create_node_async_, node_index_);
}

void Element::UpdateElement() {
  if (!IsLayoutOnly()) {
    painting_context()->UpdatePaintingNode(impl_id(), TendToFlatten(),
                                           prop_bundle_);
  } else if (!CanBeLayoutOnly()) {
    // Is layout only and can not be layout only
    TransitionToNativeView();
  }
  element_container()->StyleChanged();
}

void Element::onNodeReload() { painting_context()->OnNodeReload(impl_id()); }

void Element::Animate(const lepus::Value& args) {
  // animate's args: operation, js_name, keyframes, animation_data.
  if (!args.IsArrayOrJSArray()) {
    LOGE("Element::Animate's para must be array");
    return;
  }
  if (args.GetLength() < 2) {
    LOGE("Element::Animate's para size must >= 2");
    return;
  }
  const auto& op = static_cast<piper::JavaScriptElement::AnimationOperation>(
      args.GetProperty(0).Int32());
  StyleMap styles;
  auto& parser_configs = element_manager()->GetCSSParserConfigs();
  switch (op) {
    case piper::JavaScriptElement::AnimationOperation::START: {
      if (args.GetLength() != 4) {
        LOGE("When start Element::Animate, the para size must be 4");
        return;
      }
      lepus::Value lepus_name;
      std::string animate_name;
      const auto& table = args.GetProperty(3).Table();
      // Since autoincrement keys causes the keyframes_map to overflow, we
      // remove them from keyframes_map when the last animation was overwritten.
      if (!will_removed_keyframe_name_.empty()) {
        if (enable_new_animator()) {
          auto iter = keyframes_map_.find(will_removed_keyframe_name_);
          if (iter != keyframes_map_.end()) {
            keyframes_map_.erase(iter);
          }
        } else {
          auto remove_name = lepus::Value(will_removed_keyframe_name_);
          auto bundle =
              element_manager()->GetPropBundleCreator()->CreatePropBundle();
          bundle->SetProps("removeKeyframe", pub::ValueImplLepus(remove_name));
          painting_context()->SetKeyframes(std::move(bundle));
        }
        will_removed_keyframe_name_.clear();
      }
      BASE_STATIC_STRING_DECL(kName, "name");
      auto iter = table->find(kName);
      if (iter == table->end()) {
        // If the user has not set animation_name, the system-generated
        // autoincrement key animation_name is used, and it is logged and
        // removed when overridden.
        animate_name = args.GetProperty(1).StdString();
        will_removed_keyframe_name_ = animate_name;
      } else {
        // If the user has set animation_name, it is used.
        animate_name = iter->second.StdString();
      }

      starlight::CSSStyleUtils::UpdateCSSKeyframes(
          keyframes_map_, animate_name, args.GetProperty(2), parser_configs);
      lepus_name = lepus::Value(animate_name);
      if (!enable_new_animator()) {
        // the unique_id may be the same but the keyframes content is different
        // when Animate trigger each time.
        SetKeyframesByNames(lepus_name, keyframes_map_, true);
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
    case piper::JavaScriptElement::AnimationOperation::PAUSE: {
      BASE_STATIC_STRING_DECL(kPaused, "paused");
      UnitHandler::Process(kPropertyIDAnimationPlayState, lepus::Value(kPaused),
                           styles, parser_configs);
      break;
    }
    case piper::JavaScriptElement::AnimationOperation::PLAY: {
      BASE_STATIC_STRING_DECL(kRunning, "running");
      UnitHandler::Process(kPropertyIDAnimationPlayState,
                           lepus::Value(kRunning), styles, parser_configs);
      break;
    }
    case piper::JavaScriptElement::AnimationOperation::CANCEL: {
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
  PipelineOptions options;
  element_manager_->OnFinishUpdateProps(this, options);
  OnPatchFinish(options);
}

void Element::PreparePropBundleIfNeed() {
  if (!prop_bundle_) {
    bool use_map_buffer = element_manager_->GetEnableUseMapBuffer();
    prop_bundle_ = element_manager()->GetPropBundleCreator()->CreatePropBundle(
        use_map_buffer);
  }
}

void Element::ResetPropBundle() {
  if (prop_bundle_) {
    pre_prop_bundle_ = prop_bundle_;
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
  css_patching_.ResolveStyle(new_styles, GetRelatedCSSFragment(),
                             changed_css_vars);
}

void Element::HandlePseudoElement() {
  css_patching_.HandlePseudoElement(GetRelatedCSSFragment());
}

void Element::HandleCSSVariables(StyleMap& styles) {
  css_patching_.HandleCSSVariables(styles);
}

void Element::ResolvePseudoSelectors() {
  css_patching_.ResolvePseudoSelectors();
}

void Element::ResolvePlaceHolder() { css_patching_.ResolvePlaceHolder(); }

bool Element::DisableFlattenWithOpacity() {
  return has_opacity_ && !is_text() && !is_image();
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
         overflow_ == OVERFLOW_HIDDEN &&
         computed_css_style()->HasBorderRadius();
}

bool Element::TendToFlatten() {
  return config_flatten_ && !has_event_listener_ && !has_non_flatten_attrs_ &&
         !DisableFlattenWithOpacity() &&
         !(has_z_props_ && !is_image() && !is_text()) && !is_inline_element_ &&
         !ShouldAvoidFlattenForView();
}

double Element::GetFontSize() { return computed_css_style()->GetFontSize(); }

double Element::GetParentFontSize() {
  if (!IsCSSInheritanceEnabled() || is_parallel_flush() ||
      parent() == nullptr) {
    return element_manager()->GetLynxEnvConfig().DefaultFontSize();
  }

  return parent()->GetFontSize();
}

double Element::GetRecordedRootFontSize() {
  return computed_css_style()->GetRootFontSize();
}

double Element::GetCurrentRootFontSize() {
  return element_manager()->root()->GetFontSize();
}

void Element::SetFontSize(const tasm::CSSValue* value) {
  styles_manager_.UpdateFontSizeStyle(value);
}

void Element::SetComputedFontSize(const tasm::CSSValue& value, double font_size,
                                  double root_font_size, bool force_update) {
  if (font_size != GetFontSize()) {
    NotifyUnitValuesUpdatedToAnimation(DynamicCSSStylesManager::kUpdateEm);
  }

  if (root_font_size != GetRecordedRootFontSize()) {
    NotifyUnitValuesUpdatedToAnimation(DynamicCSSStylesManager::kUpdateRem);
  }

  computed_css_style()->SetFontSize(font_size, root_font_size);
  UpdateLayoutNodeFontSize(font_size, root_font_size);
  if (!value.IsEmpty() || force_update) {
    ResolveStyleValue(kPropertyIDFontSize, value, force_update);
  }
}

void Element::ResetFontSize() {
  auto empty = CSSValue::Empty();
  styles_manager_.UpdateFontSizeStyle(&empty);
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

  if (check_key(key) || check_key_and_value(key, value) ||
      check_clip_radius(key, value)) {
    has_non_flatten_attrs_ = true;
  }
}

void Element::CheckOverflow(CSSPropertyID id, const tasm::CSSValue& value) {
#define CHECK_OVERFLOW_VAL(mask)                            \
  if ((starlight::OverflowType)value.GetValue().Number() == \
      starlight::OverflowType::kVisible) {                  \
    overflow_ |= (mask);                                    \
  } else {                                                  \
    overflow_ &= ~(mask);                                   \
  }

  switch (id) {
    case CSSPropertyID::kPropertyIDOverflow:
      CHECK_OVERFLOW_VAL(0x03)
      break;
    case CSSPropertyID::kPropertyIDOverflowX:
      CHECK_OVERFLOW_VAL(0x01)
      break;
    case CSSPropertyID::kPropertyIDOverflowY:
      CHECK_OVERFLOW_VAL(0x02)
      break;
    default:
      break;
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
  const auto& value_str = value.StdString();
  if (value_str.empty()) {
    return;
  }

  element_manager()->AppendTimingFlag(std::move(value_str));
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
  global_bind_target_set_.clear();
  if (value_str.empty()) {
    return;
  }
  constexpr const static char kDelimiter = ',';
  std::vector<std::string> id_targets;
  // multiple id split by comma delimiter
  base::SplitString(base::TrimString(value_str), kDelimiter, id_targets);
  for (auto& s : id_targets) {
    global_bind_target_set_.insert(base::TrimString(s));
  }
}

void Element::CheckHasOpacityProps(CSSPropertyID id, bool reset) {
  if (UNLIKELY(id == CSSPropertyID::kPropertyIDOpacity)) {
    has_opacity_ = !reset;
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

bool Element::CheckZIndexProps(CSSPropertyID id, bool reset) {
  if (!GetEnableZIndex()) return false;
  if (UNLIKELY(id == CSSPropertyID::kPropertyIDZIndex)) {
    has_z_props_ = !reset;
    return true;
  }
  return false;
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
  return element_manager()->root() == this || has_z_props_ || is_fixed_ ||
         computed_css_style()->HasTransform() ||
         computed_css_style()->HasOpacity();
}

bool Element::IsCSSInheritanceEnabled() const {
  return element_manager_ &&
         element_manager_->GetDynamicCSSConfigs().enable_css_inheritance_;
}

PaintingContext* Element::painting_context() {
  return catalyzer_->painting_context();
}

void Element::MarkLayoutDirty() { element_manager_->MarkLayoutDirty(id_); }

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

void Element::PreparePropsBundleForDynamicCSS() {
  if (!styles_manager_.UpdateWithParentStatus(parent())) {
    return;
  }
  for (auto& child : children_) {
    child->PreparePropsBundleForDynamicCSS();
  }
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

void Element::SetPlaceHolderStyles(const PseudoPlaceHolderStyles& styles) {
  report::GlobalFeatureCounter::Count(
      report::LynxFeature::CPP_ENABLE_PLACE_HOLDER_STYLE,
      element_manager_->GetInstanceId());
  styles_manager_.SetPlaceHolderStyle(styles);
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

bool Element::TickAllAnimation(fml::TimePoint& frame_time,
                               PipelineOptions& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::TickAllAnimation");

  if (css_transition_manager_ != nullptr) {
    css_transition_manager_->TickAllAnimation(frame_time);
  }
  if (css_keyframe_manager_ != nullptr) {
    css_keyframe_manager_->TickAllAnimation(frame_time);
  }
  bool has_layout_style = FlushAnimatedStyle();
  if (has_layout_style) {
    // if has_layout_style is true, should call `OnPatchFinish`.
    element_manager_->OnFinishUpdateProps(this, options);
  }
  return has_layout_style;
}

void Element::UpdateFinalStyleMap(const StyleMap& styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::UpdateFinalStyleMap");
  final_animator_map_.merge(styles);
}

bool Element::FlushAnimatedStyle() {
  if (final_animator_map_.empty()) {
    return false;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::FlushAnimatedStyle");
  bool has_layout_style = false;
  for (const auto& style : final_animator_map_) {
    if (NeedFastFlushPath(style)) {
      has_layout_style = true;
      break;
    }
  }
  std::shared_ptr<PropBundle> bundle;
  if (has_layout_style) {
    bundle = nullptr;
  } else if (prop_bundle_) {
    bundle = prop_bundle_;
  } else {
    bundle = element_manager()->GetPropBundleCreator()->CreatePropBundle();
  }

  bool has_render_style = false;
  for (const auto& style : final_animator_map_) {
    // Record previous before rtl-converter for transition.
    if (style.second != CSSValue::Empty()) {
      RecordElementPreviousStyle(style.first, style.second);
    } else {
      ResetElementPreviousStyle(style.first);
    }

    if (has_layout_style || !has_painting_node_) {
      FlushAnimatedStyleInternal(style.first, style.second);
    } else {
      // If it's a render property, push it to the temporary bundle.
      if (computed_css_style()->SetValue(style.first, style.second)) {
        auto property_name = CSSProperty::GetPropertyName(style.first).c_str();
        auto style_value = computed_css_style()->GetValue(style.first);
        has_render_style = true;

        switch (style.first) {
          case kPropertyIDTransform:
            bundle->SetProps(property_name, pub::ValueImplLepus(style_value));
            break;
          case kPropertyIDColor:
          case kPropertyIDBackgroundColor:
          case kPropertyIDBorderLeftColor:
          case kPropertyIDBorderRightColor:
          case kPropertyIDBorderTopColor:
          case kPropertyIDBorderBottomColor:
            bundle->SetProps(property_name,
                             static_cast<unsigned int>(style_value.Number()));
            break;
          case kPropertyIDOpacity:
            bundle->SetProps(property_name, style_value.Number());
            break;
          default:
            LOGE("[animation] unsupported animation value type for css:"
                 << style.first);
            break;
        }
      }
    }
  }
  if (has_render_style && !prop_bundle_) {
    // // Flush prop_bundle to PaintingNode for render value.
    HandleDelayTask([this, id = impl_id(), tend_to_flatten = TendToFlatten(),
                     bundle_ = bundle]() {
      painting_context()->UpdatePaintingNode(id, tend_to_flatten,
                                             std::move(bundle_));
      painting_context()->OnNodeReady(id);
    });
  }
  final_animator_map_.clear();
  return has_layout_style || !has_painting_node_;
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

CSSKeyframesToken* Element::GetCSSKeyframesToken(
    const std::string& animation_name) {
  tasm::CSSFragment* style_sheet = GetRelatedCSSFragment();
  if (style_sheet) {
    return style_sheet->GetKeyframesRule(animation_name);
  }
  return nullptr;
}

void Element::ResolveAndFlushKeyframes() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Element::ResolveAndFlushKeyframes");
  lepus_value animation_names =
      computed_css_style()->GetValue(kPropertyIDAnimationName);
  CSSFragment* css_fragment = GetRelatedCSSFragment();
  if (!animation_names.IsNil() && css_fragment &&
      !css_fragment->GetKeyframesRuleMap().empty()) {
    SetKeyframesByNames(animation_names, css_fragment->GetKeyframesRuleMap(),
                        false);
  }
}

void Element::EnsureTagInfo() {
  if (layout_node_type_ == kLayoutNodeTypeNotInit) {
    int32_t node_info = element_manager()->GetNodeInfoByTag(tag_);
    layout_node_type_ = (node_info & 0xFFFF);
    create_node_async_ = ((node_info & 0x10000) > 0);
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

}  // namespace tasm
}  // namespace lynx
