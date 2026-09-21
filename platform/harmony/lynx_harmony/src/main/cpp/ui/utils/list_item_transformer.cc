// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/list_item_transformer.h"

#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace tasm {
namespace harmony {

std::optional<std::array<float, 4>> ListItemTransformer::TransformItem(
    float main_axis_size, float main_axis_offset, bool is_vertical,
    bool is_rtl) {
  napi_env env = env_;
  base::NapiHandleScope scope(env);
  // Pass dimensions as numbers and direction flags as booleans.
  napi_value args[4]{nullptr};
  if (napi_create_double(env, main_axis_size, &args[0]) != napi_ok ||
      napi_create_double(env, main_axis_offset, &args[1]) != napi_ok ||
      napi_get_boolean(env, is_vertical, &args[2]) != napi_ok ||
      napi_get_boolean(env, is_rtl, &args[3]) != napi_ok) {
    return std::nullopt;
  }
  return InvokeCallback(transform_callback_, 4, args);
}

std::optional<std::array<float, 4>> ListItemTransformer::ResetItem() {
  return InvokeCallback(reset_callback_, 0, nullptr);
}

std::optional<std::array<float, 4>> ListItemTransformer::InvokeCallback(
    napi_ref callback_ref, size_t argc, const napi_value* args) {
  napi_env env = env_;
  base::NapiHandleScope scope(env);
  napi_value callback{nullptr};
  if (napi_get_reference_value(env, callback_ref, &callback) != napi_ok ||
      !callback) {
    return std::nullopt;
  }
  napi_value receiver{nullptr};
  if (napi_get_undefined(env, &receiver) != napi_ok) {
    return std::nullopt;
  }
  // Invoke the callback and read the returned transform.
  napi_value result{nullptr};
  napi_status status =
      napi_call_function(env, receiver, callback, argc, args, &result);
  if (status == napi_pending_exception) {
    napi_value exception{nullptr};
    napi_get_and_clear_last_exception(env, &exception);
    return std::nullopt;
  } else if (status != napi_ok) {
    return std::nullopt;
  }
  auto parse_result = [env, result]() -> std::optional<std::array<float, 4>> {
    bool is_array = false;
    uint32_t length = 0;
    if (napi_is_array(env, result, &is_array) != napi_ok || !is_array ||
        napi_get_array_length(env, result, &length) != napi_ok || length != 4) {
      return std::nullopt;
    }
    std::array<float, 4> transform;
    for (uint32_t i = 0; i < transform.size(); ++i) {
      napi_value value{nullptr};
      double number = 0;
      if (napi_get_element(env, result, i, &value) != napi_ok ||
          napi_get_value_double(env, value, &number) != napi_ok) {
        return std::nullopt;
      }
      transform[i] = static_cast<float>(number);
    }
    return transform;
  };
  auto transform = parse_result();
  if (!transform) {
    // Reading an array element may invoke a getter that throws.
    bool exception_pending = false;
    if (napi_is_exception_pending(env, &exception_pending) == napi_ok &&
        exception_pending) {
      napi_value exception{nullptr};
      napi_get_and_clear_last_exception(env, &exception);
    }
  }
  return transform;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
