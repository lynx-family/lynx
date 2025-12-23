// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"

namespace lynx {
namespace tasm {

PlatformRendererDarwin::PlatformRendererDarwin(int id, PlatformRendererType type)
    : PlatformRendererDarwin(id, type, base::String()) {}

PlatformRendererDarwin::PlatformRendererDarwin(int id, const base::String& tag_name)
    : PlatformRendererImpl(id, PlatformRendererType::kUnknown, tag_name) {}

PlatformRendererDarwin::PlatformRendererDarwin(int id, PlatformRendererType type,
                                               const base::String& tag_name)
    : PlatformRendererImpl(id, type, tag_name) {
  // TODO: impl this function later.
}

void PlatformRendererDarwin::OnUpdateDisplayList(DisplayList display_list) {
  // TODO: impl this function later.
}

void PlatformRendererDarwin::OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                                                bool tends_to_flatten) {
  // TODO: impl this function later.
}

void PlatformRendererDarwin::OnAddChild(PlatformRenderer* child) {
  // TODO: impl this function later.
}

void PlatformRendererDarwin::OnRemoveFromParent() {
  // TODO: impl this function later.
}

}  // namespace tasm
}  // namespace lynx
