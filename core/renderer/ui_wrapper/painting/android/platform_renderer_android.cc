// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/renderer/ui_wrapper/common/native_prop_bundle.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx::tasm {

namespace {

constexpr int32_t kDisplayListItemBufferHeaderSize = sizeof(int32_t);
constexpr int32_t kSerializedDisplayListItemSize = 56;
constexpr int32_t kBeginIdOffset = 4;
constexpr int32_t kBeginTypeOffset = 8;
constexpr int32_t kBeginXOffset = 12;
constexpr int32_t kBeginYOffset = 16;
constexpr int32_t kBeginWOffset = 20;
constexpr int32_t kBeginHOffset = 24;
constexpr int32_t kFillColorOffset = 4;
constexpr int32_t kFillClipIndexOffset = 8;
constexpr int32_t kDrawViewIdOffset = 4;
constexpr int32_t kTextIdOffset = 4;
constexpr int32_t kTextBoxIndexOffset = 8;
constexpr int32_t kImageIdOffset = 4;
constexpr int32_t kImageBoxIndexOffset = 8;
constexpr int32_t kBorderOutIndexOffset = 4;
constexpr int32_t kBorderInnerIndexOffset = 8;
constexpr int32_t kBorderColorsOffset = 12;
constexpr int32_t kBorderStylesOffset = 28;
constexpr int32_t kRecordBoxXOffset = 4;
constexpr int32_t kRecordBoxYOffset = 8;
constexpr int32_t kRecordBoxWOffset = 12;
constexpr int32_t kRecordBoxHOffset = 16;
constexpr int32_t kRecordBoxRadiiOffset = 20;
constexpr int32_t kRecordBoxHasRadiiOffset = 52;
constexpr int32_t kClipRectXOffset = 4;
constexpr int32_t kClipRectYOffset = 8;
constexpr int32_t kClipRectWOffset = 12;
constexpr int32_t kClipRectHOffset = 16;
constexpr int32_t kClipRectRadiiOffset = 20;
constexpr int32_t kClipRectHasRadiiOffset = 52;
constexpr int32_t kGradientColorCountOffsetOffset = 4;
constexpr int32_t kGradientColorCountOffset = 8;
constexpr int32_t kGradientStopCountOffsetOffset = 12;
constexpr int32_t kGradientStopCountOffset = 16;
constexpr int32_t kGradientTilingIndexOffset = 20;
constexpr int32_t kGradientClipIndexOffset = 24;
constexpr int32_t kGradientRepeatXOffset = 28;
constexpr int32_t kGradientRepeatYOffset = 32;
constexpr int32_t kGradientAngleOffset = 36;
constexpr int32_t kBoxShadowShadowBoxIndexOffset = 4;
constexpr int32_t kBoxShadowClipBoxIndexOffset = 8;
constexpr int32_t kBoxShadowColorOffset = 12;
constexpr int32_t kBoxShadowBlurRadiusOffset = 16;
constexpr int32_t kBoxShadowClipModeOffset = 20;

template <typename T>
void WriteField(base::Vector<uint8_t>& buffer, size_t offset, T value) {
  static_assert(sizeof(T) == sizeof(int32_t),
                "DisplayList serialized fields are 32-bit values");
  std::memcpy(buffer.data() + offset, &value, sizeof(T));
}

bool IsDirectChildOfCompatibleComponentFromInitData(
    const fml::RefPtr<PropBundle>& init_data) {
  return init_data != nullptr &&
         init_data->Contains(kDirectChildOfCompatibleComponentInitDataKey);
}

void SerializeContentItem(const DisplayListItem& item,
                          base::Vector<uint8_t>& buffer) {
  if (buffer.empty()) {
    buffer.resize<true>(kDisplayListItemBufferHeaderSize);
    WriteField(buffer, 0, kSerializedDisplayListItemSize);
  }

  const size_t item_offset = buffer.size();
  buffer.resize<true>(item_offset + kSerializedDisplayListItemSize);
  WriteField(buffer, item_offset, static_cast<int32_t>(item.type));

  switch (item.type) {
    case DisplayListOpType::kBegin:
      WriteField(buffer, item_offset + kBeginIdOffset, item.payload.begin.id);
      WriteField(buffer, item_offset + kBeginTypeOffset,
                 item.payload.begin.type);
      WriteField(buffer, item_offset + kBeginXOffset, item.payload.begin.x);
      WriteField(buffer, item_offset + kBeginYOffset, item.payload.begin.y);
      WriteField(buffer, item_offset + kBeginWOffset, item.payload.begin.w);
      WriteField(buffer, item_offset + kBeginHOffset, item.payload.begin.h);
      break;
    case DisplayListOpType::kFill:
      WriteField(buffer, item_offset + kFillColorOffset,
                 item.payload.fill.color);
      WriteField(buffer, item_offset + kFillClipIndexOffset,
                 item.payload.fill.clip_index);
      break;
    case DisplayListOpType::kDrawView:
      WriteField(buffer, item_offset + kDrawViewIdOffset,
                 item.payload.draw_view.view_id);
      break;
    case DisplayListOpType::kText:
      WriteField(buffer, item_offset + kTextIdOffset,
                 item.payload.text.text_id);
      WriteField(buffer, item_offset + kTextBoxIndexOffset,
                 item.payload.text.box_index);
      break;
    case DisplayListOpType::kImage:
      WriteField(buffer, item_offset + kImageIdOffset,
                 item.payload.image.image_id);
      WriteField(buffer, item_offset + kImageBoxIndexOffset,
                 item.payload.image.box_index);
      break;
    case DisplayListOpType::kBorder:
      WriteField(buffer, item_offset + kBorderOutIndexOffset,
                 item.payload.border.out_index);
      WriteField(buffer, item_offset + kBorderInnerIndexOffset,
                 item.payload.border.inner_index);
      for (int i = 0; i < 4; ++i) {
        WriteField(buffer, item_offset + kBorderColorsOffset + i * 4,
                   item.payload.border.colors[i]);
        WriteField(buffer, item_offset + kBorderStylesOffset + i * 4,
                   item.payload.border.styles[i]);
      }
      break;
    case DisplayListOpType::kClipRect:
      WriteField(buffer, item_offset + kClipRectXOffset,
                 item.payload.clip_rect.x);
      WriteField(buffer, item_offset + kClipRectYOffset,
                 item.payload.clip_rect.y);
      WriteField(buffer, item_offset + kClipRectWOffset,
                 item.payload.clip_rect.w);
      WriteField(buffer, item_offset + kClipRectHOffset,
                 item.payload.clip_rect.h);
      for (int i = 0; i < 8; ++i) {
        WriteField(buffer, item_offset + kClipRectRadiiOffset + i * 4,
                   item.payload.clip_rect.radii[i]);
      }
      WriteField(buffer, item_offset + kClipRectHasRadiiOffset,
                 item.payload.clip_rect.has_radii);
      break;
    case DisplayListOpType::kRecordBox:
      WriteField(buffer, item_offset + kRecordBoxXOffset,
                 item.payload.record_box.x);
      WriteField(buffer, item_offset + kRecordBoxYOffset,
                 item.payload.record_box.y);
      WriteField(buffer, item_offset + kRecordBoxWOffset,
                 item.payload.record_box.w);
      WriteField(buffer, item_offset + kRecordBoxHOffset,
                 item.payload.record_box.h);
      for (int i = 0; i < 8; ++i) {
        WriteField(buffer, item_offset + kRecordBoxRadiiOffset + i * 4,
                   item.payload.record_box.radii[i]);
      }
      WriteField(buffer, item_offset + kRecordBoxHasRadiiOffset,
                 item.payload.record_box.has_radii);
      break;
    case DisplayListOpType::kLinearGradient:
      WriteField(buffer, item_offset + kGradientColorCountOffsetOffset,
                 item.payload.linear_gradient.color_count_offset);
      WriteField(buffer, item_offset + kGradientColorCountOffset,
                 item.payload.linear_gradient.color_count);
      WriteField(buffer, item_offset + kGradientStopCountOffsetOffset,
                 item.payload.linear_gradient.stop_count_offset);
      WriteField(buffer, item_offset + kGradientStopCountOffset,
                 item.payload.linear_gradient.stop_count);
      WriteField(buffer, item_offset + kGradientTilingIndexOffset,
                 item.payload.linear_gradient.tiling_index);
      WriteField(buffer, item_offset + kGradientClipIndexOffset,
                 item.payload.linear_gradient.clip_index);
      WriteField(buffer, item_offset + kGradientRepeatXOffset,
                 item.payload.linear_gradient.repeat_x);
      WriteField(buffer, item_offset + kGradientRepeatYOffset,
                 item.payload.linear_gradient.repeat_y);
      WriteField(buffer, item_offset + kGradientAngleOffset,
                 item.payload.linear_gradient.angle);
      break;
    case DisplayListOpType::kBoxShadow:
      WriteField(buffer, item_offset + kBoxShadowShadowBoxIndexOffset,
                 item.payload.box_shadow.shadow_box_index);
      WriteField(buffer, item_offset + kBoxShadowClipBoxIndexOffset,
                 item.payload.box_shadow.clip_box_index);
      WriteField(buffer, item_offset + kBoxShadowColorOffset,
                 item.payload.box_shadow.color);
      WriteField(buffer, item_offset + kBoxShadowBlurRadiusOffset,
                 item.payload.box_shadow.blur_radius);
      WriteField(buffer, item_offset + kBoxShadowClipModeOffset,
                 item.payload.box_shadow.clip_mode);
      break;
    case DisplayListOpType::kEnd:
    case DisplayListOpType::kCustom:
      break;
  }
}

}  // namespace

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
  if (ShouldCreatePlatformExtendedRenderer(init_data)) {
    is_platform_extended_renderer_ = true;
  }
  InitializeAndroidView(init_data);
  // Register this renderer with the context
  if (context_) {
    context_->RegisterPlatformRenderer(id, this);
  }
}

void PlatformRendererAndroid::OnUpdateDisplayList(DisplayList display_list) {
  if (display_list.GetContentItemsSize() > 0) {
    display_list_ = std::move(display_list);
    serialized_content_items_.clear();
    serialized_content_items_dirty_ = true;

    const auto* items = reinterpret_cast<const DisplayListItem*>(
        display_list_.GetContentItemsData());
    if (items != nullptr && items->type == DisplayListOpType::kBegin &&
        context_ != nullptr) {
      const float frame[4] = {items->payload.begin.x, items->payload.begin.y,
                              items->payload.begin.w, items->payload.begin.h};
      context_->UpdatePlatformRendererFrame(
          PlatformRendererImpl::GetId(), display_list_.RootNeedClipBounds(),
          frame, display_list_.GetRenderOffset());
    }
  }
}

const uint8_t* PlatformRendererAndroid::GetSerializedContentItemsData() {
  SerializeContentItemsIfNeeded();
  return serialized_content_items_.empty() ? nullptr
                                           : serialized_content_items_.data();
}

size_t PlatformRendererAndroid::GetSerializedContentItemsSize() {
  SerializeContentItemsIfNeeded();
  return serialized_content_items_.size();
}

void PlatformRendererAndroid::SerializeContentItemsIfNeeded() {
  if (!serialized_content_items_dirty_) {
    return;
  }
  serialized_content_items_.clear();

  const DisplayList& display_list = GetDisplayList();
  const size_t content_items_size = display_list.GetContentItemsSize();
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  if (items != nullptr && content_items_size > 0) {
    serialized_content_items_.reserve(kDisplayListItemBufferHeaderSize +
                                      content_items_size *
                                          kSerializedDisplayListItemSize);
    for (size_t i = 0; i < content_items_size; ++i) {
      SerializeContentItem(items[i], serialized_content_items_);
    }
  }
  serialized_content_items_dirty_ = false;
}

void PlatformRendererAndroid::OnAddChild(PlatformRenderer* child, int index) {
  if (context_ && child) {
    context_->InsertPlatformRenderer(PlatformRendererImpl::GetId(),
                                     child->GetId(), index);
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
  if (IsPlatformExtendedRenderer()) {
    const base::String extended_renderer_tag_name =
        GetExtendedRendererTagName();
    NativePropBundle* native_bundle =
        static_cast<NativePropBundle*>(init_data.get());

    if (!native_bundle) {
      context_->CreatePlatformExtendedRenderer(
          GetId(), extended_renderer_tag_name, nullptr);
      return;
    }
    // Create PropBundleAndroid from NativePropBundle
    PropBundleAndroid prop_bundle_android(*native_bundle);

    // Update attributes via JNI
    // Get the Java object from PropBundleAndroid
    jobject j_prop_bundle = prop_bundle_android.jni_object();

    context_->CreatePlatformExtendedRenderer(
        GetId(), extended_renderer_tag_name, j_prop_bundle);

  } else {
    // This is a standard platform renderer with a known type
    context_->CreatePlatformRenderer(GetId(), type_);
  }
}

bool PlatformRendererAndroid::ShouldCreatePlatformExtendedRenderer(
    const fml::RefPtr<PropBundle>& init_data) const {
  if (IsDirectChildOfCompatibleComponentFromInitData(init_data)) {
    return true;
  }
  if (type_ == PlatformRendererType::kText ||
      type_ == PlatformRendererType::kImage ||
      type_ == PlatformRendererType::kView ||
      type_ == PlatformRendererType::kPage) {
    return false;
  }
  if (type_ != PlatformRendererType::kUnknown) {
    return true;
  }
  return !tag_name_.empty();
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
