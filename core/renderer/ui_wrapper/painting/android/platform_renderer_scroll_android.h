// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_SCROLL_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_SCROLL_ANDROID_H_

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_scroll.h"

namespace lynx {
namespace tasm {

class PlatformRendererContext;
class PropBundle;

class PlatformRendererScrollAndroid : public PlatformRendererAndroid,
                                      public PlatformRendererScroll {
 public:
  explicit PlatformRendererScrollAndroid(
      PlatformRendererContext* context, int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data);
  ~PlatformRendererScrollAndroid() override;

 protected:
  void OnUpdateDisplayList(DisplayList display_list) override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_SCROLL_ANDROID_H_
