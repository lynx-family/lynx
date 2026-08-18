// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin_factory.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_root_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_scroll_darwin.h"

#import <Lynx/LynxComponentRegistry.h>

namespace lynx {
namespace tasm {

PlatformRendererDarwinFactory::PlatformRendererDarwinFactory(PlatformRendererContextDarwin* context)
    : context_(context) {}

fml::RefPtr<PlatformRenderer> PlatformRendererDarwinFactory::CreateRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  if (type == PlatformRendererType::kPage) {
    return fml::MakeRefCounted<PlatformRendererRootDarwin>(context_, id, type);
  } else if (type == PlatformRendererType::kScroll) {
    return fml::MakeRefCounted<PlatformRendererScrollDarwin>(context_, id, type, init_data);
  }
  return fml::MakeRefCounted<PlatformRendererDarwin>(context_, id, type, init_data, init_config);
}

fml::RefPtr<PlatformRenderer> PlatformRendererDarwinFactory::CreateExtendedRenderer(
    int id, const base::String& tag_name, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  return fml::MakeRefCounted<PlatformRendererDarwin>(context_, id, tag_name, init_data,
                                                     init_config);
}

}  // namespace tasm
}  // namespace lynx
