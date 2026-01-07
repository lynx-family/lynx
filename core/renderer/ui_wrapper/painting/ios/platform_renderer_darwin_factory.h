// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_FACTORY_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_FACTORY_H_

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer.h"

namespace lynx {
namespace tasm {

class PlatformRendererContextDarwin;

class PlatformRendererDarwinFactory : public PlatformRendererFactory {
 public:
  PlatformRendererDarwinFactory(PlatformRendererContextDarwin* context);
  ~PlatformRendererDarwinFactory() override = default;

  fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data) override;

  fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data) override;

 private:
  PlatformRendererContextDarwin* context_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_FACTORY_H_
