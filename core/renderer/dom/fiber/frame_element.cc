// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/frame_element.h"

#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {

namespace {
constexpr char kDefaultFrameTag[] = "frame";
}

FrameElement::FrameElement(ElementManager* element_manager)
    : FiberElement(element_manager, BASE_STATIC_STRING(kDefaultFrameTag)) {}

void FrameElement::OnNodeAdded(FiberElement* child) {
  LOGE("frame element cannot adopt any child");
}

FrameElement::~FrameElement() {
  if (ShouldDestroy()) {
    element_manager()->element_manager_delegate()->OnFrameRemoved(this);
  }
}

void FrameElement::SetAttribute(const base::String& key,
                                const lepus::Value& value,
                                bool need_update_data_model) {
  OnSetSrc(key, value);
  FiberElement::SetAttribute(key, value, need_update_data_model);
}

void FrameElement::OnSetSrc(const base::String& key,
                            const lepus::Value& value) {
  BASE_STATIC_STRING_DECL(kSrc, "src");
  if (key == kSrc && value.IsString()) {
    std::string src = value.String().str();
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FRAME_ELEMENT_ON_SET_SRC, "src", src);
    if (src != src_) {
      src_ = std::move(src);
      template_bundle_ = nullptr;
      element_manager()->element_manager_delegate()->LoadFrameBundle(src_,
                                                                     this);
    }
  }
}

bool FrameElement::DidBundleLoaded(
    const std::string& src, const std::shared_ptr<LynxTemplateBundle>& bundle) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FRAME_ELEMENT_DID_BUNDLED_LOADED, "src",
              src);
  if (src_ != src) {
    LOGE("bundle loaded with wrong src:" << src << " expect:" << src_);
    return false;
  }
  PostBundle(bundle);
  // TODO(yangguangzhao.solace): remove this when unified pipeline is ready
  painting_context()->Flush();
  return true;
}

void FrameElement::PostBundle(
    const std::shared_ptr<LynxTemplateBundle>& bundle) {
  if (HasPaintingNode()) {
    painting_context()->SetFrameAppBundle(id_, bundle);
  } else {
    template_bundle_ = bundle;
  }
}

void FrameElement::FlushProps() {
  FiberElement::FlushProps();
  if (template_bundle_ && HasPaintingNode()) {
    painting_context()->SetFrameAppBundle(id_, template_bundle_);
    template_bundle_ = nullptr;
  }
}

}  // namespace tasm
}  // namespace lynx
