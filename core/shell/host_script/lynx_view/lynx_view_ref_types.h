// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_TYPES_H_
#define CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_TYPES_H_

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace shell {

enum class LynxViewRefOperationStatus {
  kOk,
  kTargetLost,
  kPlatformError,
};

struct LynxViewRefOperationResult {
  LynxViewRefOperationStatus status = LynxViewRefOperationStatus::kOk;
  std::string message;

  static LynxViewRefOperationResult Ok() { return {}; }

  static LynxViewRefOperationResult TargetLost() {
    return {LynxViewRefOperationStatus::kTargetLost, {}};
  }

  static LynxViewRefOperationResult PlatformError(std::string message) {
    return {LynxViewRefOperationStatus::kPlatformError, std::move(message)};
  }
};

using LynxViewRefOperationCompletion =
    std::function<void(LynxViewRefOperationResult)>;

struct LynxViewRefLoadTemplateRequest {
  bool has_template = false;
  std::vector<uint8_t> template_data;
  std::string url;
  std::string initial_data_json;
  std::string global_props_json;
  std::string processor_name;
  bool read_only = false;
};

struct LynxViewRefSsrRequest {
  std::vector<uint8_t> data;
  std::string url;
  std::string initial_data_json;
};

struct LynxViewRefUpdateDataRequest {
  std::string data_json;
  std::string processor_name;
  bool read_only = false;
};

struct LynxViewRefReloadTemplateRequest {
  std::string data_json;
  std::string global_props_json;
};

struct LynxViewRefGlobalEventRequest {
  std::string name;
  std::string params_json;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_LYNX_VIEW_LYNX_VIEW_REF_TYPES_H_
