// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin_factory.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"

namespace lynx {
namespace tasm {

PlatformRendererDarwinFactory::PlatformRendererDarwinFactory(PlatformRendererContextDarwin* context)
    : context_(context) {}

fml::RefPtr<PlatformRenderer> PlatformRendererDarwinFactory::CreateRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data) {
  return fml::MakeRefCounted<PlatformRendererDarwin>(context_, id, type);
}

fml::RefPtr<PlatformRenderer> PlatformRendererDarwinFactory::CreateExtendedRenderer(
    int id, const base::String& tag_name, const fml::RefPtr<PropBundle>& init_data) {
  return fml::MakeRefCounted<PlatformRendererDarwin>(context_, id, tag_name);
}

}  // namespace tasm
}  // namespace lynx
