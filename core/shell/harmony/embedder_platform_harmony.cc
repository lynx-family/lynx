// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/harmony/embedder_platform_harmony.h"

#include <rawfile/raw_file_manager.h>

#include <string>
#include <utility>

#include "base/include/fml/platform/harmony/message_loop_harmony.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/base/harmony/vsync_monitor_harmony.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/ui_wrapper/layout/harmony/layout_context_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/painting_context_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace shell {

#define DECLARE_NAPI_METHOD(name, func) \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

#define NAPI_METHOD_COUNT(properties) sizeof(properties) / sizeof(properties[0])

EmbedderPlatformHarmony::EmbedderPlatformHarmony(
    napi_env env, napi_value capi_embedder,
    std::unique_ptr<tasm::harmony::UIDelegateHarmony> ui_delegate,
    const std::shared_ptr<tasm::harmony::LynxContext>& lynx_context)
    : ui_delegate_(std::move(ui_delegate)), env_(env), weak_factory_(this) {
  napi_create_reference(env, capi_embedder, 0, &capi_embedder_ref_);
  ui_delegate_->SetPlatform(this);
}

EmbedderPlatformHarmony::~EmbedderPlatformHarmony() {
  context_->ResetEmbedder();
  napi_delete_reference(env_, capi_embedder_ref_);
};

napi_value EmbedderPlatformHarmony::Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_METHOD("getUIDelegate", GetUIDelegate),
      {"updateRefreshRate", nullptr, UpdateRefreshRate, nullptr, nullptr,
       nullptr, napi_static, nullptr},
      DECLARE_NAPI_METHOD("destroy", Destroy),
  };

  napi_value cons;
  std::string embedder_class = "EmbedderPlatform";
  napi_define_class(env, embedder_class.c_str(), NAPI_AUTO_LENGTH, New, nullptr,
                    NAPI_METHOD_COUNT(properties), properties, &cons);

  napi_set_named_property(env, exports, embedder_class.c_str(), cons);
  return exports;
}

napi_value EmbedderPlatformHarmony::New(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  tasm::harmony::UIOwner* ui_owner;
  napi_unwrap(env, args[1], reinterpret_cast<void**>(&ui_owner));

  tasm::harmony::ShadowNodeOwner* shadow_node_owner;
  napi_unwrap(env, args[2], reinterpret_cast<void**>(&shadow_node_owner));

  auto context =
      std::make_shared<tasm::harmony::LynxContext>(shadow_node_owner, ui_owner);
  auto module_factory = std::make_unique<harmony::ModuleFactoryCAPI>(context);

  auto ui_delegate = std::make_unique<tasm::harmony::UIDelegateHarmony>(
      ui_owner, shadow_node_owner, context, std::move(module_factory));

  EmbedderPlatformHarmony* native_object = new EmbedderPlatformHarmony(
      env, args[0], std::move(ui_delegate), context);

  // TODO(chenyouhui): Remove embedder from LynxContext
  context->SetEmbedder(native_object);
  native_object->SetContext(context);
  ui_owner->SetContext(context);
  shadow_node_owner->SetContext(context);

  napi_wrap(
      env, js_this, native_object, [](napi_env env, void* data, void* hint) {},
      nullptr, nullptr);

  return js_this;
}

napi_value EmbedderPlatformHarmony::GetUIDelegate(napi_env env,
                                                  napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  EmbedderPlatformHarmony* obj;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));

  return base::NapiUtil::CreatePtrArray(
      env, reinterpret_cast<uint64_t>(obj->ui_delegate_.get()));
}

napi_value EmbedderPlatformHarmony::UpdateRefreshRate(napi_env env,
                                                      napi_callback_info info) {
  napi_status status;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (status != napi_ok) {
    return nullptr;
  }

  int64_t refresh_rate;
  status = napi_get_value_int64(env, args[0], &refresh_rate);
  if (status != napi_ok) {
    return nullptr;
  }

  DCHECK(refresh_rate > 0);
  lynx::base::VSyncMonitorHarmony::OnUpdateRefreshRate(refresh_rate);
  return nullptr;
}

napi_value EmbedderPlatformHarmony::Destroy(napi_env env,
                                            napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

  EmbedderPlatformHarmony* obj;
  napi_status status =
      napi_remove_wrap(env, js_this, reinterpret_cast<void**>(&obj));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "EmbedderPlatformHarmony napi_remove_wrap failed!");

  delete obj;
  return nullptr;
}

void EmbedderPlatformHarmony::ScreenSize(float size[2]) const {
  size[0] = ui_delegate_->ScreenWidth();
  size[1] = ui_delegate_->ScreenHeight();
}

float EmbedderPlatformHarmony::DevicePixelRatio() const {
  return ui_delegate_->DevicePixelRatio();
}

int32_t EmbedderPlatformHarmony::GetInstanceId() const {
  return ui_delegate_->GetInstanceId();
}

napi_env EmbedderPlatformHarmony::GetNapiEnv() const { return env_; }

void EmbedderPlatformHarmony::TakeSnapshot(
    size_t max_width, size_t max_height, int quality,
    const fml::RefPtr<fml::TaskRunner>& screenshot_runner,
    tasm::TakeSnapshotCompletedCallback callback) {
  TakeScreenShot(
      max_width, max_height, quality,
      [self = weak_factory_.GetWeakPtr(),
       screenshot_runner = std::move(screenshot_runner),
       callback =
           std::move(callback)](CallbackHandler::ScreenShotResponse result) {
        if (!self) {
          return;
        }
        screenshot_runner->PostTask(
            [result = std::move(result), callback = std::move(callback)]() {
              std::string str(result.data.begin(), result.data.end());
              auto snapshot_data = modp_b64_encode(str);
              float timestamp =
                  std::chrono::steady_clock::now().time_since_epoch().count();
              callback(std::move(snapshot_data), timestamp, result.width,
                       result.height, 1.0f);
            });
      });
}

void EmbedderPlatformHarmony::TakeScreenShot(
    size_t max_width, size_t max_height, int32_t quality,
    base::MoveOnlyClosure<void, CallbackHandler::ScreenShotResponse> callback) {
  napi_value call_args[4];
  auto* callback_handler = new CallbackHandler(std::move(callback));
  napi_create_int32(env_, quality, &call_args[1]);
  napi_create_int32(env_, max_width, &call_args[2]);
  napi_create_int32(env_, max_height, &call_args[3]);
  napi_create_function(env_, "callback", 9,
                       CallbackHandler::HandleScreenShotCallback,
                       callback_handler, &call_args[0]);
  base::NapiUtil::InvokeJsMethod(env_, capi_embedder_ref_, "takeScreenShot", 4,
                                 call_args);
}

}  // namespace shell
}  // namespace lynx
