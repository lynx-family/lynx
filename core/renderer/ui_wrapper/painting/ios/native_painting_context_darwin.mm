// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_darwin.h"
#include "base/include/debug/lynx_error.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/layout/ios/text_layout_darwin.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"
#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin_factory.h"
#include "core/shell/dynamic_ui_operation_queue.h"

#import <Lynx/LUIBodyView.h>

namespace lynx {
namespace tasm {

NativePaintingCtxDarwin::NativePaintingCtxDarwin(UIView<LUIBodyView> *body_view, void *textra)
    : context_(std::make_unique<PlatformRendererContextDarwin>(body_view)) {
  platform_ref_ = std::make_shared<NativePaintingCtxPlatformDarwinRef>(
      std::make_unique<PlatformRendererDarwinFactory>(context_.get()));
  if (textra != 0) {
    text_layout_impl_ = std::make_unique<TextLayoutTextra>(reinterpret_cast<intptr_t>(textra));
  } else {
    text_layout_impl_ = std::make_unique<TextLayoutDarwin>(nil, nil);
  }
}

std::unique_ptr<pub::Value> NativePaintingCtxDarwin::GetTextInfo(const std::string &content,
                                                                 const pub::Value &info) {
  // TODO: impl this function later.
  return std::unique_ptr<pub::Value>();
}

std::vector<float> NativePaintingCtxDarwin::getBoundingClientOrigin(int id) {
  // TODO: impl this function later.
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxDarwin::getWindowSize(int id) {
  // TODO: impl this function later.
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxDarwin::GetRectToWindow(int id) {
  // TODO: impl this function later.
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxDarwin::GetRectToLynxView(int64_t id) {
  // TODO: impl this function later.
  return std::vector<float>();
}

std::vector<float> NativePaintingCtxDarwin::ScrollBy(int64_t id, float width, float height) {
  // TODO: impl this function later.
  return std::vector<float>();
}

int32_t NativePaintingCtxDarwin::GetTagInfo(const std::string &tag_name) {
  // TODO: impl this function later.
  return 0;
}

bool NativePaintingCtxDarwin::IsFlatten(base::MoveOnlyClosure<bool, bool> func) { return false; }

bool NativePaintingCtxDarwin::NeedAnimationProps() { return false; }

void NativePaintingCtxDarwin::Invoke(
    int64_t id, const std::string &method, const pub::Value &params,
    const std::function<void(int32_t, const pub::Value &)> &callback) {
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::StopExposure(const pub::Value &options) {
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::ResumeExposure() {
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::UpdatePlatformExtraBundle(int32_t id, PlatformExtraBundle *bundle) {
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::FinishTasmOperation(const std::shared_ptr<PipelineOptions> &options) {
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions> &options) {
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::Flush() { queue_->Flush(); }

void NativePaintingCtxDarwin::CreatePlatformRenderer(int id, PlatformRendererType type,
                                                     const fml::RefPtr<PropBundle> &init_data) {
  Enqueue([ref = platform_ref_, id, type, init_data]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)->CreatePlatformRenderer(
        id, type, init_data);
  });
}

void NativePaintingCtxDarwin::CreatePlatformExtendedRenderer(
    int id, const base::String &tag_name, const fml::RefPtr<PropBundle> &init_data) {
  Enqueue([ref = platform_ref_, id, tag_name, init_data]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)
        ->CreatePlatformExtendedRenderer(id, tag_name, init_data);
  });
}

void NativePaintingCtxDarwin::UpdateDisplayList(int id, DisplayList display_list) {
  Enqueue([ref = platform_ref_, id, dl = std::move(display_list)]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)->UpdateDisplayList(
        id, std::move(const_cast<DisplayList &>(dl)));
  });
}

void NativePaintingCtxDarwin::UpdateTextBundle(int id, intptr_t bundle) {
  // TBD
}

void NativePaintingCtxDarwin::DestroyTextBundle(int id) {
  // TBD
}

void NativePaintingCtxDarwin::ReconstructEventTargetTreeRecursively() {
  Enqueue([ref = platform_ref_]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)
        ->ReconstructEventTargetTreeRecursively();
  });
}

void NativePaintingCtxDarwin::UpdatePlatformEventBundle(int32_t id, PlatformEventBundle bundle) {
  Enqueue([ref = platform_ref_, id, bundle = std::move(bundle)]() {
    std::static_pointer_cast<NativePaintingCtxPlatformDarwinRef>(ref)->UpdatePlatformEventBundle(
        id, std::move(bundle));
  });
}

void NativePaintingCtxDarwin::CreateImage(int id, base::String src, float width, float height,
                                          int32_t event_mask) {
  // TODO: impl this function later.
}

template <typename F>
void NativePaintingCtxDarwin::Enqueue(F &&func) {
  queue_->EnqueueUIOperation([func = std::move(func)]() {
    @autoreleasepool {
      PaintingContextDarwinUtils::ExecuteSafely(func);
    }
  });
}

}  // namespace tasm
}  // namespace lynx
