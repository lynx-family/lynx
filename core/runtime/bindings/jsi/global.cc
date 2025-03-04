// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/global.h"

#include <memory>

#include "base/include/log/logging.h"
#include "core/inspector/console_message_postman.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/big_int/jsbi.h"
#include "core/runtime/bindings/jsi/console.h"
#include "core/runtime/bindings/jsi/system_info.h"

namespace lynx {
namespace piper {

Global::~Global() { LOGI("lynx ~Global()"); }

void Global::Init(std::shared_ptr<Runtime>& runtime,
                  std::shared_ptr<piper::ConsoleMessagePostMan>& post_man) {
  SetJSRuntime(runtime);
  auto js_runtime_ = GetJSRuntime();
  if (!js_runtime_) {
    return;
  }

  Scope scope(*js_runtime_);

  piper::Object global = js_runtime_->global();
  Object console_obj = Object::createFromHostObject(
      *js_runtime_, std::make_shared<Console>(js_runtime_.get(), post_man));
  global.setProperty(*js_runtime_, "nativeConsole", console_obj);

  Object system_info_obj = Object::createFromHostObject(
      *js_runtime_, std::make_shared<SystemInfo>());
  global.setProperty(*js_runtime_, "SystemInfo", system_info_obj);

  Object jsbi_obj =
      Object::createFromHostObject(*js_runtime_, std::make_shared<JSBI>());
  global.setProperty(*js_runtime_, "LynxJSBI", jsbi_obj);

  if (tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
    auto& group_id = js_runtime_->getGroupId();
    global.setProperty(*js_runtime_, "groupId", group_id);
  }

  if (tasm::LynxEnv::GetInstance().IsDebugModeEnabled()) {
    global.setProperty(*js_runtime_, "enableDebugMode", true);
  }
}

void Global::EnsureConsole(
    std::shared_ptr<piper::ConsoleMessagePostMan>& post_man) {
  auto js_runtime = GetJSRuntime();
  if (!js_runtime) {
    return;
  }
  Scope scope(*js_runtime);
  piper::Object global = js_runtime->global();
  auto console = global.getProperty(*js_runtime, "console");
  if (console && !console->isObject()) {
    Object console_obj = Object::createFromHostObject(
        *js_runtime, std::make_shared<Console>(js_runtime.get(), post_man));
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
