// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_NATIVE_VIEW_LYNX_TEXTURE_VIEW_H_
#define PLATFORM_EMBEDDER_NATIVE_VIEW_LYNX_TEXTURE_VIEW_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "platform/embedder/public/lynx_native_view.h"

class LynxTextureView : public lynx::pub::LynxNativeView {
 public:
  LynxTextureView(void*) {}
  ~LynxTextureView() override;
  bool OnCreate() override;
  void OnDestroy() override;
  void OnAttach() override;
  void OnDetach() override;
  void OnLayoutChanged(float left, float top, float width, float height,
                       float pixel_ratio) override;
  void OnPropertiesChanged(const lynx::pub::LynxValue& attrs,
                           const lynx::pub::LynxValue& events) override {}
  void OnMethodInvoked(
      const char* method, const lynx::pub::LynxValue& attrs,
      std::function<void(int, lynx::pub::LynxValue&&)> callback) override;
  bool IsScrollEnabled() override { return true; }
  bool IsSurfaceEnabled() override { return true; }

 private:
  int width_ = 0;
  int height_ = 0;
};

#endif  // PLATFORM_EMBEDDER_NATIVE_VIEW_LYNX_TEXTURE_VIEW_H_
