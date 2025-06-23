// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HARMONY_EMBEDDER_PLATFORM_HARMONY_H_
#define CORE_SHELL_HARMONY_EMBEDDER_PLATFORM_HARMONY_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/renderer/ui_wrapper/painting/harmony/ui_delegate_harmony.h"
#include "core/resource/lynx_resource_loader_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/module/module_factory_capi.h"

namespace lynx {
namespace tasm {

namespace harmony {
class LynxContext;
}

}  // namespace tasm

namespace shell {

// TODO(chenyouhui): Rename EmbedderPlatformHarmony and namespace
// TODO(chenyouhui): Merge EmbedderPlatformHarmony and UIDelegateHarmony?
class EmbedderPlatformHarmony {
 public:
  EmbedderPlatformHarmony(
      napi_env env, napi_value capi_embedder,
      std::unique_ptr<tasm::harmony::UIDelegateHarmony> ui_delegate,
      const std::shared_ptr<tasm::harmony::LynxContext>& lynx_context);

  ~EmbedderPlatformHarmony();
  static napi_value Init(napi_env env, napi_value exports);
  static napi_value New(napi_env env, napi_callback_info info);
  static napi_value GetUIDelegate(napi_env env, napi_callback_info info);
  static napi_value UpdateRefreshRate(napi_env env, napi_callback_info info);
  static napi_value Destroy(napi_env env, napi_callback_info info);

  void ScreenSize(float size[2]) const;
  float DevicePixelRatio() const;
  int32_t GetInstanceId() const;
  napi_env GetNapiEnv() const;

  void TakeSnapshot(size_t max_width, size_t max_height, int quality,
                    const fml::RefPtr<fml::TaskRunner>& screenshot_runner,
                    tasm::TakeSnapshotCompletedCallback callback);

 private:
  class CallbackHandler {
   public:
    struct ScreenShotResponse {
      std::vector<uint8_t> data;
      float width{0.f};
      float height{0.f};
    };

    explicit CallbackHandler(
        base::MoveOnlyClosure<void, ScreenShotResponse> callback)
        : callback_(std::move(callback)) {}
    static napi_value HandleScreenShotCallback(napi_env env,
                                               napi_callback_info info) {
      napi_value js_this;
      size_t argc = 2;
      napi_value argv[2];
      void* callback_ptr;
      napi_get_cb_info(env, info, &argc, argv, &js_this, &callback_ptr);
      CallbackHandler* callback_handler =
          reinterpret_cast<CallbackHandler*>(callback_ptr);
      if (!base::NapiUtil::NapiIsType(env, argv[1], napi_undefined)) {
        ScreenShotResponse response;
        napi_value result;
        napi_value width;
        napi_value height;
        napi_get_named_property(env, argv[1], "width", &width);
        napi_get_named_property(env, argv[1], "height", &height);
        napi_get_named_property(env, argv[1], "result", &result);
        base::NapiUtil::ConvertToArrayBuffer(env, result, response.data);
        response.width = base::NapiUtil::ConvertToFloat(env, width);
        response.height = base::NapiUtil::ConvertToFloat(env, height);
        callback_handler->callback_(response);
      }
      delete callback_handler;
      return js_this;
    }

   private:
    base::MoveOnlyClosure<void, ScreenShotResponse> callback_;
  };

  void SetContext(const std::shared_ptr<tasm::harmony::LynxContext>& context) {
    context_ = context;
  }

  void TakeScreenShot(
      size_t max_width, size_t max_height, int32_t quality,
      base::MoveOnlyClosure<void, CallbackHandler::ScreenShotResponse>
          callback);

  std::unique_ptr<tasm::harmony::UIDelegateHarmony> ui_delegate_;
  std::shared_ptr<tasm::harmony::LynxContext> context_{nullptr};
  napi_env env_;
  napi_ref capi_embedder_ref_;
  fml::WeakPtrFactory<EmbedderPlatformHarmony> weak_factory_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HARMONY_EMBEDDER_PLATFORM_HARMONY_H_
