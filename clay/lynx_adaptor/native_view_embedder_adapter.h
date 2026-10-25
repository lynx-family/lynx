// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_VIEW_EMBEDDER_ADAPTER_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_VIEW_EMBEDDER_ADAPTER_H_

#include <memory>
#include <string>
#include <utility>

#include "clay/lynx_adaptor/native_platform_view.h"
#include "platform/embedder/interfaces/native_view.h"

namespace clay {
class NativeViewEmbedderAdapter final
    : public NativePlatformView,
      public lynx::embedder::NativeViewRenderer {
 public:
  static NativePlatformView* Create(lynx_native_view_t* handle) {
    if (!handle) return nullptr;
    auto* view = reinterpret_cast<lynx::embedder::NativeView*>(handle);
    auto peer = std::make_shared<NativeViewEmbedderAdapter>(view);
    auto* result = peer.get();
    view->SetRenderer(std::move(peer));
    return result;
  }
  explicit NativeViewEmbedderAdapter(lynx::embedder::NativeView* view)
      : view_(view) {}
  bool OnCreate() override { return view_->OnCreate(); }
  void OnAttach() override { view_->OnAttach(); }
  void OnDetach() override { view_->OnDetach(); }
  void OnDestroy() override {
    view_->OnDestroy();
    destroyed_ = true;
  }
  void OnLayoutChanged(float left, float top, float width, float height,
                       float ratio) override {
    view_->OnLayoutChanged(left, top, width, height, ratio);
  }
  void OnPropertiesChanged(const lynx::pub::Value& attrs,
                           const lynx::pub::Value& events) override {
    view_->OnPropertiesChanged(attrs, events);
  }
  void OnMethodInvoked(
      const std::string& method, const lynx::pub::Value& attrs,
      lynx::embedder::NativeView::MethodCallback callback) override {
    view_->OnMethodInvoked(method, attrs, std::move(callback));
  }
  bool HandleMotionEvent(native_view_motion_event_t* event) override {
    return view_->HandleMotionEvent(event);
  }
  void OnFocusChanged(bool focused, bool leaf) override {
    view_->OnFocusChanged(focused, leaf);
  }
  bool SupportScrolling() const override { return view_->SupportScrolling(); }
  bool NeedSharedImageSink() const override {
    return view_->NeedSharedImageSink();
  }
  ClaySharedImageSinkBufferMode buffer_mode() const override {
    return static_cast<ClaySharedImageSinkBufferMode>(view_->BufferMode());
  }
  lynx_surface_handle_t* AcquireSurface(int width, int height) override {
    return !destroyed_ && GetSharedImageSink()
               ? reinterpret_cast<lynx_surface_handle_t*>(
                     NativePlatformView::AcquireSurface(width, height))
               : nullptr;
  }
  lynx_surface_handle_t* AcquireSurface(int width, int height,
                                        const float* transform) override {
    static_assert(sizeof(ClayTransformation) == sizeof(float) * 9);
    return !destroyed_ && GetSharedImageSink()
               ? reinterpret_cast<lynx_surface_handle_t*>(
                     NativePlatformView::AcquireSurface(
                         width, height,
                         reinterpret_cast<const ClayTransformation*>(
                             transform)))
               : nullptr;
  }
  bool PresentSurface(int width, int height, const float* transform,
                      lynx_surface_handle_t* handle) override {
    return !destroyed_ && GetSharedImageSink() &&
           NativePlatformView::PresentSurface(
               width, height,
               reinterpret_cast<const ClayTransformation*>(transform), handle);
  }
  bool SwapBack() override {
    return !destroyed_ && NativePlatformView::SwapBack();
  }
  void TriggerEvent(const char* name, const lynx::pub::Value& params) override {
    if (!destroyed_) NativePlatformView::TriggerEvent(name, params);
  }
  void RequestFocus() override {
    if (!destroyed_) NativePlatformView::RequestFocus();
  }
  void MarkDirty() override {
    if (!destroyed_) NativePlatformView::MarkDirty();
  }

 private:
  void Release() override { view_->ReleaseRef(); }
  lynx::embedder::NativeView* view_;
  bool destroyed_ = false;
};
}  // namespace clay

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_VIEW_EMBEDDER_ADAPTER_H_
