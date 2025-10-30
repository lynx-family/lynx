// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"

#include <memory>

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx::tasm {

PlatformRendererAndroid::~PlatformRendererAndroid() { CleanupAndroidView(); }

void PlatformRendererAndroid::OnUpdateDisplayList(
    const DisplayList& display_list) {
  if (context_) {
    // TODO: Implement display list update via JNI
    // context_->UpdateDisplayList(display_list);
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
}

fml::RefPtr<PlatformRenderer> PlatformRendererAndroidFactory::CreateRenderer(
    int id, PlatformRendererType type) {
  return fml::MakeRefCounted<PlatformRendererAndroid>(context_, id, type);
}

}  // namespace lynx::tasm
