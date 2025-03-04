// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/worklet/lepus_element.h"

#include <optional>
#include <stack>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/worklet/base/worklet_utils.h"
#include "core/renderer/worklet/lepus_component.h"
#include "core/renderer/worklet/lepus_gesture.h"
#include "core/renderer/worklet/lepus_lynx.h"
#include "core/runtime/bindings/napi/napi_environment.h"
#include "core/runtime/bindings/napi/worklet/napi_lepus_component.h"
#include "core/runtime/bindings/napi/worklet/napi_lepus_element.h"
#include "core/runtime/bindings/napi/worklet/napi_lepus_gesture.h"
#include "core/runtime/bindings/napi/worklet/napi_loader_ui.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/lepus_error_helper.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/value_wrapper/value_impl_lepus.h"
#if OS_IOS
#include "trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace worklet {

// quickjs_id.h
#define LEPUS_CLASS_OBJECT 1

// magic numbers of event method
constexpr int kStopPropagationBit = 0x1;
constexpr int kStopImmediatePropagationBit = 0x2;

const uint64_t kLepusEventProtoID =
    reinterpret_cast<uint64_t>(&kLepusEventProtoID);

static_assert(
    kStopPropagationBit ==
            static_cast<int>(tasm::EventResult::kStopPropagation) &&
        kStopImmediatePropagationBit ==
            static_cast<int>(tasm::EventResult::kStopImmediatePropagation),
    "magic number mismatches");

static LEPUSValue EventAPI_method(LEPUSContext* ctx, LEPUSValueConst this_val,
                                  int argc, LEPUSValueConst* argv, int magic) {
  int* result =
      reinterpret_cast<int*>(LEPUS_GetOpaque(this_val, LEPUS_CLASS_OBJECT));
  *result |= magic;
  return LEPUS_UNDEFINED;
}

static void AddEventAPI(LEPUSContext* ctx, LEPUSValue val, const char* name,
                        int magic) {
  LEPUSValue func = LEPUS_NewCFunctionMagic(ctx, EventAPI_method, name, 0,
                                            LEPUS_CFUNC_generic_magic, magic);
  HandleScope func_scope(ctx, &func, HANDLE_TYPE_LEPUS_VALUE);
  LEPUS_DefinePropertyValueStr(ctx, val, name, func,
                               LEPUS_PROP_WRITABLE | LEPUS_PROP_CONFIGURABLE);
}

static void SetEventPrototype(Napi::Env& env, LEPUSContext* ctx,
                              LEPUSValue js_value) {
  DCHECK(LEPUS_IsObject(js_value));
  DCHECK(LEPUS_GetOpaque(js_value, LEPUS_CLASS_OBJECT) == nullptr);

  // https://developer.mozilla.org/en-US/docs/Web/API/Event
  AddEventAPI(ctx, js_value, "stopPropagation", kStopPropagationBit);
  AddEventAPI(ctx, js_value, "stopImmediatePropagation",
              kStopImmediatePropagationBit);
}

static void WrapEventTarget(
    Napi::Env& env, LEPUSContext* ctx, LEPUSValue js_value, int id,
    const char* name, tasm::TemplateAssembler* tasm,
    const std::shared_ptr<worklet::LepusApiHandler>& task_handler) {
  auto wrapper =
      NapiLepusElement::Wrap(std::unique_ptr<LepusElement>(LepusElement::Create(
                                 id, tasm->shared_from_this(), task_handler)),
                             env);
  LEPUSValue target = LEPUS_GetPropertyStr(ctx, js_value, name);
#ifdef USE_PRIMJS_NAPI
  LEPUSValue new_target =
      *reinterpret_cast<LEPUSValue*>(static_cast<napi_value_primjs>(wrapper));
#else
  LEPUSValue new_target =
      *reinterpret_cast<LEPUSValue*>(static_cast<napi_value>(wrapper));
#endif  // USE_PRIMJS_NAPI
  LEPUS_SetPropertyStr(ctx, js_value, name, LEPUS_DupValue(ctx, new_target));

  LEPUSValue prop_val = LEPUS_GetPropertyStr(ctx, target, "id");
  LEPUS_SetPropertyStr(ctx, new_target, "id", prop_val);
  prop_val = LEPUS_GetPropertyStr(ctx, target, "dataset");
  LEPUS_SetPropertyStr(ctx, new_target, "dataset", prop_val);
  LEPUS_SetPropertyStr(ctx, new_target, "uid", LEPUS_NewInt32(ctx, id));
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, target);
}

LepusElement::LepusElement(
    int32_t element_id, const std::shared_ptr<tasm::TemplateAssembler>& tasm,
    const std::shared_ptr<worklet::LepusApiHandler>& task_handler)
    : element_id_(element_id), weak_tasm_(tasm) {
  if (task_handler != nullptr) {
    task_handler_ = task_handler;
  } else {
    LOGE("LepusElement::constructor task_handler is nullptr");
  }
}

tasm::Element* LepusElement::GetElement() {
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    return nullptr;
  }
  if (tasm->destroyed()) {
    return nullptr;
  }
  return tasm->page_proxy()->element_manager()->node_manager()->Get(
      element_id_);
}

tasm::EventResult LepusElement::FireElementWorklet(
    const std::string& component_id, const std::string& entry_name,
    tasm::TemplateAssembler* tasm, lepus::Value& func_val,
    const lepus::Value& func_obj, const lepus::Value& value,
    const std::shared_ptr<worklet::LepusApiHandler>& task_handler,
    int element_id, tasm::EventType type) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::FireElementWorklet");
  if (tasm == nullptr) {
    return tasm::EventResult::kDefault;
  }

  // Get & Exec element worklet function
  auto* ctx = func_val.context();

  // Using lepus::Value to wrap LEPUSValue, so that we don't have to
  // LEPUS_FreeValue it. When using as LEPUSValue, use WrapJSValue()
  const auto func_val_wrapper = lepus::Value(ctx, func_val.ToJSValue(ctx));
  const auto func_obj_wrapper = lepus::Value(ctx, func_obj.ToJSValue(ctx));
  const auto value_wrapper = lepus::Value(ctx, value.ToJSValue(ctx, true));

  const auto func_val_js_value = func_val_wrapper.WrapJSValue();
  const auto func_obj_js_value = func_obj_wrapper.WrapJSValue();
  const auto value_js_value = value_wrapper.WrapJSValue();

  if (!LEPUS_IsFunction(ctx, func_val_js_value)) {
    return tasm::EventResult::kDefault;
  }

#ifdef USE_PRIMJS_NAPI
  Napi::Env env(reinterpret_cast<napi_env_primjs>(
      static_cast<lepus::QuickContext*>(tasm->context(entry_name))
          ->napi_env()));
#else
  Napi::Env env(reinterpret_cast<napi_env>(
      static_cast<lepus::QuickContext*>(tasm->context(entry_name))
          ->napi_env()));
#endif  // USE_PRIMJS_NAPI

  auto lepus_component = worklet::LepusComponent::Create(
      component_id, tasm->shared_from_this(),
      std::weak_ptr<worklet::LepusApiHandler>(task_handler));
  auto component_ins = worklet::NapiLepusComponent::Wrap(
      std::unique_ptr<LepusComponent>(lepus_component), env);
#ifdef USE_PRIMJS_NAPI
  auto component_obj = *reinterpret_cast<LEPUSValue*>(
      static_cast<napi_value_primjs>(component_ins));
#else
  auto component_obj =
      *reinterpret_cast<LEPUSValue*>(static_cast<napi_value>(component_ins));
#endif  // USE_PRIMJS_NAPI
  std::unique_ptr<LEPUSValue> gesture_obj = nullptr;
  if (type == tasm::EventType::kGesture) {
    auto gesture_ins = worklet::NapiLepusGesture::Wrap(
        std::unique_ptr<LepusGesture>(worklet::LepusGesture::Create(
            element_id, tasm->shared_from_this())),
        env);
#ifdef USE_PRIMJS_NAPI
    gesture_obj = std::make_unique<LEPUSValue>(*reinterpret_cast<LEPUSValue*>(
        static_cast<napi_value_primjs>(gesture_ins)));
#else
    gesture_obj = std::make_unique<LEPUSValue>(
        *reinterpret_cast<LEPUSValue*>(static_cast<napi_value>(gesture_ins)));
#endif  // USE_PRIMJS_NAPI
  }

  BASE_STATIC_STRING_DECL(kTarget, "target");
  BASE_STATIC_STRING_DECL(kUid, "uid");
  int target_id =
      value.Table()->GetValue(kTarget).Table()->GetValue(kUid).Int32();
  WrapEventTarget(env, ctx, value_js_value, target_id, "target", tasm,
                  task_handler);
  WrapEventTarget(env, ctx, value_js_value, element_id, "currentTarget", tasm,
                  task_handler);
  SetEventPrototype(env, ctx, value_js_value);

  int event_result = 0;
  LEPUS_SetOpaque(value_js_value, &event_result);

  LepusElement::CallLepusWithComponentInstance(
      tasm, ctx, func_val_js_value, func_obj_js_value, value_js_value,
      component_obj, std::move(gesture_obj));

  return static_cast<tasm::EventResult>(event_result);
}

std::optional<lepus::Value> LepusElement::TriggerWorkletFunction(
    tasm::TemplateAssembler* tasm, tasm::BaseComponent* component,
    const std::string& worklet_module_name, const std::string& method_name,
    const lepus::Value& args,
    const std::shared_ptr<worklet::LepusApiHandler>& task_handler) {
  // Lifetime of const reference worklet_module_name, method_name and args can
  // be determined at template_assembler triggerWorkletFunction

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::TriggerWorkletFunction");
  if (component == nullptr || tasm == nullptr) {
    LOGE(
        "LepusComponent::TriggerWorkletFunction failed since tasm or component "
        "is null.");
    return std::nullopt;
  }

  // We stored workelt_instance at component by key
  // For example at a ttml: <script src="./worklet.js"
  // name="worklet-module"></script> where key is "worklet-module"

  auto& instances = component->worklet_instances();
  lepus::Value worklet_instance;

  auto iter = instances.find(worklet_module_name);
  if (iter == instances.end()) {
    tasm->ReportError(
        error::E_WORKLET_MODULE_EXCEPTION,
        std::string{"Can not find worklet module of name: "}.append(
            worklet_module_name));
    return std::nullopt;
  }

  worklet_instance = iter->second;

  // Get function with method_name, and make sure it's OK

  LEPUSContext* ctx = worklet_instance.context();

  // Using lepus::Value to wrap LEPUSValue, so that we don't have to
  // LEPUS_FreeValue it. When using as LEPUSValue, use WrapJSValue()
  const auto worklet_instance_wrapper =
      lepus::Value(ctx, worklet_instance.ToJSValue(ctx));
  const auto worklet_module_function_wrapper = lepus::Value(
      ctx, LEPUS_GetPropertyStr(ctx, worklet_instance_wrapper.WrapJSValue(),
                                method_name.c_str()));

  const auto worklet_instance_js_value = worklet_instance_wrapper.WrapJSValue();
  const auto worklet_module_function_js_value =
      worklet_module_function_wrapper.WrapJSValue();

  if (!LEPUS_IsFunction(ctx, worklet_module_function_js_value)) {
    tasm->ReportError(error::E_WORKLET_MODULE_EXCEPTION,
                      std::string{"TriggerWorkletFunction failed since "}
                          .append(method_name)
                          .append(" is not a function"));

    return std::nullopt;
  }

  // Make a component_instance with NAPI wrap
  auto entry_name = component->GetEntryName();
  if (entry_name.empty()) {
    entry_name = tasm::DEFAULT_ENTRY_NAME;
  }

#ifdef USE_PRIMJS_NAPI
  Napi::Env env(reinterpret_cast<napi_env_primjs>(
      static_cast<lepus::QuickContext*>(tasm->context(entry_name))
          ->napi_env()));
#else
  Napi::Env env(reinterpret_cast<napi_env>(
      static_cast<lepus::QuickContext*>(tasm->context(entry_name))
          ->napi_env()));
#endif  // USE_PRIMJS_NAPI

  auto component_ins = worklet::NapiLepusComponent::Wrap(
      std::unique_ptr<LepusComponent>(LepusComponent::Create(
          component->ComponentStrId(), tasm->shared_from_this(), task_handler)),
      env);

#ifdef USE_PRIMJS_NAPI
  auto component_obj = *reinterpret_cast<LEPUSValue*>(
      static_cast<napi_value_primjs>(component_ins));
#else
  auto component_obj =
      *reinterpret_cast<LEPUSValue*>(static_cast<napi_value>(component_ins));
#endif  // USE_PRIMJS_NAPI

  const auto args_wrapper = lepus::Value(ctx, args.ToJSValue(ctx, true));

  std::optional<lepus::Value> call_result_value =
      LepusElement::CallLepusWithComponentInstance(
          tasm, ctx, worklet_module_function_js_value,
          worklet_instance_js_value, args_wrapper.WrapJSValue(), component_obj,
          nullptr);

  return call_result_value;
}

/**
 * @brief LEPUS_Call a function, its arguments are append with componentInstance
 * @return std::optional<lepus::Value> Return std::nullopt when failed,
 * otherwise return lepus::Value
 */
std::optional<lepus::Value> LepusElement::CallLepusWithComponentInstance(
    lynx::tasm::TemplateAssembler* tasm, LEPUSContext* ctx,
    const LEPUSValue& func_obj, const LEPUSValue& this_obj,
    const LEPUSValue& args, const LEPUSValue& component_instance,
    const std::unique_ptr<LEPUSValue> gesture_instance) {
  if (tasm == nullptr) {
    return std::nullopt;
  }

  std::vector<LEPUSValue> lepus_call_args;

  lepus_call_args.push_back(args);
  lepus_call_args.push_back(component_instance);
  if (gesture_instance != nullptr) {
    lepus_call_args.push_back(*gesture_instance);
  }

  lepus::Value call_result_wrapper = lepus::Value(
      ctx, LEPUS_Call(ctx, func_obj, this_obj,
                      static_cast<int>(lepus_call_args.size()),
                      static_cast<LEPUSValue*>(lepus_call_args.data())));
  const auto call_result_js_value = call_result_wrapper.WrapJSValue();

  if (LEPUS_IsException(call_result_js_value)) {
    base::logging::LogStream ss;
    ss << "Worklet call function failed." << std::endl;
    auto exception_wrapper = lepus::Value(ctx, LEPUS_GetException(ctx));
    auto exception_js_value = exception_wrapper.WrapJSValue();
    const auto& msg =
        lepus::LepusErrorHelper::GetErrorMessage(ctx, exception_js_value);
    const auto& stack =
        lepus::LepusErrorHelper::GetErrorStack(ctx, exception_js_value);
    ss << "The error message is : " << std::endl;
    ss << msg << std::endl;
    ss << "The call stack is : " << std::endl;
    ss << stack << std::endl;
    tasm->ReportError(error::E_WORKLET_MTS_CALL_EXCEPTION, ss.str());

    return std::nullopt;
  }
  LEPUSContext* ictx = nullptr;
  int result = 0;
  while ((result = LEPUS_ExecutePendingJob(LEPUS_GetRuntime(ctx), &ictx))) {
    if (unlikely(ictx != ctx)) {
      continue;
    }
    if (result < 0) {
      ReportPendingJobException(tasm, ctx, false);

      return std::nullopt;
    }
    while (LEPUS_MoveUnhandledRejectionToException(ctx)) {
      ReportPendingJobException(tasm, ctx, true);
    }
  }

  return call_result_wrapper;
}

void LepusElement::ReportPendingJobException(
    lynx::tasm::TemplateAssembler* tasm, LEPUSContext* ctx, bool is_ur) {
  std::ostringstream ss;
  std::string prefix;
  if (is_ur)
    prefix = "Worklet call function unhandled rejection.";
  else
    prefix = "Worklet call function pending job exception.";
  ss << prefix << std::endl;
  auto exception_wrapper = lepus::Value(ctx, LEPUS_GetException(ctx));
  auto exception_js_value = exception_wrapper.WrapJSValue();
  const auto& msg =
      lepus::LepusErrorHelper::GetErrorMessage(ctx, exception_js_value);
  const auto& stack =
      lepus::LepusErrorHelper::GetErrorStack(ctx, exception_js_value);
  ss << "The error message is : " << std::endl;
  ss << msg << std::endl;
  lynx::base::LynxError error(error::E_WORKLET_MTS_CALL_EXCEPTION, ss.str(), "",
                              base::LynxErrorLevel::Error);
  error.custom_info_.emplace("error_stack", stack);
  tasm->ReportError(std::move(error));
}

void LepusElement::SetStyles(const Napi::Object& styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::SetStyles");
  auto element = GetElement();
  if (element == nullptr) {
    LOGE("LepusElement::SetStyles failed, since element is null.");
    return;
  }

  const auto& lepus_v = ValueConverter::ConvertNapiValueToLepusValue(styles);
  if (!lepus_v.IsTable()) {
    LOGE("LepusElement::SetStyles failed, since input para is not object.");
    return;
  }

  if (element->is_radon_element()) {
    // TODO(songshourui.null): do not need to UpdateDynamicElementStyle each
    // time, fix this later.
    for (const auto& pair : *(lepus_v.Table())) {
      const auto& key = tasm::CSSProperty::GetPropertyID(pair.first);
      element->ConsumeStyle(tasm::UnitHandler::Process(
          key, pair.second, element->element_manager()->GetCSSParserConfigs()));
    }
    element->element_manager()->root()->UpdateDynamicElementStyle(
        tasm::DynamicCSSStylesManager::kAllStyleUpdate, false);
    element->FlushProps();
  } else {
    auto* fiber_element = static_cast<tasm::FiberElement*>(element);
    for (const auto& pair : *(lepus_v.Table())) {
      fiber_element->SetStyle(tasm::CSSProperty::GetPropertyID(pair.first),
                              pair.second);
    }
    tasm::PipelineOptions pipeline_options;
    fiber_element->element_manager()->OnPatchFinish(pipeline_options,
                                                    fiber_element);
  }
}

void LepusElement::SetAttributes(const Napi::Object& attributes) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::SetAttributes");
  auto element = GetElement();
  if (element == nullptr) {
    LOGE("LepusElement::SetAttributes failed, since element is null.");
    return;
  }

  const auto& lepus_v =
      ValueConverter::ConvertNapiValueToLepusValue(attributes);
  if (!lepus_v.IsTable()) {
    LOGE(
        "Element Worklet SetAttributes failed, since input para is not "
        "object.");
    return;
  }

  for (const auto& pair : *(lepus_v.Table())) {
    element->SetAttribute(pair.first, pair.second);
    constexpr const static char* kText = "text";
    if (pair.first.IsEqual(kText) && element->data_model() != nullptr &&
        element->data_model()->tag().IsEqual(kText)) {
      for (size_t i = 0; i < element->GetChildCount(); ++i) {
        const auto& child = element->GetChildAt(i);
        child->SetAttribute(pair.first, pair.second);
        if (child->is_radon_element()) {
          child->FlushProps();
        }
      }
    }
  }
  if (element->is_radon_element()) {
    element->FlushProps();
  } else {
    tasm::PipelineOptions pipeline_options;
    element->element_manager()->OnPatchFinish(pipeline_options, element);
  }
}

Napi::Object LepusElement::GetComputedStyles(
    const std::vector<Napi::String>& keys) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::GetComputedStyles");
  auto res = Napi::Object::New(NapiEnv());
  auto element = GetElement();
  if (element == nullptr) {
    LOGE("LepusElement::GetStyles failed, since element is null.");
    return res;
  }

  const auto& styles = element->GetStylesForWorklet();
  for (const auto& key : keys) {
    auto iter = styles.find(tasm::CSSProperty::GetPropertyID(key.Utf8Value()));
    if (iter == styles.end()) {
      res.Set(key, NapiEnv().Undefined());
    } else {
      res.Set(key,
              Napi::String::New(NapiEnv(), tasm::CSSDecoder::CSSValueToString(
                                               iter->first, iter->second)));
    }
  }

  constexpr const static char* kScrollView = "scroll-view";
  constexpr const static char* kXScrollView = "x-scroll-view";
  if (element->GetTag() == kScrollView || element->GetTag() == kXScrollView) {
    constexpr const static char* kDisplay = "display";
    constexpr const static char* kLinear = "linear";
    res.Set(Napi::String::New(NapiEnv(), kDisplay),
            Napi::String::New(NapiEnv(), kLinear));
  }

  return res;
}

Napi::Object LepusElement::GetAttributes(
    const std::vector<Napi::String>& keys) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::GetAttributes");
  auto res = Napi::Object::New(NapiEnv());
  auto element = GetElement();
  if (element == nullptr) {
    LOGE("LepusElement::GetAttributes failed, since element is null.");
    return res;
  }

  const auto& attributes = element->GetAttributesForWorklet();
  if (attributes.size() <= 0) {
    LOGI(
        "Element Worklet GetAttributes failed, since element's attributes is "
        "empty.");
    return res;
  }
  for (const auto& key : keys) {
    auto iter = attributes.find(key.Utf8Value());
    if (iter == attributes.end()) {
      res.Set(key, NapiEnv().Undefined());
    } else {
      res.Set(key, ValueConverter::ConvertLepusValueToNapiValue(NapiEnv(),
                                                                iter->second));
    }
  }
  return res;
}

Napi::Object LepusElement::GetDataset() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::GetDataset");
  auto env = NapiEnv();
  auto res = Napi::Object::New(env);
  auto element = GetElement();
  if (element == nullptr) {
    LOGE("LepusElement::GetDataset failed, since element is null.");
    return res;
  }

  auto data_model = element->data_model();
  if (data_model == nullptr) {
    LOGI(
        "Element Worklet GetDataset failed, since element's data_model is "
        "null.");
    return res;
  }

  const auto& data_set = data_model->dataset();
  if (data_set.empty()) {
    LOGI(
        "Element Worklet GetDataset failed, since data_model's data_set is "
        "empty.");
    return res;
  }
  for (const auto& pair : data_set) {
    res.Set(Napi::String::New(env, pair.first.str()),
            ValueConverter::ConvertLepusValueToNapiValue(env, pair.second));
  }
  return res;
}

Napi::Value LepusElement::ScrollBy(float width, float height) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::ScrollBy");
  auto env = NapiEnv();
  Napi::Object obj = Napi::Object::New(env);
  std::vector<float> res{0, 0, width, height};
  auto element = GetElement();

  if (element != nullptr) {
    res = element->ScrollBy(width * element->computed_css_style()
                                        ->GetMeasureContext()
                                        .layouts_unit_per_px_,
                            height * element->computed_css_style()
                                         ->GetMeasureContext()
                                         .layouts_unit_per_px_);
  } else {
    LOGE("LepusElement::ScrollBy failed, since element is null.");
  }
  constexpr const static char* kConsumedX = "consumedX";
  constexpr const static char* kConsumedY = "consumedY";
  constexpr const static char* kUnConsumedX = "unconsumedX";
  constexpr const static char* kUnConsumedY = "unconsumedY";

  obj.Set(kConsumedX, res[0] / element->computed_css_style()
                                   ->GetMeasureContext()
                                   .layouts_unit_per_px_);
  obj.Set(kConsumedY, res[1] / element->computed_css_style()
                                   ->GetMeasureContext()
                                   .layouts_unit_per_px_);
  obj.Set(kUnConsumedX, res[2] / element->computed_css_style()
                                     ->GetMeasureContext()
                                     .layouts_unit_per_px_);
  obj.Set(kUnConsumedY, res[3] / element->computed_css_style()
                                     ->GetMeasureContext()
                                     .layouts_unit_per_px_);
  return obj;
}

Napi::Value LepusElement::GetBoundingClientRect() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::GetBoundingClientRect");
  auto env = NapiEnv();
  Napi::Object obj = Napi::Object::New(env);
  auto element = GetElement();
  if (element == nullptr) {
    return obj;
  }
  const auto& res = element->GetRectToLynxView();
  constexpr const static int32_t kSize = 4;
  if (res.size() != kSize) {
    return obj;
  }

  constexpr const static char* kLeft = "left";
  constexpr const static char* kTop = "top";
  constexpr const static char* kRight = "right";
  constexpr const static char* kBottom = "bottom";
  constexpr const static char* kWidth = "width";
  constexpr const static char* kHeight = "height";

  obj.Set(kLeft, res[0] / element->computed_css_style()
                              ->GetMeasureContext()
                              .layouts_unit_per_px_);
  obj.Set(kTop, res[1] / element->computed_css_style()
                             ->GetMeasureContext()
                             .layouts_unit_per_px_);
  obj.Set(kWidth, res[2] / element->computed_css_style()
                               ->GetMeasureContext()
                               .layouts_unit_per_px_);
  obj.Set(kHeight, res[3] / element->computed_css_style()
                                ->GetMeasureContext()
                                .layouts_unit_per_px_);
  obj.Set(kRight, (res[0] + res[2]) / element->computed_css_style()
                                          ->GetMeasureContext()
                                          .layouts_unit_per_px_);
  obj.Set(kBottom, (res[1] + res[3]) / element->computed_css_style()
                                           ->GetMeasureContext()
                                           .layouts_unit_per_px_);
  return obj;
}

void LepusElement::Invoke(const Napi::Object& object) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusElement::Invoke");
  auto element = GetElement();
  if (element == nullptr) {
    LOGE("LepusElement::Invoke failed since element is null.");
    return;
  }

  if (!object.IsObject()) {
    LOGE("LepusElement::Invoke failed since param is not a object.");
    return;
  }

  constexpr const static char* sKeyMethod = "method";
  constexpr const static char* sKeyParams = "params";
  constexpr const static char* sKeySuccess = "success";
  constexpr const static char* sKeyFail = "fail";

  if (!object.Has(sKeyMethod).FromMaybe(false) ||
      !object.Get(sKeyMethod).IsString()) {
    LOGE("LepusElement::Invoke failed since param doesn't contain "
         << sKeyMethod << ", or it is not string");
    return;
  }

  auto arg2_cb = std::unique_ptr<NapiFuncCallback>();

  const static auto& get_func_persistent = [](Napi::Env env, Napi::Value val) {
    Napi::Value fuc = val.IsFunction() ? val : Napi::Value();
    return new NapiFuncCallback(
        Napi::Persistent(fuc).Value().As<Napi::Function>());
  };

  auto handler = task_handler_.lock();
  if (handler == nullptr) {
    LOGE(
        "LepusElement::Invoke failed since task_handler is "
        "null.");
    return;
  }

  auto success_p = get_func_persistent(NapiEnv(), object.Get(sKeySuccess));
  auto fail_p = get_func_persistent(NapiEnv(), object.Get(sKeyFail));
  auto weak_handler = task_handler_;
  auto weak_tasm = weak_tasm_;

  int64_t success_callback_id = handler->StoreTask(
      std::unique_ptr<NapiFuncCallback>(std::move(success_p)));
  int64_t fail_callback_id =
      handler->StoreTask(std::unique_ptr<NapiFuncCallback>(std::move(fail_p)));

  element->Invoke(
      object.Get(sKeyMethod).ToString().Utf8Value(),
      pub::ValueImplLepus(
          ValueConverter::ConvertNapiValueToLepusValue(object.Get(sKeyParams))),
      [env = NapiEnv(), weak_handler, weak_tasm, success_callback_id,
       fail_callback_id](int32_t code, const pub::Value& data) {
        auto handler = weak_handler.lock();
        auto tasm = weak_tasm.lock();
        if (handler == nullptr) {
          LOGE(
              "LepusElement::Invoke not callback since task_handler is "
              "null.");
          return;
        }
        if (tasm == nullptr) {
          LOGE(
              "LepusElement::Invoke not callback since task_handler is "
              "null.");
          return;
        }
        auto result_data = Napi::Object::New(env);
        result_data.Set("code", Napi::Number::New(env, code));
        result_data.Set("data",
                        ValueConverter::ConvertPubValueToNapiObject(env, data));

        if (code == 0) {
          handler->InvokeWithTaskID(success_callback_id, result_data,
                                    tasm.get());
          // remove fail callback when calling invoke success avoid memory leak
          handler->RemoveTimeTask(fail_callback_id);
        } else {
          handler->InvokeWithTaskID(fail_callback_id, result_data, tasm.get());
          // remove success callback when calling invoke fail avoid memory leak
          handler->RemoveTimeTask(success_callback_id);
        }
      });
}
}  // namespace worklet
}  // namespace lynx
