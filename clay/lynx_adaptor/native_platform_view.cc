// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_platform_view.h"

#include <memory>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "platform/embedder/public/lynx_value.h"

namespace clay {

namespace {

SharedImageSink::BufferMode convert_buffer_mode(
    ClaySharedImageSinkBufferMode clay_buffer_mode) {
  switch (clay_buffer_mode) {
    case kClaySharedImageSinkBufferModeSingleBuffer:
      return SharedImageSink::kSingleBuffer;
    case kClaySharedImageSinkBufferModeDoubleBuffer:
      return SharedImageSink::kDoubleBuffer;
    case kClaySharedImageSinkBufferModeTripleBuffer:
    default:
      return SharedImageSink::kTripleBuffer;
  }
}

}  // namespace

NativePlatformView::~NativePlatformView() {
  if (sink_ref_) {
    ClayReleaseSharedImageSink(sink_ref_);
  }
}

bool NativePlatformView::OnCreate() { return false; }
void NativePlatformView::OnAttach() {}
void NativePlatformView::OnDetach() {}
void NativePlatformView::OnDestroy() {}

void NativePlatformView::OnLayoutChanged(float left, float top, float width,
                                         float height, float pixel_ratio) {}

void NativePlatformView::OnPropertiesChanged(lynx_value attrs,
                                             lynx_value events) {}

void NativePlatformView::OnMouseClickEvent(int x, int y, int buttons,
                                           bool mouse_up) {}
void NativePlatformView::OnMouseMoveEvent(int x, int y, int modifiers,
                                          bool mouse_leave) {}
void NativePlatformView::OnMouseWheelEvent(int x, int y, int modifiers,
                                           double delta_x, double delta_y) {}
void NativePlatformView::OnFocusChanged(bool focused, bool is_leaf) {}
void NativePlatformView::OnMethodInvoked(
    const std::string& method, lynx_value attrs,
    std::function<void(int code, lynx_value data)> callback) {
  callback(kMethodNotFound, {.val_ptr = 0, .type = lynx_value_undefined});
}

void NativePlatformView::TriggerEvent(const char* name,
                                      lynx::pub::LynxValue&& params) {
  lynx_value_add_reference(nullptr, params.Value(), nullptr);
  TriggerEvent(name, params.Value());
}

void NativePlatformView::TriggerEvent(const char* name, lynx_value params) {
  lynx_api_env env = nullptr;
  auto m =
      params.type == lynx_value_map
          ? std::move(
                lynx::ValueConverter::CreateClayValue(env, params).GetMap())
          : clay::Value::Map();
  lynx_value_remove_reference(env, params, nullptr);
  native_view_->page_view()->SendCustomEvent(native_view_->id(), name,
                                             std::move(m));
}

ClaySharedImageSinkRef NativePlatformView::GetSharedImageSink() {
  if (!NeedSharedImageSink()) {
    return nullptr;
  }
  if (!sink_ref_) {
    sink_ref_ = ClayCreateSharedImageSink(buffer_mode(), backing_type(),
                                          pixel_format());
  }
  return sink_ref_;
}

bool NativePlatformView::PresentSurface(
    int width, int height, const ClayTransformation* transform,
    ClaySharedImageNativeHandle gfx_handle) {
  if (!sink_ref_) {
    return false;
  }
  FML_DCHECK(gfx_handle);
  SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref_);
  auto [backing, buffer_age, status] =
      sink->TryAcquireBack({width, height}, gfx_handle);
  if (!backing) {
    return false;
  }

  if (transform) {
    skity::Matrix mat =
        skity::Matrix(transform->scaleX, transform->skewX, transform->transX,
                      transform->skewY, transform->scaleY, transform->transY,
                      transform->pers0, transform->pers1, transform->pers2);
    backing->SetTransformation(mat);
  }

  auto fence_sync = backing->GetFenceSync();
  if (fence_sync) {
    fence_sync->ClientWait();
  }
  if (!sink->SwapBack(nullptr)) {
    FML_LOG(ERROR) << "Failed to swap back shared image sink";
    return false;
  }
  return true;
}

ClaySharedImageNativeHandle NativePlatformView::AcquireSurface(int width,
                                                               int height) {
  if (!sink_ref_) {
    return nullptr;
  }

  SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref_);
  SharedImageSink::BufferMode mode = convert_buffer_mode(buffer_mode());
  if (sink->Capacity() != mode) {
    sink->UpdateBufferMode(mode);
  }

  auto [backing, buffer_age] = sink->AcquireBack({width, height});
  if (!backing) {
    return nullptr;
  }
  auto fence_sync = backing->GetFenceSync();
  if (fence_sync) {
    fence_sync->ClientWait();
  }
  return reinterpret_cast<ClaySharedImageNativeHandle>(backing->GetGFXHandle());
}

bool NativePlatformView::SwapBack() {
  if (sink_ref_) {
    SharedImageSink* sink = reinterpret_cast<SharedImageSink*>(sink_ref_);
    return sink->SwapBack(nullptr);
  }
  return false;
}

void NativePlatformView::MarkDirty() { native_view_->MarkDirty(); }
void NativePlatformView::RequestFocus() { native_view_->RequestFocus(); }

}  // namespace clay
