// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_EMBEDDER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_EMBEDDER_HARMONY_H_

#include <node_api.h>

#include <memory>
#include <string>

#include "devtool/embedder/core/inspector_owner_embedder.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_input_event_target.h"

namespace lynx {
namespace devtool {

class InspectorOwnerHarmony;

class InspectorOwnerEmbedderHarmony : public InspectorOwnerEmbedder {
 public:
  InspectorOwnerEmbedderHarmony(napi_env env, napi_ref ref);
  ~InspectorOwnerEmbedderHarmony() override;

  void OnConsoleMessage(const std::string& message) override;
  void OnConsoleObject(const std::string& detail, int callback_id) override;

  void UpdateInputWindowInfo(const HarmonyInputWindowInfo& window_info);
  void InvalidateInputWindow();
  void Destroy();

 protected:
  void OnDevToolPlatformFacadeReady(
      const std::shared_ptr<DevToolPlatformFacade>& facade) override;

 private:
  void ClearInputEventTarget();

  napi_env env_;
  napi_ref ref_;
  std::weak_ptr<DevToolPlatformFacade> platform_facade_;
  std::shared_ptr<HarmonyInputEventTarget> input_event_target_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_EMBEDDER_HARMONY_H_
