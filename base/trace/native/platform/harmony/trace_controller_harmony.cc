// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/harmony/trace_controller_harmony.h"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/platform/harmony/napi_util.h"
#include "base/trace/native/platform/harmony/trace_controller_delegate_harmony.h"

namespace lynx {
namespace trace {

napi_value TraceControllerHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_STATIC_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_static, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_STATIC_FUNCTION("startTracing", StartTracing),
      DECLARE_NAPI_STATIC_FUNCTION("stopTracing", StopTracing),
  };
#undef DECLARE_NAPI_STATIC_FUNCTION

  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "TraceControllerHarmony", NAPI_AUTO_LENGTH,
                        Constructor, nullptr, size, properties, &cons);
  assert(status == napi_ok);

  status =
      napi_set_named_property(env, exports, "TraceControllerHarmony", cons);
  return exports;
}

napi_value TraceControllerHarmony::Constructor(napi_env env,
                                               napi_callback_info info) {
  size_t argc = 0;
  napi_value args[1];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  return js_this;
}

napi_value TraceControllerHarmony::StartTracing(napi_env env,
                                                napi_callback_info info) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::unordered_map<std::string, std::string> trace_config{};
  base::NapiUtil::ConvertToMap(env, args[0], trace_config);
  auto config = std::make_shared<lynx::trace::TraceConfig>();

  config->buffer_size = 81920;  // Default buffer: 80M
  if (trace_config.find("bufferSize") != trace_config.end()) {
    config->buffer_size = std::stoi(trace_config["bufferSize"]);
  }

  if (trace_config.find("filePath") != trace_config.end()) {
    config->file_path = trace_config["filePath"];
  }

  if (trace_config.find("enableCompress") != trace_config.end()) {
    config->enable_compress = trace_config["enableCompress"] == "true";
  }
  config->included_categories = {"*"};
  config->excluded_categories = {"*"};
  auto session_id =
      lynx::trace::GetTraceControllerInstance()->StartTracing(config);

  napi_value sessionId = nullptr;
  napi_create_int32(env, session_id, &sessionId);
  return sessionId;
#else
  return nullptr;
#endif
}

napi_value TraceControllerHarmony::StopTracing(napi_env env,
                                               napi_callback_info info) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int session_id = base::NapiUtil::ConvertToInt64(env, args[0]);
  auto success =
      lynx::trace::GetTraceControllerInstance()->StopTracing(session_id);

  napi_value ret_success = nullptr;
  napi_get_boolean(env, success, &ret_success);
  return ret_success;
#else
  return nullptr;
#endif
}

}  // namespace trace
}  // namespace lynx
