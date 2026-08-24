// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_scroll_android.h"

#include <utility>

namespace lynx {
namespace tasm {

PlatformRendererScrollAndroid::PlatformRendererScrollAndroid(
    PlatformRendererContext* context, int id, PlatformRendererType type,
    const fml::RefPtr<PropBundle>& init_data)
    : PlatformRendererAndroid(context, id, type, init_data) {
  UpdateScrollAttributes(init_data);
}

PlatformRendererScrollAndroid::~PlatformRendererScrollAndroid() = default;

void PlatformRendererScrollAndroid::OnUpdateDisplayList(
    DisplayList display_list) {
  PlatformRendererAndroid::OnUpdateDisplayList(std::move(display_list));
  GenerateContentInfoFromDisplayList(fml::RefPtr<PlatformRendererImpl>(this));
}

}  // namespace tasm
}  // namespace lynx
