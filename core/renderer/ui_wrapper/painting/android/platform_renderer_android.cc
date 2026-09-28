// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"

#include <cstdint>
#include <utility>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/renderer/ui_wrapper/common/native_prop_bundle.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx::tasm {
namespace {

constexpr float kZeroMetrics[4] = {0.f, 0.f, 0.f, 0.f};

}  // namespace

PlatformRendererAndroid::~PlatformRendererAndroid() { CleanupAndroidView(); }

void PlatformRendererAndroid::UpdateNativeInteractionEnabled(
    std::optional<bool> enabled) {
  if (context_) {
    context_->UpdatePlatformRendererNativeInteractionEnabled(GetId(), enabled);
  }
}

PlatformRendererAndroid::PlatformRendererAndroid(
    PlatformRendererContext* context, int id, PlatformRendererType type,
    const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config)
    : PlatformRendererAndroid(context, id, type, base::String(), init_data,
                              init_config) {}

PlatformRendererAndroid::PlatformRendererAndroid(
    PlatformRendererContext* context, int id, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config)
    : PlatformRendererAndroid(context, id, PlatformRendererType::kUnknown,
                              tag_name, init_data, init_config) {}

PlatformRendererAndroid::PlatformRendererAndroid(
    PlatformRendererContext* context, int id, PlatformRendererType type,
    const base::String& tag_name, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config,
    PlatformRendererContext::PreparationScheduler::TaskRef preparation)
    : PlatformRendererImpl(id, type, tag_name),
      context_(context),
      preparation_(std::move(preparation)) {
  SetDirectChildOfCompatibleComponent(
      init_config.is_direct_child_of_compatible_component);
  SetFragmentParentId(init_config.fragment_parent_id);
  if (ShouldCreatePlatformExtendedRenderer(init_config)) {
    is_platform_extended_renderer_ = true;
  }
  if (!preparation_) {
    InitializeAndroidView(init_data, nullptr);
  }
  if (context_) {
    context_->RegisterPlatformRenderer(id, this);
  }
}

void PlatformRendererAndroid::EnsureAndroidViewCreated() {
  if (!preparation_) {
    return;
  }
  if (!context_ || context_->IsDestroyed()) {
    ClearPreparation();
    return;
  }
  auto scheduler = context_->GetPreparationScheduler();
  auto runnable = scheduler->Consume(preparation_);
  if (!runnable) {
    return;
  }
  ClearPreparation();
  if (!runnable->IsNull()) {
    InitializeAndroidView(nullptr, runnable->Get());
  }
}

void PlatformRendererAndroid::OnUpdateDisplayList(DisplayList display_list) {
  if (!context_ || context_->IsDestroyed()) {
    return;
  }
  if (display_list.GetContentItemsSize() > 0) {
    display_list_ = std::move(display_list);

    const auto* items = reinterpret_cast<const DisplayListItem*>(
        display_list_.GetContentItemsData());
    if (items != nullptr && items->type == DisplayListOpType::kBegin &&
        context_ != nullptr) {
      const float frame[4] = {items->payload.begin.x, items->payload.begin.y,
                              items->payload.begin.w, items->payload.begin.h};
      context_->UpdatePlatformRendererFrame(
          PlatformRendererImpl::GetId(), display_list_.RootNeedClipBounds(),
          frame, display_list_.GetRenderOffset(),
          HasLayoutMetrics() ? GetLayoutPaddings() : kZeroMetrics,
          HasLayoutMetrics() ? GetLayoutMargins() : kZeroMetrics,
          HasLayoutMetrics() ? GetLayoutBorders() : kZeroMetrics);
    }
  }
}

void PlatformRendererAndroid::OnAddChild(PlatformRenderer* child, int index,
                                         bool should_update_ui_owner) {
  if (context_ && child) {
    context_->InsertPlatformRenderer(PlatformRendererImpl::GetId(),
                                     child->GetId(), index,
                                     should_update_ui_owner);
  }
}

void PlatformRendererAndroid::OnRemoveFromParent(bool should_update_ui_owner) {
  if (context_) {
    const auto* parent = GetParent();
    context_->RemovePlatformRenderer(parent != nullptr ? parent->GetId() : -1,
                                     PlatformRendererImpl::GetId(),
                                     should_update_ui_owner);
  }
}

void PlatformRendererAndroid::InitializeAndroidView(
    const fml::RefPtr<PropBundle>& init_data, jobject preparation) {
  if (!context_ || context_->IsDestroyed()) {
    return;
  }
  android_view_created_ = true;
  if (IsPlatformExtendedRenderer()) {
    const base::String extended_renderer_tag_name =
        GetExtendedRendererTagName();
    NativePropBundle* native_bundle =
        static_cast<NativePropBundle*>(init_data.get());

    if (preparation || !native_bundle) {
      context_->CreatePlatformExtendedRenderer(
          GetId(), extended_renderer_tag_name, nullptr, preparation);
      return;
    }
    // Create PropBundleAndroid from NativePropBundle
    PropBundleAndroid prop_bundle_android(*native_bundle);

    // Update attributes via JNI
    // Get the Java object from PropBundleAndroid
    jobject j_prop_bundle = prop_bundle_android.jni_object();

    context_->CreatePlatformExtendedRenderer(
        GetId(), extended_renderer_tag_name, j_prop_bundle, preparation);

  } else {
    // This is a standard platform renderer with a known type
    context_->CreatePlatformRenderer(GetId(), type_);
  }
}

bool PlatformRendererAndroid::ShouldCreatePlatformExtendedRenderer(
    const PlatformRendererInitConfig& init_config) const {
  if (init_config.is_direct_child_of_compatible_component) {
    return true;
  }
  if (type_ == PlatformRendererType::kText ||
      type_ == PlatformRendererType::kImage ||
      type_ == PlatformRendererType::kView ||
      type_ == PlatformRendererType::kPage) {
    return false;
  }
  return type_ != PlatformRendererType::kUnknown || !tag_name_.empty();
}

base::String PlatformRendererAndroid::PreparationTag(
    PlatformRendererType type, const base::String& tag_name,
    const PlatformRendererInitConfig& init_config) {
  if (!init_config.is_direct_child_of_compatible_component &&
      (type == PlatformRendererType::kText ||
       type == PlatformRendererType::kImage ||
       type == PlatformRendererType::kView ||
       type == PlatformRendererType::kPage)) {
    return {};
  }
  return GetExtendedRendererTagName(type, tag_name);
}

void PlatformRendererAndroid::CleanupAndroidView() {
  ClearPreparation();
  if (context_) {
    if (android_view_created_) {
      context_->DestroyPlatformRenderer(GetId());
    } else {
      context_->UnregisterPlatformRenderer(GetId());
    }
  }
}

void PlatformRendererAndroid::ClearPreparation() {
  if (!preparation_) {
    return;
  }
  preparation_ = nullptr;
  if (context_) {
    context_->OnPreparationCleared();
  }
}

fml::RefPtr<PlatformRenderer> PlatformRendererAndroidFactory::CreateRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  return fml::MakeRefCounted<PlatformRendererAndroid>(context_, id, type,
                                                      init_data, init_config);
}

fml::RefPtr<PlatformRenderer>
PlatformRendererAndroidFactory::CreateExtendedRenderer(
    int id, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  return fml::MakeRefCounted<PlatformRendererAndroid>(context_, id, tag_name,
                                                      init_data, init_config);
}

void PlatformRendererAndroid::OnUpdateAttributes(
    const fml::RefPtr<PropBundle>& attributes) {
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

void PlatformRendererAndroid::OnUpdateSubtreeProperties(
    const DisplayList& subtree_properties) {
  if (!context_ || subtree_properties.GetSubtreePropertiesSize() <= 0) {
    return;
  }
  // Forward to PlatformRendererContext for JNI transmission
  context_->UpdatePlatformRendererSubtreeProperties(
      PlatformRendererImpl::GetId(),
      subtree_properties.GetSubtreePropertiesData(),
      subtree_properties.GetSubtreePropertiesSize());
}

}  // namespace lynx::tasm
