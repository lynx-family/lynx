// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/element_manager_delegate_impl.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/dom/fiber/frame_element.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/template_assembler.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/template_bundle/lynx_template_bundle.h"

#if ENABLE_LEPUSNG_WORKLET
#include "core/renderer/pipeline/pipeline_scope.h"
#include "core/renderer/worklet/lepus_element.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#endif

namespace lynx {
namespace tasm {

void ElementManagerDelegateImpl::LoadFrameBundle(const std::string &src,
                                                 FrameElement *element) {
  // TODO(zhoupeng.z): it should be done in an asynchronous thread to prevent
  // rendering phase timing from degrading
  auto bundle = frame_bundles_.find(src);
  if (bundle != frame_bundles_.end()) {
    element->DidBundleLoaded(bundle->second);
    return;
  }
  if (bundle_loader_) {
    frame_element_set_.emplace(element);
    bundle_loader_->LoadFrameBundle(src);
  }
}

void ElementManagerDelegateImpl::DidFrameBundleLoaded(
    const LazyBundleLoader::CallBackInfo &callback_info) {
  auto bundle =
      callback_info.bundle
          ? std::make_shared<LynxTemplateBundle>(*callback_info.bundle)
          : nullptr;
  auto frame_element_data = std::make_shared<FrameElementData>(
      callback_info.component_url, std::move(bundle), callback_info.error_code,
      callback_info.error_msg);

  for (auto it = frame_element_set_.begin(); it != frame_element_set_.end();) {
    FrameElement *element = *it;
    if (element->GetSrc() == frame_element_data->src &&
        element->DidBundleLoaded(frame_element_data)) {
      it = frame_element_set_.erase(it);
    } else {
      ++it;
    }
  }

  if (callback_info.Success() && callback_info.bundle) {
    frame_bundles_.try_emplace(frame_element_data->src,
                               std::move(frame_element_data));
  }
}

void ElementManagerDelegateImpl::OnFrameRemoved(FrameElement *element) {
  frame_element_set_.erase(element);
}

PipelineContext *ElementManagerDelegateImpl::GetCurrentPipelineContext() {
  if (tasm_ == nullptr) {
    return nullptr;
  }
  return tasm_->GetCurrentPipelineContext();
}

PipelineContext *
ElementManagerDelegateImpl::CreateAndUpdateCurrentPipelineContext(
    const std::shared_ptr<PipelineOptions> &pipeline_options,
    bool is_major_updated) {
  if (tasm_ == nullptr) {
    return nullptr;
  }
  return tasm_->CreateAndUpdateCurrentPipelineContext(pipeline_options,
                                                      is_major_updated);
}

void ElementManagerDelegateImpl::SendGlobalEvent(const std::string &event,
                                                 const lepus::Value &info) {
  if (tasm_ == nullptr) {
    return;
  }
  tasm_->SendGlobalEvent(event, info);
}

void ElementManagerDelegateImpl::TriggerLepusGlobalEvent(
    const std::string &event, const lepus::Value &info) {
  if (tasm_ == nullptr) {
    return;
  }
  tasm_->TriggerLepusGlobalEvent(event, info);
}

event::DispatchEventResult ElementManagerDelegateImpl::DispatchMessageEvent(
    fml::RefPtr<runtime::MessageEvent> event) {
  if (tasm_ == nullptr) {
    return {event::EventCancelType::kNotCanceled, false};
  }
  return tasm_->DispatchMessageEvent(std::move(event));
}

bool ElementManagerDelegateImpl::EnableEventHandleRefactor() const {
  return tasm_ != nullptr && tasm_->EnableEventHandleRefactor();
}

bool ElementManagerDelegateImpl::SupportComponentJS() const {
  return tasm_ != nullptr && tasm_->SupportComponentJS();
}

runtime::MTSRuntime *ElementManagerDelegateImpl::GetDefaultEntryRuntime()
    const {
  return GetEntryRuntime(DEFAULT_ENTRY_NAME);
}

runtime::MTSRuntime *ElementManagerDelegateImpl::GetEntryRuntime(
    const std::string &entry_name) const {
  if (tasm_ == nullptr) {
    return nullptr;
  }
  const auto &entry = tasm_->FindTemplateEntry(entry_name);
  return entry == nullptr ? nullptr : entry->GetVm().get();
}

std::string ElementManagerDelegateImpl::GetDefaultEntryLogicalName() const {
  if (tasm_ == nullptr) {
    return std::string();
  }
  // The card entry is stored by DEFAULT_ENTRY_NAME, but its logical name may be
  // the app name. JS event messages historically use the logical entry name.
  const auto &entry = tasm_->FindEntry(DEFAULT_ENTRY_NAME);
  return entry == nullptr ? std::string() : entry->GetName();
}

EventResult ElementManagerDelegateImpl::FireElementWorkletAndRequestResolve(
    const std::string &component_id, const std::string &entry_name,
    const lepus::Value &callback, const lepus::Value &event_detail,
    const std::shared_ptr<worklet::LepusApiHandler> &task_handler,
    int32_t element_id, std::shared_ptr<PipelineOptions> &pipeline_options) {
#if ENABLE_LEPUSNG_WORKLET
  if (tasm_ == nullptr) {
    return EventResult::kDefault;
  }
  PipelineScope pipeline_scope(tasm_, pipeline_options);
  EventResult result = worklet::LepusElement::FireElementWorklet(
      component_id, entry_name, tasm_, callback, lepus::Value(), event_detail,
      task_handler, element_id, EventType::kTouch);
  tasm_->page_proxy()->element_manager()->SetNeedsLayout();
  tasm_->page_proxy()->element_manager()->RequestResolve(pipeline_options);
  return result;
#else
  return EventResult::kDefault;
#endif
}

void ElementManagerDelegateImpl::OnLayoutAfter(PipelineLayoutData &data) {
  if (tasm_ == nullptr) {
    return;
  }
  tasm_->OnLayoutAfter(data);
}

}  // namespace tasm
}  // namespace lynx
