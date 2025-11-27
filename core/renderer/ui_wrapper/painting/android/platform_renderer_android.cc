// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"

#include <utility>

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx::tasm {

PlatformRendererAndroid::~PlatformRendererAndroid() { CleanupAndroidView(); }

void PlatformRendererAndroid::OnUpdateDisplayList(DisplayList display_list) {
  display_list_ = std::move(display_list);
  constexpr int kFrameValueCount = 4;
  if (context_ && display_list_.GetContentFloatData() &&
      display_list_.GetContentFloatDataSize() >= kFrameValueCount) {
    float frame[4];
    // The first four float values in the display list are the frame of the
    // layer's OP_BEGIN.
    memcpy(frame, display_list_.GetContentFloatData(), 4 * sizeof(float));

    context_->UpdatePlatformRendererFrame(PlatformRendererImpl::GetId(), frame,
                                          display_list_.GetRenderOffset());

    // The drawing position on Android is affected by the frame layout and the
    // frame in OP_BEGIN togather. For a indepent layer, its position is already
    // shifted by the layers layout frame, and avoid doing it again in OP_BEGIN.
    float* offset = const_cast<float*>(display_list_.GetContentFloatData());
    memset(offset, 0, sizeof(float) * 2);
  }
}

void PlatformRendererAndroid::OnAddChild(PlatformRenderer* child) {
  if (context_ && child) {
    context_->InsertPlatformRenderer(PlatformRendererImpl::GetId(),
                                     child->GetId(), -1);
  }
}

void PlatformRendererAndroid::OnRemoveFromParent() {
  if (context_) {
    context_->RemovePlatformRenderer(PlatformRendererImpl::GetId());
  }
}

void PlatformRendererAndroid::InitializeAndroidView() {
  context_->CreatePlatformRenderer(GetId(), type_);
}

void PlatformRendererAndroid::CleanupAndroidView() {
  if (context_) {
    context_->DestroyPlatformRenderer(PlatformRendererImpl::GetId());
  }
}
PlatformRendererAndroid::PlatformRendererAndroid(
    PlatformRendererContext* context, int id, PlatformRendererType type)
    : PlatformRendererImpl(id), context_(context), type_(type) {
  InitializeAndroidView();
  // Register this renderer with the context
  if (context_) {
    context_->RegisterPlatformRenderer(id, this);
  }
}

fml::RefPtr<PlatformRenderer> PlatformRendererAndroidFactory::CreateRenderer(
    int id, PlatformRendererType type) {
  return fml::MakeRefCounted<PlatformRendererAndroid>(context_, id, type);
}

}  // namespace lynx::tasm
