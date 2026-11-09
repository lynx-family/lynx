// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_RESOURCES_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_RESOURCES_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/fml/task_runner.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx::shell {

// JS handles are owned and released on the control context's runner. File IO
// retains only copied paths and posts copied bytes back to that runner.
class HostScriptResources final
    : public std::enable_shared_from_this<HostScriptResources> {
 public:
  static std::shared_ptr<HostScriptResources> Install(
      Napi::Env env, fml::RefPtr<fml::TaskRunner> runner, std::string root);
  void Detach();

 private:
  HostScriptResources(fml::RefPtr<fml::TaskRunner> runner, std::string root);
  Napi::Value Read(const Napi::CallbackInfo& info, bool text);
  void Complete(uint64_t request, bool text, std::string bytes,
                const std::string& error);

  fml::RefPtr<fml::TaskRunner> runner_;
  std::string root_;
  bool attached_ = true;
  uint64_t next_request_ = 0;
  std::unordered_map<uint64_t, Napi::Promise::Deferred> pending_;
};

}  // namespace lynx::shell
#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_RESOURCES_H_
