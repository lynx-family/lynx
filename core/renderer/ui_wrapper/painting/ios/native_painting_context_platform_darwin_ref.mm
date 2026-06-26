// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"

#include "base/trace/native/trace_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"

#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxUIOwner.h>

namespace lynx {
namespace tasm {

NativePaintingCtxPlatformDarwinRef::NativePaintingCtxPlatformDarwinRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : NativePaintingCtxPlatformRef(std::move(view_factory)) {}

void NativePaintingCtxPlatformDarwinRef::GetRootViewLocationOnScreen(float location[2]) {
  if (location == nullptr) {
    return;
  }
  location[0] = 0.f;
  location[1] = 0.f;

  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetRootViewLocationOnScreen();
  location[0] = res.x;
  location[1] = res.y;
}

void NativePaintingCtxPlatformDarwinRef::GetScreenSize(float size[2]) {
  if (size == nullptr) {
    return;
  }
  size[0] = 0.f;
  size[1] = 0.f;

  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetScreenSize();
  size[0] = res.width;
  size[1] = res.height;
}

LynxRendererContext* NativePaintingCtxPlatformDarwinRef::GetRendererContext() {
  return static_cast<PlatformRendererDarwinFactory*>(view_factory_.get())
      ->GetContext()
      ->GetRendererContext();
}

void NativePaintingCtxPlatformDarwinRef::UpdatePlatformRendererExtraBundle(
    int32_t sign, id platform_extra_bundle) {
  if (auto it = renderers_.find(sign); it != renderers_.end()) {
    auto* renderer = static_cast<PlatformRendererDarwin*>(it->second.get());
    renderer->UpdatePlatformExtraBundle(platform_extra_bundle);
  }
}

void NativePaintingCtxPlatformDarwinRef::InsertListItemPaintingNode(int32_t list_id,
                                                                    int32_t child_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_INSERT_LIST_PAINTING_TASK);
  auto list_it = renderers_.find(list_id);
  auto child_it = renderers_.find(child_id);
  if (list_it == renderers_.end() || child_it == renderers_.end()) {
    return;
  }

  auto* list_renderer = static_cast<PlatformRendererDarwin*>(list_it->second.get());
  auto* child_renderer = static_cast<PlatformRendererDarwin*>(child_it->second.get());
  UIView<LynxRendererHost>* list_view = list_renderer->GetUIView();
  UIView<LynxRendererHost>* child_view = child_renderer->GetUIView();
  if (list_view == nil || child_view == nil) {
    return;
  }
  if (child_view.superview == list_view) {
    return;
  }
  [child_view removeFromSuperview];
  [list_view addSubview:child_view];
  [[child_view renderer] reattachHostDecorationLayers];
}

void NativePaintingCtxPlatformDarwinRef::RemoveListItemPaintingNode(int32_t list_id,
                                                                    int32_t child_id) {
  (void)list_id;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_REMOVE_LIST_PAINTING_TASK);
  auto child_it = renderers_.find(child_id);
  if (child_it == renderers_.end()) {
    return;
  }

  auto* child_renderer = static_cast<PlatformRendererDarwin*>(child_it->second.get());
  UIView<LynxRendererHost>* child_view = child_renderer->GetUIView();
  if (child_view == nil) {
    return;
  }
  [[child_view renderer] detachHostDecorationLayers];
  [child_view removeFromSuperview];
}

void NativePaintingCtxPlatformDarwinRef::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  (void)is_init_scroll_offset;
  (void)from_layout;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OPERATION_QUEUE_UPDATE_OFFSET_FOR_LIST);
  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr || factory->GetContext() == nullptr ||
      factory->GetContext()->GetUIOwner() == nil) {
    return;
  }
  [factory->GetContext()->GetUIOwner() updateContentOffsetForListContainer:container_id
                                                               contentSize:content_size
                                                                    deltaX:delta_x
                                                                    deltaY:delta_y];
}

}  // namespace tasm
}  // namespace lynx
