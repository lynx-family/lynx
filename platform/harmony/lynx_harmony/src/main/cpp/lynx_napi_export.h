// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <hilog/log.h>
#include <napi/native_api.h>
#include <uv.h>

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_NAPI_EXPORT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_NAPI_EXPORT_H_

#include "base/include/fml/platform/harmony/message_loop_harmony.h"
#include "base/trace/native/platform/harmony/trace_controller_harmony.h"
#include "core/renderer/dom/harmony/lynx_template_bundle_harmony.h"
#include "core/renderer/utils/harmony/lynx_trail_hub_impl_harmony.h"
#include "core/resource/lynx_info_reporter_helper_harmony.h"
#include "core/services/event_report/harmony/event_tracker_harmony.h"
#include "core/services/performance/harmony/performance_controller_harmony.h"
#include "core/shell/harmony/embedder_platform_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/devtool_lifecycle_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/devtool_settings_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_runtime_wrapper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_white_board_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/js_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/js_ui_base.h"

#if __has_include(                                                                 \
    "core/runtime/rts/binding/napi/harmony_napi.h") &&                             \
    __has_include(                                                                 \
        "platform/harmony/lynx_harmony/src/main/cpp/static_task_napi_bridge.h") && \
        __has_include(                                                             \
            "relax_ng/core/runtime/rts/binding/idl/napi_renderer_function_ng.h")
#define LYNX_HARMONY_HAS_RELAX2NATIVE_INIT 1
#include "core/runtime/rts/binding/napi/harmony_napi.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/static_task_napi_bridge.h"
#include "relax_ng/core/runtime/rts/binding/idl/napi_renderer_function_ng.h"
#else
#define LYNX_HARMONY_HAS_RELAX2NATIVE_INIT 0
#endif

void LynxNapiInit(napi_env env, napi_value exports) {
  lynx::harmony::LynxTemplateRenderer::Init(env, exports);
  lynx::harmony::LynxWhiteBoard::Init(env, exports);
  lynx::tasm::PropBundleHarmony::Init(env);
  lynx::tasm::report::harmony::Init(env, exports);
  lynx::tasm::performance::PerformanceControllerHarmonyJSWrapper::Init(env,
                                                                       exports);
  lynx::tasm::harmony::ShadowNodeOwner::Init(env, exports);
  lynx::tasm::harmony::UIOwner::Init(env, exports);
  lynx::shell::EmbedderPlatformHarmony::Init(env, exports);
  lynx::tasm::harmony::JSShadowNode::Init(env, exports);
  lynx::tasm::harmony::JSUIBase::Init(env, exports);
  lynx::tasm::harmony::NativeNodeContent::Init(env, exports);
  lynx::tasm::harmony::LynxTrailHubImplHarmony::Init(env, exports);
  lynx::trace::TraceControllerHarmony::Init(env, exports);
  lynx::harmony::LynxInfoReporterHelper::Init(env, exports);
  lynx::harmony::LynxTemplateBundleHarmony::Init(env, exports);
  lynx::harmony::LynxRuntimeWrapper::Init(env, exports);
  lynx::tasm::harmony::DevToolLifecycleHarmony::Init(env, exports);
  lynx::tasm::harmony::DevToolSettingsHarmony::Init(env, exports);
#if LYNX_HARMONY_HAS_RELAX2NATIVE_INIT
  lynx::harmony::StaticTaskNapiBridge::Init(env, exports);
  lynx::runtime::rts::Env rts_env(env);
  lynx::runtime::rts::Object rts_exports(env, exports);
  relax_ng::NapiRendererFunctionNG::Install(rts_env, rts_exports);
#endif
}

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_NAPI_EXPORT_H_
