// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"

#include <utility>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/renderer/ui_wrapper/common/native_prop_bundle.h"

namespace lynx::tasm {

PlatformRendererAndroid::~PlatformRendererAndroid() { CleanupAndroidView(); }

PlatformRendererAndroid::PlatformRendererAndroid(
    PlatformRendererContext* context, int id, PlatformRendererType type,
    const fml::RefPtr<PropBundle>& init_data)
    : PlatformRendererAndroid(context, id, type, base::String(), init_data) {}

PlatformRendererAndroid::PlatformRendererAndroid(
    PlatformRendererContext* context, int id, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data)
    : PlatformRendererAndroid(context, id, PlatformRendererType::kUnknown,
                              tag_name, init_data) {}

PlatformRendererAndroid::PlatformRendererAndroid(
    PlatformRendererContext* context, int id, PlatformRendererType type,
    const base::String& tag_name, const fml::RefPtr<PropBundle>& init_data)
    : PlatformRendererImpl(id, type, tag_name), context_(context) {
  InitializeAndroidView(init_data);
  // Register this renderer with the context
  if (context_) {
    context_->RegisterPlatformRenderer(id, this);
  }
}

void PlatformRendererAndroid::OnUpdateDisplayList(DisplayList display_list) {
  display_list_ = std::move(display_list);
  constexpr int kFrameValueCount = 4;
  if (context_ && display_list_.GetContentFloatData() &&
      display_list_.GetContentFloatDataSize() >= kFrameValueCount) {
    float frame[4];
    // The first four float values in the display list are the frame of the
    // layer's OP_BEGIN.
    memcpy(frame, display_list_.GetContentFloatData(), 4 * sizeof(float));

    context_->UpdatePlatformRendererFrame(
        PlatformRendererImpl::GetId(), display_list_.RootNeedClipBounds(),
        frame, display_list_.GetRenderOffset());

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

void PlatformRendererAndroid::InitializeAndroidView(
    const fml::RefPtr<PropBundle>& init_data) {
  if (!context_) {
    return;
  }
  if (type_ == PlatformRendererType::kUnknown && !tag_name_.empty()) {
    NativePropBundle* native_bundle =
        static_cast<NativePropBundle*>(init_data.get());

    if (!native_bundle) {
      context_->CreatePlatformExtendedRenderer(GetId(), tag_name_, nullptr);
      return;
    }
    // Create PropBundleAndroid from NativePropBundle
    PropBundleAndroid prop_bundle_android(*native_bundle);

    // Update attributes via JNI
    // Get the Java object from PropBundleAndroid
    jobject j_prop_bundle = prop_bundle_android.jni_object();

    context_->CreatePlatformExtendedRenderer(GetId(), tag_name_, j_prop_bundle);

  } else {
    // This is a standard platform renderer with a known type
    context_->CreatePlatformRenderer(GetId(), type_);
  }
}

void PlatformRendererAndroid::CleanupAndroidView() {
  if (context_) {
    context_->DestroyPlatformRenderer(PlatformRendererImpl::GetId());
  }
}

fml::RefPtr<PlatformRenderer> PlatformRendererAndroidFactory::CreateRenderer(
    int id, PlatformRendererType type,
    const fml::RefPtr<PropBundle>& init_data) {
  return fml::MakeRefCounted<PlatformRendererAndroid>(context_, id, type,
                                                      init_data);
}

fml::RefPtr<PlatformRenderer>
PlatformRendererAndroidFactory::CreateExtendedRenderer(
    int id, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data) {
  return fml::MakeRefCounted<PlatformRendererAndroid>(context_, id, tag_name,
                                                      init_data);
}

void PlatformRendererAndroid::OnUpdateAttributes(
    const fml::RefPtr<PropBundle>& attributes, bool tends_to_flatten) {
  if (!context_ || !is_platform_extended_renderer_) {
    return;
  }

  // Convert NativePropBundle to PropBundleAndroid
  // The attributes should be a NativePropBundle from the pipeline
  NativePropBundle* native_bundle =
      static_cast<NativePropBundle*>(attributes.get());

  // Create PropBundleAndroid from NativePropBundle
  PropBundleAndroid prop_bundle_android(*native_bundle);

  // Update attributes via JNI
  // Get the Java object from PropBundleAndroid
  jobject j_prop_bundle = prop_bundle_android.jni_object();
  if (j_prop_bundle) {
    context_->UpdatePlatformRendererAttributes(GetId(), j_prop_bundle);
  }
}

}  // namespace lynx::tasm
