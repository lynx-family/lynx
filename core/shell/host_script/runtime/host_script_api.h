// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_API_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_API_H_

#include <memory>

#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace shell {

class HostScriptSession;

bool InstallHostScriptApi(Napi::Env env,
                          const std::shared_ptr<HostScriptSession>& session);
void UninstallHostScriptApi(Napi::Env env);

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_API_H_
