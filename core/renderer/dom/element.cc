// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <future>
#include <memory>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/float_comparison.h"
#include "base/include/no_destructor.h"
#include "base/include/path_utils.h"
#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/animation_delegate.h"
#include "core/event/event.h"
#include "core/renderer/css/computed_css_style_css_text_helper.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
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
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/events/touch_event_handler.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/js/bindings/java_script_element.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/runtime/lepus/bindings/style/shared_css_fragment_wrapper.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/feature_count/global_feature_counter.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/value_wrapper/value_impl_lepus.h"

#if ENABLE_LEPUSNG_WORKLET
#include "core/renderer/worklet/lepus_raf_handler.h"
#include "core/runtime/lepusng/napi/worklet/napi_func_callback.h"
#endif

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

void CollectDirtyNodeForList(int32_t impl_id, PipelineOptions* options) {
  if (impl_id != options->list_id_) {
    // Avoid adding the parent list node to updated_list_elements_ when
    // rendering a list item.
    options->updated_list_elements_.emplace_back(impl_id);
  }
}

void ApplyEventResult(fml::RefPtr<event::Event> event, EventResult result) {
  if (event == nullptr) {
    return;
  }
  if (static_cast<int>(result) &
      static_cast<int>(EventResult::kStopImmediatePropagationBit)) {
    event->set_is_stop_immediate_propagation(true);
  } else if (static_cast<int>(result) &
             static_cast<int>(EventResult::kStopPropagationBit)) {
    event->set_is_stop_propagation(true);
  }
}

event::EventListener::Options GetEventListenerOptions(
    const base::String& type) {
  const bool is_capture = type.str() == EVENT_TYPE_CAPTURE;
  const bool is_capture_catch = type.str() == EVENT_TYPE_CAPTURE_CATCH;
  const bool is_bubble_catch = type.str() == EVENT_TYPE_CATCH;
  const bool is_global_bind = type.str() == EVENT_TYPE_GLOBAL;
  return event::EventListener::Options(
      is_capture || is_capture_catch, false, false, false,
      is_capture_catch || is_bubble_catch, is_global_bind);
}

bool IsOverlayTag(const base::String& tag) {
  return tag.IsEquals("overlay") || tag.IsEquals("x-overlay-ng");
}

template <typename MapT>
bool OptionalMapNotEqual(const MapT* old_map, const MapT* new_map) {
  if ((old_map == nullptr) != (new_map == nullptr)) {
    return true;
  }
  if (old_map == nullptr) {
    return false;
  }
  return *old_map != *new_map;
}

bool CustomPropertiesChanged(const starlight::ComputedCSSStyle* old_style,
                             const starlight::ComputedCSSStyle& new_style) {
  if (old_style == nullptr) {
    return new_style.GetRawCustomProperties() != nullptr ||
           new_style.GetCustomProperties() != nullptr;
  }
  return OptionalMapNotEqual(old_style->GetRawCustomProperties(),
                             new_style.GetRawCustomProperties()) ||
         OptionalMapNotEqual(old_style->GetCustomProperties(),
                             new_style.GetCustomProperties());
}

void ApplyAnimationPropertyResetsToResolvedInputs(
    const starlight::ComputedCSSStyle& base_style,
    const std::vector<CSSPropertyID>& property_resets,
    StyleMap& resolved_style_map, CSSIDBitset& variable_dependent_ids) {
  if (property_resets.empty()) {
    return;
  }

  const auto& base_resolved_values = base_style.GetResolvedValues();
  for (const auto id : property_resets) {
    auto resolved_iter = resolved_style_map.find(id);
    if (resolved_iter == resolved_style_map.end()) {
      variable_dependent_ids.Reset(id);
      continue;
    }

    auto base_iter = base_resolved_values.find(id);
    if (base_iter == base_resolved_values.end() ||
        base_iter->second != resolved_iter->second) {
      resolved_style_map.erase(resolved_iter);
      variable_dependent_ids.Reset(id);
    }
  }
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

Element::Element(ElementManager* manager, const base::String& tag)
    : Element(manager, tag, kInvalidCssId) {}

Element::Element(ElementManager* manager, const base::String& tag,
                 int32_t css_id)
    : Element(tag, manager, kInvalidNodeIndex) {
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSTRUCTOR, "tag",
                      tag.c_str(), "id", id_);
  dirty_ = kDirtyCreated;
  css_id_ = css_id;
  InitLayoutBundle();
  SetAttributeHolder(fml::MakeRefCounted<AttributeHolder>(this));

  if (IsOverlay()) {
    can_has_layout_only_children_ = false;
  }

  if (manager == nullptr) {
    return;
  }

  // Set font scale and font size if needed.
  const auto& env_config = manager->GetLynxEnvConfig();

  computed_css_style()->SetFontScale(env_config.FontScale());
  if (Config::DefaultFontScale() != env_config.FontScale()) {
    SetComputedFontSize(env_config.PageDefaultFontSize(),
                        env_config.PageDefaultFontSize());
  }

  if (element_manager_->GetEnableStandardCSSSelector()) {
    // in new selector, mark style dirty while Created.
    MarkDirty(kDirtyStyle);
  }
}

Element::Element(const base::String& tag, ElementManager* manager,
                 uint32_t node_index)
    : tag_(tag),
      is_overlay_(IsOverlayTag(tag)),
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

// The copy constructor of the element is now only used for copying fiber
// elements. If you want to use it to copy radon elements, you need to check the
// copy constructor to determine if there are other additional member variables
// that need to be copied.
Element::Element(const Element& element, bool clone_resolved_props)
    : tag_(element.tag_),
      is_overlay_(element.is_overlay_),
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
      animation_previous_styles_(element.animation_previous_styles_),
      committed_underlying_layout_only_styles_for_new_pipeline_(
          element.committed_underlying_layout_only_styles_for_new_pipeline_),
      template_attributes_(element.template_attributes_) {
  if (element.base_css_style() != nullptr) {
    base_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
        *(element.base_css_style()));
  }
  platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
      *(element.computed_css_style()));
  element_entry_name_ = element.element_entry_name_;

  invalidation_lists_ = element.invalidation_lists_;
  parent_component_unique_id_ = element.parent_component_unique_id_;
  dirty_ = element.dirty_ | kDirtyCreated | kDirtyCloned;
  css_id_ = element.css_id_;
  dynamic_style_flags_ = element.dynamic_style_flags_;
  has_extreme_parsed_styles_ = element.has_extreme_parsed_styles_;
  only_selector_extreme_parsed_styles_ =
      element.only_selector_extreme_parsed_styles_;
  can_be_layout_only_ = element.can_be_layout_only_;
  is_template_ = element.is_template_;
  flush_required_ = element.flush_required_;
  full_raw_inline_style_ = element.full_raw_inline_style_;
  current_raw_inline_styles_ = element.current_raw_inline_styles_;
  current_raw_inline_custom_properties_ =
      element.current_raw_inline_custom_properties_;
  extreme_parsed_styles_ = element.extreme_parsed_styles_;
  inherited_styles_ = element.inherited_styles_;
  reset_inherited_ids_ = element.reset_inherited_ids_;
  custom_properties_ = element.custom_properties_;
  updated_attr_map_ = element.updated_attr_map_;
  builtin_attr_map_ = element.builtin_attr_map_;
  reset_attr_vec_ = element.reset_attr_vec_;
  part_id_ = element.part_id_;
  SetAttributeHolder(
      fml::MakeRefCounted<AttributeHolder>(*element.data_model()));
  data_model_->SetCSSVariableBundle(*element.data_model());

  if (clone_resolved_props) {
    parsed_styles_map_ = element.parsed_styles_map_;
    updated_inherited_styles_ = element.updated_inherited_styles_;
    layout_styles_ = element.layout_styles_;
    // clone_resolved_props only carries committed resolved state. The dynamic
    // source object is treated as a mutation carrier and will be rebuilt lazily
    // from parsed_dynamic_styles_map_ when a post-clone incremental update
    // happens.
    parsed_dynamic_styles_map_ = element.parsed_dynamic_styles_map_;

    // FIXME(wujintian): The prop bundle stores the style of incremental
    // updates. If the element flush props has been executed multiple times
    // before cloning the element, then this prop bundle cannot represent all
    // the stock styles since the element was created.
    if (element.pre_prop_bundle_) {
      prop_bundle_ = element.pre_prop_bundle_->ShallowCopy();
    } else if (element.prop_bundle_) {
      prop_bundle_ = element.prop_bundle_->ShallowCopy();
    }
  }

  if (element.config().IsTable() && element.config().GetLength() > 0) {
    config_ = lepus::Value::ShallowCopy(element.config()).Table();
  }

  // TODO(wujintian): Clone animation-related objects.
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

  const auto& env_config = manager->GetLynxEnvConfig();
  if (platform_css_style_ == nullptr) {
    platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
        *manager->platform_computed_css());
  }
  record_parent_font_size_ = env_config.PageDefaultFontSize();

  platform_css_style_->SetScreenWidth(env_config.ScreenWidth());
  platform_css_style_->SetViewportHeight(env_config.ViewportHeight());
  platform_css_style_->SetViewportWidth(env_config.ViewportWidth());
  platform_css_style_->SetCssAlignLegacyWithW3c(
      manager->GetLayoutConfigs().css_align_with_legacy_w3c_);
  platform_css_style_->SetFontScaleOnlyEffectiveOnSp(
      env_config.FontScaleSpOnly());
  platform_css_style_->SetLayoutUnit(env_config.PhysicalPixelsPerLayoutUnit(),
                                     env_config.LayoutsUnitPerPx());

  computed_css_style()->SetEnableZIndex(manager->GetEnableZIndex());

  InitLayoutBundle();
  UpdateLayoutNodeFontSize(GetFontSize(), GetRecordedRootFontSize());

  if (layout_styles_.has_value()) {
    for (auto& layout_style : *layout_styles_) {
      UpdateLayoutNodeStyle(layout_style.first, layout_style.second);
    }
  }

  SetFontSizeForAllElement(GetFontSize(), GetRecordedRootFontSize());

  if (Config::DefaultFontScale() != env_config.FontScale()) {
    computed_css_style()->SetFontScale(env_config.FontScale());
    SetComputedFontSize(env_config.PageDefaultFontSize(),
                        env_config.PageDefaultFontSize());
  }

  if (element_manager_->GetEnableStandardCSSSelector()) {
    MarkDirty(kDirtyStyle);
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

void Element::PerformElementContainerCreateOrUpdate(bool need_update,
                                                    bool need_reset) {
  PushStyleToBundle();

  if (dirty_ & kDirtyCreated) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_CRATE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // FIXME(linxs): FlushProps can be optimized, for example can we just
    // create viewElement,imageElement,textElement.. directly?
    FlushProps();
    dirty_ &= ~kDirtyCreated;
  } else if (need_update || dirty_ & kDirtyForceUpdate) {
    if (prop_bundle_) {
      UpdateLayoutNodeProps(prop_bundle_);

      if (!is_virtual()) {
        TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_UPDATE_FIBER_ELEMENT,
                    [this](lynx::perfetto::EventContext ctx) {
                      UpdateTraceDebugInfo(ctx.event());
                    });
        if (!IsLayoutOnly()) {
          TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_UPDATE_PAINTING_NODE,
                      [this](lynx::perfetto::EventContext ctx) {
                        UpdateTraceDebugInfo(ctx.event());
                      });
          element_container()->UpdatePaintingNode(TendToFlatten(),
                                                  prop_bundle_);
        } else if (!CanBeLayoutOnly()) {
          TRACE_EVENT(LYNX_TRACE_CATEGORY,
                      FIBER_ELEMENT_TRANSITION_TO_NATIVE_VIEW,
                      [this](lynx::perfetto::EventContext ctx) {
                        UpdateTraceDebugInfo(ctx.event());
                      });
          // Is layout only and can not be layout only
          TransitionToNativeView();
        }
      }
    }

    // TODO(songshourui.null): Later, determine whether to call StyleChanged
    // based on whether ComputedCSSStyle is dirty.
    HandleBeforeFlushActionsTask(
        [this]() { element_container()->StyleChanged(); },
        kFlagGreedyParallel | kFlagLevelOrderParallel);
  }
  dirty_ &= ~kDirtyForceUpdate;

  UpdateLayoutNodeByBundle();

  if (need_reset) {
    ResetPropBundle();
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

std::vector<float> Element::GetRectToScreen() {
  return catalyzer_->GetRectToScreen(this);
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

void Element::AppendPendingInvokeTask(base::closure task) {
  if (!task) {
    return;
  }
  pending_invoke_tasks_.emplace_back(std::move(task));
  MarkDirty(kDirtyInvoke);
}

void Element::EnqueueInvoke(
    const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  return catalyzer_->EnqueueInvoke(impl_id(), method, params, callback);
}

void Element::FlushPendingInvokeTasks() {
  if (!(dirty_ & kDirtyInvoke)) {
    return;
  }
  for (auto& task : pending_invoke_tasks_) {
    task();
  }
  pending_invoke_tasks_.clear();
  dirty_ &= ~kDirtyInvoke;
  element_container_->FlushImmediately();
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
                           float max_height, bool display_none) {
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
  display_none_ = display_none;
  UpdateStickyPosition(sticky_positions);
  MarkSubtreeNeedUpdate();
  NotifyElementSizeUpdated();
}

void Element::UpdateLayout(float left, float top) {
  top_ = top;
  left_ = left;
}

void Element::UpdateStickyPosition(
    const std::array<float, 4>* sticky_positions) {
  if (sticky_positions != nullptr) {
    for (size_t i = 0; i < sticky_positions->size(); ++i) {
      (*sticky_positions_)[i] = (*sticky_positions)[i];
    }
  } else if (element_manager()->GetEnableNewSticky()) {
    sticky_positions_.reset();
  }
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
  const bool was_fixed = is_fixed_;
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
    if (css_id == kPropertyIDPosition && was_fixed != is_fixed_) {
      UpdateFixedNodeSet();
    }
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
  if (css_id == kPropertyIDPosition && was_fixed != is_fixed_) {
    UpdateFixedNodeSet();
  }
}

bool Element::ResolveStyleValue(CSSPropertyID id, const tasm::CSSValue& value) {
  bool resolve_success = false;
  if (IsInheritable(id)) {
    computed_css_style()->SetResolvedValue(id, value);
  }
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
  EnsureLayoutBundle();
  allow_layoutnode_inline_ = parent->IsShadowNodeCustom();
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

void Element::EnsureSLNode() {
  if (EnableLayoutInElementMode() && sl_node_ == nullptr) {
    sl_node_ = std::make_unique<SLNode>(
        element_manager()->GetLayoutConfigs(),
        computed_css_style()->GetLayoutComputedStyle());
    if (is_page()) {
      MarkAsLayoutRoot();
    }
    OnLayoutObjectCreated();
  }
}

void Element::MarkAsLayoutRoot() {
  if (EnableLayoutInElementMode()) {
    EnsureSLNode();
    // The default flex direction is column for root.
    sl_node_->GetCSSMutableStyle()->SetFlexDirection(
        starlight::FlexDirectionType::kColumn);
    sl_node_->SetContext(element_manager());
    sl_node_->MarkDirty();
    sl_node_->SetSLRequestLayoutFunc([](void* context) {
      static_cast<ElementManager*>(context)->ScheduleLayout();
    });
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->is_root = true;
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

void Element::UpdateLayoutNodeByBundle() {
  if (EnableLayoutInElementMode()) {
    EnsureSLNode();
    return;
  }

  if (layout_bundle_ == nullptr) {
    return;
  }
  EnqueueLayoutTask([element_manager = element_manager(), id = impl_id(),
                     layout_bundle = std::move(layout_bundle_)]() mutable {
    element_manager->UpdateLayoutNodeByBundle(id, std::move(layout_bundle));
  });
  layout_bundle_ = nullptr;
}

bool Element::UsingTextService() const {
  return element_manager() && element_manager()->IsUsingTextService();
}

bool Element::EnableFragmentLayerRender() const {
  return element_manager() && element_manager()->IsFragmentLayerRenderModeOn();
}

void Element::CheckDynamicUnit(CSSPropertyID id, const CSSValue& value,
                               bool reset) {
  if (reset && parsed_styles_map_.empty()) {
    // TODO(linxs): try to clear dynamic_style_flags_ here
    dynamic_style_flags_ = 0;
    return;
  }

  dynamic_style_flags_ |= DynamicCSSStylesManager::GetValueFlags(
      id, value,
      element_manager()->GetDynamicCSSConfigs().unify_vw_vh_behavior_,
      element_manager()->FixFilterDynamicUpdateBug());
}

void Element::WillResetCSSValue(CSSPropertyID& css_id) {
  if (css_id == CSSPropertyID::kPropertyIDFontSize) {
    ResetFontSize();
  }

  // remove self inherit properties if needed
  if (inherited_styles_.has_value()) {
    auto it = inherited_styles_->find(css_id);
    if (it != inherited_styles_->end()) {
      inherited_styles_->erase(it);
      reset_inherited_ids_->emplace_back(css_id);
      children_propagate_inherited_styles_flag_ = true;
    }
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

bool Element::ResetCSSValue(CSSPropertyID css_id) {
  const bool was_fixed = is_fixed_;
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
    if (css_id == kPropertyIDPosition && was_fixed != is_fixed_) {
      UpdateFixedNodeSet();
    }
    return processed;
  }
  has_layout_only_props_ = false;
  processed = computed_css_style()->ResetValue(css_id);

  // The properties of transition and keyframe no need to be pushed to bundle
  // separately here. Those properties will be pushed to bundle together
  // later.
  CheckTransitionProps(css_id);
  CheckKeyframeProps(css_id);

  if (css_id == kPropertyIDPosition && was_fixed != is_fixed_) {
    UpdateFixedNodeSet();
  }
  return processed;
}

void Element::ApplySimpleStyleWithoutTail(const tasm::CSSPropertyID id,
                                          const tasm::CSSValue& value) {
  EXEC_EXPR_FOR_INSPECTOR(
      if (element_manager_ && element_manager_->IsDomTreeEnabled()) {
        if (value.IsEmpty()) {
          data_model()->ResetInlineStyle(id);
        } else {
          data_model()->SetInlineStyle(id, value);
        }
      });

  if (value.IsEmpty()) {
    if (id == kPropertyIDFontSize) {
      ResetFontSize();
    }
    ResetStyleInternal(id);
    return;
  }

  if (id == kPropertyIDFontSize) {
    SetFontSize(value);
    dirty_ &= ~kDirtyFontSize;
  } else {
    SetStyleInternal(id, value);
  }
}

void Element::ApplySimpleStylesWithoutTail(const tasm::StyleMap& style_map) {
  std::for_each(style_map.begin(), style_map.end(),
                [this](const auto& pair) -> void {
                  ApplySimpleStyleWithoutTail(pair.first, pair.second);
                });
}

void Element::ApplyDynamicSimpleStylesWithoutTail(
    const tasm::StyleMap& dynamic_style_map,
    const tasm::StyleMap& base_style_map) {
  std::for_each(dynamic_style_map.begin(), dynamic_style_map.end(),
                [this, &base_style_map](const auto& pair) -> void {
                  if (!pair.second.IsEmpty()) {
                    ApplySimpleStyleWithoutTail(pair.first, pair.second);
                    return;
                  }

                  // Empty values in the dynamic layer are tombstones: they stop
                  // the dynamic override and reveal the current static/base
                  // value, or fall back to default when base doesn't have it.
                  if (const auto it = base_style_map.find(pair.first);
                      it != base_style_map.end()) {
                    ApplySimpleStyleWithoutTail(pair.first, it->second);
                  } else {
                    ApplySimpleStyleWithoutTail(pair.first, CSSValue());
                  }
                });
}

void Element::HandleKeyframePropsChange() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_KEYFRAME_PROPS_CHANGE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!enable_new_animator()) {
    ResolveAndFlushKeyframes();
  } else {
    SetDataToNativeKeyframeAnimator();
  }
  has_keyframe_props_changed_ = false;
}

void Element::FinalizeSimpleStyleUpdate() {
  if (has_keyframe_props_changed_) {
    HandleDelayTask([this]() { HandleKeyframePropsChange(); });
    if (!enable_new_animator()) {
      PushToBundle(kPropertyIDAnimation);
    }
  }

  if (has_transition_props_changed_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_TRANSITION_PROPS,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    if (!enable_new_animator()) {
      PushToBundle(kPropertyIDTransition);
    } else {
      SetDataToNativeTransitionAnimator();
    }
    has_transition_props_changed_ = false;
  }
  EXEC_EXPR_FOR_INSPECTOR(
      element_manager()->OnElementNodeSetForInspector(this););
  MarkDirty(kDirtyForceUpdate);
}

void Element::FinalizeAnimationPropsChange(bool& need_update) {
  // Report when enableNewAnimator is the default value.
  if ((has_transition_props_changed_ || has_keyframe_props_changed_) &&
      !enable_new_animator()) {
    report::GlobalFeatureCounter::Count(
        report::LynxFeature::CPP_ENABLE_NEW_ANIMATOR_DEFAULT,
        element_manager()->GetInstanceId());
  }
  // keyframe props
  if (has_keyframe_props_changed_) {
    HandleDelayTask([this]() { HandleKeyframePropsChange(); });
    if (!enable_new_animator()) {
      PushToBundle(kPropertyIDAnimation);
    }
    need_update = true;
  }

  if (has_transition_props_changed_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_TRANSITION_PROPS,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    if (!enable_new_animator()) {
      PushToBundle(kPropertyIDTransition);
    } else {
      SetDataToNativeTransitionAnimator();
    }
    has_transition_props_changed_ = false;
    need_update = true;
  }
}

Element::AnimationPropertyChangeAnalysisForLegacyAnimator
Element::AnalyzeAnimationPropChangesForLegacyAnimator(
    const starlight::ComputedCSSStyle& final_style,
    const starlight::ComputedCSSStyle* previous_final_style,
    const StyleMap& resolved_style_map) const {
  AnimationPropertyChangeAnalysisForLegacyAnimator analysis;
  const bool is_first_render = IsNewlyCreated();
  auto analyze_property = [&analysis](CSSPropertyID id) {
    if (CSSProperty::IsTransitionProps(id)) {
      analysis.has_transition_props_changed = true;
    }
    if (CSSProperty::IsKeyframeProps(id)) {
      analysis.has_keyframe_props_changed = true;
    }
  };

  if (is_first_render) {
    for (const auto& [id, _] : resolved_style_map) {
      analyze_property(id);
    }
  } else {
    final_style.ForEachChangedProperty(analyze_property);
    final_style.ForEachResetProperty(analyze_property);
    // Computed animation/transition data changes are intentionally tracked
    // outside the generic platform dirty bits. Setting those dirty bits for
    // the new animator path would make both C++ animator and platform receive
    // the same animation update. This function is called only from the legacy
    // animator branch, where FinalizeAnimationPropsChange() owns bundle
    // writing.
    if (previous_final_style != nullptr) {
      const auto* new_animation_data = final_style.animation_data_or_null();
      const auto* old_animation_data =
          previous_final_style->animation_data_or_null();
      if ((new_animation_data == nullptr) != (old_animation_data == nullptr) ||
          (new_animation_data != nullptr && old_animation_data != nullptr &&
           *new_animation_data != *old_animation_data)) {
        analysis.has_keyframe_props_changed = true;
      }
      if (final_style.HasTransition() !=
              previous_final_style->HasTransition() ||
          (final_style.HasTransition() &&
           final_style.transition_data() !=
               previous_final_style->transition_data())) {
        analysis.has_transition_props_changed = true;
      }
    }
  }

  if (dirty_ & kDirtyCloned) {
    for (const auto& [id, _] : parsed_styles_map_) {
      analyze_property(id);
    }
  }

  return analysis;
}

void Element::NewPipelineStyleMutationPlan::AddUpdate(CSSPropertyID id,
                                                      const CSSValue& value) {
  update_values.insert_or_assign(id, value);
  update_ids.Set(id);
  reset_ids.Reset(id);
  source_changed = true;
}

void Element::NewPipelineStyleMutationPlan::AddReset(CSSPropertyID id) {
  update_values.erase(id);
  update_ids.Reset(id);
  reset_ids.Set(id);
  source_changed = true;
}

bool Element::NewPipelineStyleMutationPlan::HasOperations() const {
  return update_ids.HasAny() || reset_ids.HasAny();
}

bool Element::NewPipelineStyleMutationPlan::NeedsSemanticCommit() const {
  return source_changed || custom_properties_changed ||
         font_size_context_changed || root_font_size_context_changed;
}

void Element::UpdateSimpleStyles(tasm::StyleMap&& style_map) {
  parsed_styles_map_ = std::move(style_map);
  ApplySimpleStylesWithoutTail(parsed_styles_map_);
  FinalizeSimpleStyleUpdate();
}

void Element::UpdateStaticAndDynamicSimpleStyles(
    tasm::StyleMap&& style_map, tasm::StyleMap&& dynamic_style_map) {
  parsed_styles_map_ = std::move(style_map);
  if (dynamic_style_map.empty()) {
    parsed_dynamic_styles_map_.reset();
  } else {
    *parsed_dynamic_styles_map_ = std::move(dynamic_style_map);
  }

  ApplySimpleStylesWithoutTail(parsed_styles_map_);
  if (parsed_dynamic_styles_map_.has_value()) {
    ApplyDynamicSimpleStylesWithoutTail(*parsed_dynamic_styles_map_,
                                        parsed_styles_map_);
  }
  FinalizeSimpleStyleUpdate();
}

void Element::UpdateDynamicSimpleStyles(tasm::StyleMap&& style_map) {
  if (style_map.empty()) {
    parsed_dynamic_styles_map_.reset();
  } else {
    *parsed_dynamic_styles_map_ = std::move(style_map);
  }

  if (parsed_dynamic_styles_map_.has_value()) {
    ApplyDynamicSimpleStylesWithoutTail(*parsed_dynamic_styles_map_,
                                        parsed_styles_map_);
  }
  FinalizeSimpleStyleUpdate();
}

void Element::UpdateSimpleStyles(const tasm::StyleMap& style_map) {
  ApplySimpleStylesWithoutTail(style_map);
  FinalizeSimpleStyleUpdate();
}

void Element::ResetSimpleStyle(const tasm::CSSPropertyID id,
                               const tasm::CSSValue& value) {
  ApplySimpleStyleWithoutTail(id, value);
}

void Element::ResetSimpleStyle(const tasm::CSSPropertyID id) {
  ApplySimpleStyleWithoutTail(id, CSSValue());
}

void Element::ResolveSimpleStyles() {
  const bool static_dirty = (dirty_ & kDirtyStyleObjects) != 0;
  const bool dynamic_dirty = (dirty_ & kDirtyDynamicStyleObjects) != 0;
  if (!static_dirty && !dynamic_dirty) {
    return;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_STYLE_OBJECTS);
  if (element_manager_->EnablePropertyBasedSimpleStyle()) {
    StyleResolver::ResolveStyleObjectsBasedOnExistingMap(
        parsed_styles_map_, style_objects_ ? style_objects_.get() : nullptr,
        parsed_dynamic_styles_map_.has_value() ? &*parsed_dynamic_styles_map_
                                               : nullptr,
        dynamic_simple_object_.get(), static_dirty, dynamic_dirty, this);
  } else if (static_dirty) {
    DCHECK(!dynamic_dirty)
        << "dynamic simple style requires property-based simple style";
    StyleResolver::ResolveStyleObjects(
        last_style_objects_ ? last_style_objects_.get() : nullptr,
        style_objects_ ? style_objects_.get() : nullptr, this);
  }
  if (has_keyframe_props_changed_) {
    HandleDelayTask([this]() { HandleKeyframePropsChange(); });
  }
  dirty_ &= ~(kDirtyStyleObjects | kDirtyDynamicStyleObjects);
}

DynamicCSSStylesManager::StyleUpdateFlags
Element::CollectDynamicFlagsForNewPipeline(
    const StyleMap& resolved_style_map) const {
  DynamicCSSStylesManager::StyleUpdateFlags flags = 0;
  const auto& css_config = element_manager()->GetDynamicCSSConfigs();
  for (const auto& [id, value] : resolved_style_map) {
    flags |= DynamicCSSStylesManager::GetValueFlags(
        id, value, css_config.unify_vw_vh_behavior_,
        element_manager()->FixFilterDynamicUpdateBug());
  }
  return flags;
}

Element::AnimationSampleAnalysisForNewPipeline
Element::AnalyzeAnimationSampleForNewPipeline(
    const animation::AnimationSampleForNewPipeline& animation_sample) {
  AnimationSampleAnalysisForNewPipeline analysis;
  analysis.has_style_effects =
      !animation_sample.property_overrides.empty() ||
      !animation_sample.custom_property_overrides.empty() ||
      !animation_sample.property_resets.empty() ||
      !animation_sample.custom_property_resets.empty() ||
      animation_sample.requires_base_style_rebuild;
  analysis.has_animated_font_size =
      animation_sample.property_overrides.find(kPropertyIDFontSize) !=
          animation_sample.property_overrides.end() ||
      std::find(animation_sample.property_resets.begin(),
                animation_sample.property_resets.end(),
                kPropertyIDFontSize) != animation_sample.property_resets.end();
  analysis.has_custom_property_effects =
      !animation_sample.custom_property_overrides.empty() ||
      !animation_sample.custom_property_resets.empty();
  analysis.changes_resolve_context =
      analysis.has_animated_font_size || analysis.has_custom_property_effects ||
      animation_sample.requires_base_style_rebuild;
  return analysis;
}

std::unique_ptr<starlight::ComputedCSSStyle>
Element::BuildFinalStyleFromAnimationSampleForNewPipeline(
    const starlight::ComputedCSSStyle& base_style,
    const starlight::ComputedCSSStyle* parent_style,
    const starlight::ComputedCSSStyle* previous_final_style,
    const animation::AnimationSampleForNewPipeline& animation_sample,
    StyleMap& resolved_style_map, CSSIDBitset& variable_dependent_ids) {
  const auto sample_analysis =
      AnalyzeAnimationSampleForNewPipeline(animation_sample);
  const StyleMap* animated_property_overrides =
      animation_sample.property_overrides.empty()
          ? nullptr
          : &animation_sample.property_overrides;
  if (!sample_analysis.changes_resolve_context) {
    ApplyAnimationPropertyResetsToResolvedInputs(
        base_style, animation_sample.property_resets, resolved_style_map,
        variable_dependent_ids);
    return style_resolver_.BuildFinalStyleFromBaseFastPath(
        base_style, animated_property_overrides, &resolved_style_map,
        &variable_dependent_ids);
  }

  const CustomPropertiesMap* animated_custom_properties =
      animation_sample.custom_property_overrides.empty()
          ? nullptr
          : &animation_sample.custom_property_overrides;
  return style_resolver_.RebuildFinalStyleFromParent(
      parent_style, previous_final_style, animated_custom_properties,
      animated_property_overrides, &resolved_style_map,
      &variable_dependent_ids);
}

animation::AnimationEventRecordsForNewPipeline
Element::TakeAnimationEventsForNewPipeline() {
  animation::AnimationEventRecordsForNewPipeline event_records;
  auto append_event_records = [&event_records](auto* manager) {
    if (manager == nullptr) {
      return;
    }
    auto pending_event_records =
        manager->TakePendingAnimationEventsForNewPipeline();
    for (auto& event_record : pending_event_records) {
      event_records.push_back(std::move(event_record));
    }
  };
  append_event_records(css_keyframe_manager_.get());
  append_event_records(css_transition_manager_.get());
  return event_records;
}

bool Element::NeedsAnimationFrameForNewPipeline() const {
  if (!enable_new_animator_) {
    return false;
  }
  return (css_keyframe_manager_ != nullptr &&
          css_keyframe_manager_->NeedsFutureTickForNewPipeline()) ||
         (css_transition_manager_ != nullptr &&
          css_transition_manager_->NeedsFutureTickForNewPipeline());
}

Element::NewPipelineDynamicStyleInputs
Element::BuildDynamicStyleInputsForNewPipeline(
    const starlight::ComputedCSSStyle& final_style,
    const StyleMap& explicit_resolved_style_map) const {
  NewPipelineDynamicStyleInputs result;
  result.resolved_style_map = explicit_resolved_style_map;

  if (!IsCSSInheritanceEnabled()) {
    return result;
  }

  const auto explicit_style_ids =
      CSSIDBitset::FromKeys(explicit_resolved_style_map);
  const auto& css_config = element_manager()->GetDynamicCSSConfigs();
  for (const auto& [id, value] : final_style.GetResolvedValues()) {
    if (id == kPropertyIDFontSize || explicit_style_ids.Has(id) ||
        !IsInheritable(id)) {
      continue;
    }

    const auto value_flags = DynamicCSSStylesManager::GetValueFlags(
        id, value, css_config.unify_vw_vh_behavior_,
        element_manager()->FixFilterDynamicUpdateBug());
    if (value_flags == 0) {
      continue;
    }

    result.resolved_style_map.insert_or_assign(id, value);
    result.inherited_dynamic_ids.Set(id);
    result.inherited_dynamic_flags |= value_flags;
  }
  return result;
}

Element::NewPipelineStyleMutationPlan
Element::BuildNewPipelineStyleMutationPlan(
    const NewPipelineStyleResolveResult& resolved_styles,
    const NewPipelineDynamicStyleInputs& dynamic_inputs,
    DynamicCSSStylesManager::StyleUpdateFlags requested_dynamic_flags,
    bool first_render, double old_font_size, double old_root_font_size) const {
  NewPipelineStyleMutationPlan plan;
  plan.first_render = first_render;
  plan.source_style_ids =
      CSSIDBitset::FromKeys(resolved_styles.resolved_style_map);

  const auto* previous_final_style =
      first_render ? nullptr : resolved_styles.previous_final_style;
  const auto& current_values = resolved_styles.final_style->GetResolvedValues();
  static const StyleMap empty_style_map;
  const auto& previous_values = previous_final_style != nullptr
                                    ? previous_final_style->GetResolvedValues()
                                    : empty_style_map;

  if (previous_values.empty()) {
    for (const auto& [id, value] : current_values) {
      plan.AddUpdate(id, value);
    }
  } else if (current_values.empty()) {
    for (const auto& [id, _] : previous_values) {
      plan.AddReset(id);
    }
  } else {
    std::array<const CSSValue*, kCSSPropertyCount> previous_values_by_id{};
    for (const auto& [id, value] : previous_values) {
      DCHECK(CSSProperty::IsPropertyValid(id));
      previous_values_by_id[static_cast<size_t>(id)] = &value;
    }

    CSSIDBitset current_value_ids;
    for (const auto& [id, value] : current_values) {
      DCHECK(CSSProperty::IsPropertyValid(id));
      current_value_ids.Set(id);
      const auto* old_value = previous_values_by_id[static_cast<size_t>(id)];
      if (old_value == nullptr || *old_value != value) {
        plan.AddUpdate(id, value);
      }
    }

    for (const auto& [id, _] : previous_values) {
      if (!current_value_ids.Has(id)) {
        plan.AddReset(id);
      }
    }
  }

  plan.custom_properties_changed = CustomPropertiesChanged(
      previous_final_style, *resolved_styles.final_style);
  if (plan.custom_properties_changed) {
    for (const auto id : resolved_styles.variable_dependent_ids) {
      auto it = current_values.find(id);
      auto old_it = previous_values.find(id);
      if (it != current_values.end() &&
          (old_it == previous_values.end() || old_it->second != it->second)) {
        plan.AddUpdate(id, it->second);
      }
    }
  }

  plan.font_size_context_changed = base::FloatsNotEqual(
      old_font_size, resolved_styles.final_style->GetFontSize());
  plan.root_font_size_context_changed = base::FloatsNotEqual(
      old_root_font_size, resolved_styles.final_style->GetRootFontSize());

  auto effective_dynamic_flags = requested_dynamic_flags;
  if (plan.font_size_context_changed) {
    effective_dynamic_flags |= DynamicCSSStylesManager::kUpdateEm;
  }
  if (plan.root_font_size_context_changed) {
    effective_dynamic_flags |= DynamicCSSStylesManager::kUpdateRem;
  }

  if (effective_dynamic_flags != 0) {
    const auto& css_config = element_manager()->GetDynamicCSSConfigs();
    for (const auto& [id, value] : dynamic_inputs.resolved_style_map) {
      const auto value_flags = DynamicCSSStylesManager::GetValueFlags(
          id, value, css_config.unify_vw_vh_behavior_,
          element_manager()->FixFilterDynamicUpdateBug());
      if ((value_flags & effective_dynamic_flags) != 0) {
        plan.AddUpdate(id, value);
      }
    }
  }

  return plan;
}

bool Element::MaterializeNewPipelineStyleMutationPlan(
    const NewPipelineStyleMutationPlan& plan,
    const starlight::ComputedCSSStyle& baseline_style,
    starlight::ComputedCSSStyle& final_style) const {
  auto should_materialize_dirty_bit = [this](CSSPropertyID id) {
    // Layout-only changes are replayed to layout bundles from the mutation
    // plan. Generic style dirty bits are consumed by the platform prop bundle.
    // In layout-in-element mode, the legacy pipeline also wrote supported
    // layout-only properties through ComputedCSSStyle, which could create an
    // empty prop bundle and drive UpdatePaintingNode().
    return !LayoutProperty::IsLayoutOnly(id) || EnableLayoutInElementMode();
  };
  const auto& baseline_context = baseline_style.GetMeasureContext();
  starlight::ComputedCSSStyle replay_style(
      baseline_context.layouts_unit_per_px_,
      baseline_context.physical_pixels_per_layout_unit_);
  replay_style.CopyFrom(baseline_style);
  replay_style.ClearDirtyBits();

  if (plan.reset_ids.Has(kPropertyIDFontSize)) {
    replay_style.SetFontSize(final_style.GetFontSize(),
                             final_style.GetRootFontSize());
    replay_style.ResetValue(kPropertyIDFontSize);
  }
  if (plan.update_ids.Has(kPropertyIDFontSize)) {
    auto it = plan.update_values.find(kPropertyIDFontSize);
    if (it != plan.update_values.end()) {
      replay_style.SetFontSize(final_style.GetFontSize(),
                               final_style.GetRootFontSize());
      replay_style.SetValue(kPropertyIDFontSize, it->second);
    }
  }

  for (const auto id : plan.reset_ids) {
    if (id == kPropertyIDFontSize || !should_materialize_dirty_bit(id)) {
      continue;
    }
    replay_style.ResetValue(id);
  }

  for (const auto id : plan.update_ids) {
    if (id == kPropertyIDFontSize || !should_materialize_dirty_bit(id)) {
      continue;
    }
    auto it = plan.update_values.find(id);
    if (it != plan.update_values.end()) {
      replay_style.SetValue(id, it->second);
    }
  }

  final_style.CopyDirtyBitsFrom(replay_style);
  return final_style.IsDirty();
}

bool Element::HasInheritedPropertyMutation(
    const NewPipelineStyleMutationPlan& plan) const {
  const auto& configs = element_manager()->GetDynamicCSSConfigs();
  const auto& inheritable_props =
      !configs.custom_inherit_list_.empty()
          ? configs.custom_inherit_list_
          : DynamicCSSStylesManager::GetInheritableProps();
  bool result = false;
  auto check_property = [&result, &inheritable_props](const auto id) {
    result |= id != kPropertyIDFontSize &&
              inheritable_props.find(id) != inheritable_props.end();
  };
  for (const auto id : plan.update_ids) {
    check_property(id);
  }
  for (const auto id : plan.reset_ids) {
    check_property(id);
  }
  return result;
}

void Element::ReplayMaterializedStyleSideEffects(
    const starlight::ComputedCSSStyle& computed_style,
    CSSIDBitset* replayed_ids, const NewPipelineStyleMutationPlan* plan) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              FIBER_ELEMENT_REPLAY_MATERIALIZED_STYLE_SIDE_EFFECTS);
  auto mark_replayed = [replayed_ids](CSSPropertyID id) {
    if (replayed_ids != nullptr) {
      replayed_ids->Set(id);
    }
  };
  auto replay_mode = [this, plan](CSSPropertyID id) {
    return plan != nullptr && ShouldPreserveLayoutOnlyForInheritedPlatformStyle(
                                  id, plan->source_style_ids)
               ? StyleSideEffectReplayMode::kPreserveLayoutOnly
               : StyleSideEffectReplayMode::kNormal;
  };

  computed_style.ForEachResetProperty([&](const auto id) {
    ReplayResetStyleSideEffect(id, replay_mode(id));
    mark_replayed(id);
  });

  const auto& resolved_values = computed_style.GetResolvedValues();
  computed_style.ForEachChangedProperty([&](const auto id) {
    if (id == kPropertyIDFontSize) {
      return;
    }
    auto it = resolved_values.find(id);
    if (it == resolved_values.end()) {
      return;
    }
    ReplayChangedStyleSideEffect(id, it->second, replay_mode(id));
    mark_replayed(id);
  });
}

void Element::ReplayNewPipelineStyleMutationPlanSideEffects(
    const NewPipelineStyleMutationPlan& plan, CSSIDBitset* replayed_ids) {
  auto already_replayed = [replayed_ids](CSSPropertyID id) {
    return replayed_ids != nullptr && replayed_ids->Has(id);
  };
  auto mark_replayed = [replayed_ids](CSSPropertyID id) {
    if (replayed_ids != nullptr) {
      replayed_ids->Set(id);
    }
  };

  for (const auto id : plan.reset_ids) {
    if (already_replayed(id)) {
      continue;
    }
    const auto mode = ShouldPreserveLayoutOnlyForInheritedPlatformStyle(
                          id, plan.source_style_ids)
                          ? StyleSideEffectReplayMode::kPreserveLayoutOnly
                          : StyleSideEffectReplayMode::kNormal;
    ReplayResetStyleSideEffect(id, mode);
    mark_replayed(id);
  }

  for (const auto& [id, value] : plan.update_values) {
    if (already_replayed(id) || id == kPropertyIDFontSize) {
      continue;
    }
    const auto mode = ShouldPreserveLayoutOnlyForInheritedPlatformStyle(
                          id, plan.source_style_ids)
                          ? StyleSideEffectReplayMode::kPreserveLayoutOnly
                          : StyleSideEffectReplayMode::kNormal;
    ReplayChangedStyleSideEffect(id, value, mode);
    mark_replayed(id);
  }
}

void Element::ReplayDynamicResolvedStyleSideEffects(
    const StyleMap& resolved_style_map,
    DynamicCSSStylesManager::StyleUpdateFlags update_flags,
    const CSSIDBitset& replayed_ids, const CSSIDBitset* source_style_ids,
    const CSSIDBitset* inherited_dynamic_ids) {
  if (update_flags == 0) {
    return;
  }
  const auto& css_config = element_manager()->GetDynamicCSSConfigs();
  for (const auto& [id, value] : resolved_style_map) {
    if (replayed_ids.Has(id) || id == kPropertyIDFontSize ||
        CSSProperty::IsTransitionProps(id) ||
        CSSProperty::IsKeyframeProps(id)) {
      continue;
    }
    const auto value_flags = DynamicCSSStylesManager::GetValueFlags(
        id, value, css_config.unify_vw_vh_behavior_,
        element_manager()->FixFilterDynamicUpdateBug());
    if ((value_flags & update_flags) == 0) {
      continue;
    }
    const bool preserve_layout_only =
        source_style_ids != nullptr && inherited_dynamic_ids != nullptr &&
        inherited_dynamic_ids->Has(id) &&
        ShouldPreserveLayoutOnlyForInheritedPlatformStyle(id,
                                                          *source_style_ids);
    ReplayChangedStyleSideEffect(
        id, value,
        preserve_layout_only ? StyleSideEffectReplayMode::kPreserveLayoutOnly
                             : StyleSideEffectReplayMode::kNormal);
  }
}

void Element::ReplayChangedStyleSideEffect(CSSPropertyID id,
                                           const CSSValue& value,
                                           StyleSideEffectReplayMode mode) {
  CheckDynamicUnit(id, value, false);
  MarkLayoutInElementTextMeasurerPropertyIfNeeded(id);
  const bool preserve_layout_only =
      mode == StyleSideEffectReplayMode::kPreserveLayoutOnly;

  const bool is_layout_only = LayoutProperty::IsLayoutOnly(id);
  const bool need_layout = is_layout_only || LayoutProperty::IsLayoutWanted(id);
  if (need_layout) {
    const bool was_fixed = is_fixed_;
    CheckFixedSticky(id, value);
    if (id == kPropertyIDPosition && was_fixed != is_fixed_) {
      UpdateFixedNodeSet();
    }
    UpdateLayoutNodeStyle(id, value);
    if (element_manager()->GetEnableDumpElementTree()) {
      (*layout_styles_)[id] = value;
    }
  }

  if (is_layout_only) {
    if (EnableLayoutInElementMode()) {
      RequestLayout();
    }
    return;
  }

  ReplayElementSpecificStyleSideEffect(id);

  if (id == kPropertyIDOverflow || id == kPropertyIDOverflowX ||
      id == kPropertyIDOverflowY) {
    if (!preserve_layout_only && !computed_css_style()->IsOverflowXY()) {
      has_layout_only_props_ = false;
    }
    return;
  }

  if (!preserve_layout_only &&
      (!enable_extended_layout_only_opt_ || !IsExtendedLayoutOnlyProps(id))) {
    has_layout_only_props_ = false;
  }
  if (CSSProperty::IsTransitionProps(id) || CSSProperty::IsKeyframeProps(id)) {
    has_non_flatten_attrs_ = true;
  } else {
    CheckHasNonFlattenCSSProps(id);
  }
}

void Element::ReplayResetStyleSideEffect(CSSPropertyID id,
                                         StyleSideEffectReplayMode mode) {
  MarkLayoutInElementTextMeasurerPropertyIfNeeded(id);
  if (id == kPropertyIDFontSize) {
    return;
  }
  const bool preserve_layout_only =
      mode == StyleSideEffectReplayMode::kPreserveLayoutOnly;

  const bool is_layout_only = LayoutProperty::IsLayoutOnly(id);
  const bool need_layout = is_layout_only || LayoutProperty::IsLayoutWanted(id);
  if (need_layout) {
    ResetLayoutNodeStyle(id);
    if (element_manager()->GetEnableDumpElementTree() &&
        layout_styles_.has_value()) {
      layout_styles_->erase(id);
    }
    if (is_layout_only && EnableLayoutInElementMode()) {
      RequestLayout();
    }
  }

  if (id == kPropertyIDPosition) {
    const bool was_fixed = is_fixed_;
    if (was_fixed) {
      fixed_changed_ = true;
    }
    is_sticky_ = false;
    is_fixed_ = false;
    if (was_fixed) {
      UpdateFixedNodeSet();
    }
  }

  if (is_layout_only) {
    return;
  }

  ReplayElementSpecificStyleSideEffect(id);

  if (!preserve_layout_only) {
    has_layout_only_props_ = false;
  }
  if (CSSProperty::IsTransitionProps(id) || CSSProperty::IsKeyframeProps(id)) {
    has_non_flatten_attrs_ = true;
  }
}

bool Element::ShouldPreserveLayoutOnlyForInheritedPlatformStyle(
    CSSPropertyID id, const CSSIDBitset& source_style_ids) {
  // Legacy inheritance consumes platform text props through InheritValue(),
  // which avoids the normal non-layout style path that invalidates layout-only.
  const bool layout_only_relevant = can_be_layout_only_ || is_layout_only_;
  return layout_only_relevant && IsCSSInheritanceEnabled() &&
         GetParentComputedCSSStyle() != nullptr && IsInheritable(id) &&
         starlight::ComputedCSSStyle::IsPlatformInheritableProperty(id) &&
         !source_style_ids.Has(id);
}

void Element::CommitFontContext(
    const starlight::ComputedCSSStyle& computed_style, double old_font_size,
    double old_root_font_size) {
  const auto new_font_size = computed_style.GetFontSize();
  const auto new_root_font_size = computed_style.GetRootFontSize();

  if (base::FloatsNotEqual(old_font_size, new_font_size)) {
    NotifyUnitValuesUpdatedToAnimation(DynamicCSSStylesManager::kUpdateEm);
  }
  if (base::FloatsNotEqual(old_root_font_size, new_root_font_size)) {
    NotifyUnitValuesUpdatedToAnimation(DynamicCSSStylesManager::kUpdateRem);
  }

  SetFontSizeForAllElement(new_font_size, new_root_font_size);
  UpdateLayoutNodeFontSize(new_font_size, new_root_font_size);
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
    if (ShouldUseLegacyTransitionInterception() && css_transition_manager_ &&
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
        if (name == PlatformEventPropName::kEventThrough ||
            name == PlatformEventPropName::kEventThroughActiveRegions ||
            name == PlatformEventPropName::kEventsPassThrough) {
          fragment->SetEventProp(name, lepus::Value());
        } else {
          fragment->SetEventProp(name, lepus::Value(0));
        }
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
      child->MarkStyleDirty(recursive);
    }
  }
}

// If a child's related CSS variable is updated, invalidate the child's style.
void Element::RecursivelyMarkChildrenCSSVariableDirty(
    const lepus::Value& css_variable_updated) {
  for (const auto& child : scoped_children_) {
    if (IsCSSInlineVariablesEnabled()) {
      // Entering RecursivelyMarkChildrenCSSVariableDirty means current
      // element's CSS variables changed.
      // Mark children's custom_properties dirty so CollectCustomProperties can
      // pick up the latest values.
      child->MarkCustomPropertiesDirty();
    }
    if (child->is_raw_text()) {
      continue;
    }
    lepus::Value css_variable_updated_merged = css_variable_updated;
    // First, merge changing_css_variables with element's css_variable.
    // Element's css_variable has higher priority.
    child->data_model()->MergeWithCSSVariables(css_variable_updated_merged);
    if (IsRelatedCSSVariableUpdated(child->data_model(),
                                    css_variable_updated_merged)) {
      child->MarkStyleDirty(false);
    }
    child->RecursivelyMarkChildrenCSSVariableDirty(css_variable_updated_merged);
  }
}

void Element::RecursivelyMarkCustomPropertiesDirty() {
  for (const auto& child : scoped_children_) {
    if (!child->is_raw_text()) {
      child->MarkStyleDirty(false);
    }
    child->RecursivelyMarkCustomPropertiesDirty();
  }
}

void Element::MarkFontSizeInvalidateRecursively() {
  MarkDirty(kDirtyFontSize);
  auto* child = first_render_child_;
  while (child) {
    child->MarkFontSizeInvalidateRecursively();
    child = child->next_render_sibling_;
  }
}

void Element::InvalidateChildrenFontSizeRecursively() {
  auto* child = first_render_child_;
  while (child) {
    child->MarkFontSizeInvalidateRecursively();
    child = child->next_render_sibling_;
  }
}

void Element::InvalidateChildrenInheritedStylesRecursively() {
  for (const auto& child : scoped_children_) {
    child->ApplyFunctionRecursive([](Element* element) {
      if (!element->is_raw_text()) {
        element->MarkDirtyLite(kDirtyPropagateInherited);
      }
    });
  }
}

void Element::MarkDirectChildrenStyleDirtyForInheritedPropertyMutation() {
  for (const auto& child : scoped_children_) {
    if (!child->is_raw_text()) {
      child->MarkStyleDirty(false);
    }
  }
}

void Element::SetStyleObjects(
    std::unique_ptr<style::StyleObject*, style::StyleObjectArrayDeleter>
        style_objects) {
  last_style_objects_ = std::move(style_objects_);

  style_objects_ = std::move(style_objects);

  MarkDirty(kDirtyForceUpdate | kDirtyStyleObjects);
}

void Element::ReplaceDynamicSimpleStyles(
    style::DynamicStyleObjectRef new_style_object) {
  const bool has_committed_dynamic = parsed_dynamic_styles_map_.has_value() &&
                                     !parsed_dynamic_styles_map_->empty();
  // Pure no-op: no incoming source, no current source, and no committed
  // dynamic state to clear.
  if (!new_style_object && !dynamic_simple_object_ && !has_committed_dynamic) {
    return;
  }

  dynamic_simple_object_ = std::move(new_style_object);
  MarkDirty(kDirtyForceUpdate | kDirtyDynamicStyleObjects);
}

void Element::AddDynamicSimpleStyles(tasm::StyleMap&& new_styles) {
  if (new_styles.empty()) {
    return;
  }

  if (!dynamic_simple_object_ && parsed_dynamic_styles_map_.has_value() &&
      !parsed_dynamic_styles_map_->empty()) {
    // A resolved-only clone does not keep the dynamic mutation carrier.
    // Rebuild it from the committed resolved dynamic map on the first
    // post-clone mutation so incremental updates keep previous dynamic state.
    dynamic_simple_object_ =
        style::CreateDynamicStyleObjectRef(*parsed_dynamic_styles_map_);
  }

  if (!dynamic_simple_object_) {
    dynamic_simple_object_ =
        style::CreateDynamicStyleObjectRef(std::move(new_styles));
    MarkDirty(kDirtyForceUpdate | kDirtyDynamicStyleObjects);
    return;
  }

  dynamic_simple_object_->MergeStyleMap(std::move(new_styles));
  MarkDirty(kDirtyForceUpdate | kDirtyDynamicStyleObjects);
}

void Element::RemoveDynamicSimpleStyleKV(tasm::CSSPropertyID id) {
  if (!dynamic_simple_object_ && parsed_dynamic_styles_map_.has_value() &&
      !parsed_dynamic_styles_map_->empty()) {
    dynamic_simple_object_ =
        style::CreateDynamicStyleObjectRef(*parsed_dynamic_styles_map_);
  }
  if (!dynamic_simple_object_) {
    return;
  }

  if (!dynamic_simple_object_->RemoveStyleValue(id)) {
    return;
  }

  if (dynamic_simple_object_->Properties().empty()) {
    dynamic_simple_object_ = nullptr;
  }
  MarkDirty(kDirtyForceUpdate | kDirtyDynamicStyleObjects);
}

void Element::AddDynamicSimpleStyleKV(tasm::CSSPropertyID id,
                                      tasm::CSSValue&& value) {
  if (!dynamic_simple_object_ && parsed_dynamic_styles_map_.has_value() &&
      !parsed_dynamic_styles_map_->empty()) {
    dynamic_simple_object_ =
        style::CreateDynamicStyleObjectRef(*parsed_dynamic_styles_map_);
  }

  if (!dynamic_simple_object_) {
    StyleMap dynamic_styles;
    dynamic_styles.insert_or_assign(id, std::move(value));
    dynamic_simple_object_ =
        style::CreateDynamicStyleObjectRef(std::move(dynamic_styles));
    MarkDirty(kDirtyForceUpdate | kDirtyDynamicStyleObjects);
    return;
  }

  dynamic_simple_object_->UpdateStyleMap(id, std::move(value));
  MarkDirty(kDirtyForceUpdate | kDirtyDynamicStyleObjects);
}

// TODO(wujintian) : Perhaps we can provide an rvalue version of the API to
// achieve better performance. However, this would result in the need to
// maintain two versions of the code: one for lvalues and one for rvalues.
void Element::SetClass(const base::String& clazz) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_CLASS);
  data_model_->SetClass(clazz);
  MarkStyleDirty(NeedForceClassChangeTransmit());
}

void Element::OnClassChanged(const ClassList& old_classes,
                             const ClassList& new_classes) {
  if (element_manager() && element_manager()->GetEnableStandardCSSSelector()) {
    if (element_manager()->CSSFragmentParsingOnTASMWorkerMTSRender()) {
      element_manager()->GetTasmWorkerTaskRunner()->PostTask(
          [this, old_classes_ = old_classes, new_classes_ = new_classes]() {
            CheckHasInvalidationForClass(old_classes_, new_classes_);
          });
    } else {
      CheckHasInvalidationForClass(old_classes, new_classes);
    }
  }
}

bool Element::CheckHasInvalidationForClass(const ClassList& old_classes,
                                           const ClassList& new_classes) {
  auto* css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (!css_fragment || !css_fragment->enable_css_invalidation()) {
    return false;
  }
  auto old_size = invalidation_lists_.descendants.size();
  CSSFragment::CollectClassChangedInvalidation(
      css_fragment, invalidation_lists_, old_classes, new_classes);
  return invalidation_lists_.descendants.size() != old_size;
}

bool Element::CheckHasInvalidationForId(const std::string& old_id,
                                        const std::string& new_id) {
  auto* css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (!css_fragment || !css_fragment->enable_css_invalidation()) {
    return false;
  }
  auto old_size = invalidation_lists_.descendants.size();
  CSSFragment::CollectIdChangedInvalidation(css_fragment, invalidation_lists_,
                                            old_id, new_id);
  return invalidation_lists_.descendants.size() != old_size;
}

void Element::InvalidateChildren(css::InvalidationSet* invalidation_set) {
  if (invalidation_set->WholeSubtreeInvalid() || !invalidation_set->IsEmpty()) {
    VisitChildren([invalidation_set](Element* child) {
      if (!child->StyleDirty() && !child->is_raw_text() &&
          invalidation_set->InvalidatesElement(*child->data_model())) {
        child->MarkStyleDirty(false);
      }
    });
  }
}

void Element::InvalidateChildrenIfNeeded() {
  for (auto* invalidation_set : invalidation_lists_.descendants) {
    InvalidateChildren(invalidation_set);
  }
  invalidation_lists_.descendants.clear_and_shrink();
}

void Element::VisitChildren(
    const base::MoveOnlyClosure<void, Element*>& visitor) {
  for (auto& child : scoped_children_) {
    auto* child_element = child.get();
    // In fiber mode, we skip the children in component
    if (!child_element->is_component()) {
      visitor(child_element);
      child_element->VisitChildren(visitor);
    }
  }
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

void Element::RemoveDataset(const base::String& key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_DATA_SET);

  if (data_model_->RemoveDataSet(key)) {
    MarkDirty(kDirtyDataset);
  }
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

void Element::SetWorkletEventHandler(const base::String& name,
                                     const base::String& type,
                                     const lepus::Value& worklet_info,
                                     const std::string& context_name) {
  auto* context = element_manager_ == nullptr
                      ? nullptr
                      : element_manager_->GetEntryRuntime(context_name);
  SetWorkletEventHandler(name, type, worklet_info, context);
}

event::DispatchEventResult Element::DispatchMessageEvent(
    fml::RefPtr<runtime::MessageEvent> event) {
  auto* delegate = element_manager_ == nullptr
                       ? nullptr
                       : element_manager_->element_manager_delegate();
  if (delegate == nullptr) {
    return {event::EventCancelType::kNotCanceled, false};
  }
  return delegate->DispatchMessageEvent(std::move(event));
}

void Element::RemoveEvent(const base::String& name, const base::String& type) {
  data_model_->RemoveEvent(name, type);
  MarkDirty(kDirtyEvent);
}

void Element::RemoveAllEvents() {
  data_model_->RemoveAllEvents();
  MarkDirty(kDirtyEvent);
}

void Element::FiberAddEvent(const base::String& type, const base::String& name,
                            const lepus::Value& callback,
                            const std::string& context_name) {
  auto event_options = GetEventListenerOptions(type);
  auto event_name = name.str();
  auto* manager = element_manager();
  const bool should_sync_listener =
      manager != nullptr && manager->EnableEventHandleRefactor();
  auto remove_event_listener =
      [this, &event_name,
       &event_options](event::ClosureEventListener::ClosureType closure_type) {
        RemoveEventListener(
            event_name, std::make_unique<event::ClosureEventListener>(
                            [](lepus::Value) {}, event_options, closure_type));
      };

  if (callback.IsEmpty()) {
    RemoveEvent(name, type);
    if (should_sync_listener) {
      remove_event_listener(event::ClosureEventListener::ClosureType::kJS);
      remove_event_listener(event::ClosureEventListener::ClosureType::kCore);
    }
    return;
  }

  if (callback.IsString()) {
    SetJSEventHandler(name, type, callback.String());
    if (should_sync_listener) {
      auto handler_name = callback.StdString();
      const bool should_handle_air_fiber_event =
          manager != nullptr && !manager->IsEmbeddedModeOn() &&
          manager->IsAirModeFiberEnabled();
      const bool support_component_js = manager->SupportComponentJS();
      auto* default_vm_context = manager->GetDefaultEntryRuntime();
      auto default_entry_name = manager->GetDefaultEntryLogicalName();
      remove_event_listener(event::ClosureEventListener::ClosureType::kJS);
      AddEventListener(
          event_name,
          std::make_unique<event::ClosureEventListener>(
              [element = this, event_name, handler_name,
               should_handle_air_fiber_event, support_component_js,
               default_vm_context, default_entry_name](lepus::Value args) {
                const auto& args_array = args.Array();
                if (!args.IsArray() || args_array->size() != 3) {
                  return;
                }
                const auto& event_info = args_array->get(0);
                const auto& event_detail = args_array->get(1);
                const auto& event_info_array = event_info.Array();
                if (!event_info.IsArray() || event_info_array->size() != 2) {
                  return;
                }
                if (should_handle_air_fiber_event) {
                  auto event = fml::static_ref_ptr_cast<event::Event>(
                      args_array->get(2).RefCounted());
                  if (event == nullptr || !event->target() ||
                      !event->current_target()) {
                    return;
                  }
                  auto* target = static_cast<Element*>(event->target().get());
                  auto* current_target =
                      static_cast<Element*>(event->current_target().get());
                  auto* parent_component =
                      current_target->GetParentComponentElement();
                  if (default_vm_context == nullptr) {
                    return;
                  }
                  if (!current_target->InComponent()) {
                    if (parent_component) {
                      BASE_STATIC_STRING_DECL(kCallPageEvent, "$callPageEvent");
                      default_vm_context->Call(
                          kCallPageEvent, lepus::Value(handler_name),
                          lepus_value::ShallowCopy(event_detail),
                          lepus::Value(parent_component->impl_id()));
                    }
                  } else if (parent_component && target) {
                    BASE_STATIC_STRING_DECL(kCallComponentEvent,
                                            "$callComponentEvent");
                    default_vm_context->Call(
                        kCallComponentEvent,
                        lepus::Value(parent_component->impl_id()),
                        lepus::Value(handler_name),
                        lepus_value::ShallowCopy(event_detail),
                        lepus::Value(target->impl_id()));
                  }
                  return;
                }

                auto call_method_name =
                    !support_component_js || event_info_array->get(0).Bool();
                auto page_name_or_component_id =
                    call_method_name ? default_entry_name
                                     : event_info_array->get(1).StdString();
                TRACE_EVENT(
                    LYNX_TRACE_CATEGORY, CLOSURE_EVENT_LISTENER_CLOSURE,
                    [&event_name, &handler_name, &page_name_or_component_id](
                        lynx::perfetto::EventContext ctx) {
                      ctx.event()->add_debug_annotations("name", event_name);
                      ctx.event()->add_debug_annotations("callback",
                                                         handler_name);
                      ctx.event()->add_debug_annotations(
                          "component", page_name_or_component_id);
                    });
                LOGI("Invoke the Closure of ClosureEventListener for event: "
                     << event_name << " with callback: " << handler_name
                     << " in component: " << page_name_or_component_id)
                auto message = lepus::CArray::Create();
                message->emplace_back(page_name_or_component_id);
                message->emplace_back(handler_name);
                message->emplace_back(lepus_value::ShallowCopy(event_detail));
                auto event = fml::MakeRefCounted<runtime::MessageEvent>(
                    call_method_name
                        ? runtime::kMessageEventTypeSendPageEvent
                        : runtime::kMessageEventTypePublishComponentEvent,
                    runtime::ContextProxy::Type::kCoreContext,
                    runtime::ContextProxy::Type::kJSContext,
                    std::make_unique<pub::ValueImplLepus>(
                        lepus::Value(std::move(message))));
                element->DispatchMessageEvent(std::move(event));
              },
              event_options, event::ClosureEventListener::ClosureType::kJS));
    }
    return;
  }

  if (callback.IsCallable()) {
    SetLepusEventHandler(name, type, lepus::Value(), callback);
#if ENABLE_LEPUSNG_WORKLET
    if (should_sync_listener) {
      auto callback_value = callback;
      remove_event_listener(event::ClosureEventListener::ClosureType::kCore);
      AddEventListener(
          event_name,
          std::make_unique<event::ClosureEventListener>(
              [element = this, callback_value](lepus::Value args) {
                const auto& args_array = args.Array();
                if (!args.IsArray() || args_array->size() != 3) {
                  return;
                }
                const auto& event_info = args_array->get(0);
                const auto& event_detail = args_array->get(1);
                auto event = fml::static_ref_ptr_cast<event::Event>(
                    args_array->get(2).RefCounted());
                const auto& event_info_array = event_info.Array();
                if (!event_info.IsArray() || event_info_array->size() != 3) {
                  return;
                }
                const auto& component_id = event_info_array->get(0).StdString();
                const auto& entry_name = event_info_array->get(1).StdString();
                int32_t element_id = event_info_array->get(2).Int32();
                auto* manager = element->element_manager();
                if (manager == nullptr) {
                  return;
                }

                auto task_handler =
                    std::make_shared<worklet::LepusApiHandler>();
                auto current_option = std::make_shared<PipelineOptions>();
                EventResult result =
                    manager->FireElementWorkletAndRequestResolve(
                        component_id, entry_name, callback_value, event_detail,
                        task_handler, element_id, current_option);
                ApplyEventResult(event, result);
              },
              event_options, event::ClosureEventListener::ClosureType::kCore));
    }
#endif  // ENABLE_LEPUSNG_WORKLET
    return;
  }

  if (callback.IsObject()) {
    BASE_STATIC_STRING_DECL(kType, "type");
    BASE_STATIC_STRING_DECL(kValue, "value");
    const auto& obj_type = callback.GetProperty(kType).StdString();
    const auto& value = callback.GetProperty(kValue);

    if (obj_type == tasm::kWorklet) {
      SetWorkletEventHandler(name, type, value, context_name);
    }
    if (should_sync_listener) {
      auto worklet_value = value;
      remove_event_listener(event::ClosureEventListener::ClosureType::kCore);
      AddEventListener(
          event_name,
          std::make_unique<event::ClosureEventListener>(
              [element = this, context_name, worklet_value](lepus::Value args) {
                auto* manager = element->element_manager();
                if (manager == nullptr || worklet_value.IsEmpty()) {
                  return;
                }
                auto* context = manager->GetEntryRuntime(context_name);
                if (context == nullptr) {
                  return;
                }
                const auto& args_array = args.Array();
                if (!args.IsArray() || args_array->size() != 3) {
                  return;
                }
                const auto& event_detail = args_array->get(1);
                auto event = fml::static_ref_ptr_cast<event::Event>(
                    args_array->get(2).RefCounted());

                BASE_STATIC_STRING_DECL(kEntryFunction, "runWorklet");
                BASE_STATIC_STRING_DECL(kRunWorkletSource, "source");

                const auto worklet_function_value =
                    context->GetGlobalData(kEntryFunction);
                auto param_array = lepus::CArray::Create();
                param_array->push_back(event_detail);

                auto options = lepus::Dictionary::Create();
                options.get()->SetValue(
                    kRunWorkletSource,
                    static_cast<int>(tasm::RunWorkletType::kEvents));

                EventResult result = EventResult::kDefault;
                auto call_result_value =
                    context->CallClosure(worklet_function_value, worklet_value,
                                         lepus::Value(std::move(param_array)),
                                         lepus::Value(std::move(options)));
                BASE_STATIC_STRING_DECL(kEventResult, "eventReturnResult");
                if (call_result_value.IsObject()) {
                  result = static_cast<EventResult>(
                      call_result_value.GetProperty(kEventResult).Number());
                }
                ApplyEventResult(event, result);
              },
              event_options, event::ClosureEventListener::ClosureType::kCore));
    }
    return;
  }

  LOGW(
      "FiberAddEvent's 3rd parameter must be undefined, null, string or "
      "callable.");
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
  const bool track_imperative_animations =
      ShouldTrackImperativeAnimationsForNewPipeline();
  switch (op) {
    case runtime::js::JavaScriptElement::AnimationOperation::START: {
      if (args.GetLength() != 4) {
        LOGE("When start Element::Animate, the para size must be 4");
        return;
      }
      lepus::Value lepus_name;
      base::String animate_name;
      base::String js_name = args.GetProperty(1).String();
      const auto& table = args.GetProperty(3).Table();
      if (!track_imperative_animations &&
          !will_removed_keyframe_name_.empty()) {
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
      bool owns_generated_keyframe = iter == table->end();
      if (iter == table->end()) {
        // If the user has not set animation_name, the system-generated
        // autoincrement key animation_name is used, and it is logged and
        // removed when overridden.
        animate_name = js_name;
        if (!track_imperative_animations) {
          will_removed_keyframe_name_ = animate_name;
        }
      } else {
        // If the user has set animation_name, it is used.
        animate_name = iter->second.String();
      }

      starlight::CSSStyleUtils::UpdateCSSKeyframes(
          *keyframes_map_, animate_name, args.GetProperty(2), parser_configs);
      lepus_name = lepus::Value(animate_name);
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
      if (track_imperative_animations) {
        RecordImperativeAnimationStart(
            ImperativeAnimationState::Source::kAnimate, js_name, animate_name,
            owns_generated_keyframe, styles);
      }
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::PAUSE: {
      BASE_STATIC_STRING_DECL(kPaused, "paused");
      UnitHandler::Process(kPropertyIDAnimationPlayState, lepus::Value(kPaused),
                           styles, parser_configs);
      if (track_imperative_animations) {
        UpdateImperativeAnimationPlayState(
            ImperativeAnimationState::Source::kAnimate,
            args.GetProperty(1).String(), styles, true);
      }
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::PLAY: {
      BASE_STATIC_STRING_DECL(kRunning, "running");
      UnitHandler::Process(kPropertyIDAnimationPlayState,
                           lepus::Value(kRunning), styles, parser_configs);
      if (track_imperative_animations) {
        UpdateImperativeAnimationPlayState(
            ImperativeAnimationState::Source::kAnimate,
            args.GetProperty(1).String(), styles, false);
      }
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::CANCEL: {
      if (track_imperative_animations) {
        CancelImperativeAnimation(ImperativeAnimationState::Source::kAnimate,
                                  args.GetProperty(1).String());
      }
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
    case runtime::js::JavaScriptElement::AnimationOperation::FINISH: {
      if (track_imperative_animations) {
        FinishImperativeAnimation(ImperativeAnimationState::Source::kAnimate,
                                  args.GetProperty(1).String());
      }
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
  const bool track_imperative_animations =
      ShouldTrackImperativeAnimationsForNewPipeline();
  switch (op) {
    case runtime::js::JavaScriptElement::AnimationOperation::START: {
      if (args.GetLength() != 4) {
        LOGE("When start Element::Animate, the para size must be 4");
        return;
      }
      lepus::Value lepus_name;
      base::String animate_name;
      base::String js_name = args.GetProperty(1).String();
      const auto& table = args.GetProperty(3).Table();
      BASE_STATIC_STRING_DECL(kName, "name");
      auto iter = table->find(kName);
      bool owns_generated_keyframe = iter == table->end();
      if (iter == table->end()) {
        // If the user has not set animation_name, the system-generated
        // autoincrement key animation_name is used, and it is logged and
        // removed when overridden.
        animate_name = js_name;
      } else {
        // If the user has set animation_name, it is used.
        animate_name = iter->second.String();
      }

      starlight::CSSStyleUtils::UpdateCSSKeyframes(
          *keyframes_map_, animate_name, args.GetProperty(2), parser_configs);
      lepus_name = lepus::Value(animate_name);
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
      if (track_imperative_animations) {
        RecordImperativeAnimationStart(
            ImperativeAnimationState::Source::kAnimateV2, js_name, animate_name,
            owns_generated_keyframe, styles);
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
      if (track_imperative_animations) {
        UpdateImperativeAnimationPlayState(
            ImperativeAnimationState::Source::kAnimateV2,
            args.GetProperty(1).String(), styles, true);
      }
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
      if (track_imperative_animations) {
        UpdateImperativeAnimationPlayState(
            ImperativeAnimationState::Source::kAnimateV2,
            args.GetProperty(1).String(), styles, false);
      }
      break;
    }
    case runtime::js::JavaScriptElement::AnimationOperation::CANCEL: {
      if (track_imperative_animations) {
        CancelImperativeAnimation(ImperativeAnimationState::Source::kAnimateV2,
                                  args.GetProperty(1).String());
      }
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
    case runtime::js::JavaScriptElement::AnimationOperation::FINISH: {
      if (track_imperative_animations) {
        FinishImperativeAnimation(ImperativeAnimationState::Source::kAnimateV2,
                                  args.GetProperty(1).String());
      }
      break;
    }
    default:
      break;
  }
  if (!styles.empty() &&
      (!track_imperative_animations ||
       styles.find(kPropertyIDAnimationName) != styles.end())) {
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

fml::RefPtr<PropBundle> Element::GetPropBundleForRecording() {
  auto bundle = element_manager()->GetPropBundleCreator()->CreatePropBundle(
      element_manager_->GetEnableUseMapBuffer(), EnableFragmentLayerRender());
  PushCurrentPropsToBundleForRecording(bundle.get());
  return bundle;
}

void Element::PushCurrentPropsToBundleForRecording(PropBundle* bundle) {
  if (bundle == nullptr) {
    return;
  }

  if (data_model_) {
    for (const auto& [key, value] : data_model_->attributes()) {
      bundle->SetProps(key.c_str(), pub::ValueImplLepus(value));
    }
    for (const auto& [key, value] : updated_attr_map_) {
      bundle->SetProps(key.c_str(), pub::ValueImplLepus(value));
    }

    const auto& dataset = data_model_->dataset();
    if (!dataset.empty()) {
      lepus::Value dataset_val(lepus::Dictionary::Create());
      for (const auto& [key, value] : dataset) {
        dataset_val.SetProperty(key, value);
      }
      bundle->SetProps("dataset", pub::ValueImplLepus(dataset_val));
    }

    auto push_events = [bundle](const EventMap& events) {
      for (const auto& event : events) {
        if (event.second) {
          bundle->SetEventHandler(event.second->ToPubLepusValue());
        }
      }
    };
    push_events(data_model_->static_events());
    push_events(data_model_->lepus_events());
    push_events(data_model_->global_bind_events());

    for (const auto& gesture : data_model_->gesture_detectors()) {
      if (gesture.second) {
        bundle->SetGestureDetector(*gesture.second);
      }
    }
  }

  if (pseudo_elements_.has_value()) {
    for (const auto& pseudo_element : *pseudo_elements_) {
      pseudo_element.second->PushCurrentPropertiesToBundle(bundle);
    }
  }

  if (EnableFragmentLayerRender() && !IsShadowNodeCustom()) {
    return;
  }

  auto* style = computed_css_style();
  if (style == nullptr) {
    return;
  }
  for (const auto& style_prop : style->GetResolvedValues()) {
    const auto id = style_prop.first;
    if (CSSProperty::IsTransitionProps(id) ||
        CSSProperty::IsKeyframeProps(id) || LayoutProperty::IsLayoutOnly(id) ||
        !starlight::ComputedCSSStyle::IsPlatformProperty(id)) {
      continue;
    }
    PropBundleStyleWriter::PushStyleToBundle(bundle, id, style);
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

void Element::PrepareOrUpdatePseudoElement(PseudoState state,
                                           StyleMap& style_map) {
  if (style_map.empty() &&
      (!pseudo_elements_.has_value() ||
       pseudo_elements_->find(state) == pseudo_elements_->end())) {
    return;
  }

  PseudoElement* pseudo_element = CreatePseudoElementIfNeed(state);
  pseudo_element->UpdateStyleMap(style_map);
}

PseudoElement* Element::CreatePseudoElementIfNeed(PseudoState state) {
  if (pseudo_elements_.has_value()) {
    auto it = pseudo_elements_->find(state);
    if (it != pseudo_elements_->end()) {
      return it->second.get();
    }
  }

  auto new_pseudo_element = std::make_unique<PseudoElement>(state, this);
  auto* result = new_pseudo_element.get();
  (*pseudo_elements_)[state] = std::move(new_pseudo_element);
  return result;
}

void Element::HandleCSSVariables(StyleMap& styles) {
  style_resolver_.HandleCSSVariables(styles);
}

bool Element::DisableFlattenWithOpacity() {
  return computed_css_style()->HasOpacity() && !is_text() && !is_image();
}

void Element::SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func) {
  if (customized_layout_node_ != nullptr) {
    customized_layout_node_->SetMeasureFunc(std::move(measure_func));
  }
}

void Element::SetMeasureFunc(void* context,
                             starlight::SLMeasureFunc measure_func) {
  sl_node_->SetContext(context);
  sl_node_->SetSLMeasureFunc(std::move(measure_func));
}

void Element::SetAlignmentFunc(void* context,
                               starlight::SLAlignmentFunc alignment_func) {
  sl_node_->SetSLAlignmentFunc(std::move(alignment_func));
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

starlight::ComputedCSSStyle* Element::GetParentBaseComputedCSSStyle() {
  auto temp = parent();
  while (temp != nullptr && temp->is_wrapper()) {
    temp = temp->parent();
  }

  if (temp == nullptr) {
    return nullptr;
  }

  return temp->base_css_style();
}

void Element::PrepareSelfForThreadedElementResolution() {
  EnsureTagInfo();
  GetRelatedCSSFragment();
  if (is_component()) {
    static_cast<ComponentElement*>(this)->GetCSSFragment();
  }
}

bool Element::ShouldAvoidFlattenForView() {
  return is_view() && element_manager()->GetDefaultOverflowVisible() &&
         computed_css_style()->IsOverflowHidden() &&
         computed_css_style()->HasBorderRadius();
}

void Element::DispatchLayoutBeforeRecursively() {
  if (!is_wrapper()) {
    if (sl_node_ == nullptr || !sl_node_->IsDirty()) {
      return;
    }

    if (sl_node_->GetSLMeasureFunc()) {
      DispatchLayoutBefore();
    }
  }

  for (auto& child : scoped_children_) {
    child->DispatchLayoutBeforeRecursively();
  }
}

void Element::DispatchLayoutBefore() {
  if (customized_layout_node_) {
    customized_layout_node_->OnLayoutBefore();
  }
}

bool Element::TendToFlatten() {
  return config_flatten_ &&
         (!has_event_listener_ || EnableFragmentLayerRender()) &&
         !has_non_flatten_attrs_ && !DisableFlattenWithOpacity() &&
         !(has_z_props() && !is_image() && !is_text()) && !is_inline_element_ &&
         !ShouldAvoidFlattenForView() &&
         // Note: sticky item should not be flatten on Android platform.
         (!element_manager_->GetEnableNewSticky() || !is_sticky_)
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
         && (!is_view() || computed_css_style()->IsOverflowXY());
#endif
  ;
}

double Element::GetFontSize() { return computed_css_style()->GetFontSize(); }

const Element::InheritedProperty Element::GetInheritedProperty() {
  return {
      children_propagate_inherited_styles_flag_, inherited_styles_.get(),
      reset_inherited_ids_.get(),
      custom_properties_.Get() ? &custom_properties_.Get()->Value() : nullptr};
}

const Element::InheritedProperty Element::GetParentInheritedProperty() {
  // If in a parallel flush process or if the parent is null, return empty
  // InheritedProperty indicating that inheritance logic is not needed now.
  if (this->is_greedy_parallel_flush()) {
    return {false, nullptr, nullptr,
            custom_properties_.Get() ? &custom_properties_.Get()->Value()
                                     : nullptr};
  }

  Element* real_parent = parent();
  if (real_parent == nullptr) {
    return InheritedProperty();
  }

  return real_parent->GetInheritedProperty();
}

const tasm::CSSValue& Element::ResolveCurrentStyleValue(
    const CSSPropertyID& key, const tasm::CSSValue& default_value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_RESOLVE_CURRENT_STYLE);
  auto iter = parsed_styles_map_.find(key);
  if (iter != parsed_styles_map_.end()) {
    return iter->second;
  }

  const auto inherited_property = GetParentInheritedProperty();
  if (inherited_property.inherited_styles_ != nullptr) {
    auto iter = inherited_property.inherited_styles_->find(key);
    if (iter != inherited_property.inherited_styles_->end()) {
      return iter->second;
    }
  }

  return default_value;
}

bool Element::CollectCustomProperties(AttributeHolder* holder) {
  if (custom_properties_.Get() != nullptr) {
    return true;
  }

  if (!holder) {
    return false;
  }

  Element* real_parent = parent();
  if (real_parent) {
    if (!real_parent->CollectCustomProperties(real_parent->data_model())) {
      return false;
    }
    // Share parent's map (Copy-On-Write).
    custom_properties_ = real_parent->custom_properties_;
  }

  const auto& variables = holder->css_variables_map();
  const auto& inline_variables = holder->GetCSSInlineVariables();

  // If we don't have any new variables, we can just share the parent's map.
  if (variables.empty() && inline_variables.empty()) {
    if (custom_properties_.Get() == nullptr) {
      custom_properties_.Init();
    }
    return true;
  }

  // Access() will copy the map if it's shared (refcount > 1).
  if (custom_properties_.Get() == nullptr) {
    custom_properties_.Init();
  }
  auto* map = &custom_properties_.Access()->Value();

  // TODO(renzhongyue): Variables declared in CSS must use the normal
  // custom-property declaration syntax, not {{}}.
  for (const auto& [name, value] : variables) {
    CSSStringParser parser{value.c_str(), static_cast<uint32_t>(value.length()),
                           element_manager()->GetCSSParserConfigs()};
    map->insert_or_assign(name, parser.ParseVariable());
  }

  for (const auto& [name, value] : inline_variables) {
    CSSStringParser parser{value.c_str(), static_cast<uint32_t>(value.length()),
                           element_manager()->GetCSSParserConfigs()};
    map->insert_or_assign(name, parser.ParseVariable());
  }

  CSSValue::SubstituteAll(*map);
  return true;
}

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

void Element::SetFontSizeForAllElement(double cur_node_font_size,
                                       double root_node_font_size) {
  computed_css_style()->SetFontSize(cur_node_font_size, root_node_font_size);

  if (pseudo_elements_.has_value()) {
    for (const auto& [key, pseudo_element] : *pseudo_elements_) {
      pseudo_element->SetFontSize(cur_node_font_size, root_node_font_size);
    }
  }
}

void Element::SetFontSize(const tasm::CSSValue& value) {
  SetFontSize(value, computed_css_style());
}

void Element::SetFontSize(const tasm::CSSValue& value,
                          starlight::ComputedCSSStyle* target_style) {
  base::flex_optional<float> result;
  const auto current_font_size = target_style->GetFontSize();
  const auto root_font_size = target_style->GetRootFontSize();
  if (!value.IsEmpty()) {
    CheckDynamicUnit(CSSPropertyID::kPropertyIDFontSize, value, false);
    const auto& env_config = element_manager()->GetLynxEnvConfig();
    auto unify_vw_vh_behavior =
        element_manager()->GetDynamicCSSConfigs().unify_vw_vh_behavior_;
    result = starlight::CSSStyleUtils::ResolveFontSize(
        value, env_config, unify_vw_vh_behavior, GetParentFontSize(),
        GetRecordedRootFontSize(), element_manager()->GetCSSParserConfigs());
  } else {
    result = GetParentFontSize();
  }

  if (result.has_value()) {
    target_style->SetResolvedValue(kPropertyIDFontSize,
                                   CSSValue(*result, CSSValuePattern::NUMBER));
  } else {
    target_style->RemoveResolvedValue(kPropertyIDFontSize);
  }

  if (result.has_value() && *result != current_font_size) {
    NotifyUnitValuesUpdatedToAnimation(DynamicCSSStylesManager::kUpdateEm);

    if (is_page()) {
      if (target_style == computed_css_style()) {
        SetFontSizeForAllElement(*result, *result);
      } else {
        target_style->SetFontSize(*result, *result);
      }
      UpdateLayoutNodeFontSize(*result, *result);
    } else {
      if (target_style == computed_css_style()) {
        SetFontSizeForAllElement(*result, root_font_size);
      } else {
        target_style->SetFontSize(*result, root_font_size);
      }
      UpdateLayoutNodeFontSize(*result, root_font_size);
    }

    if (!EnableLayoutInElementMode() || IsShadowNodeCustom()) {
      target_style->SetValue(kPropertyIDFontSize,
                             CSSValue(*result, CSSValuePattern::NUMBER));
    }
    if (is_page() && !is_greedy_parallel_flush()) {
      MarkFontSizeInvalidateRecursively();
    } else {
      MarkDirty(kDirtyFontSize);
    }
  }
}

void Element::ResetFontSize() {
  CheckDynamicUnit(CSSPropertyID::kPropertyIDFontSize, CSSValue(), true);
  auto font_size = element_manager()->GetLynxEnvConfig().PageDefaultFontSize();
  auto root_font_size = is_page() ? font_size : GetCurrentRootFontSize();

  if (font_size != GetFontSize()) {
    SetFontSizeForAllElement(font_size, root_font_size);
    if (!EnableLayoutInElementMode() || IsShadowNodeCustom()) {
      computed_css_style()->SetValue(
          kPropertyIDFontSize, CSSValue(font_size, CSSValuePattern::NUMBER));
    }
    UpdateLayoutNodeFontSize(font_size, root_font_size);
  }
}

void Element::UpdateLengthContextValueForAllElement(
    const LynxEnvConfig& env_config) {
  computed_css_style()->SetFontScale(env_config.FontScale());
  computed_css_style()->SetViewportWidth(env_config.ViewportWidth());
  computed_css_style()->SetViewportHeight(env_config.ViewportHeight());
  computed_css_style()->SetScreenWidth(env_config.ScreenWidth());
  computed_css_style()->SetLayoutUnit(env_config.PhysicalPixelsPerLayoutUnit(),
                                      env_config.LayoutsUnitPerPx());

  if (pseudo_elements_.has_value()) {
    for (const auto& [key, pseudo_element] : *pseudo_elements_) {
      pseudo_element->ComputedCSSStyle()->SetFontScale(env_config.FontScale());
      pseudo_element->ComputedCSSStyle()->SetViewportWidth(
          env_config.ViewportWidth());
      pseudo_element->ComputedCSSStyle()->SetViewportHeight(
          env_config.ViewportHeight());
      pseudo_element->ComputedCSSStyle()->SetScreenWidth(
          env_config.ScreenWidth());
      pseudo_element->ComputedCSSStyle()->SetLayoutUnit(
          env_config.PhysicalPixelsPerLayoutUnit(),
          env_config.LayoutsUnitPerPx());
    }
  }
}

void Element::UpdateDynamicChildrenStyleRecursively(uint32_t style,
                                                    bool force_update) {
  auto* child = first_render_child_;
  while (child) {
    child->UpdateDynamicElementStyle(style, force_update);
    child = child->next_render_sibling_;
  }
}

void Element::UpdateDynamicElementStyleForNewPipeline(
    uint32_t& style, bool& inner_force_update) {
  constexpr uint32_t kMediaQueryEnvMask =
      DynamicCSSStylesManager::kUpdateViewport |
      DynamicCSSStylesManager::kUpdateScreenMetrics |
      DynamicCSSStylesManager::kUpdateRem | DynamicCSSStylesManager::kUpdateEm |
      DynamicCSSStylesManager::kUpdateColorScheme;
  bool media_query_env_changed = false;
  if (is_component() &&
      ((style & kMediaQueryEnvMask) != 0 || (dirty_ & kDirtyFontSize))) {
    auto* fragment = GetRelatedCSSFragment();
    if (StyleResolver::FragmentsHasMediaQueries(fragment)) {
      media_query_env_changed = true;
      inner_force_update = true;
    }
  }

  if ((dynamic_style_flags_ > 0 || inner_force_update ||
       media_query_env_changed) &&
      !is_wrapper()) {
    NotifyUnitValuesUpdatedToAnimation(style);
    const auto& env_config = element_manager()->GetLynxEnvConfig();

    bool font_scale_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateFontScale) &&
        (style & DynamicCSSStylesManager::kUpdateFontScale) &&
        (computed_css_style()->GetMeasureContext().font_scale_ !=
         env_config.FontScale());
    bool viewport_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateViewport) &&
        (style & DynamicCSSStylesManager::kUpdateViewport) &&
        !(env_config.ViewportWidth() ==
              computed_css_style()->GetMeasureContext().viewport_width_ &&
          env_config.ViewportHeight() ==
              computed_css_style()->GetMeasureContext().viewport_height_);
    bool screen_matrix_changed =
        (dynamic_style_flags_ &
         DynamicCSSStylesManager::kUpdateScreenMetrics) &&
        (style & DynamicCSSStylesManager::kUpdateScreenMetrics) &&
        (env_config.ScreenWidth() !=
         computed_css_style()->GetMeasureContext().screen_width_);
    bool rem_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateRem) &&
        (style & DynamicCSSStylesManager::kUpdateRem);
    bool em_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm) &&
        ((style & DynamicCSSStylesManager::kUpdateEm) ||
         (dirty_ & kDirtyFontSize));

    if (GetCurrentRootFontSize() != GetRecordedRootFontSize()) {
      computed_css_style()->SetFontSize(GetFontSize(),
                                        GetCurrentRootFontSize());
      UpdateLayoutNodeFontSize(GetFontSize(), GetCurrentRootFontSize());
    }

    if (inner_force_update || font_scale_changed || viewport_changed ||
        screen_matrix_changed || rem_changed || em_changed ||
        media_query_env_changed) {
      UpdateLengthContextValueForAllElement(env_config);

      NewPipelineResolveRequest request;
      request.force_resolve = true;
      request.force_platform_update = inner_force_update;
      request.dynamic_update_flags = style;
      if (dirty_ & kDirtyFontSize) {
        request.dynamic_update_flags |= DynamicCSSStylesManager::kUpdateEm;
      }

      auto outcome = ResolveCSSStylesNewPipelineCore(request);
      if (outcome.need_update) {
        RequestLayout();
        PerformElementContainerCreateOrUpdate(
            true, element_manager_->FixNewAnimatorFlushBug());
      }
      style |= outcome.child_update_flags;
      inner_force_update |= outcome.force_children;
    }
  }

  if (dirty_ & kDirtyFontSize) {
    if (is_page()) {
      style |= DynamicCSSStylesManager::kUpdateRem;
    }
    dirty_ &= ~kDirtyFontSize;
  }
}

void Element::UpdateDynamicElementStyleRecursively(uint32_t style,
                                                   bool force_update) {
  if (is_raw_text()) {
    return;
  }
  bool inner_force_update = force_update;

  if (element_manager()->EnableNewStylingPipeline() &&
      !element_manager()->EnableSimpleStyle()) {
    UpdateDynamicElementStyleForNewPipeline(style, inner_force_update);
    UpdateDynamicChildrenStyleRecursively(style, inner_force_update);
    return;
  }

  constexpr uint32_t kMediaQueryEnvMask =
      DynamicCSSStylesManager::kUpdateViewport |
      DynamicCSSStylesManager::kUpdateScreenMetrics |
      DynamicCSSStylesManager::kUpdateRem | DynamicCSSStylesManager::kUpdateEm |
      DynamicCSSStylesManager::kUpdateColorScheme;
  if (is_component() &&
      ((style & kMediaQueryEnvMask) != 0 || (dirty_ & kDirtyFontSize))) {
    auto* fragment = GetRelatedCSSFragment();
    if (StyleResolver::FragmentsHasMediaQueries(fragment)) {
      MarkStyleDirty(true);
    }
  }

  if ((dynamic_style_flags_ > 0 || inner_force_update) && !is_wrapper()) {
    // Style could never be "all" here.
    NotifyUnitValuesUpdatedToAnimation(style);
    const auto& env_config = element_manager()->GetLynxEnvConfig();
    const auto& css_config = element_manager()->GetDynamicCSSConfigs();

    bool font_scale_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateFontScale) &&
        (style & DynamicCSSStylesManager::kUpdateFontScale) &&
        (computed_css_style()->GetMeasureContext().font_scale_ !=
         env_config.FontScale());
    bool viewport_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateViewport) &&
        (style & DynamicCSSStylesManager::kUpdateViewport) &&
        !(env_config.ViewportWidth() ==
              computed_css_style()->GetMeasureContext().viewport_width_ &&
          env_config.ViewportHeight() ==
              computed_css_style()->GetMeasureContext().viewport_height_);
    bool screen_matrix_changed =
        (dynamic_style_flags_ &
         DynamicCSSStylesManager::kUpdateScreenMetrics) &&
        (style & DynamicCSSStylesManager::kUpdateScreenMetrics) &&
        (env_config.ScreenWidth() !=
         computed_css_style()->GetMeasureContext().screen_width_);
    bool rem_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateRem) &&
        (style & DynamicCSSStylesManager::kUpdateRem);

    if (GetCurrentRootFontSize() != GetRecordedRootFontSize()) {
      computed_css_style()->SetFontSize(GetFontSize(),
                                        GetCurrentRootFontSize());
      UpdateLayoutNodeFontSize(GetFontSize(), GetCurrentRootFontSize());
    }

    if (inner_force_update || font_scale_changed || viewport_changed ||
        screen_matrix_changed || rem_changed) {
      UpdateLengthContextValueForAllElement(env_config);
      const auto property = GetParentInheritedProperty();

      ConsumeStyleInternal(
          parsed_styles_map_, property.inherited_styles_,
          [this, style, &css_config](auto id, const auto& value) {
            if (CSSProperty::IsTransitionProps(id) ||
                CSSProperty::IsKeyframeProps(id)) {
              return true;
            }

            if (ShouldUseLegacyTransitionInterception() &&
                css_transition_manager_) {
              if (IsFiberArch()) {
                const bool skip_transition =
                    element_manager_ &&
                    !element_manager_
                         ->FixFiberDynamicUpdateTransitionConsumeBug();
                if (skip_transition &&
                    css_transition_manager_->NeedsTransition(id)) {
                  return true;
                }
              } else {
                const bool skip_transition =
                    element_manager_ &&
                    element_manager_->FixDynamicUpdateTransitionConsumeBug();
                if (!skip_transition &&
                    css_transition_manager_->NeedsTransition(id)) {
                  return true;
                }
              }
            }

            auto new_flags = DynamicCSSStylesManager::GetValueFlags(
                id, value, css_config.unify_vw_vh_behavior_,
                element_manager()->FixFilterDynamicUpdateBug());

            if ((new_flags & (style | ((dirty_ & kDirtyFontSize)
                                           ? DynamicCSSStylesManager::kUpdateEm
                                           : 0))) == 0) {
              return true;
            }

            return false;
          });

      if (element_manager()->EnableAnimationForwardUpdatePreservation() &&
          animation_override_styles_map_.has_value() &&
          !animation_override_styles_map_->empty()) {
        ConsumeStyleInternal(*animation_override_styles_map_, nullptr,
                             [](auto id, const auto& value) {
                               if (CSSProperty::IsTransitionProps(id) ||
                                   CSSProperty::IsKeyframeProps(id)) {
                                 return true;
                               }
                               return false;
                             });
      }

      if (inherited_styles_.has_value() && !inherited_styles_->empty()) {
        inner_force_update |= true;
      }

      PerformElementContainerCreateOrUpdate(
          true, element_manager_->FixNewAnimatorFlushBug());
    }
  }

  if (dirty_ & kDirtyFontSize) {
    if (is_page()) {
      style |= DynamicCSSStylesManager::kUpdateRem;
    }
    dirty_ &= ~kDirtyFontSize;
  }

  UpdateDynamicChildrenStyleRecursively(style, inner_force_update);
}

void Element::UpdateDynamicElementStyle(uint32_t style, bool force_update) {
  UpdateDynamicElementStyleRecursively(style, force_update);
}

bool Element::HasLayoutInElementPlatformNode() {
  return EnableLayoutInElementMode() && customized_layout_node_;
}

int Element::GetLayoutInElementPlatformChildIndex(Element* child) {
  if (child == nullptr || child->render_parent() == nullptr) {
    return -1;
  }
  int index = 0;
  auto* render_parent = child->render_parent();
  for (auto* current = render_parent->first_render_child(); current != nullptr;
       current = current->next_render_sibling()) {
    if (current == child) {
      return index;
    }
    if (current->HasLayoutInElementPlatformNode()) {
      ++index;
    }
  }
  return -1;
}

void Element::AddScopedVirtualChild(const fml::RefPtr<Element>& child) {
  scoped_virtual_children_->push_back(child);
}

void Element::RemoveScopedVirtualChild(const fml::RefPtr<Element>& child) {
  if (!scoped_virtual_children_.has_value()) {
    return;
  }
  scoped_virtual_children_->erase(
      std::remove(scoped_virtual_children_->begin(),
                  scoped_virtual_children_->end(), child),
      scoped_virtual_children_->end());
}

void Element::StoreLayoutNode(Element* child, Element* ref) {
  child->render_parent_ = this;
  Element* next_layout_sibling = ref;
  Element* previous_layout_sibling =
      next_layout_sibling ? next_layout_sibling->previous_render_sibling_
                          : last_render_child_;
  if (previous_layout_sibling) {
    previous_layout_sibling->next_render_sibling_ = child;
  } else {
    first_render_child_ = child;
  }
  child->previous_render_sibling_ = previous_layout_sibling;

  if (next_layout_sibling) {
    next_layout_sibling->previous_render_sibling_ = child;
  } else {
    last_render_child_ = child;
  }
  child->next_render_sibling_ = next_layout_sibling;
}

void Element::RestoreLayoutNode(Element* node) {
  if (node->previous_render_sibling_) {
    node->previous_render_sibling_->next_render_sibling_ =
        node->next_render_sibling_;
  } else {
    first_render_child_ = node->next_render_sibling_;
  }
  if (node->next_render_sibling_) {
    node->next_render_sibling_->previous_render_sibling_ =
        node->previous_render_sibling_;
  } else {
    last_render_child_ = node->previous_render_sibling_;
  }
  node->render_parent_ = nullptr;
  node->previous_render_sibling_ = nullptr;
  node->next_render_sibling_ = nullptr;
}

void Element::InsertLayoutNode(Element* child, Element* ref) {
  DCHECK(!ref || !ref->is_wrapper());
  if (EnableLayoutInElementMode()) {
    Element* container = FindFirstNonVirtualRenderAncestor();
    bool inserted = false;
    if (container && !child->is_virtual()) {
      container->EnsureSLNode();
      child->EnsureSLNode();
      Element* ref_node =
          ref ? ref->FindFirstNonVirtualRenderSibling() : nullptr;
      if (ref_node) {
        ref_node->EnsureSLNode();
      }
      container->sl_node_->InsertChildBefore(
          child->sl_node_.get(), ref_node ? ref_node->sl_node_.get() : nullptr);
      child->UpdateFixedNodeSetRecursively(true);
      container->MarkLayoutDirtyLite();
      if (container->customized_layout_node_ &&
          child->HasLayoutInElementPlatformNode()) {
        int index = container->GetLayoutInElementPlatformChildIndex(child);
        if (index >= 0) {
          element_manager()->layout_context()->InsertLayoutNode(
              container->impl_id(), child->impl_id(), index);
        }
      }
      inserted = true;
    }
    child->attached_to_layout_parent_ = inserted || child->is_virtual();
    return;
  }

  if (child->attached_to_layout_parent_) {
    LOGE("Element layout node already inserted !");
    this->LogNodeInfo();
    child->LogNodeInfo();
  }
  EnqueueLayoutTask([element_manager = element_manager(), id = id_,
                     child_id = child->impl_id(),
                     ref_id = ref ? ref->impl_id() : -1]() {
    element_manager->InsertLayoutNodeBefore(id, child_id, ref_id);
  });
  child->attached_to_layout_parent_ = true;
}

void Element::RemoveLayoutNode(Element* child,
                               int layout_in_element_platform_index) {
  if (EnableLayoutInElementMode()) {
    if (auto* child_layout_node = child->slnode();
        child_layout_node && child_layout_node->parent()) {
      child->UpdateFixedNodeSetRecursively(false);
      if (customized_layout_node_ && child->HasLayoutInElementPlatformNode()) {
        int index = layout_in_element_platform_index >= 0
                        ? layout_in_element_platform_index
                        : GetLayoutInElementPlatformChildIndex(child);
        element_manager()->layout_context()->RemoveLayoutNode(
            impl_id(), child->impl_id(), index);
      }
      // FIXME: this->sl_node_ is accidentally not the parent of
      // child->sl_node_->parent_. Should try to figure out why it happens.
      if (child_layout_node->parent() != sl_node_.get()) {
        LOGD(
            "Trying to remove a LayoutObject that doesn't belong to the "
            "Element.");
      }
      child->slnode()->parent()->RemoveChild(child->slnode());
      MarkLayoutDirtyLite();
    }
    child->attached_to_layout_parent_ = false;
    return;
  }

  EnqueueLayoutTask([element_manager = element_manager(), id = id_,
                     child_id = child->impl_id()]() {
    element_manager->RemoveLayoutNode(id, child_id);
  });
  child->attached_to_layout_parent_ = false;
}

void Element::UpdateFixedNodeSet() {
  if (!EnableLayoutInElementMode() || !IsFixedNewOrUnifiedEnabled() ||
      sl_node_ == nullptr) {
    return;
  }
  if (is_fixed_ && !attached_to_layout_parent_ && !is_page()) {
    return;
  }
  element_manager()->UpdateFixedNodeSet(sl_node_.get(), is_fixed_);
}

void Element::UpdateFixedNodeSetRecursively(bool is_insert) {
  if (!EnableLayoutInElementMode() || !IsFixedNewOrUnifiedEnabled()) {
    return;
  }
  if (sl_node_ != nullptr && is_fixed_) {
    element_manager()->UpdateFixedNodeSet(sl_node_.get(), is_insert);
  }
  for (auto& child : scoped_children_) {
    child->UpdateFixedNodeSetRecursively(is_insert);
  }
}

void Element::RecursivelyMarkRenderRootElement(Element* render_root) {
  render_root_element_ = render_root;
  for (const auto& child : scoped_children_) {
    if (!child->is_list_item()) {
      child->RecursivelyMarkRenderRootElement(render_root);
    }
  }
}

void Element::UpdateRenderRootElementIfNecessary(Element* child) {
  if (child->render_root_element_ == this->render_root_element_) {
    // 1. Child has same render root as parent, indicating tree mutation inside
    // same render root, no need to propagate change
    return;
  }
  if (child->render_root_element_ == nullptr) {
    // 2. child doesn't hava valid render_root_element, propagate parent's
    // render_root_element to child subtree
    child->RecursivelyMarkRenderRootElement(this->render_root_element_);
    return;
  }
  if (this->render_root_element_ == nullptr) {
    // 3. parent doesn't have valid render_root_element, reset chlld subtree
    // render root
    child->RecursivelyMarkRenderRootElement(nullptr);
    return;
  }
  // 4.child and parent have different valid render_root_elements, throw warning
  LOGW(
      "FiberElement move element to a different render root, inefficient "
      "operation");
  // Update child subtree render root with parent render root
  child->RecursivelyMarkRenderRootElement(this->render_root_element_);
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
    bool is_new_fixed = IsNewFixed();
    // Legacy sticky only handled direct scroll-view children, which are native
    // views. New sticky can target any descendant, so sticky nodes must stay
    // non-layout-only to keep their subtree positioned by platform.
    bool is_new_sticky = is_sticky_ && element_manager_->GetEnableNewSticky();
    if (is_new_fixed || is_new_sticky) {
      // Fixed or sticky nodes should not be layout-only. We need them to locate
      // the entire subtree.
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

void Element::AsyncResolveSubtreeProperty() {
  if (element_manager()->GetEnableParallelElement() &&
      ((dirty_ & ~kDirtyTree) != 0) && scheduler_adapter_.get()) {
    element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
      scheduler_adapter_->ResolveSubtreeProperty();

      std::promise<ParallelFlushReturn> promise;
      std::future<ParallelFlushReturn> future = promise.get_future();
      auto task_info_ptr =
          fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
              [promise = std::move(promise),
               scheduler = scheduler_adapter_.get()]() mutable {
                promise.set_value(
                    scheduler->GenerateReduceTaskForResolveProperty());
              },
              std::move(future));
      element_manager()->ParallelTasks().emplace_back(std::move(task_info_ptr));
    });
  }
}

bool Element::IsRelatedCSSVariableUpdated(
    AttributeHolder* holder, const lepus::Value changing_css_variables) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_IS_RELATED_CSS_UPDATED,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  bool changed = false;
  ForEachLepusValue(
      changing_css_variables,
      [holder, &changed](const lepus::Value& key, const lepus::Value& value) {
        if (!changed) {
          auto it = holder->css_variable_related().find(key.String());
          if (it != holder->css_variable_related().end() &&
              !it->second.IsEqual(value.String())) {
            changed = true;
          }
        }
      });
  return changed;
}

#if ENABLE_TRACE_PERFETTO
void Element::UpdateTraceDebugInfo(TraceEvent* event) {
  auto* tagInfo = event->add_debug_annotations();
  tagInfo->set_name("tagName");
  tagInfo->set_string_value(tag_.str());

  if (!data_model_) {
    return;
  }

  if (!data_model_->idSelector().empty()) {
    auto* idInfo = event->add_debug_annotations();
    idInfo->set_name("idSelector");
    idInfo->set_string_value(data_model_->idSelector().str());
  }
  if (!data_model_->classes().empty()) {
    std::string class_str = "";
    for (auto& aClass : data_model_->classes()) {
      class_str = class_str + " " + aClass.str();
    }
    if (!class_str.empty()) {
      auto* classInfo = event->add_debug_annotations();
      classInfo->set_name("class");
      classInfo->set_string_value(class_str);
    }
  }
  auto* nodeIndexInfo = event->add_debug_annotations();
  nodeIndexInfo->set_name("nodeIndex");
  nodeIndexInfo->set_uint_value(node_index_);
}
#endif

PaintingContext* Element::painting_context() {
  return catalyzer_->painting_context();
}

void Element::MarkLayoutDirty() {
  if (EnableLayoutInElementMode()) {
    MarkLayoutDirtyLite();
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->is_dirty = true;
}

void Element::MarkLayoutDirtyLite() {
  if (!is_virtual_) {
    EnsureSLNode();
    sl_node_->MarkDirtyAndRequestLayout();
  } else {
    auto* parent = render_parent_;
    while (parent) {
      if (!parent->is_virtual_) {
        parent->MarkLayoutDirtyLite();
        break;
      }
      parent = parent->render_parent_;
    }
  }
}

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

  if (element_manager_->EnableNewStylingPipeline() && enable_new_animator_) {
    RequireFlush();
    SetAnimationSampleTimeForNewPipeline(frame_time);
    MarkStyleDirty();
    options->resolve_requested = true;
    options->target_node = this->impl_id();
    return true;
  }
  if (element_manager_->EnableNewStylingPipeline() && !enable_new_animator_) {
    return false;
  }

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
      MarkPropsDirty();
    } else {
      element_manager_->OnFinishUpdateProps(this, options);
    }
  }
  return need_layout;
}

void Element::SetAnimationSampleTimeForNewPipeline(
    const fml::TimePoint& sample_time) {
  animation_sample_time_for_new_pipeline_ = sample_time;
}

base::flex_optional<fml::TimePoint>
Element::TakeAnimationSampleTimeForNewPipeline() {
  auto sample_time = std::move(animation_sample_time_for_new_pipeline_);
  animation_sample_time_for_new_pipeline_ = std::nullopt;
  return sample_time;
}

void Element::DispatchAnimationEventsForNewPipeline(
    const animation::AnimationEventRecordsForNewPipeline& event_records) {
  for (const auto& event_record : event_records) {
    auto animation = event_record.animation;
    if (animation == nullptr) {
      continue;
    }
    if (event_record.send_cancel_event) {
      animation->SendCancelEvent();
    }
    if (event_record.send_start_event) {
      animation->SendStartEvent();
    }
    for (int i = 0; i < event_record.iteration_events_due; ++i) {
      animation->SendIterationEvent();
    }
    if (event_record.send_end_event) {
      animation->SendEndEvent();
    }
  }
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
  return (ShouldUseLegacyTransitionInterception() && HasPaintingNode());
}

bool Element::ShouldUseLegacyTransitionInterception() const {
  return enable_new_animator_ && element_manager_ != nullptr &&
         !element_manager_->EnableNewStylingPipeline();
}

// Since the previous element styles cannot be accessed in element, we
// need to record some necessary styles which New Animator transition needs.
// TODO(wujintian): We only need to record layout-only properties, while other
// properties can be accessed through ComputedCSSStyle.
void Element::RecordElementPreviousStyle(CSSPropertyID css_id,
                                         const tasm::CSSValue& value) {
  if (!ShouldUseLegacyTransitionInterception()) {
    return;
  }
  if (animation::IsAnimatableProperty(css_id)) {
    animation_previous_styles_[css_id] = value;
  }
}

void Element::ResetElementPreviousStyle(CSSPropertyID css_id) {
  if (!ShouldUseLegacyTransitionInterception()) {
    return;
  }
  if (animation::IsAnimatableProperty(css_id)) {
    animation_previous_styles_.erase(css_id);
  }
}

std::optional<CSSValue> Element::GetElementPreviousStyle(
    tasm::CSSPropertyID css_id) {
  if (!ShouldUseLegacyTransitionInterception()) {
    return std::optional<CSSValue>();
  }
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
  if (ShouldTrackImperativeAnimationsForNewPipeline() &&
      keyframes_map_.has_value() &&
      imperative_animation_state_.HasAnimationName(animation_name)) {
    if (auto it = keyframes_map_->find(animation_name);
        it != keyframes_map_->end()) {
      return it->second.get();
    }
  }

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

  if (animation_names->size() == 0) {
    return;
  }

  CSSFragment* css_fragment = GetRelatedCSSFragment();
  if (css_fragment) {
    struct Ctx {
      Element* self;
      const fml::RefPtr<lepus::CArray>* animation_names;
    };
    Ctx ctx{this, &animation_names};
    css_fragment->ForEachKeyframesMap(
        [](const CSSKeyframesTokenMap& keyframes_map, void* cb_data) {
          auto* c = static_cast<Ctx*>(cb_data);
          if (!keyframes_map.empty()) {
            c->self->SetKeyframesByNames(lepus::Value(*c->animation_names),
                                         keyframes_map, false);
          }
        },
        &ctx);
  } else if (element_manager_ && element_manager_->GetSimpleStyleKeyframes()) {
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
  auto* render_root = GetRenderRootElement();
  if (render_root && render_root->GetSchedulerAdapter() &&
      render_root->GetSchedulerAdapter()->IsBatchResolvingTree()) {
    render_root->GetSchedulerAdapter()
        ->resolve_element_tree_queue()
        .emplace_back(std::move(operation));
    return;
  }
  if (element_manager()->GetParallelWithSyncLayout() &&
      ShouldProcessParallelTasks()) {
    EnqueueReduceTask(std::move(operation));
    return;
  }
  operation();
}

void Element::HandleDelayTask(base::MoveOnlyClosure<void> operation) {
  if (this->is_parallel_flush()) {
    parallel_reduce_tasks_->emplace_back(std::move(operation));
  } else {
    operation();
  }
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
  if (IsDetached()) {
    LOGE("Element::IsEventPathCatch error: the target is detached.");
    return true;
  }

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
      event->event_path().push_back(this->GetWeakTarget());
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
    BASE_STATIC_STRING_DECL(kNodeIndex, "nodeIndex");

    dict.get()->SetValue(kId, data_model_->idSelector());
    auto dataset = lepus::Dictionary::Create();
    for (const auto& [key, value] : data_model_->dataset()) {
      dataset.get()->SetValue(key, value);
    }
    dict.get()->SetValue(kDataset, std::move(dataset));
    dict.get()->SetValue(kUid, id_);
    if (element_manager_ &&
        element_manager_->GetEnableEventTargetInfoNodeIndex()) {
      dict.get()->SetValue(kNodeIndex, node_index_);
    }
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

void Element::UpdateLayoutInfoRecursively(PipelineOptions* options) {
  if (!is_wrapper()) {
    if (sl_node_ == nullptr || !(sl_node_->IsDirty())) {
      return;
    }

    if (IfNeedsUpdateLayoutInfo()) {
      UpdateLayoutInfo();
    }

    sl_node_->MarkUpdated();
  }

  for (auto& child : scoped_children_) {
    child->UpdateLayoutInfoRecursively(options);
  }

  // A dirty child can change the list's content layout, so collect dirty list
  // nodes regardless of whether their own layout info needs to be updated. This
  // is intentionally post-order so nested lists are collected before their
  // parent list.
  if (is_list()) {
    CollectDirtyNodeForList(impl_id(), options);
  }
}

void Element::UpdateLayoutInfo() {
  const auto& layout_result = sl_node_->GetLayoutResult();
  width_ = layout_result.size_.width_;
  height_ = layout_result.size_.height_;
  top_ = layout_result.offset_.Y();
  left_ = layout_result.offset_.X();
  paddings_[0] = layout_result.padding_[starlight::kLeft];
  paddings_[1] = layout_result.padding_[starlight::kTop];
  paddings_[2] = layout_result.padding_[starlight::kRight];
  paddings_[3] = layout_result.padding_[starlight::kBottom];
  margins_[0] = layout_result.margin_[starlight::kLeft];
  margins_[1] = layout_result.margin_[starlight::kTop];
  margins_[2] = layout_result.margin_[starlight::kRight];
  margins_[3] = layout_result.margin_[starlight::kBottom];
  borders_[0] = layout_result.border_[starlight::kLeft];
  borders_[1] = layout_result.border_[starlight::kTop];
  borders_[2] = layout_result.border_[starlight::kRight];
  borders_[3] = layout_result.border_[starlight::kBottom];
  display_none_ = sl_node_->GetShouldDisplayNone();

  if (IsShadowNodeCustom()) {
    element_manager_->layout_context()->OnLayout(id_, left_, top_, width_,
                                                 height_, paddings_, borders_);
    customized_layout_node_->OnLayoutAfter();
  }
  if (EnableFragmentLayerRender()) {
    static_cast<Fragment*>(element_container())->UpdateLayout(layout_result);
  }
  frame_changed_ = true;
}

void Element::ResetStyleSheet() { style_sheet_ = nullptr; }

void Element::ResetSheetRecursively(
    const std::shared_ptr<CSSStyleSheetManager>& manager) {
  if (is_page() || is_component() || css_id_ != kInvalidCssId) {
    set_style_sheet_manager(manager);
  }

  // reset style sheet.
  ResetStyleSheet();
  for (const auto& child : children()) {
    child->ResetSheetRecursively(manager);
  }
}

const base::String& Element::GetRawInlineStyles() {
  return full_raw_inline_style_;
}

void Element::SetRawInlineStyles(base::String value) {
  full_raw_inline_style_ = std::move(value);
  MarkDirty(kDirtyStyle);
}

void Element::ParseRawInlineStyles(CSSVariableMap* changed_css_vars) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PARSE_RAW_INLINE_STYLES);
  auto& configs = element_manager_->GetCSSParserConfigs();
  const auto& str = full_raw_inline_style_.str();
  current_raw_inline_custom_properties_->clear();
  data_model()->MoveAndClearCSSInlineVariables(changed_css_vars);

  if (current_raw_important_inline_styles_.has_value()) {
    current_raw_important_inline_styles_->clear();
  }

  ParseStyleDeclarationList(
      str.c_str(), static_cast<uint32_t>(str.size()),
      [this, &configs](const char* key_start, uint32_t key_length,
                       const char* value_start, uint32_t value_length,
                       bool important) {
        (void)configs;
        auto id = CSSProperty::GetPropertyID(
            base::static_string::GenericCacheKey(key_start, key_length));
        if (CSSProperty::IsPropertyValid(id)) {
          auto value = lepus::Value(base::String(value_start, value_length));
          auto& target_map = important ? current_raw_important_inline_styles_
                                       : current_raw_inline_styles_;
          target_map->insert_or_assign(id, std::move(value));
        } else if (IsCSSInlineVariablesEnabled() &&
                   CSSProperty::IsCustomProperty(key_start, key_length)) {
          current_raw_inline_custom_properties_->insert_or_assign(
              base::String(key_start, key_length),
              base::String(value_start, value_length));
          data_model()->UpdateCSSInlineVariables(
              base::String(key_start, key_length),
              base::String(value_start, value_length));
        }

        // DevTool needs to get InlineStyle information from DataModel's
        // InlineStyle, so when DevTool is enabled, it needs to set the
        // corresponding InlineStyle for DataModel.
        EXEC_EXPR_FOR_INSPECTOR(if (element_manager()->IsDomTreeEnabled()) {
          if (data_model() == nullptr) {
            return;
          }
          data_model()->SetInlineStyle(
              id, base::String(value_start, value_length), configs);
        });
      });

  data_model()->UpdateInlineStyleChangedVars(changed_css_vars);

  EXEC_EXPR_FOR_INSPECTOR(if (element_manager()->IsDomTreeEnabled()) {
    element_manager()->OnElementNodeSetForInspector(this);
  });
}

void Element::ProcessFullRawInlineStyle(CSSVariableMap* changed_css_vars) {
  // If self has raw inline styles, parse to current_raw_inline_styles_ but do
  // not process to final style map. Inline styles will be merged finally by
  // MergeInlineStyles.
  if (!full_raw_inline_style_.empty()) {
    ParseRawInlineStyles(changed_css_vars);
    full_raw_inline_style_ = base::String();
  }
}

bool Element::MergeInlineStyles(StyleMap& new_styles,
                                StyleMap& important_styles) {
  // Styles stored by full_raw_inline_style_ had already been parsed to
  // current_raw_inline_styles_. So we only handle current_raw_inline_styles_
  // here.
  bool res = false;
  auto& configs = element_manager_->GetCSSParserConfigs();

  auto process_inline_map = [&](const RawLepusStyleMap& src_map,
                                StyleMap& dst_map) {
    for (const auto& [id, style_value] : src_map) {
      bool process_result =
          UnitHandler::Process(id, style_value, dst_map, configs);
      if (!process_result && IsCSSInlineVariablesEnabled()) {
        base::String style_str = style_value.String();
        CSSStringParser parser{style_str.c_str(),
                               static_cast<uint32_t>(style_str.length()),
                               configs};
        CSSValue css_value = parser.ParseVariable();
        if (parser.HasMetVarToken()) {
          dst_map[id] = std::move(css_value);
          res = true;
        }
      }
    }
  };

  if (current_raw_inline_styles_.has_value()) {
    process_inline_map(*current_raw_inline_styles_, new_styles);
  }
  if (current_raw_important_inline_styles_.has_value()) {
    process_inline_map(*current_raw_important_inline_styles_, important_styles);
  }

  return res;
}

void Element::PersistAnimationFillStyles(const StyleMap& styles) {
  if (!element_manager()->EnableAnimationForwardUpdatePreservation() ||
      !enable_new_animator() || styles.empty()) {
    return;
  }
  for (const auto& [id, value] : styles) {
    animation_override_styles_map_->insert_or_assign(id, value);
  }
}

void Element::ClearPersistedAnimationFillStyle(CSSPropertyID id) {
  if (!animation_override_styles_map_.has_value()) {
    return;
  }
  animation_override_styles_map_->erase(id);
}

void Element::SetDefaultOverflow(bool visible) {
  computed_css_style()->SetOverflowDefaultVisible(visible);
}

void Element::DestroyPlatformNode() {
  if (ShouldTrackImperativeAnimationsForNewPipeline()) {
    ClearImperativeAnimationState();
  }
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
    if (element_manager_ && element_manager_->IsDomTreeEnabled() &&
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
  current_raw_inline_custom_properties_.reset();

  MarkDirty(kDirtyStyle);
}

void Element::RemoveAllImportantInlineStyles() {
  EXEC_EXPR_FOR_INSPECTOR({
    if (element_manager_ && element_manager_->IsDomTreeEnabled() &&
        current_raw_important_inline_styles_.has_value()) {
      for (const auto& pair : *current_raw_important_inline_styles_) {
        const static base::String kNull;
        data_model()->SetInlineStyle(pair.first, kNull,
                                     element_manager_->GetCSSParserConfigs());
      }
    }
  });

  current_raw_important_inline_styles_.reset();
  MarkDirty(kDirtyStyle);
}

void Element::CacheStyleFromAttributes(CSSPropertyID id, CSSValue&& value) {
  CacheCommittedStyleFromAttributes(id, value);
  styles_from_attributes_->insert_or_assign(id, std::move(value));
}

void Element::CacheStyleFromAttributes(CSSPropertyID id,
                                       const lepus::Value& value) {
  CacheCommittedStyleFromAttributes(id, value);
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
  if (element_manager_ && element_manager_->EnableNewStylingPipeline()) {
    const auto& resolved_values = computed_css_style()->GetResolvedValues();
    auto iter = resolved_values.find(css_id);
    if (iter != resolved_values.end()) {
      return iter->second;
    }
    return {};
  }

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
