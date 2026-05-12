// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_SCREEN_CAST_HELPER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_SCREEN_CAST_HELPER_EMBEDDER_H_

#include <memory>

namespace lynx {
namespace devtool {

class LynxDevToolProxy;
class ScreenCastHelper;
class DevtoolPlatformEmbedder;

class ScreenCastHelperEmbedder {
 public:
  explicit ScreenCastHelperEmbedder(
      const std::shared_ptr<DevtoolPlatformEmbedder>& platform_embedder);
  ~ScreenCastHelperEmbedder();

  void StartCasting(int32_t quality, int32_t max_width, int32_t max_height);
  void StopCasting();
  void ContinueCasting();
  void PauseCasting();
  void GetLynxScreenShot();
  void AttachProxy(devtool::LynxDevToolProxy* proxy);

 private:
  bool paused_;
  std::weak_ptr<DevtoolPlatformEmbedder> weak_platform_embedder_;
  std::shared_ptr<ScreenCastHelper> screen_cast_helper_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_SCREEN_CAST_HELPER_EMBEDDER_H_
