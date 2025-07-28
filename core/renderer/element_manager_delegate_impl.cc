// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/element_manager_delegate_impl.h"

#include <utility>

#include "core/renderer/dom/fiber/frame_element.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/template_assembler.h"

namespace lynx {
namespace tasm {

void ElementManagerDelegateImpl::LoadFrameBundle(const std::string &src,
                                                 FrameElement *element) {
  // TODO(zhoupeng.z): it should be done in an asynchronous thread to prevent
  // rendering phase timing from degrading
  auto bundle = frame_bundles_.find(src);
  if (bundle != frame_bundles_.end()) {
    element->DidBundleLoaded(src, bundle->second);
    return;
  }
  if (bundle_loader_) {
    frame_element_set_.emplace(element);
    bundle_loader_->LoadFrameBundle(src);
  }
}

void ElementManagerDelegateImpl::DidFrameBundleLoaded(
    const std::string &src, LynxTemplateBundle bundle) {
  auto bundle_ptr = std::make_shared<LynxTemplateBundle>(std::move(bundle));
  for (auto element = frame_element_set_.begin();
       element != frame_element_set_.end();) {
    if ((*element)->DidBundleLoaded(src, bundle_ptr)) {
      element = frame_element_set_.erase(element);
    } else {
      ++element;
    }
  }
  frame_bundles_.try_emplace(src, std::move(bundle_ptr));
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
