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
#include "core/runtime/common/utils.h"
#include "core/runtime/js/bindings/big_int/jsbi.h"
#include "core/runtime/js/bindings/console.h"
#include "core/runtime/js/bindings/text_codec_helper.h"

namespace lynx {
namespace piper {

static piper::Value GetSystemInfo(Runtime& rt) {
  BASE_STATIC_STRING_DECL(kOSVersion, "osVersion");
  BASE_STATIC_STRING_DECL(kRuntimeType, "runtimeType");
  const char* runtime_type = "";
  switch (rt.type()) {
    case JSRuntimeType::v8:
      runtime_type = "v8";
      break;
    case JSRuntimeType::jsc:
      runtime_type = "jsc";
      break;
    case JSRuntimeType::quickjs:
      runtime_type = "quickjs";
      break;
    case JSRuntimeType::jsvm:
      runtime_type = "jsvm";
      break;
  }

  lepus::Value system_info = tasm::GenerateSystemInfo(nullptr);

  // Properties only for BTS
  system_info.SetProperty(kOSVersion,
                          lepus::Value(tasm::Config::GetOsVersion()));
  system_info.SetProperty(kRuntimeType, lepus::Value(runtime_type));

  auto js_value = valueFromLepus(rt, system_info);
  return std::move(*js_value);
}

Global::~Global() { LOGI("lynx ~Global()"); }

void Global::Init(std::shared_ptr<Runtime>& runtime,
                  std::shared_ptr<piper::ConsoleMessagePostMan>& post_man,
                  const tasm::PageOptions& page_options) {
  SetJSRuntime(runtime);
  auto js_runtime_ = GetJSRuntime();
  if (!js_runtime_) {
    return;
  }

  Scope scope(*js_runtime_);

  piper::Object global = js_runtime_->global();
  Object console_obj = Object::createFromHostObject(
      *js_runtime_,
      std::make_shared<Console>(post_man, page_options.GetDebuggable()));
  global.setProperty(*js_runtime_, "nativeConsole", console_obj);

  piper::Value system_info = GetSystemInfo(*js_runtime_);
  global.setProperty(*js_runtime_, "SystemInfo", std::move(system_info));

  Object jsbi_obj =
      Object::createFromHostObject(*js_runtime_, std::make_shared<JSBI>());
  global.setProperty(*js_runtime_, "LynxJSBI", jsbi_obj);

  Object text_codec_helper_obj = Object::createFromHostObject(
      *js_runtime_, std::make_shared<TextCodecHelper>());
  global.setProperty(*js_runtime_, "TextCodecHelper", text_codec_helper_obj);

  if (tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
    auto& group_id = js_runtime_->getGroupId();
    global.setProperty(*js_runtime_, "groupId", group_id);
  }

  if (tasm::LynxEnv::GetInstance().IsDebugModeEnabled()) {
    global.setProperty(*js_runtime_, "enableDebugMode", true);
  }
}

void Global::EnsureConsole(
    std::shared_ptr<piper::ConsoleMessagePostMan>& post_man,
    const tasm::PageOptions& page_options) {
  auto js_runtime = GetJSRuntime();
  if (!js_runtime) {
    return;
  }
  Scope scope(*js_runtime);
  piper::Object global = js_runtime->global();
  auto console = global.getProperty(*js_runtime, "console");
  if (console && !console->isObject()) {
    Object console_obj = Object::createFromHostObject(
        *js_runtime,
        std::make_shared<Console>(post_man, page_options.GetDebuggable()));
    global.setProperty(*js_runtime, "console", console_obj);
  }
}

void Global::Release() { LOGI("lynx Global::Release"); }

void SharedContextGlobal::SetJSRuntime(std::shared_ptr<Runtime> js_runtime) {
  js_runtime_ = js_runtime;
}

std::shared_ptr<Runtime> SharedContextGlobal::GetJSRuntime() {
  return js_runtime_;
}

void SharedContextGlobal::Release() { js_runtime_.reset(); }

SingleGlobal::~SingleGlobal() { LOGI("lynx ~SingleGlobal"); }

void SingleGlobal::SetJSRuntime(std::shared_ptr<Runtime> js_runtime) {
  js_runtime_ = js_runtime;
}

std::shared_ptr<Runtime> SingleGlobal::GetJSRuntime() {
  return js_runtime_.lock();
}

void SingleGlobal::Release() {}

}  // namespace piper
}  // namespace lynx
