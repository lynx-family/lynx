// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"

namespace lynx {
namespace tasm {

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               PlatformRendererType type)
    : PlatformRendererDarwin(context, id, type, base::String()) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               const base::String& tag_name)
    : PlatformRendererImpl(id, PlatformRendererType::kUnknown, tag_name) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               PlatformRendererType type,
                                               const base::String& tag_name)
    : PlatformRendererImpl(id, type, tag_name), context_(context) {
  InitializeUIView();
}

void PlatformRendererDarwin::OnUpdateDisplayList(DisplayList display_list) {
  display_list_ = std::move(display_list);
  // TODO: impl this function later.

  if (_view != nil) {
    [_view setNeedsDisplay];
  }
}

void PlatformRendererDarwin::OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                                                bool tends_to_flatten) {
  // TODO: impl this function later.
}

void PlatformRendererDarwin::OnAddChild(PlatformRenderer* child) {
  if (_view == nil) {
    return;
  }

  [_view addSubview:static_cast<PlatformRendererDarwin*>(child)->GetUIView()];
}

void PlatformRendererDarwin::OnRemoveFromParent() {
  if (_view == nil) {
    return;
  }

  [_view removeFromSuperview];
}

void PlatformRendererDarwin::OnUpdateSubtreeProperties(const DisplayList& subtree_properties) {
  // TODO: impl this function later.
}

void PlatformRendererDarwin::InitializeUIView() {
  if (IsPlatformExtendedRenderer()) {
    // TODO: impl this function later.
    return;
  }

  switch (GetPlatformRendererType()) {
    // TODO(songshourui.null): Consruct specific UIView for each type later.
    case PlatformRendererType::kView:
    case PlatformRendererType::kText:
    case PlatformRendererType::kImage:
    case PlatformRendererType::kScroll:
    case PlatformRendererType::kList:
    case PlatformRendererType::kListItem:
      _view = [[UIView alloc] init];
      break;
    case PlatformRendererType::kPage:
      _view = context_ != nullptr ? context_->GetContainerView() : nil;
      break;
    default:
      break;
  }
}

}  // namespace tasm
}  // namespace lynx
