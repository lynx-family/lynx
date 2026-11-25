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

void ElementManagerDelegateImpl::OnLayoutAfter(PipelineLayoutData &data) {
  if (tasm_ == nullptr) {
    return;
  }
  tasm_->OnLayoutAfter(data);
}

}  // namespace tasm
}  // namespace lynx
