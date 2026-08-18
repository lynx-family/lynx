// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_scroll_darwin.h"

namespace lynx {
namespace tasm {

PlatformRendererScrollDarwin::PlatformRendererScrollDarwin(PlatformRendererContextDarwin* context,
                                                           int id, PlatformRendererType type,
                                                           const fml::RefPtr<PropBundle>& init_data)
    : PlatformRendererDarwin(context, id, type, init_data) {
  UpdateScrollAttributes(init_data);
}

PlatformRendererScrollDarwin::~PlatformRendererScrollDarwin() = default;

void PlatformRendererScrollDarwin::OnUpdateDisplayList(DisplayList display_list) {
  PlatformRendererDarwin::OnUpdateDisplayList(std::move(display_list));
  GenerateContentInfoFromDisplayList(fml::RefPtr<PlatformRendererImpl>(this));
}

}  // namespace tasm
}  // namespace lynx
