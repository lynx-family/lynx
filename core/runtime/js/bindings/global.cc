// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/global.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "core/inspector/console_message_postman.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/bindings/big_int/jsbi.h"
#include "core/runtime/js/bindings/console.h"
#include "core/runtime/js/bindings/text_codec_helper.h"
#include "core/runtime/js/utils.h"

namespace lynx {
namespace runtime {
namespace js {
static Value GetSystemInfo(Runtime& rt) {
  BASE_STATIC_STRING_DECL(kOSVersion, "osVersion");
  BASE_STATIC_STRING_DECL(kRuntimeType, "runtimeType");
  const char* runtime_type = JSRuntimeTypeToString(rt.type());

  lepus::Value system_info = tasm::GenerateSystemInfo(nullptr);

  // Properties only for BTS
  system_info.SetProperty(kOSVersion,
                          lepus::Value(tasm::Config::GetOsVersion()));
  system_info.SetProperty(kRuntimeType, lepus::Value(runtime_type));

  auto js_value = valueFromLepus(rt, system_info);
  return std::move(*js_value);
}

Global::~Global() { LOGI("lynx ~Global()"); }

void Global::Init(Runtime& runtime,
                  std::shared_ptr<ConsoleMessagePostMan>& post_man,
                  const tasm::PageOptions& page_options,
                  bool install_shared_host_objects) {
  Scope scope(runtime);

  Object global = runtime.global();
  Object console_obj = Object::createFromHostObject(
      runtime,
      std::make_shared<Console>(post_man, page_options.GetDebuggable()));
  global.setProperty(runtime, "nativeConsole", console_obj);
  global.setProperty(runtime, "__lynxDisableModuleCache",
                     page_options.GetDebuggable());

  // These are stateless shared host objects. For the new "shared Isolate/VM +
  // per-page isolated Context" scheme the page context copies them by reference
  // from the group's global context, so skip re-creating them here.
  if (install_shared_host_objects) {
    Value system_info = GetSystemInfo(runtime);
    global.setProperty(runtime, "SystemInfo", std::move(system_info));

    Object jsbi_obj =
        Object::createFromHostObject(runtime, std::make_shared<JSBI>());
    global.setProperty(runtime, "LynxJSBI", jsbi_obj);

    Object text_codec_helper_obj = Object::createFromHostObject(
        runtime, std::make_shared<TextCodecHelper>());
    global.setProperty(runtime, "TextCodecHelper", text_codec_helper_obj);
  }

  if (tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
    auto& group_id = runtime.getGroupId();
    global.setProperty(runtime, "groupId", group_id);
  }

  if (tasm::LynxEnv::GetInstance().IsDebugModeEnabled()) {
    global.setProperty(runtime, "enableDebugMode", true);
  }
}

void Global::EnsureConsole(Runtime& runtime,
                           std::shared_ptr<ConsoleMessagePostMan>& post_man,
                           const tasm::PageOptions& page_options) {
  Scope scope(runtime);
  Object global = runtime.global();
  auto console = global.getProperty(runtime, "console");
  if (console && !console->isObject()) {
    Object console_obj = Object::createFromHostObject(
        runtime,
        std::make_shared<Console>(post_man, page_options.GetDebuggable()));
    global.setProperty(runtime, "console", console_obj);
  }
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
