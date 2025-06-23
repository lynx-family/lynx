// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lynx_info_reporter_helper_harmony.h"

#include "base/include/no_destructor.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace harmony {

namespace {

napi_value Constructor(napi_env env, napi_callback_info info) {
  napi_throw_error(env, nullptr, "Illegal constructor");
  return nullptr;
}

napi_status RegisterLynxInfoReporterHelperImpl(napi_env env,
                                               napi_value exports) {
  constexpr char kClassName[] = "LynxInfoReporterHelper";
  constexpr char kMethodName[] = "nativeRegister";
  constexpr char kInvokeNAPIFailed[] = "%s %s failed";
  napi_property_descriptor properties[] = {
      {kMethodName, nullptr, LynxInfoReporterHelper::Register, nullptr, nullptr,
       nullptr, napi_static, nullptr}};
  // define class
  napi_value constructor;
  napi_status status = napi_define_class(
      env, kClassName, NAPI_AUTO_LENGTH, Constructor, nullptr,
      sizeof(properties) / sizeof(properties[0]), properties, &constructor);
  NAPI_THROW_IF_FAILED_STATUS(env, status, kInvokeNAPIFailed, "define",
                              kClassName);

  // register class
  status = napi_set_named_property(env, exports, kClassName, constructor);
  NAPI_THROW_IF_FAILED_STATUS(env, status, kInvokeNAPIFailed, "register",
                              kClassName);

  return status;
}
}  // namespace

LynxInfoReporterHelper& LynxInfoReporterHelper::GetInstance() {
  static base::NoDestructor<LynxInfoReporterHelper> instance;
  return *instance;
}

napi_value LynxInfoReporterHelper::Init(napi_env env, napi_value exports) {
  RegisterLynxInfoReporterHelperImpl(env, exports);
  return exports;
}

napi_value LynxInfoReporterHelper::Register(napi_env env,
                                            napi_callback_info info) {
  /**
   * 0 - trailValues: Record<string, string>
   */
  napi_value js_object;
  size_t argc = 1;
  napi_value argv[argc];
  if (napi_ok !=
      napi_get_cb_info(env, info, &argc, argv, &js_object, nullptr)) {
    return nullptr;
  }

  LynxInfoReporterHelper& instance = LynxInfoReporterHelper::GetInstance();
  instance.env_ = env;
  napi_create_reference(env, argv[0], 0, &instance.info_reporter_);
  return nullptr;
}

void LynxInfoReporterHelper::ReportTemplateInfo(const std::string& url) {
  if (!info_reporter_) {
    return;
  }
  // report last_async_component_url
  // assign LynxTemplateType.ASYNC_COMPONENT
  constexpr int32_t kAsyncComponentType = 1;
  constexpr char kReportLynxTemplateEvent[] = "reportLynxTemplateEvent";
  napi_value call_args[2];
  napi_create_int32(env_, kAsyncComponentType, &call_args[0]);
  napi_create_string_utf8(env_, url.c_str(), NAPI_AUTO_LENGTH, &call_args[1]);
  base::NapiUtil::InvokeJsMethod(env_, info_reporter_, kReportLynxTemplateEvent,
                                 2, call_args);
}

}  // namespace harmony
}  // namespace lynx
