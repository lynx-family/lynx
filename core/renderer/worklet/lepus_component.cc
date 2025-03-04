// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/worklet/lepus_component.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/data/template_data.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/selector/fiber_element_selector.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/dom/vdom/radon/radon_base.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/worklet/base/worklet_utils.h"
#include "core/runtime/bindings/napi/worklet/napi_frame_callback.h"
#include "core/runtime/bindings/napi/worklet/napi_func_callback.h"
#include "core/runtime/bindings/napi/worklet/napi_lepus_element.h"

namespace lynx {
namespace worklet {

namespace {

tasm::BaseComponent* GetComponentWithID(tasm::TemplateAssembler* tasm,
                                        const std::string& id) {
  if (tasm->GetPageConfig()->GetEnableFiberArch()) {
    auto* element = tasm->page_proxy()->ComponentElementWithStrId(id);
    if (element != nullptr) {
      return static_cast<tasm::ComponentElement*>(element);
    }
    return static_cast<tasm::PageElement*>(
        tasm->page_proxy()->GetPageElement());
  } else {
    if (id == std::to_string(tasm::kRadonPageID)) {
      return tasm->page_proxy()->Page();
    }
    return tasm->page_proxy()->ComponentWithId(std::atoi(id.c_str()));
  }
}

}  // namespace

LepusComponent::LepusComponent(
    const std::string& component_id,
    const std::shared_ptr<tasm::TemplateAssembler>& assembler,
    std::weak_ptr<worklet::LepusApiHandler> task_handler)
    : component_id_(component_id),
      weak_tasm_(assembler),
      raf_handler_(std::make_unique<LepusAnimationFrameTaskHandler>()),
      task_handler_(task_handler) {}

LepusComponent::~LepusComponent() = default;

LepusElement* LepusComponent::QuerySelector(const std::string& selector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::QuerySelector", "selector",
              selector);
  const auto& res = QuerySelector(selector, true);
  if (res.empty()) {
    return nullptr;
  }
  return res[0];
}

void LepusComponent::HandleJSCallbackLepus(const int64_t callback_id,
                                           const lepus::Value& data) {
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::HandleJSCallbackLepus failed since tasm is null.");
    return;
  }
  auto task = task_handler_.lock();
  if (task == nullptr) {
    LOGE(
        "LepusComponent::HandleJSCallbackLepus failed since task_handler_ is "
        "null.");
    return;
  }
  task->InvokeWithTaskID(
      callback_id,
      ValueConverter::ConvertLepusValueToNapiValue(NapiEnv(), data),
      tasm.get());
}

std::vector<LepusElement*> LepusComponent::QuerySelectorAll(
    const std::string& selector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::QuerySelectorAll",
              "selector", selector);
  return QuerySelector(selector, false);
}

int64_t LepusComponent::RequestAnimationFrame(
    std::unique_ptr<NapiFrameCallback> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::RequestAnimationFrame");
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::RequestAnimationFrame failed since tasm is null.");
    constexpr static int64_t sFailInt = -1;
    return sFailInt;
  }

  tasm->GetDelegate().RequestVsync(
      reinterpret_cast<uintptr_t>(this),
      fml::MakeCopyable([this, weak = Napi::Weak(NapiObject())](
                            int64_t frame_start, int64_t frame_end) {
        if (!weak.Value().IsUndefined()) {
          this->DoFrame(frame_start, frame_end);
        }
      }));

  return raf_handler_->RequestAnimationFrame(std::move(callback));
}

void LepusComponent::CancelAnimationFrame(int64_t id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::CancelAnimationFrame");
  raf_handler_->CancelAnimationFrame(id);
}

void LepusComponent::TriggerEvent(const std::string& event_name,
                                  Napi::Object event_detail,
                                  Napi::Object event_option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::TriggerEvent", "event_name",
              event_name);
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::TriggerEvent failed since tasm is null.");
    return;
  }
  auto env = NapiEnv();
  constexpr const static char* kEventToLepus = "toLepus";
  constexpr const static char* kEventToJS = "toJS";
  constexpr const static char* kEventDetail = "eventDetail";
  constexpr const static char* kEventOption = "eventOption";
  constexpr const static char* kEventComponentId = "componentId";

  // Event triggered from lepus, toLepus default value is ture, toJS default
  // value is false.
  if (event_option.IsNull() || event_option.IsUndefined() ||
      !event_option.IsObject()) {
    event_option = Napi::Object::New(env);
  }
  if (event_option.Get(kEventToLepus).IsUndefined()) {
    event_option.Set(kEventToLepus, Napi::Boolean::New(env, true));
  }
  if (event_option.Get(kEventToJS).IsUndefined()) {
    event_option.Set(kEventToJS, Napi::Boolean::New(env, false));
  }

  // Construct event para.
  Napi::Object para = Napi::Object::New(NapiEnv());
  para.Set(kEventDetail, event_detail);
  para.Set(kEventOption, event_option);
  para.Set(kEventComponentId, Napi::String::New(NapiEnv(), component_id_));
  const auto& lepus_para = ValueConverter::ConvertNapiValueToLepusValue(para);

  tasm->TriggerComponentEvent(event_name, lepus_para);
}

void LepusComponent::CallJSFunction(const std::string& func_name,
                                    Napi::Object func_param) {
  CallJSFunction(func_name, std::move(func_param), nullptr);
}

void LepusComponent::CallJSFunction(
    const std::string& func_name, Napi::Object func_param,
    std::unique_ptr<NapiFuncCallback> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::CallJSFunction",
              "func_name", func_name);
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::CallJSFunctionAsync failed since tasm is null. ");
    return;
  }

  auto* component = GetComponentWithID(tasm.get(), component_id_);
  if (component == nullptr) {
    LOGE(
        "LepusComponent::CallJSFunctionAsync failed since can not find "
        "component.");
    return;
  }

  auto handler = task_handler_.lock();
  if (handler == nullptr && callback != nullptr) {
    LOGE(
        "LepusComponent::CallJSFunctionAsync failed since task_handler is "
        "null.");
    return;
  }
  constexpr const static char* kEventCallbackId = "callbackId";
  // store callback id in func_param
  int64_t callback_id;
  if (callback == nullptr) {
    // no need callback
    callback_id = -1;
  } else {
    callback_id = handler->StoreTask(std::move(callback));
  }
  func_param.Set(kEventCallbackId, callback_id);

  const auto& lepus_para =
      ValueConverter::ConvertNapiValueToLepusValue(func_param);
  tasm->CallJSFunctionInLepusEvent(component_id_, std::move(func_name),
                                   lepus_para);
  return;
}

std::vector<LepusElement*> LepusComponent::QuerySelector(
    const std::string& selector, bool single) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::QuerySelector", "selector",
              selector, "single", single);
  std::vector<LepusElement*> res;
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::QuerySelectorInner failed since tasm is null.");
    return res;
  }
  auto* component = GetComponentWithID(tasm.get(), component_id_);
  if (component == nullptr) {
    LOGE(
        "LepusComponent::QuerySelectorInner failed since can not find "
        "component.");
    return res;
  }
  auto task_handler = task_handler_.lock();
  tasm::NodeSelectOptions options(
      tasm::NodeSelectOptions::IdentifierType::CSS_SELECTOR, selector);
  options.first_only = single;
  options.only_current_component = false;
  if (tasm->GetPageConfig()->GetEnableFiberArch()) {
    const auto& targets =
        tasm::FiberElementSelector::Select(
            static_cast<tasm::ComponentElement*>(component), options)
            .nodes;
    res.resize(targets.size());
    auto unary_op = [tasm, task_handler](tasm::Element* base) {
      return LepusElement::Create(base->impl_id(), tasm, task_handler);
    };
    std::transform(targets.begin(), targets.end(), res.begin(), unary_op);
  } else {
    const auto& targets =
        tasm::RadonNodeSelector()
            .Select(static_cast<tasm::RadonComponent*>(component), options)
            .nodes;
    res.reserve(targets.size());
    // TODO(songshourui.null): When using the worklet's querySelector-related
    // API in list, the radon node's element may be null, leading to a crash due
    // to null pointer access. Therefore, when executing the
    // LepusComponent::QuerySelector function, if the radon node does not have a
    // corresponding element, the corresponding LepusElement will not be
    // generated. There is no unit test for querySelector now, unit test will be
    // added for this corner case in the future.
    for (const auto& node : targets) {
      if (node == nullptr || node->element() == nullptr) {
        continue;
      }
      res.emplace_back(
          LepusElement::Create(node->element()->impl_id(), tasm, task_handler));
    }
  }

  return res;
}

void LepusComponent::DoFrame(int64_t start_time, int64_t end_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::DoFrame");
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::DoFrame failed since tasm is null.");
    return;
  }

  // start_time in worklet must be mill secs;
  int64_t cur = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  // for fiber arch, exec lepus raf task and return.
  if (tasm->GetPageConfig()->GetEnableFiberArch()) {
    raf_handler_->DoFrame(cur, tasm);
    return;
  }

  // first, update data from last tick
  auto* component = GetComponentWithID(tasm.get(), component_id_);
  if (component != nullptr) {
    if (component->IsPageForBaseComponent()) {
      tasm::UpdatePageOption update_page_option;
      update_page_option.from_native = true;
      tasm::PipelineOptions pipeline_options;
      tasm->UpdateDataByPreParsedData(
          std::make_shared<tasm::TemplateData>(data_updated_, true),
          update_page_option, pipeline_options);
    } else {
      tasm::PipelineOptions pipeline_options;
      tasm->page_proxy()->UpdateComponentData(component->ComponentStrId(),
                                              data_updated_, pipeline_options);
    }
  }
  data_updated_ = lepus::Value();

  // second, exec lepus raf task
  raf_handler_->DoFrame(cur, tasm);
}

Napi::Object LepusComponent::GetStore() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::GetStore");
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::GetStore failed since tasm is null.");
    return Napi::Object::New(NapiEnv());
  }

  auto* component = GetComponentWithID(tasm.get(), component_id_);
  if (component == nullptr) {
    LOGE("LepusComponent::GetStore failed since can not find component.");
    return Napi::Object::New(NapiEnv());
  }

  return ValueConverter::ConvertLepusValueToNapiObject(
      NapiEnv(), component->inner_state());
}

void LepusComponent::SetStore(const Napi::Object& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::SetStore");
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::SetStore failed since tasm is null.");
    return;
  }

  auto* component = GetComponentWithID(tasm.get(), component_id_);
  if (component == nullptr) {
    LOGE("LepusComponent::SetStore failed since can not find component.");
    return;
  }

  component->set_inner_state(
      ValueConverter::ConvertNapiValueToLepusValue(value));
}

Napi::Object LepusComponent::GetData() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::GetData");
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::GetData failed since tasm is null.");
    return Napi::Object::New(NapiEnv());
  }

  auto* component = GetComponentWithID(tasm.get(), component_id_);
  if (component == nullptr) {
    LOGE("LepusComponent::GetData failed since can not find component.");
    return Napi::Object::New(NapiEnv());
  }

  return ValueConverter::ConvertLepusValueToNapiObject(NapiEnv(),
                                                       component->GetData());
}

void LepusComponent::SetData(const Napi::Object& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::SetData");
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::SetData failed since tasm is null.");
    return;
  }

  // request vsync, merge data to data_updated_ and update it in next tick.
  tasm->GetDelegate().RequestVsync(
      reinterpret_cast<uintptr_t>(this),
      fml::MakeCopyable([this, strong{Napi::Persistent(NapiObject())}](
                            int64_t frame_start, int64_t frame_end) {
        if (!strong.Value().IsUndefined()) {
          this->DoFrame(frame_start, frame_end);
        }
      }));

  if (data_updated_.IsEmpty()) {
    data_updated_ = ValueConverter::ConvertNapiValueToLepusValue(value);
  } else {
    lepus::Value::MergeValue(
        data_updated_, ValueConverter::ConvertNapiValueToLepusValue(value));
  }
}

Napi::Object LepusComponent::GetProperties() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LepusComponent::GetProperties");
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    LOGE("LepusComponent::GetProperties failed since tasm is null.");
    return Napi::Object::New(NapiEnv());
  }

  auto* component = GetComponentWithID(tasm.get(), component_id_);
  if (component == nullptr) {
    LOGE("LepusComponent::GetProperties failed since can not find component.");
    return Napi::Object::New(NapiEnv());
  }
  return ValueConverter::ConvertLepusValueToNapiObject(
      NapiEnv(), component->GetProperties());
}

}  // namespace worklet
}  // namespace lynx
