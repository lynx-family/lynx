// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LYNX_INFO_REPORTER_HELPER_HARMONY_H_
#define CORE_RESOURCE_LYNX_INFO_REPORTER_HELPER_HARMONY_H_

#include <node_api.h>

#include <string>

namespace lynx {
namespace harmony {

/**
 * Helper class for reporting last_async_component_url
 * TODO(zhoupeng.z): move to base namespace for iOS and Android
 */
class LynxInfoReporterHelper {
 public:
  static LynxInfoReporterHelper& GetInstance();

  static napi_value Init(napi_env env, napi_value exports);

  static napi_value Register(napi_env env, napi_callback_info info);

  void ReportTemplateInfo(const std::string& url);

 private:
  napi_env env_;
  napi_ref info_reporter_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // CORE_RESOURCE_LYNX_INFO_REPORTER_HELPER_HARMONY_H_
