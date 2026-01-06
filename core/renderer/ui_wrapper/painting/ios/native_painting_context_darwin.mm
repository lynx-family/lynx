// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_darwin.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/layout/ios/text_layout_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin_factory.h"
#include "core/shell/dynamic_ui_operation_queue.h"

namespace lynx {
namespace tasm {

NativePaintingCtxDarwin::NativePaintingCtxDarwin() {
  platform_ref_ = std::make_shared<NativePaintingCtxPlatformRef>(
      std::make_unique<PlatformRendererDarwinFactory>());
  text_layout_impl_ = std::make_unique<TextLayoutDarwin>(nil, nil);
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
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::CreatePlatformExtendedRenderer(
    int id, const base::String &tag_name, const fml::RefPtr<PropBundle> &init_data) {
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::UpdateDisplayList(int id, DisplayList display_list) {
  // TODO: impl this function later.
}

void NativePaintingCtxDarwin::CreateImage(int id, base::String src, float width, float height) {
  // TODO: impl this function later.
}

}  // namespace tasm
}  // namespace lynx
