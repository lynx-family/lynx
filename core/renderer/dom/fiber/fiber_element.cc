// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/fiber_element.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/include/compiler_specific.h"
#include "base/include/path_utils.h"
#include "base/include/timer/time_utils.h"
#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_defines.h"
#include "base/trace/native/trace_event.h"
#include "core/event/event.h"
#include "core/renderer/css/computed_css_style_css_text_helper.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_property_bitset.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/dom/fiber/block_element.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/none_element.h"
#include "core/renderer/dom/fiber/platform_layout_function_wrapper.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/template_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/list_item_fragment_behavior.h"
#include "core/renderer/dom/fragment/platform_extended_fragment_behavior.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/style_resolver.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/events/events.h"
#include "core/renderer/events/touch_event_handler.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"
#include "core/renderer/utils/value_utils.h"
#include "core/renderer/worklet/lepus_raf_handler.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/js/bindings/java_script_element.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/runtime/lepusng/napi/worklet/napi_func_callback.h"
#include "core/runtime/mts_context.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/feature_count/global_feature_counter.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

namespace {

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

FiberElement *ResolveTemplateRootForAction(Element *element) {
  if (element == nullptr) {
    return nullptr;
  }
  auto *fiber_element = static_cast<FiberElement *>(element);
  if (!fiber_element->is_template()) {
    return fiber_element;
  }

  auto root = static_cast<TemplateElement *>(fiber_element)->GetRoot();
  return root != nullptr ? root.get() : fiber_element;
}

bool ContainsLayoutOnlyStyle(const StyleMap &resolved_style_map) {
  for (const auto &[id, _] : resolved_style_map) {
    if (LayoutProperty::IsLayoutOnly(id)) {
      return true;
    }
  }
  return false;
}

bool DiffPreviousResolvedLayoutOnlyStylesForNewPipeline(
    const starlight::ComputedCSSStyle &previous_final_style,
    const StyleMap &resolved_style_map, StyleMap &changed_layout_only_styles,
    base::InlineVector<CSSPropertyID, 16> &reset_layout_only_ids) {
  const auto &previous_resolved_values =
      previous_final_style.GetResolvedValues();
  changed_layout_only_styles.clear();
  reset_layout_only_ids.clear();

  for (const auto &[id, value] : resolved_style_map) {
    if (!LayoutProperty::IsLayoutOnly(id)) {
      continue;
    }

    auto it = previous_resolved_values.find(id);
    if (it == previous_resolved_values.end() || it->second != value) {
      changed_layout_only_styles.insert_or_assign(id, value);
    }
  }

  for (const auto &[id, _] : previous_resolved_values) {
    if (!LayoutProperty::IsLayoutOnly(id)) {
      continue;
    }
    if (resolved_style_map.find(id) == resolved_style_map.end()) {
      reset_layout_only_ids.emplace_back(id);
    }
  }

  return !changed_layout_only_styles.empty() || !reset_layout_only_ids.empty();
}

struct NewPipelineDynamicStyleInputs {
  StyleMap resolved_style_map;
  DynamicCSSStylesManager::StyleUpdateFlags inherited_dynamic_flags{0};
};

NewPipelineDynamicStyleInputs BuildDynamicStyleInputsForNewPipeline(
    const FiberElement &element, const starlight::ComputedCSSStyle &final_style,
    const StyleMap &explicit_resolved_style_map) {
  NewPipelineDynamicStyleInputs result;
  result.resolved_style_map = explicit_resolved_style_map;

  if (!element.IsCSSInheritanceEnabled()) {
    return result;
  }

  const auto explicit_style_ids =
      CSSIDBitset::FromKeys(explicit_resolved_style_map);
  const auto &css_config = element.element_manager()->GetDynamicCSSConfigs();
  for (const auto &[id, value] : final_style.GetResolvedValues()) {
    if (id == kPropertyIDFontSize || explicit_style_ids.Has(id) ||
        !element.IsInheritable(id)) {
      continue;
    }

    const auto value_flags = DynamicCSSStylesManager::GetValueFlags(
        id, value, css_config.unify_vw_vh_behavior_,
        element.element_manager()->FixFilterDynamicUpdateBug());
    if (value_flags == 0) {
      continue;
    }

    result.resolved_style_map.insert_or_assign(id, value);
    result.inherited_dynamic_flags |= value_flags;
  }
  return result;
}
}  // namespace

event::EventListener::Options FiberElement::GetEventListenerOptions(
    const base::String &type) {
  const bool is_capture = type.str() == EVENT_TYPE_CAPTURE;
  const bool is_capture_catch = type.str() == EVENT_TYPE_CAPTURE_CATCH;
  const bool is_bubble_catch = type.str() == EVENT_TYPE_CATCH;
  const bool is_global_bind = type.str() == EVENT_TYPE_GLOBAL;
  return event::EventListener::Options(
      is_capture || is_capture_catch, false, false, false,
      is_capture_catch || is_bubble_catch, is_global_bind);
}

FiberElement::NewPipelineStyleResolveResult FiberElement::ResolveComputedStyles(
    const starlight::ComputedCSSStyle *previous_final_style,
    double old_font_size, double old_root_font_size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_RESOLVE_COMPUTED_STYLES);
  NewPipelineStyleResolveResult resolve_result;
  resolve_result.previous_final_style = previous_final_style;
  resolve_result.parent_inheritance_style = GetParentComputedCSSStyle();
  if (IsNewlyCreated()) {
    style_resolver_.ResolveBaseStyleInPlace(
        *platform_css_style_, resolve_result.parent_inheritance_style,
        previous_final_style, &resolve_result.resolved_style_map);
  } else {
    resolve_result.owned_base_style = style_resolver_.ResolveBaseStyle(
        resolve_result.parent_inheritance_style, previous_final_style,
        &resolve_result.resolved_style_map);
  }
  resolve_result.BindResolvedStyles(platform_css_style_.get());
  return resolve_result;
}

FiberElement::FiberElement(ElementManager *manager, const base::String &tag)
    : FiberElement(manager, tag, kInvalidCssId) {}

FiberElement::FiberElement(ElementManager *manager, const base::String &tag,
                           int32_t css_id)
    : Element(tag, manager, kInvalidNodeIndex) {
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSTRUCTOR, "tag",
                      tag.c_str(), "id", id_);
  dirty_ = kDirtyCreated;
  css_id_ = css_id;
  InitLayoutBundle();
  SetAttributeHolder(fml::MakeRefCounted<AttributeHolder>(this));

  if (tag.IsEquals("x-overlay-ng") || tag.IsEquals("overlay")) {
    can_has_layout_only_children_ = false;
  }

  if (manager == nullptr) {
    return;
  }

  element_context_delegate_ = manager;

  // Set font scale and font size if needed.
  const auto &env_config = manager->GetLynxEnvConfig();

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

FiberElement::FiberElement(const FiberElement &element,
                           bool clone_resolved_props)
    : Element(element, clone_resolved_props) {
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

  element_context_delegate_ = element.element_context_delegate_;
  // TODO(wujintian): Clone animation-related objects.
}

void FiberElement::FiberAddEvent(const base::String &type,
                                 const base::String &name,
                                 const lepus::Value &callback,
                                 const std::string &context_name) {
  auto event_options = GetEventListenerOptions(type);
  auto event_name = name.str();
  auto *manager = element_manager();
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
      auto *default_vm_context = manager->GetDefaultEntryRuntime();
      auto default_entry_name = manager->GetDefaultEntryLogicalName();
      remove_event_listener(event::ClosureEventListener::ClosureType::kJS);
      AddEventListener(
          event_name,
          std::make_unique<event::ClosureEventListener>(
              [element = this, event_name, handler_name,
               should_handle_air_fiber_event, support_component_js,
               default_vm_context, default_entry_name](lepus::Value args) {
                const auto &args_array = args.Array();
                if (!args.IsArray() || args_array->size() != 3) {
                  return;
                }
                const auto &event_info = args_array->get(0);
                const auto &event_detail = args_array->get(1);
                const auto &event_info_array = event_info.Array();
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
                  auto *target =
                      static_cast<FiberElement *>(event->target().get());
                  auto *current_target = static_cast<FiberElement *>(
                      event->current_target().get());
                  auto *parent_component =
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
                const auto &args_array = args.Array();
                if (!args.IsArray() || args_array->size() != 3) {
                  return;
                }
                const auto &event_info = args_array->get(0);
                const auto &event_detail = args_array->get(1);
                auto event = fml::static_ref_ptr_cast<event::Event>(
                    args_array->get(2).RefCounted());
                const auto &event_info_array = event_info.Array();
                if (!event_info.IsArray() || event_info_array->size() != 3) {
                  return;
                }
                const auto &component_id = event_info_array->get(0).StdString();
                const auto &entry_name = event_info_array->get(1).StdString();
                int32_t element_id = event_info_array->get(2).Int32();
                auto *manager = element->element_manager();
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
    const auto &obj_type = callback.GetProperty(kType).StdString();
    const auto &value = callback.GetProperty(kValue);

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
                auto *manager = element->element_manager();
                if (manager == nullptr || worklet_value.IsEmpty()) {
                  return;
                }
                auto *context = manager->GetEntryRuntime(context_name);
                if (context == nullptr) {
                  return;
                }
                const auto &args_array = args.Array();
                if (!args.IsArray() || args_array->size() != 3) {
                  return;
                }
                const auto &event_detail = args_array->get(1);
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

void FiberElement::AttachToElementManager(
    ElementManager *manager,
    const std::shared_ptr<CSSStyleSheetManager> &style_manager,
    bool keep_element_id) {
  Element::AttachToElementManager(manager, style_manager, keep_element_id);

  const auto &env_config = manager->GetLynxEnvConfig();
  if (platform_css_style_ == nullptr) {
    platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
        *manager->platform_computed_css());
  }
  record_parent_font_size_ = env_config.PageDefaultFontSize();

  // ComputedCSSStyle
  platform_css_style_->SetScreenWidth(env_config.ScreenWidth());
  platform_css_style_->SetViewportHeight(env_config.ViewportHeight());
  platform_css_style_->SetViewportWidth(env_config.ViewportWidth());
  platform_css_style_->SetCssAlignLegacyWithW3c(
      manager->GetLayoutConfigs().css_align_with_legacy_w3c_);
  platform_css_style_->SetFontScaleOnlyEffectiveOnSp(
      manager->GetLynxEnvConfig().FontScaleSpOnly());
  platform_css_style_->SetLayoutUnit(env_config.PhysicalPixelsPerLayoutUnit(),
                                     env_config.LayoutsUnitPerPx());

  computed_css_style()->SetEnableZIndex(manager->GetEnableZIndex());

  // Create layout node and update layout styles
  InitLayoutBundle();
  UpdateLayoutNodeFontSize(GetFontSize(), GetRecordedRootFontSize());

  if (layout_styles_.has_value()) {
    for (auto &layout_style : *layout_styles_) {
      UpdateLayoutNodeStyle(layout_style.first, layout_style.second);
    }
  }

  SetFontSizeForAllElement(GetFontSize(), GetRecordedRootFontSize());

  if (Config::DefaultFontScale() != env_config.FontScale()) {
    computed_css_style()->SetFontScale(env_config.FontScale());
  }

  if (Config::DefaultFontScale() != env_config.FontScale()) {
    SetComputedFontSize(env_config.PageDefaultFontSize(),
                        env_config.PageDefaultFontSize());
  }

  if (element_manager_->GetEnableStandardCSSSelector()) {
    // in new selector, mark style dirty while Created.
    MarkDirty(kDirtyStyle);
  }

  element_context_delegate_ = manager;
}

void FiberElement::OnNodeAdded(FiberElement *child) {
  if (child != nullptr) {
    bool is_compatible_parent =
        !is_page() && !is_view() && !is_text() && !is_image();
    if (is_wrapper() || is_component()) {
      is_compatible_parent = is_direct_child_of_compatible_component();
    }
    child->MarkAsDirectChildOfCompatibleComponent(is_compatible_parent);
  }
  if (IsRadonArch()) {
    if (element_manager_ && element_manager_->FixRadonInlineConvertBug()) {
      if (child != nullptr && is_inline_element() &&
          (!is_component() || is_wrapper())) {
        child->ConvertToInlineElement();
      }
    } else {
      if (is_inline_element() && child != nullptr && !is_component()) {
        child->ConvertToInlineElement();
      }
    }
  } else {
    if (is_inline_element() && child != nullptr && !is_component()) {
      child->ConvertToInlineElement();
    }
  }

  UpdateRenderRootElementIfNecessary(child);
}

void FiberElement::OnNodeRemoved(FiberElement *child) {
  if (child != nullptr) {
    child->MarkAsDirectChildOfCompatibleComponent(false);
  }
}

FiberElement::~FiberElement() {
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_DESTRUCTOR, "id", id_);
  if (ShouldDestroy()) {
    element_manager_->EraseGlobalBindElementId(global_bind_event_map(),
                                               impl_id());
    element_manager()->NotifyElementDestroy(this);
    DestroyPlatformNode();
    if (!EnableLayoutInElementMode()) {
      EnqueueLayoutTask([manager = element_manager(), id = impl_id()]() {
        manager->DestroyLayoutNode(id);
      });
    } else if (customized_layout_node_) {
      EnqueueLayoutTask([layout_node_ = std::move(customized_layout_node_),
                         manager = element_manager(),
                         id = impl_id()]() mutable {
        manager->DestroyLayoutNode(id);
        if (layout_node_) {
          layout_node_->Destroy();
        }
      });
    }
    element_manager()->node_manager()->Erase(id_);
    // If FiberElement to be destroyed is the root of its ElementContext, need
    // to remove corresponding ElementContext from tree
    if (element_context_delegate_ &&
        element_context_delegate_->GetElementContextRoot() == this) {
      element_context_delegate_->RemoveSelf();
    }
  }
}

const FiberElement::InheritedProperty FiberElement::GetInheritedProperty() {
  return {
      children_propagate_inherited_styles_flag_, inherited_styles_.get(),
      reset_inherited_ids_.get(),
      custom_properties_.Get() ? &custom_properties_.Get()->Value() : nullptr};
}

const FiberElement::InheritedProperty
FiberElement::GetParentInheritedProperty() {
  // If in a parallel flush process or if the parent is null, return
  // empty InheritedProperty indicating that it is not necessary to consider the
  // inheritance logic at this time.
  if (this->is_greedy_parallel_flush()) {
    return {false, nullptr, nullptr,
            custom_properties_.Get() ? &custom_properties_.Get()->Value()
                                     : nullptr};
  }

  FiberElement *real_parent = static_cast<FiberElement *>(parent());
  if (real_parent == nullptr) {
    return InheritedProperty();
  }

  return real_parent->GetInheritedProperty();
}

CSSFragment *FiberElement::GetRelatedCSSFragment() {
  if (css_id_ != kInvalidCssId) {
    if (!style_sheet_) {
      if (!css_style_sheet_manager_ && GetParentComponentElement()) {
        css_style_sheet_manager_ =
            static_cast<ComponentElement *>(GetParentComponentElement())
                ->style_sheet_manager();
      }
      CSSFragment *fragment =
          css_style_sheet_manager_
              ? css_style_sheet_manager_->GetCSSStyleSheetForComponent(css_id_)
              : nullptr;
      style_sheet_ =
          std::make_unique<CSSFragmentDecorator>(fragment, element_manager());
      if (style_sheet_ && style_sheet_->HasTouchPseudoToken()) {
        element_manager()->UpdateTouchPseudoStatus(true);
      }
    }
    return style_sheet_.get();
  } else {
    auto *parent_component = GetParentComponentElement();
    CSSFragment *result = nullptr;
    if (parent_component) {
      result =
          static_cast<ComponentElement *>(parent_component)->GetCSSFragment();
      auto css_fragment_decorator = static_cast<CSSFragmentDecorator *>(result);
      if (css_fragment_decorator &&
          css_fragment_decorator->IntrinsicStyleSheetHasTouchPseudoToken()) {
        element_manager()->UpdateTouchPseudoStatus(true);
      }
    }
    if (!result && element_manager_ &&
        element_manager_->HasAdoptedStyleSheets()) {
      if (!style_sheet_) {
        style_sheet_ =
            std::make_unique<CSSFragmentDecorator>(nullptr, element_manager());
      }
      result = style_sheet_.get();
    }
    return result;
  }
}

int32_t FiberElement::GetCSSID() const {
  if (css_id_ != kInvalidCssId) {
    return css_id_;
  } else {
    auto *parent_component = GetParentComponentElement();
    if (parent_component) {
      return static_cast<ComponentElement *>(parent_component)
          ->GetComponentCSSID();
    } else {
      return kInvalidCssId;
    }
  }
}

bool FiberElement::MergeInlineStyles(StyleMap &new_styles,
                                     StyleMap &important_styles) {
  // Styles stored by full_raw_inline_style_ had already been parsed to
  // current_raw_inline_styles_. So we only handle current_raw_inline_styles_
  // here.
  bool res = false;
  auto &configs = element_manager_->GetCSSParserConfigs();

  auto process_inline_map = [&](const RawLepusStyleMap &src_map,
                                StyleMap &dst_map) {
    for (const auto &[id, style_value] : src_map) {
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

void FiberElement::PersistAnimationFillStyles(const StyleMap &styles) {
  if (!element_manager()->EnableAnimationForwardUpdatePreservation() ||
      !enable_new_animator() || styles.empty()) {
    return;
  }
  for (const auto &[id, value] : styles) {
    animation_override_styles_map_->insert_or_assign(id, value);
  }
}

void FiberElement::ClearPersistedAnimationFillStyle(CSSPropertyID id) {
  if (!animation_override_styles_map_.has_value()) {
    return;
  }
  animation_override_styles_map_->erase(id);
}

void FiberElement::ProcessFullRawInlineStyle(CSSVariableMap *changed_css_vars) {
  // If self has raw inline styles, parse to current_raw_inline_styles_ but do
  // not process to final style map. Inline styles will be merged finally by
  // MergeInlineStyles.
  if (!full_raw_inline_style_.empty()) {
    ParseRawInlineStyles(changed_css_vars);
    full_raw_inline_style_ = base::String();
  }
}

void FiberElement::DispatchAsyncResolveProperty() {
  if ((dirty_ & ~kDirtyTree) != 0 && IsAttached()) {
    UpdateResolveStatus(AsyncResolveStatus::kPreparing);
    ResolveParentComponentElement();
    if (parent()) {
      parent()->EnsureTagInfo();
    }
    PostResolveTaskToThreadPool(false, element_manager()->ParallelTasks());
  }
}

#pragma region simple styling

void FiberElement::SetStyleObjects(
    std::unique_ptr<style::StyleObject *, style::StyleObjectArrayDeleter>
        style_objects) {
  last_style_objects_ = std::move(style_objects_);

  style_objects_ = std::move(style_objects);

  MarkDirty(kDirtyForceUpdate | kDirtyStyleObjects);
}

void FiberElement::ReplaceDynamicSimpleStyles(
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

void FiberElement::AddDynamicSimpleStyles(tasm::StyleMap &&new_styles) {
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

void FiberElement::RemoveDynamicSimpleStyleKV(tasm::CSSPropertyID id) {
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

void FiberElement::AddDynamicSimpleStyleKV(tasm::CSSPropertyID id,
                                           tasm::CSSValue &&value) {
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

void FiberElement::ApplySimpleStyleWithoutTail(const tasm::CSSPropertyID id,
                                               const tasm::CSSValue &value) {
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

void FiberElement::ApplySimpleStylesWithoutTail(
    const tasm::StyleMap &style_map) {
  std::for_each(style_map.begin(), style_map.end(),
                [this](const auto &pair) -> void {
                  ApplySimpleStyleWithoutTail(pair.first, pair.second);
                });
}

void FiberElement::ApplyDynamicSimpleStylesWithoutTail(
    const tasm::StyleMap &dynamic_style_map,
    const tasm::StyleMap &base_style_map) {
  std::for_each(dynamic_style_map.begin(), dynamic_style_map.end(),
                [this, &base_style_map](const auto &pair) -> void {
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

void FiberElement::FinalizeSimpleStyleUpdate() {
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

void FiberElement::UpdateSimpleStyles(tasm::StyleMap &&style_map) {
  parsed_styles_map_ = std::move(style_map);
  ApplySimpleStylesWithoutTail(parsed_styles_map_);
  FinalizeSimpleStyleUpdate();
}

void FiberElement::UpdateStaticAndDynamicSimpleStyles(
    tasm::StyleMap &&style_map, tasm::StyleMap &&dynamic_style_map) {
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

void FiberElement::UpdateDynamicSimpleStyles(tasm::StyleMap &&style_map) {
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

void FiberElement::UpdateSimpleStyles(const tasm::StyleMap &style_map) {
  ApplySimpleStylesWithoutTail(style_map);
  FinalizeSimpleStyleUpdate();
}

void FiberElement::ResetSimpleStyle(const tasm::CSSPropertyID id,
                                    const tasm::CSSValue &value) {
  ApplySimpleStyleWithoutTail(id, value);
}

void FiberElement::ResetSimpleStyle(const tasm::CSSPropertyID id) {
  ApplySimpleStyleWithoutTail(id, CSSValue());
}

#pragma endregion  // simple styling

void FiberElement::AsyncResolveProperty() {
  if ((dirty_ & ~kDirtyTree) != 0) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_ASYNC_RESOLVE_PROPERTY);
    UpdateResolveStatus(AsyncResolveStatus::kPrepareRequested);
    if (this->IsAttached()) {
      AsyncPostResolveTaskToThreadPool();
    }
  }
}

void FiberElement::AsyncPostResolveTaskToThreadPool() {
  if ((dirty_ & ~kDirtyTree) != 0) {
    UpdateResolveStatus(AsyncResolveStatus::kPrepareTriggered);
    element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
      UpdateResolveStatus(AsyncResolveStatus::kPreparing);
      ResolveParentComponentElement();
      if (parent()) {
        parent()->EnsureTagInfo();
      }
      PostResolveTaskToThreadPool(false, element_manager()->ParallelTasks());
    });
  }
}

void FiberElement::ReplaceElements(
    const base::Vector<fml::RefPtr<FiberElement>> &inserted,
    const base::Vector<fml::RefPtr<FiberElement>> &removed, FiberElement *ref) {
  if (removed.empty()) {
    for (const auto &child : inserted) {
      InsertNodeBeforeInternal(child, ref);
    }
    return;
  }

  // 1. Make sure remove first.
  // 2. And exec InsertNodeBeforeInternal(child, ref).

  for (const auto &child : removed) {
    RemoveNode(child);
  }
  if (!inserted.empty()) {
    for (const auto &child : inserted) {
      InsertNodeBeforeInternal(child, ref);
    }
  }
}

void FiberElement::InsertNode(const fml::RefPtr<Element> &raw_child) {
  InsertNode(raw_child, static_cast<int32_t>(scoped_children_.size()));
}

void FiberElement::InsertLogicalChildBefore(
    const fml::RefPtr<FiberElement> &child, FiberElement *ref_node) {
  if (ref_node == nullptr) {
    logical_children_.push_back(child);
    return;
  }

  auto it = std::find_if(logical_children_.begin(), logical_children_.end(),
                         [ref_node](const fml::RefPtr<Element> &logical_child) {
                           return logical_child.get() == ref_node;
                         });
  if (it != logical_children_.end()) {
    logical_children_.insert(it, child);
    return;
  }

  logical_children_.push_back(child);
}

void FiberElement::RemoveLogicalChild(const fml::RefPtr<FiberElement> &child) {
  auto it = std::find_if(logical_children_.begin(), logical_children_.end(),
                         [&child](const fml::RefPtr<Element> &logical_child) {
                           return logical_child.get() == child.get();
                         });
  if (it != logical_children_.end()) {
    logical_children_.erase(it);
  }
}

void FiberElement::InsertNode(const fml::RefPtr<Element> &raw_child,
                              int32_t index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_INSERT_NODE);
  auto child = fml::static_ref_ptr_cast<FiberElement>(raw_child);

  if (index < 0 || index > static_cast<int>(scoped_children_.size())) {
    LOGE("[FiberElement] InsertNode index is out of bounds, index:"
         << index << ",size:" << scoped_children_.size());
    return;
  }
  FiberElement *ref =
      (index < static_cast<int>(scoped_children_.size()))
          ? static_cast<FiberElement *>(scoped_children_[index].get())
          : nullptr;
  // reserve parent node for block element in AirModeFiber
  if (element_manager() && element_manager()->IsAirModeFiberEnabled() &&
      child->is_block()) {
    child->set_parent(this);
    InsertLogicalChildBefore(child, ref);
    scoped_virtual_children_->push_back(child);
    return;
  }
  // ref_node: nullptr: means to append this node to the end
  InsertNodeBeforeInternal(child, ref);
}

void FiberElement::InsertNodeBeforeInternal(
    const fml::RefPtr<FiberElement> &child, FiberElement *ref_node) {
  InsertNodeBeforeInternal(child, ref_node, true);
}

void FiberElement::InsertNodeBeforeInternal(
    const fml::RefPtr<FiberElement> &child, FiberElement *ref_node,
    bool update_logical_children) {
  int index = -1;
  if (ref_node) {
    index = IndexOf(ref_node);
    if (index >= static_cast<int>(scoped_children_.size()) || index < 0) {
      LOGE("[Fiber] can not find the ref node:" << ref_node);
      return;
    }
  }
  if (child->parent_ != nullptr) {
    LOGE(
        "FiberElement re-insert node, try to do remove node from old parent "
        "first");
    this->LogNodeInfo();
    child->LogNodeInfo();
    static_cast<FiberElement *>(child->parent_)->LogNodeInfo();
    static_cast<FiberElement *>(child->parent_)->RemoveNode(child);
  }
  if (update_logical_children) {
    InsertLogicalChildBefore(child, ref_node);
  }
  // FIXME(linxs): use linked element to reduce the Element index calculation
  AddChildAt(child, index);

  // the insert Action should be inserted to Child, should make sure the child
  // has been flushed
  if (has_to_store_insert_remove_actions_) {
    action_param_list_.emplace_back(Action::kInsertChildAct, this, child, index,
                                    ref_node, child->is_fixed_);
  }

  if (IsCSSInheritanceEnabled()) {
    // new inserted child should be marked to do inheritance from parent
    child->MarkDirty(kDirtyPropagateInherited);
  }

  // Invalidate ref_node for next-sibling combinator (A + B).
  if (ref_node && HasAdjacentSiblingRulesInStyleSheets()) {
    ref_node->MarkStyleDirty(false);
  }

  MarkDirty(kDirtyTree);
}

void FiberElement::InsertNodeBefore(
    const fml::RefPtr<FiberElement> &child,
    const fml::RefPtr<FiberElement> &reference_child) {
  InsertNodeBeforeInternal(child, reference_child.get());
}

void FiberElement::RemoveNode(const fml::RefPtr<Element> &raw_child,
                              bool destroy) {
  auto child = fml::static_ref_ptr_cast<FiberElement>(raw_child);
  RemoveNodeInternal(child, destroy, true);
}

void FiberElement::RemoveNodeInternal(const fml::RefPtr<FiberElement> &child,
                                      bool destroy,
                                      bool update_logical_children) {
  // FIXME(linxs): to use linked node to avoid the index calculation asap!
  int index = IndexOf(child.get());
  if (index >= static_cast<int>(scoped_children_.size()) || index < 0) {
    LOGE("FiberElement RemoveNode got wrong child index!!");
    return;
  }

  // Capture next sibling before removal for next-sibling combinator (A + B).
  FiberElement *next_sibling_of_removed =
      static_cast<FiberElement *>(child->next_sibling());

  // the Remove Action should be inserted to Parent, due to child has been
  // removed from element tree here
  if (has_to_store_insert_remove_actions_) {
    action_param_list_.emplace_back(Action::kRemoveChildAct, this, child, index,
                                    nullptr, child->is_fixed_,
                                    child->ZIndex() != 0);
  }

  // take care: NotifyNodeRemoved after removeAction inserted!
  OnNodeRemoved(child.get());
  TreeResolver::NotifyNodeRemoved(this, child.get());

  FiberElement *removed =
      static_cast<FiberElement *>(scoped_children_[index].get());
  scoped_children_.erase(scoped_children_.begin() + index);
  if (update_logical_children) {
    RemoveLogicalChild(child);
  }
  removed->set_parent(nullptr);

  // Invalidate next sibling for next-sibling combinator (A + B).
  if (next_sibling_of_removed && HasAdjacentSiblingRulesInStyleSheets()) {
    next_sibling_of_removed->MarkStyleDirty(false);
  }

  MarkDirty(kDirtyTree);
}

void FiberElement::InsertedInto(FiberElement *insertion_point) {
  MarkAttached();
  if (resolve_status_ == AsyncResolveStatus::kPrepareRequested) {
    AsyncPostResolveTaskToThreadPool();
  }
  EXEC_EXPR_FOR_INSPECTOR(if (element_manager() != nullptr) {
    element_manager()->RunDevToolFunction(
        lynx::devtool::DevToolFunction::InitStyleRoot, std::make_tuple(this));
  });
}

void FiberElement::RemovedFrom(FiberElement *insertion_point) {
  // We need to handle the intergenerational node which has zIndex or fixed,
  // they may be inserted to difference parent in UI/layout tree instead of dom
  // parent If the removed node's parent is the insertion_point, no need to do
  // any special action

  // Todo(kechenglong): Remove IsRadonArch.
  if (IsRadonArch()) {
    if (IsDetached()) {
      return;
    }

    // If EnableFragmentLayerRender(), we need to handle the removal and
    // insertion of descendant nodes with z-index or fixed in Fragment, so
    // should not check the action_param_list_ here.
    if (!action_param_list_.empty() && !EnableFragmentLayerRender()) {
      auto iter = action_param_list_.begin();
      while (iter != action_param_list_.end()) {
        if (iter->type_ == Action::kRemoveIntergenerationAct ||
            (iter->type_ == Action::kRemoveChildAct &&
             (iter->is_fixed_ || iter->has_z_index_))) {
          iter->type_ = Action::kRemoveIntergenerationAct;
          insertion_point->action_param_list_.emplace_back(std::move(*iter));
          iter = action_param_list_.erase(iter);
        } else {
          ++iter;
        }
      }
    }
  }

  // If EnableFragmentLayerRender(), we need to handle the removal and insertion
  // of descendant nodes with z-index or fixed in Fragment, so should not check
  // the action_param_list_ here.
  if ((parent() != insertion_point) && (ZIndex() != 0 || is_fixed_) &&
      !EnableFragmentLayerRender()) {
    insertion_point->action_param_list_.emplace_back(
        Action::kRemoveIntergenerationAct, insertion_point,
        fml::RefPtr<FiberElement>(this), 0, nullptr, is_fixed_);
    MarkDirty(kDirtyReAttachContainer);
  }

  MarkDetached();
}

StyleMap FiberElement::GetStylesForWorklet() {
  if (element_manager()->EnableNewStylingPipeline()) {
    return computed_css_style()->GetResolvedValues();
  }

  if (!IsCSSInheritanceEnabled()) {
    return parsed_styles_map_;
  }

  StyleMap result;
  const auto inherited_property = GetParentInheritedProperty();
  if (inherited_property.inherited_styles_ != nullptr) {
    result = *inherited_property.inherited_styles_;
  }
  for (const auto &pair : parsed_styles_map_) {
    result.emplace_or_assign(pair.first, pair.second);
  }
  return result;
}

static bool DiffStyleImpl(StyleMap &old_map, StyleMap &new_map,
                          StyleMap &update_styles) {
  if (new_map.empty()) {
    return false;
  }
  // When the first screen is rendered, old_map must be empty, so there is no
  // need to perform the following for loop.
  if (old_map.empty()) {
    update_styles = new_map;
    return true;
  }
  update_styles.reserve(old_map.size() + new_map.size());
  bool need_update = false;
  // iterate all styles in new_map
  for (const auto &[key, value] : new_map) {
    // try to find the corresponding style in old_map
    auto it_old_map = old_map.find(key);
    // if r does not exist in lhs, r is a new style to add
    // if r exist in lhs but with different value, update it
    if (it_old_map == old_map.end() || value != it_old_map->second) {
      need_update = true;
      update_styles.insert_or_assign(key, value);
    }
    // erase old property which is already in new_map, then the remaining
    // properties in old_map need to be removed
    if (it_old_map != old_map.end()) {
      old_map.erase(it_old_map);
    }
  }
  return need_update;
}

void FiberElement::ResetDirectionAwareProperty(const CSSPropertyID &id,
                                               const CSSValue &value) {
  auto css_id = id;
  auto direction_mapping = CheckDirectionMapping(css_id);
  auto is_direction_aware_property =
      direction_mapping.rtl_property_ != kPropertyStart ||
      direction_mapping.ltr_property_ != kPropertyStart;
  if (is_direction_aware_property) {
    auto current_direction = computed_css_style()->GetDirection();
    auto tran_css_id =
        (IsRTL(current_direction) && direction_mapping.is_logic_) ||
                IsLynxRTL(current_direction)
            ? direction_mapping.rtl_property_
            : direction_mapping.ltr_property_;
    ResetCSSValue(tran_css_id);
    (*pending_updated_direction_related_styles_)[css_id] = {
        value, direction_mapping.is_logic_};
  }
}

void FiberElement::HandleKeyframePropsChange() {
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

void FiberElement::HandleDelayTask(base::MoveOnlyClosure<void> operation) {
  if (this->is_parallel_flush()) {
    parallel_reduce_tasks_->emplace_back(std::move(operation));
  } else {
    operation();
  }
}

void FiberElement::ResolveSimpleStyles() {
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
FiberElement::CollectDynamicFlagsForNewPipeline(
    const StyleMap &resolved_style_map) const {
  DynamicCSSStylesManager::StyleUpdateFlags flags = 0;
  const auto &css_config = element_manager()->GetDynamicCSSConfigs();
  for (const auto &[id, value] : resolved_style_map) {
    flags |= DynamicCSSStylesManager::GetValueFlags(
        id, value, css_config.unify_vw_vh_behavior_,
        element_manager()->FixFilterDynamicUpdateBug());
  }
  return flags;
}

FiberElement::NewPipelineResolveOutcome
FiberElement::ResolveCSSStylesNewPipelineCore(
    const NewPipelineResolveRequest &request) {
  NewPipelineResolveOutcome outcome;
  const bool has_cached_styles_from_attributes =
      PeekCachedStylesFromAttributes() != nullptr;
  const bool should_resolve =
      (dirty_ & (kDirtyStyle | kDirtyRefreshCSSVariables |
                 kDirtyPropagateInherited | kDirtyFontSize)) ||
      has_cached_styles_from_attributes || request.force_resolve;
  if (should_resolve) {
    const auto *old_final_style = computed_css_style();
    const auto old_font_size = old_final_style->GetFontSize();
    const auto old_root_font_size = old_final_style->GetRootFontSize();
    bool first_render = (dirty_ & kDirtyCreated) > 0;
    auto resolved_styles = ResolveComputedStyles(old_final_style, old_font_size,
                                                 old_root_font_size);

    // Resolve pseudo-element styles for the new pipeline.
    style_resolver_.ResolvePseudoElementsForNewPipeline(
        GetRelatedCSSFragment());

    StyleMap changed_layout_only_styles;
    base::InlineVector<CSSPropertyID, 16> reset_layout_only_ids;

    bool style_changed = false;
    if (first_render) {
      style_changed =
          resolved_styles.final_style->IsDirty() ||
          ContainsLayoutOnlyStyle(resolved_styles.resolved_style_map);
    } else {
      style_changed = style_resolver_.ComputeStyleDiff(
          *resolved_styles.final_style, *resolved_styles.previous_final_style);
      style_changed = DiffPreviousResolvedLayoutOnlyStylesForNewPipeline(
                          *resolved_styles.previous_final_style,
                          resolved_styles.resolved_style_map,
                          changed_layout_only_styles, reset_layout_only_ids) ||
                      style_changed;
    }

    const auto dynamic_style_inputs = BuildDynamicStyleInputsForNewPipeline(
        *this, *resolved_styles.final_style,
        resolved_styles.resolved_style_map);
    const auto resolved_dynamic_style_flags = CollectDynamicFlagsForNewPipeline(
        dynamic_style_inputs.resolved_style_map);
    outcome.dynamic_style_flags = resolved_dynamic_style_flags;
    const bool dynamic_refresh_needed =
        request.force_platform_update ||
        ((request.dynamic_update_flags & resolved_dynamic_style_flags) != 0);
    const bool should_commit = style_changed || dynamic_refresh_needed;

    if (!first_render) {
      auto custom_properties_changed = [old_final_style, &resolved_styles]() {
        const auto *new_raw_custom_properties =
            resolved_styles.final_style->GetRawCustomProperties();
        const auto *old_raw_custom_properties =
            old_final_style->GetRawCustomProperties();
        const auto *new_custom_properties =
            resolved_styles.final_style->GetCustomProperties();
        const auto *old_custom_properties =
            old_final_style->GetCustomProperties();
        return (new_raw_custom_properties == nullptr) !=
                   (old_raw_custom_properties == nullptr) ||
               (new_raw_custom_properties != nullptr &&
                old_raw_custom_properties != nullptr &&
                *new_raw_custom_properties != *old_raw_custom_properties) ||
               (new_custom_properties == nullptr) !=
                   (old_custom_properties == nullptr) ||
               (new_custom_properties != nullptr &&
                old_custom_properties != nullptr &&
                *new_custom_properties != *old_custom_properties);
      }();
      const bool font_size_changed = style_resolver_.HasPropertyDiff(
          *resolved_styles.final_style, kPropertyIDFontSize);
      const bool root_font_size_changed = base::FloatsNotEqual(
          old_root_font_size, resolved_styles.final_style->GetRootFontSize());
      const bool inherited_dynamic_refresh_needed =
          (request.dynamic_update_flags &
           dynamic_style_inputs.inherited_dynamic_flags) != 0;
      bool inherited_property_changed = false;
      resolved_styles.final_style->ForEachChangedProperty(
          [this, &inherited_property_changed](const auto id) {
            inherited_property_changed |=
                id != kPropertyIDFontSize && IsInheritable(id);
          });
      resolved_styles.final_style->ForEachResetProperty(
          [this, &inherited_property_changed](const auto id) {
            inherited_property_changed |=
                id != kPropertyIDFontSize && IsInheritable(id);
          });
      outcome.force_children = custom_properties_changed || font_size_changed ||
                               root_font_size_changed ||
                               inherited_dynamic_refresh_needed ||
                               inherited_property_changed;
      if (font_size_changed ||
          base::FloatsNotEqual(old_font_size,
                               resolved_styles.final_style->GetFontSize())) {
        outcome.child_update_flags |= DynamicCSSStylesManager::kUpdateEm;
      }
      if (root_font_size_changed) {
        outcome.child_update_flags |= DynamicCSSStylesManager::kUpdateRem;
      }

      if (custom_properties_changed) {
        HandleBeforeFlushActionsTask(
            [this]() { RecursivelyMarkCustomPropertiesDirty(); },
            kFlagGreedyParallel | kFlagLevelOrderParallel);
      }
      if (font_size_changed) {
        HandleBeforeFlushActionsTask(
            [this]() { InvalidateChildrenFontSizeRecursively(); },
            kFlagGreedyParallel | kFlagLevelOrderParallel);
      }
      if (inherited_property_changed) {
        InvalidateChildrenInheritedStylesRecursively();
      }
    }

    CSSIDBitset replayed_ids;
    if (should_commit) {
      outcome.need_update = true;
      resolved_styles.CommitPlatformStyleIfNeeded(platform_css_style_,
                                                  should_commit);
      CommitFontContext(*platform_css_style_, old_font_size,
                        old_root_font_size);
      if (style_changed) {
        ReplayCommitSideEffects(*platform_css_style_,
                                resolved_styles.resolved_style_map,
                                &replayed_ids);
      }
      if (style_changed && !first_render) {
        for (const auto id : reset_layout_only_ids) {
          if (replayed_ids.Has(id)) {
            continue;
          }
          ReplayResetStyleSideEffect(id);
          replayed_ids.Set(id);
        }

        for (const auto &[id, value] : changed_layout_only_styles) {
          if (replayed_ids.Has(id)) {
            continue;
          }
          ReplayChangedStyleSideEffect(id, value);
          replayed_ids.Set(id);
        }
      }
      if (dynamic_refresh_needed) {
        ReplayDynamicResolvedStyleSideEffects(
            dynamic_style_inputs.resolved_style_map,
            request.dynamic_update_flags, replayed_ids);
      }
    }

    resolved_styles.PersistBaseStyle(base_css_style_, should_commit);
    dynamic_style_flags_ = resolved_dynamic_style_flags;

    dirty_ &= ~(kDirtyStyle | kDirtyRefreshCSSVariables |
                kDirtyPropagateInherited | kDirtyFontSize);

    if (has_cached_styles_from_attributes) {
      ClearCachedStylesFromAttributes();
    }
  }
  return outcome;
}

void FiberElement::ResolveCSSStylesNewPipeline(bool &need_update) {
  NewPipelineResolveRequest request;
  auto outcome = ResolveCSSStylesNewPipelineCore(request);
  need_update |= outcome.need_update;
}

void FiberElement::ResolveCSSStyles(
    StyleMap &parsed_styles,
    base::InlineVector<CSSPropertyID, 16> &reset_style_ids, bool &need_update,
    bool &force_use_current_parsed_style_map) {
  const bool enable_new_styling_pipeline =
      element_manager()->EnableNewStylingPipeline();
  if (enable_new_styling_pipeline) {
    ResolveCSSStylesNewPipeline(need_update);
    return;
  }

  if (dirty_ & kDirtyStyle) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_STYLE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });

    RefreshStyle(parsed_styles, reset_style_ids,
                 force_use_current_parsed_style_map);

    // TODO(songshourui.null): This tricky logic can be removed after
    // hierarchical parallel full-update is implemented. Currently, a situation
    // like this might occur: <parent class="a1"> <child class="b1"> changes to
    // <parent class="a1 a2"> <child class="b1 b2">. If the style for 'b1 b2' is
    // affected by both a descendant selector (from 'a1 a2') and a CSS variable,
    // the child will be marked with both kDirtyStyle and
    // kDirtyRefreshCSSVariables.
    if (!this->is_greedy_parallel_flush()) {
      dirty_ &= ~kDirtyRefreshCSSVariables;
    }
    dirty_ &= ~kDirtyStyle;
  } else if (dirty_ & kDirtyRefreshCSSVariables) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_STYLE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    RefreshStyle(parsed_styles, reset_style_ids);

    dirty_ &= ~kDirtyRefreshCSSVariables;
  }

  if (!this->is_greedy_parallel_flush() && IsCSSInheritanceEnabled()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_PROPAGATE_INHERITED,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });

    const auto inherited_property = GetParentInheritedProperty();
    // process inherit related
    // quick check if any id in reset_style_ids is in parent inherited styles
    if (inherited_property.inherited_styles_) {
      for (auto it = reset_style_ids.begin(); it != reset_style_ids.end();) {
        // do not reset style if it's parent inherited_styles contains it
        const auto &parent_inherited_styles =
            *(inherited_property.inherited_styles_);
        if (parent_inherited_styles.find(*it) !=
            parent_inherited_styles.end()) {
          // we need to mark flag to do self recalculation for inherited
          // styles, if the style is updated instead of reset
          MarkDirtyLite(kDirtyPropagateInherited);
          it = reset_style_ids.erase(it);
        } else {
          ++it;
        }
      }
    }

    if (dirty_ & kDirtyPropagateInherited) {
      // comes here means parent propagates this change
      // there are two status:
      // 1. parent inherited style deleted; 2.parent inherited style changed;
      // #1 parent inherited style deleted
      if (inherited_property.reset_inherited_ids_ &&
          updated_inherited_styles_.has_value()) {
        for (const auto reset_id : *(inherited_property.reset_inherited_ids_)) {
          auto it = parsed_styles_map_.find(reset_id);
          if (it == parsed_styles_map_.end()) {
            if (updated_inherited_styles_->find(reset_id) !=
                updated_inherited_styles_->end()) {
              reset_style_ids.push_back(reset_id);
            }
          }
        }
      }

      // #2.parent inherited style changed
      //  merge the inherited styles, but they have lower priority
      if (inherited_property.inherited_styles_) {
        updated_inherited_styles_->clear();
        updated_inherited_styles_->reserve(
            inherited_property.inherited_styles_->size());
        for (auto &pair : *(inherited_property.inherited_styles_)) {
          auto it = parsed_styles_map_.find(pair.first);
          if (it == parsed_styles_map_.end()) {
            if (EnableLayoutInElementMode()) {
              auto inherited_it = inherited_styles_->find(pair.first);
              if (inherited_it != inherited_styles_->end() &&
                  inherited_it->second == pair.second) {
                continue;
              }
            }
            updated_inherited_styles_->insert_or_assign(pair.first,
                                                        pair.second);
            need_update = true;
          }
        }
      }
    }

    // kDirtyPropagateInherited flag is expected to be consumed in above logic
    // Special case: When PrepareForCreateOrUpdate function is executing
    // parallel flush pass, and CSS Inheritance is enabled, CSS Styles
    // inherited from parent element cannot be fully resolved in parallel
    // flush pass, thus only in such scenario, kDirtyPropagateInherited flag
    // need to preserved to force refresh in next pass
    dirty_ &= ~kDirtyPropagateInherited;
  }

  // Process update_map for cloned elements.
  if (dirty_ & kDirtyCloned) {
    // Because cloned elements typically do not undergo style changes,
    // animation-related styles must be reapplied to initiate keyframe or
    // transition animations.
    for (const auto &pair : parsed_styles_map_) {
      if (CSSProperty::IsTransitionProps(pair.first) ||
          CSSProperty::IsKeyframeProps(pair.first)) {
        parsed_styles.insert_or_assign(pair.first, pair.second);
      }
    }
    dirty_ &= ~kDirtyCloned;
  }

  // Process reset before update styles.

  // If the new animator is activated and this element has been created
  // before, we need to reset the transition styles in advance. Additionally,
  // the transition manager should verify each property to decide whether to
  // intercept the reset. Therefore, we break down the operations related to
  // the transition reset process into three steps:
  // 1. We check whether we need to reset transition styles in advance.
  // 2. If these styles have been reset beforehand, we can skip the transition
  // styles in the later steps.
  // 3. We review each property to determine whether the reset should be
  // intercepted.
  bool should_consume_trans_styles_in_advance =
      ShouldConsumeTransitionStylesInAdvance();
  // #1. Consume all transition styles in advance, either update_map or
  // reset_map.
  if (should_consume_trans_styles_in_advance) {
    has_transition_props_ |= ResetTransitionStylesInAdvance(reset_style_ids);
  }
  auto &update_map =
      force_use_current_parsed_style_map ? parsed_styles_map_ : parsed_styles;
  if (should_consume_trans_styles_in_advance) {
    has_transition_props_ |= ConsumeTransitionStylesInAdvance(update_map);
  }

  // #2. Check whether direction need reset
  bool direction_reset = false;
  bool text_align_reset = false;
  for (const auto &id : reset_style_ids) {
    // #2. If these transition styles have been reset beforehand, skip them
    // here.
    if (should_consume_trans_styles_in_advance &&
        CSSProperty::IsTransitionProps(id)) {
      continue;
    }
    // #3. Review each property to determine whether the reset should be
    // intercepted.
    if (css_transition_manager_ &&
        css_transition_manager_->ConsumeCSSProperty(id, CSSValue())) {
      continue;
    }

    if (id == kPropertyIDDirection) {
      direction_reset = true;
    }

    // #4. If it is text-align property, delay reset to next step.
    if (id == kPropertyIDTextAlign) {
      text_align_reset = true;
      continue;
    }

    // Since the previous element styles cannot be accessed in element, we
    // need to record some necessary styles which New Animator transition
    // needs, and it needs to be saved before rtl converted logic.
    ResetElementPreviousStyle(id);
    auto direction_aware_pair = ConvertRtlCSSPropertyID(id);
    ResetStyleInternal(direction_aware_pair.second);
    need_update = true;
  }

  // #5. Reset text_align property depending on whether direction is changed
  if (text_align_reset) {
    // #5.1 Remove id from inherited_styles_
    CSSPropertyID text_align_id = CSSPropertyID::kPropertyIDTextAlign;
    WillResetCSSValue(text_align_id);
    // #5.2 Check whether direction property is changed
    auto direction_updated =
        update_map.find(CSSPropertyID::kPropertyIDDirection) !=
        update_map.end();
    auto direction_changed = direction_reset || direction_updated;
    // #5.3 Update element text_align depending on whether direction is
    // changed
    ResetTextAlign(update_map, direction_changed);
  }

  // process direction: rtl/lynx-rtl firstly
  if (IsDirectionChangedEnabled()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_DIRECTION_CHANGED,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    do {
      // case 1: direction changed, trigger to re calculate all direction
      // related styles case 2: only direction related style updated, just do
      // rtl for this style
      auto get_direction = [](const auto &update_map,
                              const auto &updated_inherited_map,
                              const auto &pre_direction) {
        auto update_map_it =
            update_map.find(CSSPropertyID::kPropertyIDDirection);
        if (update_map_it != update_map.end()) {
          return std::make_pair(update_map_it->second,
                                static_cast<starlight::DirectionType>(
                                    update_map_it->second.GetNumber()));
        }
        if (updated_inherited_map.has_value()) {
          auto updated_inherited_map_it =
              updated_inherited_map->find(CSSPropertyID::kPropertyIDDirection);
          if (updated_inherited_map_it != updated_inherited_map->end()) {
            return std::make_pair(
                updated_inherited_map_it->second,
                static_cast<starlight::DirectionType>(
                    updated_inherited_map_it->second.GetNumber()));
          }
        }
        return std::make_pair(CSSValue(), pre_direction);
      };

      auto previous_direction = computed_css_style()->GetDirection();
      auto new_direction = get_direction(update_map, updated_inherited_styles_,
                                         previous_direction);
      if (new_direction.second == previous_direction) {
        break;
      }

      // Reset all direction related styles when not switching between normal
      // and ltr
      if (IsAnyRTL(new_direction.second) || IsAnyRTL(previous_direction)) {
        if (updated_inherited_styles_.has_value()) {
          for (const auto &css_pair : *updated_inherited_styles_) {
            ResetDirectionAwareProperty(css_pair.first, css_pair.second);
          }
        }
        for (const auto &css_pair : parsed_styles_map_) {
          ResetDirectionAwareProperty(css_pair.first, css_pair.second);
        }
      }
      if (is_text() || NeedProcessDirection()) {
        auto current_text_align = CSSValue(starlight::TextAlignType::kStart);
        current_text_align =
            ResolveCurrentStyleValue(kPropertyIDTextAlign, current_text_align);
        DynamicCSSStylesManager::UpdateDirectionAwareDefaultStyles(
            this, new_direction.second, current_text_align);
      }
      SetStyleInternal(kPropertyIDDirection, new_direction.first);
    } while (false);
  }

  bool root_font_size_changed =
      GetCurrentRootFontSize() != GetRecordedRootFontSize();
  if (root_font_size_changed) {
    SetFontSizeForAllElement(GetFontSize(), GetCurrentRootFontSize());
    UpdateLayoutNodeFontSize(GetFontSize(), GetCurrentRootFontSize());
  }

  // TODO: A refactor of the animation-related style handling is needed later,
  // once the correct dependencies between animation and other special CSS
  // property changes are identified. set updated Styles to element in the end
  if (!update_map.empty() ||
      (updated_inherited_styles_.has_value() &&
       !updated_inherited_styles_->empty()) ||
      (styles_from_attributes_.has_value() &&
       !styles_from_attributes_->empty())) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_SET_STYLE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // if kDirtyPropagateInherited, need to delay to SetStyle in inherit
    // process
    ConsumeStyle(update_map, IsCSSInheritanceEnabled()
                                 ? updated_inherited_styles_.get()
                                 : nullptr);
    need_update = true;
  }

  // direction change: we always handle direction change after all styles
  // resolved
  if (pending_updated_direction_related_styles_.has_value()) {
    for (const auto &style_pair : *pending_updated_direction_related_styles_) {
      TryDoDirectionRelatedCSSChange(style_pair.first, style_pair.second.first,
                                     style_pair.second.second);
    }
  }

  // Handle font size change
  // TODO: A refactor of the font-size-related style handling is needed later,
  // once the correct dependencies between font-size and other special CSS
  // property(text-align, direction) changes are identified.
  if (dirty_ & kDirtyFontSize) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_FONT_SIZE_CHANGE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    do {
      // If `dirty_ & kDirtyCreated`, the `parsed_styles_map_` has already
      // been fully consumed, so there is no possibility of `update_map` being
      // different from `parsed_styles_map_`. Therefore, skip this logic.
      if (dirty_ & kDirtyCreated) {
        break;
      }

      // If `dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm ==
      // false` and `root_font_size_changed` and `dynamic_style_flags_ &
      // DynamicCSSStylesManager::kUpdateRem == false`, it indicates that the
      // current `parsed_styles_map_` does not contain any font size-sensitive
      // styles, and thus this part of the processing logic can be skipped.
      if (!(dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm) &&
          !(root_font_size_changed &&
            dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateRem)) {
        break;
      }

      // We need to reset the styles for the following style pairs because
      // they are possibly font size-sensitive:
      // 1. If the unit of the style property value is EM, CALC, MAP or ARRAY
      // 2. If the unit of the style property value is REM and
      // `root_font_size_changed`
      // 3. If the style property ID is `kPropertyIDTransform` or
      // `kPropertyIDLineHeight`
      auto should_update_em_rem_style = [](const auto &style_pair,
                                           bool root_font_size_changed) {
        return style_pair.second.GetPattern() == CSSValuePattern::EM ||
               style_pair.second.GetPattern() == CSSValuePattern::CALC ||
               style_pair.second.GetPattern() == CSSValuePattern::MAP ||
               style_pair.second.GetPattern() == CSSValuePattern::ARRAY ||
               (style_pair.second.GetPattern() == CSSValuePattern::REM &&
                root_font_size_changed) ||
               style_pair.first == CSSPropertyID::kPropertyIDTransform ||
               style_pair.first == CSSPropertyID::kPropertyIDLineHeight;
      };

      // If the style pair is font size-sensitive and the current `update_map`
      // does not include this style pair, then force the reset of this style
      // pair. And process kPropertyIDFontSize first.
      auto iter = parsed_styles_map_.find(CSSPropertyID::kPropertyIDFontSize);
      if (iter != parsed_styles_map_.end() &&
          should_update_em_rem_style(*iter, root_font_size_changed) &&
          update_map.find(CSSPropertyID::kPropertyIDFontSize) ==
              update_map.end()) {
        SetFontSize(iter->second);
        need_update = true;
      }

      for (const auto &style : parsed_styles_map_) {
        bool need_handle_pending_updated_direction_related_style =
            pending_updated_direction_related_styles_.has_value() &&
            pending_updated_direction_related_styles_->find(style.first) !=
                pending_updated_direction_related_styles_->end();
        if (style.first != CSSPropertyID::kPropertyIDFontSize &&
            should_update_em_rem_style(style, root_font_size_changed) &&
            update_map.find(style.first) == update_map.end()) {
          if (need_handle_pending_updated_direction_related_style) {
            auto style_pair =
                *pending_updated_direction_related_styles_->find(style.first);
            TryDoDirectionRelatedCSSChange(style.first, style_pair.second.first,
                                           style_pair.second.second);
          } else {
            SetStyleInternal(style.first, style.second);
          }
          need_update = true;
        }
      }
    } while (false);
    dirty_ &= ~kDirtyFontSize;
  }

  if (pending_updated_direction_related_styles_.has_value()) {
    // reset cached style map impacted by direction
    pending_updated_direction_related_styles_.reset();
  }

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

ParallelFlushReturn FiberElement::PrepareForCreateOrUpdate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PREPARE_FOR_CRATE_OR_UPDATE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  bool need_update = false;

  // Need process attributes first.
  need_update = ConsumeAllAttributes();

  // If it's the first flush of the element and parsed_styles_map_ is empty, we
  // can take the fast path, directly using parsed_styles_map_ as the updated
  // style. If the element is cloned, its parsed_styles_map_ may not be empty
  // and be in the kDirtyCreated state at the same time.
  bool force_use_current_parsed_style_map =
      (dirty_ & kDirtyCreated) && parsed_styles_map_.empty();
  StyleMap parsed_styles;
  base::InlineVector<CSSPropertyID, 16> reset_style_ids;

  if (this->is_greedy_parallel_flush() && IsCSSInheritanceEnabled()) {
    MarkDirtyLite(kDirtyPropagateInherited);
  }

  if (element_manager()->EnableSimpleStyle()) {
    ResolveSimpleStyles();
  } else {
    ResolveCSSStyles(parsed_styles, reset_style_ids, need_update,
                     force_use_current_parsed_style_map);
  }

  // If above props and styles need to be updated, this patch needs trigger
  // layout.
  if (need_update || dirty_ & kDirtyCreated || dirty_ & kDirtyForceUpdate) {
    RequestLayout();
  }

  // events
  if (dirty_ & kDirtyEvent) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_EVENTS,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // OPTME(linxs): pass event diff result later?
    element_manager_->ResolveEvents(data_model_.get(), this);
    dirty_ &= ~kDirtyEvent;
  }

  // gestures
  if (dirty_ & kDirtyGesture) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_GESTURES,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    PreparePropBundleIfNeed();
    element_manager_->ResolveGestures(data_model_.get(), this);
    dirty_ &= ~kDirtyGesture;
    need_update = true;
  }

  // dataset
  if (dirty_ & kDirtyDataset) {
    // Pass the element's dataset as an attribute, with the key 'dataset', into
    // the propbundle for LynxUI.
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_DATASET);
    PreparePropBundleIfNeed();
    lepus::Value dataset_val(lepus::Dictionary::Create());
    for (const auto &pair : data_model()->dataset()) {
      dataset_val.SetProperty(pair.first, pair.second);
    }
    if (EnableFragmentLayerRender()) {
      if (auto fragment = fragment_impl()) {
        fragment->SetEventProp(PlatformEventPropName::kDataset, dataset_val);
      }
    }
    prop_bundle_->SetProps("dataset", pub::ValueImplLepus(dataset_val));
    dirty_ &= ~kDirtyDataset;
    need_update = true;
  }

  {
    // FIXME(linxs): [workaround]!!!!!to be removed later, current layout has an
    // issue: inline node can not mark parent dirty when only layout property
    // updated!
    if (need_update && !prop_bundle_ && is_inline_element()) {
      PreparePropBundleIfNeed();
    }
  }

  // Commit Create or Update UI Ops
  PerformElementContainerCreateOrUpdate(need_update, true);

  if (ShouldProcessParallelTasks()) {
    return CreateParallelTaskHandler();
  }

  FlushPendingInvokeTasks();
  VerifyKeyframePropsChangedHandling();

  return []() {};
}

void FiberElement::ReplayChangedStyleSideEffect(CSSPropertyID id,
                                                const CSSValue &value) {
  CheckDynamicUnit(id, value, false);

  const bool is_layout_only = LayoutProperty::IsLayoutOnly(id);
  const bool need_layout = is_layout_only || LayoutProperty::IsLayoutWanted(id);
  if (need_layout) {
    CheckFixedSticky(id, value);
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

  if (id == kPropertyIDOverflow || id == kPropertyIDOverflowX ||
      id == kPropertyIDOverflowY) {
    if (!computed_css_style()->IsOverflowXY()) {
      has_layout_only_props_ = false;
    }
    return;
  }

  if (!enable_extended_layout_only_opt_ || !IsExtendedLayoutOnlyProps(id)) {
    has_layout_only_props_ = false;
  }
  if (CSSProperty::IsTransitionProps(id) || CSSProperty::IsKeyframeProps(id)) {
    has_non_flatten_attrs_ = true;
  } else {
    CheckHasNonFlattenCSSProps(id);
  }
}

void FiberElement::ReplayResetStyleSideEffect(CSSPropertyID id) {
  if (id == kPropertyIDFontSize) {
    return;
  }

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
    if (is_fixed_) {
      fixed_changed_ = true;
    }
    is_sticky_ = false;
    is_fixed_ = false;
  }

  if (is_layout_only) {
    return;
  }

  has_layout_only_props_ = false;
  if (CSSProperty::IsTransitionProps(id) || CSSProperty::IsKeyframeProps(id)) {
    has_non_flatten_attrs_ = true;
  }
}

void FiberElement::CommitFontContext(
    const starlight::ComputedCSSStyle &computed_style, double old_font_size,
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

void FiberElement::ReplayCommitSideEffects(
    const starlight::ComputedCSSStyle &computed_style,
    const StyleMap &resolved_style_map, CSSIDBitset *replayed_ids) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_REPLAY_COMMIT_SIDE_EFFECTS);
  const bool is_first_render = IsNewlyCreated();
  const auto &resolved_values = computed_style.GetResolvedValues();
  const auto resolved_style_map_end = resolved_style_map.end();
  const auto resolved_values_end = resolved_values.end();
  auto mark_replayed = [replayed_ids](CSSPropertyID id) {
    if (replayed_ids != nullptr) {
      replayed_ids->Set(id);
    }
  };
  if (!is_first_render) {
    computed_style.ForEachResetProperty([&](const auto id) {
      if (resolved_style_map.find(id) != resolved_style_map_end) {
        return;
      }
      ReplayResetStyleSideEffect(id);
      mark_replayed(id);
    });
  }

  if (is_first_render) {
    CSSIDBitset explicit_style_ids = CSSIDBitset::FromKeys(resolved_style_map);
    for (const auto &[id, value] : resolved_style_map) {
      if (id == kPropertyIDFontSize) {
        continue;
      }
      ReplayChangedStyleSideEffect(id, value);
      mark_replayed(id);
    }
    if (IsCSSInheritanceEnabled()) {
      for (const auto &[id, value] : resolved_values) {
        if (id == kPropertyIDFontSize || explicit_style_ids.Has(id)) {
          continue;
        }
        ReplayChangedStyleSideEffect(id, value);
        mark_replayed(id);
      }
    }
  } else {
    computed_style.ForEachChangedProperty([&](const auto id) {
      if (id == kPropertyIDFontSize) {
        return;
      }
      auto it = resolved_style_map.find(id);
      if (it != resolved_style_map_end) {
        ReplayChangedStyleSideEffect(id, it->second);
        mark_replayed(id);
        return;
      }
      auto resolved_it = resolved_values.find(id);
      if (resolved_it != resolved_values_end) {
        ReplayChangedStyleSideEffect(id, resolved_it->second);
        mark_replayed(id);
      }
    });
  }
}

void FiberElement::ReplayDynamicResolvedStyleSideEffects(
    const StyleMap &resolved_style_map,
    DynamicCSSStylesManager::StyleUpdateFlags update_flags,
    const CSSIDBitset &replayed_ids) {
  if (update_flags == 0) {
    return;
  }
  const auto &css_config = element_manager()->GetDynamicCSSConfigs();
  for (const auto &[id, value] : resolved_style_map) {
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
    ReplayChangedStyleSideEffect(id, value);
  }
}

void FiberElement::FlushActionsAsRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_ACTIONS_AS_ROOT,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (parent() == nullptr) {
    LOGE("FiberElement::FlushActionsAsRoot failed since parent is nullptr");
    return;
  }

  // find the first non wrapper && non dirty parent to get the flush option
  auto *flush_parent = static_cast<FiberElement *>(parent());

  // find the first non dirty parent to do flush,if flush from subtree
  if (flush_parent->dirty_) {
    LOGW("FiberElement::FlushActionsAsRoot maybe from a wrong parent, this tag:"
         << tag_.str() << ",component:" << ParentComponentEntryName());
    return flush_parent->FlushActionsAsRoot();
  }

  // find the first non block parent to get the flush option for AirModeFiber
  if (element_manager()->IsAirModeFiberEnabled() && is_block() &&
      flush_parent) {
    return flush_parent->FlushActionsAsRoot();
  }

  // find the first non wrapper parent to get the flush option
  while (flush_parent && flush_parent->is_wrapper()) {
    flush_parent = static_cast<FiberElement *>(flush_parent->parent());
  }

  if (!flush_parent) {
    LOGE(
        "FiberElement::FlushActionsAsRoot failed since can not find a clean "
        "flush parent!");
    return;
  }

  if (IsDetached()) {
    LOGE(
        "FiberElement::FlushActionsAsRoot failed since current node is "
        "detached!");
    return;
  }

  element_manager()->SetCurrentEngineThreadId(std::this_thread::get_id());
  ParallelFlushAsRoot();
  FlushActions();
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    element_context_delegate_->FlushEnqueuedTasks();
  }
}

void FiberElement::FlushSelf() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_SELF,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (parallel_before_flush_action_tasks_.has_value()) {
    for (const auto &task : *parallel_before_flush_action_tasks_) {
      task();
    }
    parallel_before_flush_action_tasks_.reset();
  }

  if ((dirty_ & ~kDirtyTree) != 0) {
    // create or update Platform Op
    PrepareForCreateOrUpdate();
  }

  // handle fixed style changed if needed
  if (fixed_changed_) {
    HandleSelfFixedChange();
    fixed_changed_ = false;
  }
}

// need parent's option
void FiberElement::FlushActions() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_ACTIONS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!flush_required_) {
    return;
  }

  // Step I: Handle Action for current element: Prepare&HandleFixedChange
  FlushSelf();

  // Step II: process insert or remove related actions
  PrepareAndGenerateChildrenActions();

  // Throw exception on purpose to catch logic flaw
  DCHECK(dirty_ == 0);

  InvalidateChildrenIfNeeded();

  // Step III: recursively call FlushActions for each child
  for (const auto &child : scoped_children_) {
    auto *fiber_child = static_cast<FiberElement *>(child.get());
    if (NeedPropagateInheritedDirtyFlag(false)) {
      fiber_child->MarkDirtyLite(kDirtyPropagateInherited);
    }
    fiber_child->FlushActions();
  }
  // below flags should be delayed until children flushed
  children_propagate_inherited_styles_flag_ = false;
  reset_inherited_ids_.reset();

  flush_required_ = false;
  is_async_flush_root_ = false;
}

void FiberElement::OnParallelFlushAsRoot(PerfStatistic &stats) {
  stats.enable_report_stats_ =
      element_manager()->GetEnableReportThreadedElementFlushStatistic();
  stats.total_processing_start_ = base::CurrentTimeMicroseconds();
}

void FiberElement::PrepareSelfForThreadedElementResolution() {
  // Get Tag info
  EnsureTagInfo();
  // Decode first
  GetRelatedCSSFragment();
  if (is_component()) {
    static_cast<ComponentElement *>(this)->GetCSSFragment();
  }
}

bool FiberElement::ShouldFallbackToSerialForNewStylingPipeline() const {
  return element_manager()->GetEnableParallelElement() &&
         element_manager()->EnableNewStylingPipeline() &&
         !element_manager()->EnableLevelOrderTraversing();
}

void FiberElement::ParallelFlushAsRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PARALLEL_FLUSH_AS_ROOT);
  if (!element_manager()->GetEnableParallelElement()) {
    return;
  }
  if (ShouldFallbackToSerialForNewStylingPipeline()) {
    return;
  }
  if (element_manager()->GetEnableParallelElement() &&
      element_manager()->EnableLevelOrderTraversing()) {
    TreeResolver::TraverseDom(this, TreeResolver::kWorkUnitSize);
    element_manager()->FlushLevelOrderTasks();
    element_manager()->WaitForAllLevelOrderResolveTasks();
    return;
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, TASM_TASK_RUNNER_WAIT_FOR_COMPLETION);
    element_manager()->GetTasmWorkerTaskRunner()->WaitForCompletion();
  }
  ParallelFlushRecursively();

  auto &task_queue = element_manager()->ParallelTasks();
  if (task_queue.empty()) {
    return;
  }

  uint32_t total_task_count = static_cast<uint32_t>(task_queue.size());

  PerfStatistic perf_stats(total_task_count);
  OnParallelFlushAsRoot(perf_stats);

  while (!task_queue.empty()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_PARALLEL_TASK);
    if (task_queue.front().get()->GetFuture().wait_for(
            std::chrono::seconds(element_manager()->GetTaskWaitTimeout())) ==
        std::future_status::ready) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_LEFT_ITER);
      task_queue.front().get()->GetFuture().get()();
      task_queue.pop_front();
    } else if (task_queue.back().get()->Run()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_RIGHT_ITER);
      task_queue.back().get()->GetFuture().get()();
      task_queue.pop_back();
      ++perf_stats.engine_thread_task_count_;
    } else {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_WAIT_LEFT_ITER);
      ParallelFlushReturn task;
      if (perf_stats.enable_report_stats_) {
        uint64_t wait_start = base::CurrentTimeMicroseconds();
        task = task_queue.front().get()->GetFuture().get();
        perf_stats.total_waiting_time_ +=
            (base::CurrentTimeMicroseconds() - wait_start);
      } else {
        task = task_queue.front().get()->GetFuture().get();
      }

      task();
      task_queue.pop_front();
    }
  }

  DidParallelFlushAsRoot(perf_stats);
}

void FiberElement::DidParallelFlushAsRoot(PerfStatistic &stats) {
  if (stats.enable_report_stats_) {
    uint64_t total_processing_end = base::CurrentTimeMicroseconds();
    report::EventTracker::OnEvent(
        [perf_stats = std::move(stats),
         total_processing_end](report::MoveOnlyEvent &event) {
          auto thread_pool_task_count = perf_stats.total_task_count_ -
                                        perf_stats.engine_thread_task_count_;
          event.SetName("lynxsdk_threaded_element_flush_statistic");
          event.SetProps("total_task_count", perf_stats.total_task_count_);
          event.SetProps("thread_pool_task_count", thread_pool_task_count);
          event.SetProps("mode", kFiberParallelPrepareMode);
          event.SetProps("tasm_thread_processing_duration",
                         static_cast<int>(total_processing_end -
                                          perf_stats.total_processing_start_));
          event.SetProps("tasm_thread_waiting_duration",
                         static_cast<int>(perf_stats.total_waiting_time_));
        });
  }
}

void FiberElement::PostResolveTaskToThreadPool(
    bool is_engine_thread, ParallelReduceTaskQueue &task_queue) {
  UpdateResolveStatus(AsyncResolveStatus::kPreparing);
  PrepareSelfForThreadedElementResolution();

  std::promise<ParallelFlushReturn> promise;
  std::future<ParallelFlushReturn> future = promise.get_future();

  auto task_info_ptr = fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
      [target = this, promise = std::move(promise)]() mutable {
        TRACE_EVENT(
            LYNX_TRACE_CATEGORY,
            FIBER_ELEMENT_PREPARE_FOR_CRATE_OR_UPDATE_ASYNC,
            [target](lynx::perfetto::EventContext ctx) {
              if (target->element_manager()) {
                ctx.event()->add_debug_annotations(
                    INSTANCE_ID,
                    std::to_string(target->element_manager()->GetInstanceId()));
              }
            });

        target->UpdateResolveStatus(AsyncResolveStatus::kResolving);
        target->MarkParallelFlushFlag(kFlagGreedyParallel);
        promise.set_value(target->PrepareForCreateOrUpdate());
      },
      std::move(future));

  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [task_info_ptr]() { task_info_ptr->Run(); },
      base::ConcurrentTaskType::HIGH_PRIORITY);
  task_queue.emplace_back(std::move(task_info_ptr));
}

void FiberElement::ParallelFlushRecursively() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PARALLEL_FLUSH_RECURSIVELY);
  if (!flush_required_) {
    return;
  }

  if (ShouldResolveStyle()) {
    PostResolveTaskToThreadPool(true, element_manager()->ParallelTasks());
  }

  for (const auto &child : scoped_children_) {
    static_cast<FiberElement *>(child.get())->ParallelFlushRecursively();
  }
}

void FiberElement::PrepareChildren() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PREPARE_CHILDREN,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  for (auto iter = scoped_children_.begin(); iter != scoped_children_.end();
       ++iter) {
    auto *fiber_child = ReplaceTemplateChildIfNeeded(iter);

    if (NeedPropagateInheritedDirtyFlag(false)) {
      // mark propagateInherited when necessary
      fiber_child->MarkDirtyLite(kDirtyPropagateInherited);
    }

    if ((fiber_child->dirty_ & ~kDirtyTree) != 0) {
      fiber_child->PrepareForCreateOrUpdate();
    }

    if (fiber_child->is_layout_only_ && !fiber_child->is_raw_text()) {
      fiber_child->PrepareChildren();
    }
  }
}

FiberElement *FiberElement::ReplaceTemplateChildIfNeeded(
    base::InlineVector<fml::RefPtr<Element>,
                       kChildrenInlineVectorSize>::iterator child_iter) {
  auto *fiber_child = static_cast<FiberElement *>(child_iter->get());
  if (!fiber_child->is_template()) {
    return fiber_child;
  }

  auto *template_child = static_cast<TemplateElement *>(fiber_child);
  auto root = template_child->GetRoot();
  if (root == nullptr || root.get() == template_child) {
    return fiber_child;
  }

  // Template nodes should hand over child preparation to the materialized root
  // so the rest of the flush walks the real rendered subtree.
  fml::RefPtr<TemplateElement> template_ref(template_child);
  EXEC_EXPR_FOR_INSPECTOR(if (element_manager() != nullptr) {
    element_manager()->OnElementNodeRemovedForInspector(template_child);
  });
  OnNodeRemoved(template_child);
  TreeResolver::NotifyNodeRemoved(this, template_child);
  template_child->set_parent(nullptr);

  // TODO(songshourui.null): Keep logical_children_ aligned with the
  // materialized template root when this becomes observable.

  *child_iter = root;
  fiber_child = root.get();
  OnNodeAdded(fiber_child);
  TreeResolver::NotifyNodeInserted(this, fiber_child);
  fiber_child->set_parent(this);
  EXEC_EXPR_FOR_INSPECTOR(if (element_manager() != nullptr) {
    element_manager()->OnElementNodeAddedForInspector(fiber_child);
  });
  return fiber_child;
}

void FiberElement::PrepareChildForInsertion(FiberElement *child) {
  if (child->dirty() & FiberElement::kDirtyCreated) {
    // make sure the child has been created,before insert op
    if (NeedPropagateInheritedDirtyFlag(false)) {
      child->MarkDirtyLite(FiberElement::kDirtyPropagateInherited);
    }
    child->PrepareForCreateOrUpdate();
  }
  if (child->IsLayoutOnly() && !child->is_raw_text()) {
    for (const auto &grand : child->children()) {
      child->PrepareChildForInsertion(static_cast<FiberElement *>(grand.get()));
    }
  }
}

void FiberElement::PrepareAndGenerateChildrenActions() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              FIBER_ELEMENT_PREPARE_AND_GENERATE_CHILDREN_ACTIONS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // When need propagate inherited styles or tree structure is updated, prepare
  // children
  if (dirty_ & kDirtyTree || children_propagate_inherited_styles_flag_) {
    PrepareChildren();
  }
  // process insert or remove related actions
  if (dirty_ & kDirtyTree) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_CHILDREN_ACTION,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    if (!has_to_store_insert_remove_actions_) {
      for (const auto &child : scoped_children_) {
        auto *fiber_child = static_cast<FiberElement *>(child.get());
        if (!fiber_child->render_parent_) {
          // if no pending tree actions, we just do insertion here
          if (!fiber_child->is_fixed_ || IsFixedNewOrUnifiedEnabled()) {
            this->HandleInsertChildAction(fiber_child, -1, nullptr);
          } else {
            if (IsFiberArch()) {
              InsertFixedElement(fiber_child, nullptr);
            } else {
              fiber_child->need_handle_fixed_ = true;
            }
          }
        }
      }
    }

    for (const auto &param : action_param_list_) {
      switch (param.type_) {
        case Action::kInsertChildAct: {
          auto *param_child = ResolveTemplateRootForAction(param.child_.get());
          auto *param_ref = ResolveTemplateRootForAction(param.ref_node_);
          PrepareChildForInsertion(param_child);
          if (!param.is_fixed_ || IsFixedNewOrUnifiedEnabled()) {
            HandleInsertChildAction(param_child, static_cast<int>(param.index_),
                                    param_ref);
          } else {
            if (IsFiberArch()) {
              InsertFixedElement(param_child, param_ref);
            } else {
              param_child->need_handle_fixed_ = true;
            }
          }
        } break;

        case Action::kRemoveChildAct: {
          auto *param_child = ResolveTemplateRootForAction(param.child_.get());
          if (!param.is_fixed_ || IsFixedNewOrUnifiedEnabled()) {
            HandleRemoveChildAction(param_child);
          } else {
            RemoveFixedElement(param_child);
          }
        } break;

        case Action::kRemoveIntergenerationAct: {
          auto *param_child = ResolveTemplateRootForAction(param.child_.get());
          if (param_child->parent_ == this) {
            break;
          }
          if (param.is_fixed_ && !IsFixedNewOrUnifiedEnabled()) {
            RemoveFixedElement(param_child);
          } else if (param_child->ZIndex() != 0 || param.is_fixed_) {
            if (param.is_fixed_) {
              // new fixed, remove fixed node and its layout node from its
              // parent.
              param_child->HandleRemoveSelf(
                  this,
                  static_cast<FiberElement *>(param_child->render_parent_));
            } else {
              // node with z-index only needs remove its element container.
              element_container()->RemoveElementContainerAccordingToElement(
                  param_child, false);
            }
          }
        } break;

        default:
          break;
      }
    }
    dirty_ &= ~kDirtyTree;
    RequestLayout();

    // if has any child, mark the flag true, otherwise set it false
    has_to_store_insert_remove_actions_ = (scoped_children_.size() > 0);
  }

  action_param_list_.clear_and_shrink();

  if (dirty_ & kDirtyReAttachContainer) {
    if (is_fixed_ && !IsFixedNewOrUnifiedEnabled()) {
      InsertFixedElement(this, nullptr);
    } else if (ZIndex() != 0 || is_fixed_) {
      // new fixed.
      if (is_fixed_) {
        // When new fixed is enabled, layout node should be re-inserted to its
        // render_parent, with an full insertion call.
        if (parent_) {
          static_cast<FiberElement *>(parent_)->HandleInsertChildAction(
              this, 0, static_cast<FiberElement *>(next_render_sibling_));
        }
      } else {
        // z-index only has to insert its element container again.
        HandleContainerInsertion(
            static_cast<FiberElement *>(render_parent_), this,
            static_cast<FiberElement *>(next_render_sibling_));
      }
    }
    dirty_ &= ~kDirtyReAttachContainer;
  }
}

void FiberElement::HandleInsertChildAction(FiberElement *child, int to_index,
                                           FiberElement *ref_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_INSERT_CHILD_ACTION,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  auto *parent = this;
  child->element_container()->UpdateGlobalInsertionOrder();

  if (child->render_parent_ != nullptr) {
    LOGE("FiberElement do re-insert child action");
    this->LogNodeInfo();
    child->LogNodeInfo();
    if (element_manager()->FixFiberReinsertDetachFromOldRenderParent()) {
      // A moved child may still be attached to its old render parent when the
      // new parent's insert action flushes before the old parent's queued
      // remove action. Detach it first so render sibling links stay local to
      // one parent.
      static_cast<FiberElement *>(child->render_parent_)
          ->HandleRemoveChildAction(child);
    }
  }

  if (!IsFixedNewOrUnifiedEnabled()) {
    while (ref_node != nullptr &&
           (ref_node->is_fixed() || ref_node->fixed_changed_ ||
            ref_node->render_parent() == nullptr)) {
      // Two cases:
      // 1. `ref_node` is a fixed node, find its `next_sibling`.
      // 2. `ref_node` changed from fixed to non-fixed; since
      // `ref_node->HandleSelfFixedChange` was not executed, also find its
      // `next_sibling`.
      ref_node = static_cast<FiberElement *>(ref_node->next_sibling());
    }
  }

  StoreLayoutNode(child, ref_node);

  if (child->is_wrapper()) {
    // try to mark for wrapper element related.
    FindEnclosingNoneWrapper(parent, child);
  }

  if (UNLIKELY(parent->is_wrapper() || (parent->wrapper_element_count_ > 0) ||
               child->is_wrapper())) {
    TreeResolver::AttachChildToTargetParentForWrapper(parent, child, ref_node);
  } else {
    InsertLayoutNode(child, ref_node);
  }

  HandleContainerInsertion(parent, child, ref_node);
}

void FiberElement::HandleRemoveChildAction(FiberElement *child) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_REMOVE_CHILD_ACTION,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  child->ResetGlobalInsertionOrder();
  auto *parent = this;

  if (child->render_parent_ != this) {
    LOGE("FiberElement remove wrong child node !");
    parent->LogNodeInfo();
    child->LogNodeInfo();
    return;
  }

  RestoreLayoutNode(child);
  if (!child->is_wrapper() && !child->attached_to_layout_parent_ &&
      !child->IsFixedNewOrUnified()) {
    // parent is detached, child is removed from parent, and then the parent is
    // inserted to view tree,but the action is still stored in its parent

    // 1.if the child is not wrapper and not attached to layout tree, just
    // return
    // 2. if the child is wrapper, remove the wrapper's children recursively in
    // RemoveFromParentForWrapperChild
    // 3. if the parent is wrapper, just handle in
    // RemoveFromParentForWrapperChild
    return;
  }

  if (UNLIKELY(parent->is_wrapper() || parent->wrapper_element_count_ > 0) ||
      child->is_wrapper()) {
    if (child->enclosing_none_wrapper_) {
      static_cast<FiberElement *>(child->enclosing_none_wrapper_)
          ->wrapper_element_count_--;
    }
    TreeResolver::RemoveFromParentForWrapperChild(parent, child);
  } else {
    RemoveLayoutNode(child);
  }

  element_container()->RemoveElementContainerAccordingToElement(child, false);
}

void FiberElement::HandleRemoveSelf(FiberElement *removal_point,
                                    FiberElement *render_parent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_REMOVE_SELF,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!element_manager()->FixNewFixedRemovalBug()) {
    render_parent->HandleRemoveChildAction(this);
    return;
  }

  if (render_parent == nullptr) {
    removal_point->element_container()
        ->RemoveElementContainerAccordingToElement(this, false);
    LOGE("FiberElement double remove child node!");
    this->LogNodeInfo();
    return;
  }

  render_parent->HandleRemoveChildAction(this);
}

void FiberElement::HandleContainerInsertion(FiberElement *parent,
                                            FiberElement *child,
                                            FiberElement *ref_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_CONTAINER_INSERTION,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!child->element_container()->parent()) {
    // the child has been inserted to parent in
    // AttachChildToTargetContainerRecursive, just ignore it
    parent->element_container()->InsertElementContainerAccordingToElement(
        child, ref_node);
  }
}

FiberElement *FiberElement::FindEnclosingNoneWrapper(FiberElement *parent,
                                                     FiberElement *node) {
  while (parent) {
    if (!parent->is_wrapper()) {
      node->enclosing_none_wrapper_ = parent;
      parent->wrapper_element_count_++;
      break;
    }
    parent = static_cast<FiberElement *>(parent->parent_);
  }
  return parent;
}

void FiberElement::AddChildAt(fml::RefPtr<FiberElement> child, int index) {
  if (index == -1) {
    scoped_children_.push_back(child);
  } else {
    scoped_children_.insert(scoped_children_.begin() + index, child);
  }
  OnNodeAdded(child.get());
  TreeResolver::NotifyNodeInserted(this, child.get());
  child->set_parent(this);
}

// If new animator is enabled and this element has been created before, we
// should consume transition styles in advance. Also transition manager needs to
// verify every property to determine whether to intercept this update.
// Therefore, the operations related to Transition in the SetStyle process are
// divided into three steps:
// 1. Check whether to consume all transition styles in advance if needed.
// 2. Skip all transition styles in the later process if they have been consume
// in advance.
// 3. Check every property to determine whether to intercept this update.
void FiberElement::ConsumeStyle(const StyleMap &styles,
                                const StyleMap *inherit_styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_STYLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  bool should_consume_trans_styles_in_advance =
      ShouldConsumeTransitionStylesInAdvance();

  ConsumeStyleInternal(
      styles, inherit_styles,
      [this, should_consume_trans_styles_in_advance](auto id,
                                                     const auto &value) {
        // #2. Skip all transition styles in the later process if they have been
        // consume in advance.
        if (should_consume_trans_styles_in_advance &&
            CSSProperty::IsTransitionProps(id)) {
          return true;
        }
        // #3. Check every property to determine whether to intercept this
        // update.
        if (css_transition_manager_ &&
            css_transition_manager_->ConsumeCSSProperty(id, value)) {
          return true;
        }

        return false;
      });

  if (element_manager()->EnableAnimationForwardUpdatePreservation() &&
      animation_override_styles_map_.has_value() &&
      !animation_override_styles_map_->empty()) {
    ConsumeStyleInternal(
        *animation_override_styles_map_, nullptr,
        [should_consume_trans_styles_in_advance](auto id, const auto &value) {
          if (should_consume_trans_styles_in_advance &&
              CSSProperty::IsTransitionProps(id)) {
            return true;
          }
          return false;
        });
  }

  DidConsumeStyle();
}

void FiberElement::ConsumeStyleInternal(
    const StyleMap &styles, const StyleMap *inherit_styles,
    std::function<bool(CSSPropertyID, const tasm::CSSValue &)> should_skip) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_STYLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (styles.empty() &&
      (inherit_styles == nullptr || inherit_styles->empty())) {
    return;
  }

  // Handle font-size first. Other css may use this to calc rem or em.
  // Prioritize the new font-size from `styles`. If it's not available,
  // fall back to the value in `parsed_styles_map_`.
  CSSValue font_value;
  const auto new_it = styles.find(CSSPropertyID::kPropertyIDFontSize);
  if (new_it != styles.end()) {
    font_value = new_it->second;
  } else {
    const auto parsed_it =
        parsed_styles_map_.find(CSSPropertyID::kPropertyIDFontSize);
    if (parsed_it != parsed_styles_map_.end()) {
      font_value = parsed_it->second;
    }
  }
  SetFontSize(font_value);

  auto consume_func = [this, should_skip = std::move(should_skip)](
                          const StyleMap &styles, bool process_inherit) {
    for (const auto &style : styles) {
      bool is_inherit_style = false;
      if (!is_raw_text() && IsInheritable(style.first)) {
        is_inherit_style = true;
        auto iter = inherited_styles_->find(style.first);
        if (iter == inherited_styles_->end() || iter->second != style.second) {
          // save the css value to inherited styles map
          inherited_styles_->insert_or_assign(style.first, style.second);
          children_propagate_inherited_styles_flag_ = true;
        }
      }

      if (style.first == CSSPropertyID::kPropertyIDDirection ||
          style.first == CSSPropertyID::kPropertyIDFontSize) {
        // direction has been resolved before
        continue;
      }

      const bool is_platform_inheritable_property =
          process_inherit && is_inherit_style &&
          starlight::ComputedCSSStyle::IsPlatformInheritableProperty(
              style.first);

      if (const auto *parent_computed_css = GetParentComputedCSSStyle();
          is_platform_inheritable_property && parent_computed_css != nullptr) {
        if (parsed_styles_map_.find(style.first) != parsed_styles_map_.end()) {
          // Inline style or matched selectors has same style property
          continue;
        }
        computed_css_style()->InheritValue(style.first, *parent_computed_css);
        continue;
      }

      if (LIKELY(!TryResolveLogicStyleAndSaveDirectionRelatedStyle(
              style.first, style.second))) {
        if (should_skip(style.first, style.second)) {
          continue;
        }

        // Since the previous element styles cannot be accessed in element, we
        // need to record some necessary styles which New Animator transition
        // needs, and it needs to be saved before rtl converted logic.
        RecordElementPreviousStyle(style.first, style.second);
        SetStyleInternal(style.first, style.second);
      }
    }
  };

  if (inherit_styles != nullptr) {
    consume_func(*inherit_styles, true);
  }

  consume_func(styles, false);
}

bool FiberElement::ConsumeAllAttributes() {
  bool need_update = false;
  if (dirty_ & kDirtyAttr) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_ATTR,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    for (const auto &attr : updated_attr_map_) {
      SetAttributeInternal(attr.first, attr.second);
      need_update = true;
    }
    if (reset_attr_vec_.has_value()) {
      for (const auto &attr : *reset_attr_vec_) {
        ResetAttribute(attr);
        need_update = true;
      }
      reset_attr_vec_.reset();
    }
    if (updated_attr_map_.size() > 0) {
      PropsUpdateFinish();
      updated_attr_map_.clear();
    }

    dirty_ &= ~kDirtyAttr;
  }
  return need_update;
}

void FiberElement::PerformElementContainerCreateOrUpdate(bool need_update,
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
        UpdateFiberElement();
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

ParallelFlushReturn FiberElement::CreateParallelTaskHandler() {
  // Remaining Layout Task should be returned to be executed in threaded flush
  // or sync resolving(i.e. PageElement) scenario
  this->ResetParallelFlushFlag();
  this->UpdateResolveStatus(AsyncResolveStatus::kResolved);
  return [this]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                FIBER_ELEMENT_HANDLE_PARALLEL_REDUCE_TASKS);
    if (parallel_reduce_tasks_.has_value()) {
      for (const auto &task : *parallel_reduce_tasks_) {
        task();
      }
      parallel_reduce_tasks_.reset();
    }
    // Executing task in parallel_reduce_tasks_ may produce prop_bundle_,
    // need to consume newly created prop_bundle_
    PerformElementContainerCreateOrUpdate(
        prop_bundle_ != nullptr || computed_css_style()->IsDirty(), true);
    FlushPendingInvokeTasks();

    this->UpdateResolveStatus(AsyncResolveStatus::kUpdated);
    VerifyKeyframePropsChangedHandling();
  };
}

void FiberElement::MarkHasLayoutOnlyPropsIfNecessary(
    const base::String &attribute_key) {
  // Any attribute will cause has_layout_only_props_ = false
  has_layout_only_props_ = false;
}

void FiberElement::SetAttributeInternal(const base::String &key,
                                        const lepus::Value &value) {
  WillConsumeAttribute(key, value);

  PreparePropBundleIfNeed();

  MarkHasLayoutOnlyPropsIfNecessary(key);

  prop_bundle_->SetProps(key.c_str(), pub::ValueImplLepus(value));

  if (EnableFragmentLayerRender()) {
    if (auto name = PlatformEventPropNameFromString(key.str());
        name != PlatformEventPropName::kUnknown) {
      if (auto fragment = fragment_impl()) {
        fragment->SetEventProp(name, value);
      }
    }
  }

  // If the current node is a list child node, it is necessary to convert
  // kFullSpan's value to ListComponentInfo::Type and synchronize it to
  // LayoutNode.
  constexpr const static char kFullSpan[] = "full-span";
  if (parent() != nullptr && static_cast<FiberElement *>(parent())->is_list()) {
    if (key.IsEquals(kFullSpan)) {
      ListComponentInfo::Type type = ListComponentInfo::Type::DEFAULT;
      if (value.IsBool() && value.Bool()) {
        type = ListComponentInfo::Type::LIST_ROW;
      }
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kListCompType,
                                lepus::Value(static_cast<int32_t>(type)));
      // If the key is already equal to ListComponentInfo::Type, just sync it to
      // LayoutNode.
    } else if (key.IsEqual(ListComponentInfo::kListCompType)) {
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kListCompType,
                                value);
    }
  }

#if 0  // TODO(linxs): to process it in CLI compile period
    StyleMap attr_styles;
    if (key.IsEquals("scroll-x") && value.String().IsEqual("true")) {
        attr_styles.insert_or_assign(
                                     kPropertyIDLinearOrientation,
                                     CSSValue(starlight::LinearOrientationType::kHorizontal));
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals("scroll-y") && value.String().IsEqual("true")) {
        attr_styles.insert_or_assign(
                                     kPropertyIDLinearOrientation,
                                     CSSValue(starlight::LinearOrientationType::kVertical));
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals("scroll-x-reverse") &&
               value.String().IsEqual("true")) {
        attr_styles.insert_or_assign(
                                     kPropertyIDLinearOrientation,
                                     CSSValue(starlight::LinearOrientationType::kHorizontalReverse));
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals("scroll-y-reverse") &&
               value.String().IsEqual("true")) {
        attr_styles.insert_or_assign(
                                     kPropertyIDLinearOrientation,
                                     CSSValue(starlight::LinearOrientationType::kVerticalReverse));
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEqual("column-count")) {
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kColumnCount, value);
    } else if (key.IsEqual(ListComponentInfo::kListCompType)) {
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kListCompType, value);
    }
    SetStyle(attr_styles);
#endif
}

void FiberElement::SetNativeProps(
    const lepus::Value &native_props,
    std::shared_ptr<PipelineOptions> &pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_NATIVE_PROPS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!native_props.IsTable()) {
    LOGE("SetNativeProps's param must be a Table!");
    return;
  }

  if (native_props.Table()->size() == 0) {
    LOGE("SetNativeProps's param must not be empty!");
    return;
  }

  ForEachLepusValue(
      native_props, [this](const lepus::Value &key, const lepus::Value &value) {
        auto key_str = key.String();
        auto id = CSSProperty::GetPropertyID(key_str);
        if (id != kPropertyEnd) {
          SetStyle(id, value);
          EXEC_EXPR_FOR_INSPECTOR(element_manager()->OnSetNativeProps(
              this, key.ToString(), value, true));
        } else {
          if (IsRadonArch()) {
            SetAttribute(key_str, value, false);
          } else {
            SetAttribute(key_str, value);
          }
          EXEC_EXPR_FOR_INSPECTOR(element_manager()->OnSetNativeProps(
              this, key.ToString(), value, false));
        }
      });
  if (IsAttached()) {
    // TODO(nihao.royal): use `enable_unified_pixel_pipeline` to switch multi
    // behaviours. After `RunPixelPipeline` is unified, we may remove the
    // redundant logic here.
    if (pipeline_options->enable_unified_pixel_pipeline) {
      pipeline_options->resolve_requested = true;
      pipeline_options->target_node = this;
    } else {
      element_manager()->OnPatchFinish(pipeline_options, this);
    }
  } else {
    LOGE("FiberElement::SetNativeProps to an detached element!");
  }
}

void FiberElement::MarkFontSizeInvalidateRecursively() {
  MarkDirty(kDirtyFontSize);
  auto *child = static_cast<FiberElement *>(first_render_child_);
  while (child) {
    child->MarkFontSizeInvalidateRecursively();
    child = static_cast<FiberElement *>(child->next_render_sibling_);
  }
}

void FiberElement::InvalidateChildrenFontSizeRecursively() {
  auto *child = static_cast<FiberElement *>(first_render_child_);
  while (child) {
    child->MarkFontSizeInvalidateRecursively();
    child = static_cast<FiberElement *>(child->next_render_sibling_);
  }
}

void FiberElement::InvalidateChildrenInheritedStylesRecursively() {
  for (const auto &child : scoped_children_) {
    static_cast<FiberElement *>(child.get())
        ->ApplyFunctionRecursive([](FiberElement *element) {
          if (!element->is_raw_text()) {
            element->MarkDirtyLite(kDirtyPropagateInherited);
          }
        });
  }
}

void FiberElement::FlushProps() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_PROPS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  PushStyleToBundle();

  if (is_scroll_view() || is_list()) {
    UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                              lepus::Value(true));
  }

  // Update The root if needed
  if (!has_painting_node_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CATALYZER_NO_PAINTING_NODE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });

    PreparePropBundleIfNeed();

    // check is in inlineContainer before attachLayoutNode
    auto *real_parent =
        static_cast<FiberElement *>((!is_fixed_ || IsFixedNewOrUnifiedEnabled())
                                        ? parent_
                                        : element_manager_->root());
    while (real_parent && real_parent->is_wrapper()) {
      real_parent = static_cast<FiberElement *>(real_parent->parent());
    }
    if (real_parent) {
      CheckHasInlineContainer(real_parent);
    }
    AttachLayoutNode(prop_bundle_);
    EnsureSLNode();

    // FIXME(linxs): any other case has platform layout nodes??
    is_virtual_ = IsShadowNodeVirtual();
    bool platform_is_flatten = true;
    base::MoveOnlyClosure<bool, bool> func =
        [element = this, has_z_props = has_z_props(),
         is_fixed = is_fixed_](bool judge_by_props) {
          if (judge_by_props) {
            return !(has_z_props || is_fixed);
          } else {
            return element->TendToFlatten();
          }
        };
    if (!is_virtual_) {
      platform_is_flatten = element_container()->CheckFlatten(std::move(func));
    }
    bool is_layout_only = CanBeLayoutOnly() || is_virtual_;
    is_layout_only_ = is_layout_only;
    // native layer don't flatten.
    CreateElementContainer(platform_is_flatten);
    has_painting_node_ = true;
  }
  has_transition_props_changed_ = false;
}

// if child's related css variable is updated, invalidate child's style.
void FiberElement::RecursivelyMarkChildrenCSSVariableDirty(
    const lepus::Value &css_variable_updated) {
  for (const auto &child : scoped_children_) {
    auto *fiber_child = static_cast<FiberElement *>(child.get());
    if (IsCSSInlineVariablesEnabled()) {
      // Entering RecursivelyMarkChildrenCSSVariableDirty means current
      // element's CSS variables changed.
      // Mark children's custom_properties dirty so CollectCustomProperties can
      // pick up the latest values.
      fiber_child->MarkCustomPropertiesDirty();
    }
    if (fiber_child->is_raw_text()) {
      continue;
    }
    lepus::Value css_variable_updated_merged = css_variable_updated;
    // first, merge changing_css_variables with element's css_variable,
    // element's css_variable is with high priority.
    fiber_child->data_model()->MergeWithCSSVariables(
        css_variable_updated_merged);
    if (IsRelatedCSSVariableUpdated(fiber_child->data_model(),
                                    css_variable_updated_merged)) {
      fiber_child->MarkStyleDirty(false);
    }
    fiber_child->RecursivelyMarkChildrenCSSVariableDirty(
        css_variable_updated_merged);
  }
}

void FiberElement::RecursivelyMarkCustomPropertiesDirty() {
  for (const auto &child : scoped_children_) {
    auto *fiber_child = static_cast<FiberElement *>(child.get());
    if (!fiber_child->is_raw_text()) {
      fiber_child->MarkStyleDirty(false);
    }
    fiber_child->RecursivelyMarkCustomPropertiesDirty();
  }
}

void FiberElement::EnsureSLNode() {
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

void FiberElement::SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func) {
  if (customized_layout_node_ != nullptr) {
    customized_layout_node_->SetMeasureFunc(std::move(measure_func));
  }
}

void FiberElement::MarkAsLayoutRoot() {
  if (EnableLayoutInElementMode()) {
    EnsureSLNode();
    // The default flex direction is column for root
    sl_node_->GetCSSMutableStyle()->SetFlexDirection(
        starlight::FlexDirectionType::kColumn);
    sl_node_->SetContext(element_manager());
    sl_node_->MarkDirty();
    sl_node_->SetSLRequestLayoutFunc([](void *context) {
      static_cast<ElementManager *>(context)->ScheduleLayout();
    });
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->is_root = true;
}

void FiberElement::MarkLayoutDirty() {
  if (EnableLayoutInElementMode()) {
    MarkLayoutDirtyLite();
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->is_dirty = true;
}

void FiberElement::AttachLayoutNode(const fml::RefPtr<PropBundle> &props) {
  if (EnableLayoutInElementMode()) {
    if (IsShadowNodeCustom()) {
      customized_layout_node_ =
          std::make_unique<PlatformLayoutFunctionWrapper>(*this, props);
      element_manager()->layout_context()->CreateLayoutNode(id_, tag_.str(),
                                                            props.get(), false);
    }
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->shadownode_prop_bundle = props;
  layout_bundle_->allow_inline = allow_layoutnode_inline_;
}

void FiberElement::UpdateLayoutNodeProps(const fml::RefPtr<PropBundle> &props) {
  if (EnableLayoutInElementMode()) {
    if (customized_layout_node_) {
      customized_layout_node_->UpdateLayoutNodeProps(props);
    }
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->update_prop_bundles.emplace_back(props);
}

void FiberElement::UpdateLayoutNodeStyle(CSSPropertyID css_id,
                                         const tasm::CSSValue &value) {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->styles.emplace_back(css_id, value);
}

void FiberElement::ResetLayoutNodeStyle(tasm::CSSPropertyID css_id) {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->reset_styles.emplace_back(css_id);
}

void FiberElement::UpdateLayoutNodeFontSize(double cur_node_font_size,
                                            double root_node_font_size) {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->font_scale = element_manager_->GetLynxEnvConfig().FontScale();
  layout_bundle_->cur_node_font_size = cur_node_font_size;
  layout_bundle_->root_node_font_size = root_node_font_size;
}

void FiberElement::UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                             const lepus::Value &value) {
  if (EnableLayoutInElementMode()) {
    EnsureSLNode();
    bool changed = false;
    if (key == starlight::LayoutAttribute::kScroll) {
      changed = sl_node_->attr_map().setScroll(
          value.IsBool() ? std::optional<bool>(value.Bool()) : std::nullopt);
    } else if (key == starlight::LayoutAttribute::kColumnCount) {
      changed = sl_node_->attr_map().setColumnCount(
          value.IsNumber() ? std::optional<int>(value.Number()) : std::nullopt);
    } else if (key == starlight::LayoutAttribute::kListCompType) {
      changed = sl_node_->attr_map().setListCompType(
          value.IsNumber() ? std::optional<int>(value.Number()) : std::nullopt);
    }

    if (changed) {
      if (is_list()) {
        sl_node_->MarkChildrenDirtyWithoutTriggerLayout();
      }
      sl_node_->MarkDirty();
    }
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->attrs.emplace_back(std::make_pair(key, value));
}

void FiberElement::UpdateLayoutNodeByBundle() {
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

void FiberElement::CheckHasInlineContainer(Element *parent) {
  EnsureLayoutBundle();
  allow_layoutnode_inline_ = parent->IsShadowNodeCustom();
}

void FiberElement::EnqueueLayoutTask(base::MoveOnlyClosure<void> operation) {
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    element_context_delegate_->EnqueueTask(std::move(operation));
  } else {
    element_manager()->LegacyHandleLayoutTask(this, std::move(operation));
  }
}

void FiberElement::RequestLayout() {
  if (EnableLayoutInElementMode()) {
    HandleBeforeFlushActionsTask(
        [manager = element_manager(), this]() {
          MarkLayoutDirty();
          manager->SetNeedsLayout();
        },
        kFlagGreedyParallel | kFlagLevelOrderParallel);
    return;
  }

  HandleDelayTask(
      [manager = element_manager()]() { manager->SetNeedsLayout(); });
}

void FiberElement::RequestNextFrame() {
  HandleDelayTask([this]() { element_manager()->RequestNextFrame(this); });
}

void FiberElement::UpdateFiberElement() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_UPDATE_FIBER_ELEMENT,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!is_layout_only_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_UPDATE_PAINTING_NODE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    element_container()->UpdatePaintingNode(TendToFlatten(), prop_bundle_);
  } else if (!CanBeLayoutOnly()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_TRANSITION_TO_NATIVE_VIEW,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // Is layout only and can not be layout only
    TransitionToNativeView();
  }
}

bool FiberElement::IsRelatedCSSVariableUpdated(
    AttributeHolder *holder, const lepus::Value changing_css_variables) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_IS_RELATED_CSS_UPDATED,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  bool changed = false;
  ForEachLepusValue(
      changing_css_variables,
      [holder, &changed](const lepus::Value &key, const lepus::Value &value) {
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

void FiberElement::UpdateCSSVariable(
    const lepus::Value &css_variable_updated,
    std::shared_ptr<PipelineOptions> &pipeline_option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_UPDATE_CSS_VARIABLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  ForEachLepusValue(
      css_variable_updated,
      [self = this](const lepus::Value &key, const lepus::Value &value) {
        self->data_model()->UpdateCSSVariableFromSetProperty(key.String(),
                                                             value.String());
      });
  // merge updated css_variable to merged_,
  // since it may not related with updated variables.
  MarkCustomPropertiesDirty();
  if (IsRelatedCSSVariableUpdated(data_model(), css_variable_updated)) {
    // invalidate self.
    MarkStyleDirty(false);
  }
  RecursivelyMarkChildrenCSSVariableDirty(css_variable_updated);

  // TODO(nihao.royal): use `enable_unified_pixel_pipeline` to switch multi
  // behaviours. After `RunPixelPipeline` is unified, we may remove the
  // redundant logic here.
  if (pipeline_option->enable_unified_pixel_pipeline) {
    pipeline_option->resolve_requested = true;
    pipeline_option->target_node = this;
  } else {
    element_manager()->OnPatchFinish(pipeline_option, this);
  }
}

void FiberElement::SetFontSize(const tasm::CSSValue &value) {
  SetFontSize(value, computed_css_style());
}

void FiberElement::SetFontSize(const tasm::CSSValue &value,
                               starlight::ComputedCSSStyle *target_style) {
  base::flex_optional<float> result;
  const auto current_font_size = target_style->GetFontSize();
  const auto root_font_size = target_style->GetRootFontSize();
  if (!value.IsEmpty()) {
    CheckDynamicUnit(CSSPropertyID::kPropertyIDFontSize, value, false);
    const auto &env_config = element_manager()->GetLynxEnvConfig();
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
    // TODO(ZHOUZHITAO): Figure out why need to determine whether it is a page
    if (is_page() && !is_greedy_parallel_flush()) {
      // FIXME(linxs): if parent's font-size changed, we need to invalidate all
      // descendant‘ style, otherwise the descendant em pattern values will not
      // be updated dynamically. But it may cause perf issue, we can support it
      // later.
      MarkFontSizeInvalidateRecursively();
    } else {
      // TODO(zhouzhitao): to find a better way to make this work
      // TODO(zhouzhitao): Check whether really need to RequireFlush
      MarkDirty(kDirtyFontSize);
    }
  }
}

void FiberElement::ResetFontSize() {
  CheckDynamicUnit(CSSPropertyID::kPropertyIDFontSize, CSSValue(), true);
  // root_font_size_&font_size_ here are used to computed rem&rem
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

void FiberElement::InsertLayoutNode(FiberElement *child, FiberElement *ref) {
  DCHECK(!ref || !ref->is_wrapper());
  if (EnableLayoutInElementMode()) {
    FiberElement *container =
        static_cast<FiberElement *>(FindFirstNonVirtualRenderAncestor());
    bool inserted = false;
    if (container && !child->is_virtual()) {
      container->EnsureSLNode();
      child->EnsureSLNode();
      FiberElement *ref_node =
          ref ? static_cast<FiberElement *>(
                    ref->FindFirstNonVirtualRenderSibling())
              : nullptr;
      if (ref_node) {
        ref_node->EnsureSLNode();
      }
      container->sl_node_->InsertChildBefore(
          child->sl_node_.get(), ref_node ? ref_node->sl_node_.get() : nullptr);
      container->MarkLayoutDirtyLite();
      inserted = true;
    }
    child->attached_to_layout_parent_ = inserted || child->is_virtual();
    return;
  }

  if (child->attached_to_layout_parent_) {
    LOGE("FiberElement layout node already inserted !");
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

void FiberElement::RemoveLayoutNode(FiberElement *child) {
  if (EnableLayoutInElementMode()) {
    if (auto *child_layout_node = child->slnode();
        child_layout_node && child_layout_node->parent()) {
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

void FiberElement::StoreLayoutNode(FiberElement *child, FiberElement *ref) {
  child->render_parent_ = this;
  FiberElement *next_layout_sibling = ref;
  FiberElement *previous_layout_sibling =
      next_layout_sibling ? static_cast<FiberElement *>(
                                next_layout_sibling->previous_render_sibling_)
                          : static_cast<FiberElement *>(last_render_child_);
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

void FiberElement::RestoreLayoutNode(FiberElement *node) {
  if (node->previous_render_sibling_) {
    static_cast<FiberElement *>(node->previous_render_sibling_)
        ->next_render_sibling_ = node->next_render_sibling_;
  } else {
    first_render_child_ = node->next_render_sibling_;
  }
  if (node->next_render_sibling_) {
    static_cast<FiberElement *>(node->next_render_sibling_)
        ->previous_render_sibling_ = node->previous_render_sibling_;
  } else {
    last_render_child_ = node->previous_render_sibling_;
  }
  node->render_parent_ = nullptr;
  node->previous_render_sibling_ = nullptr;
  node->next_render_sibling_ = nullptr;
}

void FiberElement::ParseRawInlineStyles(CSSVariableMap *changed_css_vars) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PARSE_RAW_INLINE_STYLES);
  auto &configs = element_manager_->GetCSSParserConfigs();
  const auto &str = full_raw_inline_style_.str();
  current_raw_inline_custom_properties_->clear();
  data_model()->MoveAndClearCSSInlineVariables(changed_css_vars);

  if (current_raw_important_inline_styles_.has_value()) {
    current_raw_important_inline_styles_->clear();
  }

  ParseStyleDeclarationList(
      str.c_str(), static_cast<uint32_t>(str.size()),
      [this, &configs](const char *key_start, uint32_t key_length,
                       const char *value_start, uint32_t value_length,
                       bool important) {
        (void)configs;
        auto id = CSSProperty::GetPropertyID(
            base::static_string::GenericCacheKey(key_start, key_length));
        if (CSSProperty::IsPropertyValid(id)) {
          auto value = lepus::Value(base::String(value_start, value_length));
          auto &target_map = important ? current_raw_important_inline_styles_
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

void FiberElement::DoFullCSSResolving() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_DO_FULL_STYLE_RESOLVE);

  CSSVariableMap changed_css_vars;
  ResolveStyle(parsed_styles_map_, &changed_css_vars);
  HandlePseudoElement();

  if (!(dirty_ & kDirtyCreated) && !changed_css_vars.empty()) {
    auto table = lepus::Dictionary::Create();

    for (const auto &iter : changed_css_vars) {
      table.get()->SetValue(iter.first, iter.second);
    }
    auto css_var_table = lepus::Value(std::move(table));

    if (IsRelatedCSSVariableUpdated(data_model(), css_var_table)) {
      // invalidate self.
      MarkStyleDirty(false);
    }
    HandleBeforeFlushActionsTask(
        [this, css_var_table_clone = lepus::Value::Clone(css_var_table)]() {
          RecursivelyMarkChildrenCSSVariableDirty(css_var_table_clone);
        },
        kFlagGreedyParallel);
  }
}

const tasm::CSSValue &FiberElement::ResolveCurrentStyleValue(
    const CSSPropertyID &key, const tasm::CSSValue &default_value) {
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

bool FiberElement::RefreshStyle(StyleMap &parsed_styles,
                                base::Vector<CSSPropertyID> &reset_ids,
                                bool force_use_parsed_styles_map) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_REFRESH_STYLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  StyleMap pre_parsed_styles_map;
  if (!parsed_styles_map_.empty()) {
    pre_parsed_styles_map = std::move(parsed_styles_map_);
  }

  MarkCustomPropertiesDirty();
  if (!has_extreme_parsed_styles_) {
    DoFullCSSResolving();
  } else {
    // if extreme_parsed_styles_ has set, we should ignore any class&inline
    // styles
    parsed_styles_map_ = *extreme_parsed_styles_;
    if (only_selector_extreme_parsed_styles_) {
      ProcessFullRawInlineStyle(nullptr);
      StyleMap important_styles;
      MergeInlineStyles(parsed_styles_map_, important_styles);
    }
    // Handle CSS variable
    HandleCSSVariables(parsed_styles_map_);
  }
  if (force_use_parsed_styles_map) {
    // first flush does not need to do diff, just use parsed_styles_map_
    // directly
    return true;
  }

  // diff styles if needed
  bool ret =
      DiffStyleImpl(pre_parsed_styles_map, parsed_styles_map_, parsed_styles);
  // styles left in old_map need to be removed
  pre_parsed_styles_map.for_each(
      [&](const CSSPropertyID &k, const CSSValue &v) {
        // Filter shorthand property that need to be expanded
        if (!CSSProperty::IsShorthandProperty(k)) {
          reset_ids.emplace_back(k);
        }
      });
  return ret;
}

void FiberElement::OnClassChanged(const ClassList &old_classes,
                                  const ClassList &new_classes) {
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

// For snapshot test
void FiberElement::DumpStyle(StyleMap &computed_styles) {
  if (element_manager()->EnableNewStylingPipeline()) {
    computed_styles = computed_css_style()->GetResolvedValues();
    return;
  }

  StyleMap styles;
  base::InlineVector<CSSPropertyID, 16> reset_style_ids;
  this->RefreshStyle(styles, reset_style_ids);
  computed_styles = parsed_styles_map_;
}

void FiberElement::OnPseudoStatusChanged(PseudoState prev_status,
                                         PseudoState current_status) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PSEUDO_CHANGED,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto current_context =
      element_manager_->element_manager_delegate()->GetCurrentPipelineContext();
  std::shared_ptr<PipelineOptions> pipeline_options;
  if (current_context) {
    pipeline_options = current_context->GetOptions();
  } else {
    pipeline_options = std::make_shared<PipelineOptions>();
  }
  // FIXME: Every element will emit the OnPseudoStatusChanged event
  auto *css_fragment = GetRelatedCSSFragment();
  if (css_fragment && css_fragment->enable_css_selector()) {
    // If disable the invalidation do nothing
    if (!css_fragment->enable_css_invalidation()) {
      return;
    }
    css::InvalidationLists invalidation_lists;
    CSSFragment::CollectPseudoChangedInvalidation(
        css_fragment, invalidation_lists, prev_status, current_status);
    data_model_->SetPseudoState(current_status);
    bool inheritance_invalidated = false;
    for (auto *invalidation_set : invalidation_lists.descendants) {
      if (invalidation_set->InvalidatesSelf()) {
        MarkStyleDirty(false);
        if (!inheritance_invalidated && IsCSSInheritanceEnabled()) {
          inheritance_invalidated = true;
          // Self-mark ensures the element re-inherits from ancestors when
          // pseudo-dependent inherited properties (e.g. color) are removed
          // from parsed_styles_map_, avoiding missing inherited values.
          // Cf. self-mark in PrepareForCreateOrUpdate().
          MarkDirtyLite(kDirtyPropagateInherited);
          InvalidateChildrenInheritedStylesRecursively();
        }
      }
      InvalidateChildren(invalidation_set);
      element_manager_->RequestResolve(pipeline_options);
    }
    return;
  }

  if (!css_fragment || css_fragment->pseudo_map().empty()) {
    // no need do any pseudo changing logic if no any touch pseudo token
    return;
  }

  bool cascade_pseudo_enabled = element_manager_->GetEnableCascadePseudo();
  MarkStyleDirty(cascade_pseudo_enabled);
  if (IsCSSInheritanceEnabled()) {
    MarkDirtyLite(kDirtyPropagateInherited);
  }

  has_extreme_parsed_styles_ = false;

  data_model_->SetPseudoState(current_status);
  element_manager_->RequestResolve(pipeline_options);
}

bool FiberElement::TryResolveLogicStyleAndSaveDirectionRelatedStyle(
    CSSPropertyID id, const CSSValue &value) {
  if (!IsDirectionChangedEnabled()) {
    return false;
  }
  // special case.
  if (id == kPropertyIDTextAlign) {
    CSSStyleValue style_type =
        ResolveTextAlign(id, value, computed_css_style()->GetDirection());
    SetStyleInternal(style_type.first, style_type.second);
    return true;
  }
  auto res = ConvertRtlCSSPropertyID(id);
  if (res.first) {
    // Consume and record transition style first before RTL mode.
    if (css_transition_manager_ &&
        css_transition_manager_->ConsumeCSSProperty(id, value)) {
      return true;
    }
    RecordElementPreviousStyle(id, value);
    SetStyleInternal(res.second, value);
    return true;
  }
  return false;
}

// try to Resolve Direction css
void FiberElement::TryDoDirectionRelatedCSSChange(CSSPropertyID id,
                                                  const CSSValue &value,
                                                  IsLogic is_logic_style) {
  CSSPropertyID trans_id = id;
  auto current_direction = computed_css_style()->GetDirection();
  if ((IsRTL(current_direction) && is_logic_style) ||
      IsLynxRTL(current_direction)) {
    auto direction_mapping = CheckDirectionMapping(id);
    trans_id = direction_mapping.rtl_property_;
  } else if (is_logic_style) {
    // Logical property need to be converted to non-logical property that can be
    // recognized by layout engine,i.e. start->left/right
    auto direction_mapping = CheckDirectionMapping(id);
    trans_id = direction_mapping.ltr_property_;
  }

  SetStyleInternal(trans_id, value);
}

void FiberElement::ResetTextAlign(StyleMap &update_map,
                                  bool direction_changed) {
  // If direction has been changed in current render loop, text_align will be
  // reset when handling direction change. Thus when reset text align,
  // set kPropertyIDTextAlign to kStart only when direction is not changed.
  if (!direction_changed) {
    update_map[CSSPropertyID::kPropertyIDTextAlign] =
        CSSValue(starlight::TextAlignType::kStart);
  }
}

void FiberElement::WillResetCSSValue(CSSPropertyID &css_id) {
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

void FiberElement::TraversalInsertFixedElementOfTree() {
  if (IsFixedUnifiedEnabled()) {
    return;
  }

  if (!is_page() && need_handle_fixed_) {
    HandleSelfFixedChange();
    need_handle_fixed_ = false;
  }
  for (auto child : scoped_children_) {
    static_cast<FiberElement *>(child.get())
        ->TraversalInsertFixedElementOfTree();
  }
}

void FiberElement::HandleSelfFixedChange() {
  // 1. If enableFixedNew is `true`, return directly.
  if (IsFixedNewOrUnifiedEnabled()) {
    return;
  }
  // 2. When Using NoDiff, if the element's fixed status is not changed or the
  // element don't have its render_parnet_, return directly.
  // 3. When Using RadonDiff, if the element is not fixed and its fixed status
  // is not changed, return directly.
  bool early_return_condition = false;
  if (IsFiberArch()) {
    early_return_condition = !fixed_changed_ || !render_parent_;
  } else if (IsRadonArch()) {
    early_return_condition = !is_fixed_ && !fixed_changed_;
  }
  if (early_return_condition) {
    return;
  }

  if (is_fixed_) {
    // non-fixed to fixed
    auto *parent = static_cast<FiberElement *>(render_parent_);
    if (!IsFiberArch() && !parent) {
      parent = element_manager()->GetPageElement();
    } else if (parent) {
      parent->HandleRemoveChildAction(this);
    }
    parent->InsertFixedElement(
        this, static_cast<FiberElement *>(next_render_sibling_));
  } else {
    // fixed to non-fixed
    RemoveFixedElement(this);
    auto *parent = static_cast<FiberElement *>(this->parent_);
    auto index = parent->IndexOf(this);
    auto *ref_node = static_cast<FiberElement *>(parent->GetChildAt(index + 1));
    parent->HandleInsertChildAction(this, -1, ref_node);
  }
}

void FiberElement::InsertFixedElement(FiberElement *child,
                                      FiberElement *ref_node) {
  DCHECK(child->is_fixed_);
  // FIXME(linxs): insert fixed child, to be refined later, currently always
  // insert to the end
  auto *parent = static_cast<FiberElement *>(element_manager_->root());
  parent->HandleInsertChildAction(child, 0, nullptr);
  child->fixed_changed_ = false;
}

void FiberElement::RemoveFixedElement(FiberElement *child) {
  // FIXME(linxs): remove fixed child, to be refined later
  if (child->render_parent_ != element_manager_->root()) {
    LOGE("FiberElement::RemoveFixedElement got error for wrong render parent");
    return;
  }

  auto *parent = static_cast<FiberElement *>(element_manager_->root());
  parent->HandleRemoveChildAction(child);
  child->fixed_changed_ = false;
}

void FiberElement::CheckDynamicUnit(CSSPropertyID id, const CSSValue &value,
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

bool FiberElement::CheckHasInvalidationForId(const std::string &old_id,
                                             const std::string &new_id) {
  auto *css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (!css_fragment || !css_fragment->enable_css_invalidation()) {
    return false;
  }
  auto old_size = invalidation_lists_.descendants.size();
  CSSFragment::CollectIdChangedInvalidation(css_fragment, invalidation_lists_,
                                            old_id, new_id);
  return invalidation_lists_.descendants.size() != old_size;
}

bool FiberElement::CheckHasInvalidationForClass(const ClassList &old_classes,
                                                const ClassList &new_classes) {
  auto *css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (!css_fragment || !css_fragment->enable_css_invalidation()) {
    return false;
  }
  auto old_size = invalidation_lists_.descendants.size();
  CSSFragment::CollectClassChangedInvalidation(
      css_fragment, invalidation_lists_, old_classes, new_classes);
  return invalidation_lists_.descendants.size() != old_size;
}

void FiberElement::InvalidateChildren(css::InvalidationSet *invalidation_set) {
  if (invalidation_set->WholeSubtreeInvalid() || !invalidation_set->IsEmpty()) {
    VisitChildren([invalidation_set](FiberElement *child) {
      if (!child->StyleDirty() && !child->is_raw_text() &&
          invalidation_set->InvalidatesElement(*child->data_model())) {
        child->MarkStyleDirty(false);
      }
    });
  }
}

void FiberElement::VisitChildren(
    const base::MoveOnlyClosure<void, FiberElement *> &visitor) {
  for (auto &child : scoped_children_) {
    auto *fiber_child = static_cast<FiberElement *>(child.get());
    // In fiber mode, we skip the children in component
    if (!fiber_child->is_component()) {
      visitor(fiber_child);
      fiber_child->VisitChildren(visitor);
    }
  }
}

void FiberElement::UpdateDynamicElementStyleForNewPipeline(
    uint32_t &style, bool &inner_force_update) {
  constexpr uint32_t kMediaQueryEnvMask =
      DynamicCSSStylesManager::kUpdateViewport |
      DynamicCSSStylesManager::kUpdateScreenMetrics |
      DynamicCSSStylesManager::kUpdateRem | DynamicCSSStylesManager::kUpdateEm;
  bool media_query_env_changed = false;
  if (!is_wrapper() &&
      ((style & kMediaQueryEnvMask) != 0 || (dirty_ & kDirtyFontSize))) {
    auto *fragment = GetRelatedCSSFragment();
    if (StyleResolver::FragmentsHasMediaQueries(fragment)) {
      media_query_env_changed = true;
    }
  }

  if ((dynamic_style_flags_ > 0 || inner_force_update ||
       media_query_env_changed) &&
      !is_wrapper()) {
    NotifyUnitValuesUpdatedToAnimation(style);
    const auto &env_config = element_manager()->GetLynxEnvConfig();

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

void FiberElement::UpdateDynamicChildrenStyleRecursively(uint32_t style,
                                                         bool force_update) {
  auto *child = static_cast<FiberElement *>(first_render_child_);
  while (child) {
    child->UpdateDynamicElementStyleRecursively(style, force_update);
    child = static_cast<FiberElement *>(child->next_render_sibling_);
  }
}

void FiberElement::UpdateDynamicElementStyleRecursively(uint32_t style,
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
      DynamicCSSStylesManager::kUpdateRem | DynamicCSSStylesManager::kUpdateEm;
  if (!is_wrapper() &&
      ((style & kMediaQueryEnvMask) != 0 || (dirty_ & kDirtyFontSize))) {
    auto *fragment = GetRelatedCSSFragment();
    if (StyleResolver::FragmentsHasMediaQueries(fragment)) {
      MarkStyleDirty(false);
    }
  }

  if ((dynamic_style_flags_ > 0 || inner_force_update) && !is_wrapper()) {
    // Style could never be "all" here.
    NotifyUnitValuesUpdatedToAnimation(style);
    const auto &env_config = element_manager()->GetLynxEnvConfig();
    const auto &css_config = element_manager()->GetDynamicCSSConfigs();

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
          [this, style, &css_config](auto id, const auto &value) {
            if (CSSProperty::IsTransitionProps(id) ||
                CSSProperty::IsKeyframeProps(id)) {
              return true;
            }

            if (css_transition_manager_) {
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
                             [](auto id, const auto &value) {
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

void FiberElement::UpdateDynamicElementStyle(uint32_t style,
                                             bool force_update) {
  UpdateDynamicElementStyleRecursively(style, force_update);
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    element_context_delegate_->FlushEnqueuedTasks();
  }
}

void FiberElement::ResetSheetRecursively(
    const std::shared_ptr<CSSStyleSheetManager> &manager) {
  if (is_page() || is_component() || css_id_ != kInvalidCssId) {
    set_style_sheet_manager(manager);
  }

  // reset style sheet.
  ResetStyleSheet();
  for (const auto &child : children()) {
    static_cast<FiberElement *>(child.get())->ResetSheetRecursively(manager);
  }
}

void FiberElement::PrepareOrUpdatePseudoElement(PseudoState state,
                                                StyleMap &style_map) {
  if (style_map.empty() &&
      (!pseudo_elements_.has_value() ||
       pseudo_elements_->find(state) == pseudo_elements_->end())) {
    return;
  }

  PseudoElement *pseudo_element = CreatePseudoElementIfNeed(state);
  pseudo_element->UpdateStyleMap(style_map);
}

PseudoElement *FiberElement::CreatePseudoElementIfNeed(PseudoState state) {
  if (pseudo_elements_.has_value()) {
    auto it = pseudo_elements_->find(state);
    if (it != pseudo_elements_->end()) {
      return it->second.get();
    }
  }

  auto new_pseudo_element = std::make_unique<PseudoElement>(state, this);
  auto result = new_pseudo_element.get();
  (*pseudo_elements_)[state] = std::move(new_pseudo_element);
  return result;
}

void FiberElement::RecursivelyMarkRenderRootElement(FiberElement *render_root) {
  render_root_element_ = render_root;
  if (render_root) {
    element_context_delegate_ = render_root->element_context_delegate_;
  }
  for (const auto &child : scoped_children_) {
    auto *fiber_child = static_cast<FiberElement *>(child.get());
    if (!fiber_child->is_list_item()) {
      fiber_child->RecursivelyMarkRenderRootElement(render_root);
    }
  }
}

void FiberElement::UpdateRenderRootElementIfNecessary(FiberElement *child) {
  if (child->render_root_element_ == this->render_root_element_) {
    // 1. Child has same render root as parent, indicating tree mutation inside
    // same render root, no need to propagate change
    return;
  }
  if (child->render_root_element_ == nullptr) {
    // 2. child doesn't hava valid render_root_element, propagate parent's
    // render_root_element to child subtree
    child->RecursivelyMarkRenderRootElement(
        static_cast<FiberElement *>(this->render_root_element_));
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
  child->RecursivelyMarkRenderRootElement(
      static_cast<FiberElement *>(this->render_root_element_));
}

void FiberElement::SetFontSizeForAllElement(double cur_node_font_size,
                                            double root_node_font_size) {
  computed_css_style()->SetFontSize(cur_node_font_size, root_node_font_size);

  if (pseudo_elements_.has_value()) {
    for (const auto &[key, pseudo_element] : *pseudo_elements_) {
      pseudo_element->SetFontSize(cur_node_font_size, root_node_font_size);
    }
  }
}

void FiberElement::UpdateLengthContextValueForAllElement(
    const LynxEnvConfig &env_config) {
  computed_css_style()->SetFontScale(env_config.FontScale());
  computed_css_style()->SetViewportWidth(env_config.ViewportWidth());
  computed_css_style()->SetViewportHeight(env_config.ViewportHeight());
  computed_css_style()->SetScreenWidth(env_config.ScreenWidth());
  computed_css_style()->SetLayoutUnit(env_config.PhysicalPixelsPerLayoutUnit(),
                                      env_config.LayoutsUnitPerPx());

  if (pseudo_elements_.has_value()) {
    for (const auto &[key, pseudo_element] : *pseudo_elements_) {
      pseudo_element.get()->ComputedCSSStyle()->SetFontScale(
          env_config.FontScale());
      pseudo_element.get()->ComputedCSSStyle()->SetViewportWidth(
          env_config.ViewportWidth());
      pseudo_element.get()->ComputedCSSStyle()->SetViewportHeight(
          env_config.ViewportHeight());
      pseudo_element.get()->ComputedCSSStyle()->SetScreenWidth(
          env_config.ScreenWidth());
      pseudo_element.get()->ComputedCSSStyle()->SetLayoutUnit(
          env_config.PhysicalPixelsPerLayoutUnit(),
          env_config.LayoutsUnitPerPx());
    }
  }
}

// TODO: Move this method out of fiber_element when a more general render root
// is introduced.
void FiberElement::AsyncResolveSubtreeProperty() {
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    if (element_manager()->GetEnableParallelElement() &&
        ((dirty_ & ~kDirtyTree) != 0) && element_context_delegate_ &&
        element_context_delegate_->IsListItemElementContext()) {
      element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
        auto list_item_context_ptr =
            static_cast<ListItemSchedulerAdapter *>(element_context_delegate_);
        list_item_context_ptr->ResolveSubtreeProperty();

        std::promise<ParallelFlushReturn> promise;
        std::future<ParallelFlushReturn> future = promise.get_future();
        auto task_info_ptr =
            fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
                [promise = std::move(promise),
                 context_ptr = list_item_context_ptr]() mutable {
                  promise.set_value(
                      context_ptr->GenerateReduceTaskForResolveProperty());
                },
                std::move(future));
        element_manager()->ParallelTasks().emplace_back(
            std::move(task_info_ptr));
      });
    }
  } else {
    // TODO(ZHOUZHITAO): remove this branch once
    // ENABLE_BATCH_LAYOUT_TASK_WITH_SYNC_LAYOUT is fully rolled out
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
        element_manager()->ParallelTasks().emplace_back(
            std::move(task_info_ptr));
      });
    }
  }
}

void FiberElement::CreateListItemScheduler(
    list::BatchRenderStrategy batch_render_strategy,
    ElementContextDelegate *parent_context, bool continuous_resolve_tree) {
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    std::shared_ptr<ElementContextDelegate> element_context_delegate_ptr =
        std::make_shared<ListItemSchedulerAdapter>(this, batch_render_strategy,
                                                   parent_context,
                                                   continuous_resolve_tree);
    element_context_delegate_ = element_context_delegate_ptr.get();
    parent_context->OnChildElementContextAdded(element_context_delegate_ptr);
  } else {
    scheduler_adapter_ = std::make_unique<ListItemSchedulerAdapter>(
        this, batch_render_strategy, parent_context, continuous_resolve_tree);
  }
}

void FiberElement::DispatchAsyncResolveSubtreeProperty() {
  if (element_manager()->GetEnableParallelElement() &&
      ((dirty_ & ~kDirtyTree) != 0) && this->IsAttached()) {
    UpdateResolveStatus(AsyncResolveStatus::kPrepareTriggered);
    element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
      std::deque<FiberElement *> queue;
      auto root = this;
      queue.emplace_back(root);
      while (!queue.empty()) {
        auto current = queue.front();
        if ((current != root && current->IsAsyncFlushRoot()) ||
            current->IsAsyncResolveResolving()) {
          // skip async flush root element
          queue.pop_front();
          continue;
        }
        {
          current->UpdateResolveStatus(AsyncResolveStatus::kPreparing);
          current->ResolveParentComponentElement();
          if (current->parent()) {
            current->parent()->EnsureTagInfo();
          }
          current->PostResolveTaskToThreadPool(
              false, element_manager()->ParallelTasks());
        }
        for (const auto &child : current->children()) {
          queue.emplace_back(static_cast<FiberElement *>(child.get()));
        }
        queue.pop_front();
      }
    });
  }
}

bool FiberElement::CanBeLayoutOnly() const {
  return can_be_layout_only_ && element_manager()->GetEnableLayoutOnly() &&
         has_layout_only_props_ && computed_css_style()->IsOverflowXY();
}

void FiberElement::MarkLayoutDirtyLite() {
  if (!is_virtual_) {
    EnsureSLNode();
    sl_node_->MarkDirtyAndRequestLayout();
  } else {
    auto *parent = static_cast<FiberElement *>(render_parent_);
    while (parent) {
      if (!parent->is_virtual_) {
        parent->MarkLayoutDirtyLite();
        break;
      }
      parent = static_cast<FiberElement *>(parent->render_parent_);
    }
  }
}

/**
 * Reference {@link LayoutContext#LayoutRecursively }
 */
void FiberElement::UpdateLayoutInfoRecursively(PipelineOptions *options) {
  if (!is_wrapper()) {
    if (sl_node_ == nullptr || !(sl_node_->IsDirty())) {
      return;
    }

    if (IfNeedsUpdateLayoutInfo()) {
      if (is_list()) {
        // TODO(songshourui.null): emplace_back element later
        options->updated_list_elements_.emplace_back(impl_id());
      }
      UpdateLayoutInfo();
    }

    sl_node_->MarkUpdated();
  }

  for (auto &child : scoped_children_) {
    static_cast<FiberElement *>(child.get())
        ->UpdateLayoutInfoRecursively(options);
  }
}

/**
 * Reference {@link LayoutContext#UpdateLayoutInfo }
 */
void FiberElement::UpdateLayoutInfo() {
  const auto &layout_result = sl_node_->GetLayoutResult();
  width_ = layout_result.size_.width_;
  height_ = layout_result.size_.height_;
  top_ = layout_result.offset_.Y();
  left_ = layout_result.offset_.X();
  // paddings
  paddings_[0] = layout_result.padding_[starlight::kLeft];
  paddings_[1] = layout_result.padding_[starlight::kTop];
  paddings_[2] = layout_result.padding_[starlight::kRight];
  paddings_[3] = layout_result.padding_[starlight::kBottom];
  // margins
  margins_[0] = layout_result.margin_[starlight::kLeft];
  margins_[1] = layout_result.margin_[starlight::kTop];
  margins_[2] = layout_result.margin_[starlight::kRight];
  margins_[3] = layout_result.margin_[starlight::kBottom];
  // borders
  borders_[0] = layout_result.border_[starlight::kLeft];
  borders_[1] = layout_result.border_[starlight::kTop];
  borders_[2] = layout_result.border_[starlight::kRight];
  borders_[3] = layout_result.border_[starlight::kBottom];

  if (IsShadowNodeCustom()) {
    element_manager_->layout_context()->OnLayout(id_, left_, top_, width_,
                                                 height_, paddings_, borders_);
    customized_layout_node_->OnLayoutAfter();
  }
  if (EnableFragmentLayerRender()) {
    static_cast<Fragment *>(element_container())->UpdateLayout(layout_result);
  }
  frame_changed_ = true;
}

void FiberElement::SetMeasureFunc(void *context,
                                  starlight::SLMeasureFunc measure_func) {
  sl_node_->SetContext(context);
  sl_node_->SetSLMeasureFunc(std::move(measure_func));
}

void FiberElement::SetAlignmentFunc(void *context,
                                    starlight::SLAlignmentFunc alignment_func) {
  sl_node_->SetSLAlignmentFunc(std::move(alignment_func));
}

/**
 * Reference {@link LayoutContext#DispatchLayoutBeforeRecursively }
 */
void FiberElement::DispatchLayoutBeforeRecursively() {
  if (!is_wrapper()) {
    if (sl_node_ == nullptr || !(sl_node_->IsDirty())) {
      return;
    }

    if (sl_node_->GetSLMeasureFunc()) {
      DispatchLayoutBefore();
    }
  }

  for (auto &child : scoped_children_) {
    static_cast<FiberElement *>(child.get())->DispatchLayoutBeforeRecursively();
  }
}

void FiberElement::DispatchLayoutBefore() {
  if (customized_layout_node_) {
    customized_layout_node_->OnLayoutBefore();
  }
}

#if ENABLE_TRACE_PERFETTO
void FiberElement::UpdateTraceDebugInfo(TraceEvent *event) {
  auto *tagInfo = event->add_debug_annotations();
  tagInfo->set_name("tagName");
  tagInfo->set_string_value(tag_.str());

  if (!data_model_) {
    return;
  }

  if (!data_model_->idSelector().empty()) {
    auto *idInfo = event->add_debug_annotations();
    idInfo->set_name("idSelector");
    idInfo->set_string_value(data_model_->idSelector().str());
  }
  if (!data_model_->classes().empty()) {
    std::string class_str = "";
    for (auto &aClass : data_model_->classes()) {
      class_str = class_str + " " + aClass.str();
    }
    if (!class_str.empty()) {
      auto *classInfo = event->add_debug_annotations();
      classInfo->set_name("class");
      classInfo->set_string_value(class_str);
    }
  }
  auto *nodeIndexInfo = event->add_debug_annotations();
  nodeIndexInfo->set_name("nodeIndex");
  nodeIndexInfo->set_uint_value(node_index_);
}
#endif

bool FiberElement::IsEventPathCatch(event::EventTarget *target,
                                    event::Event *event) {
  if (IsDetached()) {
    LOGE("FiberElement::IsEventPathCatch error: the target is detached.");
    return true;
  }
  return Element::IsEventPathCatch(target, event);
}

bool FiberElement::CollectCustomProperties(AttributeHolder *holder) {
  if (custom_properties_.Get() != nullptr) {
    return true;
  }

  if (!holder) {
    return false;
  }

  if (FiberElement *real_parent = static_cast<FiberElement *>(parent());
      real_parent) {
    if (!real_parent->CollectCustomProperties(real_parent->data_model())) {
      return false;
    }
    // Share parent's map (Copy-On-Write)
    // This is cheap - just increments refcount
    custom_properties_ = real_parent->custom_properties_;
  }

  const auto &variables = holder->css_variables_map();
  const auto &inline_variables = holder->GetCSSInlineVariables();

  // If we don't have any new variables, we can just share the parent's map.
  if (variables.empty() && inline_variables.empty()) {
    if (custom_properties_.Get() == nullptr) {
      custom_properties_.Init();
    }
    return true;
  }

  // Access() will copy the map if it's shared (refcount > 1)
  if (custom_properties_.Get() == nullptr) {
    custom_properties_.Init();
  }
  auto *map = &custom_properties_.Access()->Value();

  // TODO(renzhongyue): Variables declared in CSS must use the normal
  // custom-property declaration syntax, not {{}}.
  for (const auto &[name, value] : variables) {
    CSSStringParser parser{value.c_str(), static_cast<uint32_t>(value.length()),
                           element_manager()->GetCSSParserConfigs()};
    map->insert_or_assign(name, parser.ParseVariable());
  }

  for (const auto &[name, value] : inline_variables) {
    CSSStringParser parser{value.c_str(), static_cast<uint32_t>(value.length()),
                           element_manager()->GetCSSParserConfigs()};
    map->insert_or_assign(name, parser.ParseVariable());
  }

  CSSValue::SubstituteAll(*map);
  return true;
}

// Returns true if any active stylesheet (the element's own CSS fragment or any
// adopted stylesheet from the element manager) contains a next-sibling
// combinator rule (A + B). Used to guard sibling invalidation on insertion /
// removal so we don't dirty siblings when no such rules exist.
bool FiberElement::HasAdjacentSiblingRulesInStyleSheets() {
  auto *css_fragment = GetRelatedCSSFragment();
  return css_fragment && css_fragment->enable_css_selector() &&
         css_fragment->HasAdjacentSiblingRules();
}

void FiberElement::InvalidateChildrenIfNeeded() {
  for (auto *invalidation_set : invalidation_lists_.descendants) {
    InvalidateChildren(invalidation_set);
  }
  invalidation_lists_.descendants.clear_and_shrink();
}

void FiberElement::SetupFragmentBehavior(Fragment *fragment) {
  if (is_list_item()) {
    fragment->SetBehavior(std::make_unique<ListItemFragmentBehavior>(fragment));
    return;
  }

  fragment->SetBehavior(
      std::make_unique<PlatformExtendedFragmentBehavior>(fragment, GetTag()));
}

}  // namespace tasm
}  // namespace lynx
