// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_MODULE_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_MODULE_H_

#include <cstddef>
#include <memory>
#include <string>

#include "core/runtime/common/napi/napi_environment.h"

namespace lynx {
namespace shell {

class HostScriptSession;

class HostScriptModule final : public runtime::js::NapiEnvironment::Module {
 public:
  explicit HostScriptModule(std::shared_ptr<HostScriptSession> session);

  bool IsLazy() override;
  void OnEnvDetach(Napi::Env env) override;
  void OnLoad(Napi::Object& target) override;
  void Populate(Napi::Object& target);

  static void Register(napi_env env,
                       const std::shared_ptr<HostScriptSession>& session);
  static Napi::Error CreateError(Napi::Env env, const char* code,
                                 const std::string& message);

 private:
  static HostScriptSession* StateFromInfo(const Napi::CallbackInfo& info);
  static bool IsKnownEvent(const std::string& event);
  static Napi::Value ThrowApiError(Napi::Env env, const char* code,
                                   const std::string& message,
                                   bool type_error = false);
  static bool ReadOptionalString(const Napi::CallbackInfo& info, size_t index,
                                 std::string* result);
  static bool ReadOptionalBoolean(const Napi::CallbackInfo& info, size_t index,
                                  bool* result);

  static Napi::Value HasCurrent(const Napi::CallbackInfo& info);
  static Napi::Value WaitForCurrent(const Napi::CallbackInfo& info);
  static Napi::Value LoadURL(const Napi::CallbackInfo& info);
  static Napi::Value LoadTemplate(const Napi::CallbackInfo& info);
  static Napi::Value LoadSsr(const Napi::CallbackInfo& info, bool hydrate);
  static Napi::Value LoadSSR(const Napi::CallbackInfo& info);
  static Napi::Value HydrateSSR(const Napi::CallbackInfo& info);
  static Napi::Value UpdateMetaData(const Napi::CallbackInfo& info);
  static Napi::Value SetGlobalProps(const Napi::CallbackInfo& info);
  static Napi::Value ReloadTemplate(const Napi::CallbackInfo& info);
  static Napi::Value SendGlobalEvent(const Napi::CallbackInfo& info);
  static Napi::Value On(const Napi::CallbackInfo& info);
  static Napi::Value Off(const Napi::CallbackInfo& info);
  static Napi::Value ReportEntryResult(const Napi::CallbackInfo& info);

  std::shared_ptr<HostScriptSession> session_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_MODULE_H_
