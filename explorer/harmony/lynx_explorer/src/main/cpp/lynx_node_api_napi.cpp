// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <node_api.h>
#include <stdio.h>

#include <cstdint>
#include <string>
#include <utility>

#include "LynxNodeAPI.h"

namespace {

static bool ParseEnvArrayArg(napi_env env, napi_value env_arg,
                             void** out_env_ptr) {
  napi_valuetype valuetype;
  napi_status status = napi_typeof(env, env_arg, &valuetype);
  if (status != napi_ok || valuetype != napi_object) {
    napi_throw_type_error(env, nullptr, "env_arg must be an array-like object");
    return false;
  }

  uint32_t length = 0;
  status = napi_get_array_length(env, env_arg, &length);
  if (status != napi_ok || length != 2) {
    napi_throw_range_error(env, nullptr,
                           "env_arg must be an array of length 2");
    return false;
  }

  napi_value ptr_high_value = nullptr;
  napi_value ptr_low_value = nullptr;
  status = napi_get_element(env, env_arg, 0, &ptr_high_value);
  if (status != napi_ok || ptr_high_value == nullptr) {
    napi_throw_type_error(env, nullptr, "env_arg[0] must be a uint32");
    return false;
  }
  status = napi_get_element(env, env_arg, 1, &ptr_low_value);
  if (status != napi_ok || ptr_low_value == nullptr) {
    napi_throw_type_error(env, nullptr, "env_arg[1] must be a uint32");
    return false;
  }

  uint32_t ptr_high = 0;
  uint32_t ptr_low = 0;
  status = napi_get_value_uint32(env, ptr_high_value, &ptr_high);
  if (status != napi_ok) {
    napi_throw_type_error(env, nullptr, "env_arg[0] must be a uint32");
    return false;
  }
  status = napi_get_value_uint32(env, ptr_low_value, &ptr_low);
  if (status != napi_ok) {
    napi_throw_type_error(env, nullptr, "env_arg[1] must be a uint32");
    return false;
  }

  uint64_t env_ptr_val = (static_cast<uint64_t>(ptr_high) << 32) + ptr_low;
  if (env_ptr_val == 0) {
    napi_throw_range_error(env, nullptr, "env pointer must be non-zero");
    return false;
  }
  *out_env_ptr = reinterpret_cast<void*>(env_ptr_val);
  return true;
}

static bool ParseUint32Arg(napi_env env, napi_value value,
                           uint64_t* out_value) {
  uint32_t low = 0;
  napi_status status = napi_get_value_uint32(env, value, &low);
  if (status != napi_ok) {
    return false;
  }
  *out_value = static_cast<uint64_t>(low);
  return true;
}

static bool ParseStringArg(napi_env env, napi_value value,
                           std::string* out_value) {
  size_t length = 0;
  napi_status status =
      napi_get_value_string_utf8(env, value, nullptr, 0, &length);
  if (status != napi_ok) {
    return false;
  }

  std::string buffer(length + 1, '\0');
  size_t copied = 0;
  status = napi_get_value_string_utf8(env, value, buffer.data(), buffer.size(),
                                      &copied);
  if (status != napi_ok) {
    return false;
  }
  buffer.resize(copied);
  *out_value = buffer;
  return true;
}

static napi_value RequireNodeAddon(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  napi_value undefined_val;
  napi_get_undefined(env, &undefined_val);

  if (argc < 2) return undefined_val;

  void* napiEnv = nullptr;
  if (!ParseEnvArrayArg(env, args[0], &napiEnv)) {
    return undefined_val;
  }

  std::string addon_name;
  if (!ParseStringArg(env, args[1], &addon_name)) {
    return undefined_val;
  }

  lynx::explorer::LynxNodeAPI::GetInstance().RequireNodeAddon(napiEnv,
                                                              addon_name);

  return undefined_val;
}

static napi_value PutEnvByToken(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  napi_value undefined_val;
  napi_get_undefined(env, &undefined_val);
  if (argc < 2) return undefined_val;

  uint64_t token_id = 0;
  void* napi_env_ptr = nullptr;
  if (!ParseUint32Arg(env, args[0], &token_id) ||
      !ParseEnvArrayArg(env, args[1], &napi_env_ptr)) {
    return undefined_val;
  }
  lynx::explorer::LynxNodeAPI::GetInstance().PutEnvByToken(token_id,
                                                           napi_env_ptr);
  return undefined_val;
}

static napi_value RemoveEnvByToken(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  napi_value undefined_val;
  napi_get_undefined(env, &undefined_val);
  if (argc < 1) return undefined_val;

  uint64_t token_id = 0;
  if (!ParseUint32Arg(env, args[0], &token_id)) {
    return undefined_val;
  }
  lynx::explorer::LynxNodeAPI::GetInstance().RemoveEnvByToken(token_id);
  return undefined_val;
}

static napi_value RequireNodeAddonByToken(napi_env env,
                                          napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  napi_value undefined_val;
  napi_get_undefined(env, &undefined_val);
  if (argc < 2) return undefined_val;

  uint64_t token_id = 0;
  std::string addon_name;
  if (!ParseUint32Arg(env, args[0], &token_id) ||
      !ParseStringArg(env, args[1], &addon_name)) {
    return undefined_val;
  }
  lynx::explorer::LynxNodeAPI::GetInstance().RequireNodeAddonByToken(
      token_id, addon_name);
  return undefined_val;
}

}  // namespace

extern "C" {
static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"requireNodeAddon", nullptr, RequireNodeAddon, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"putEnvByToken", nullptr, PutEnvByToken, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"removeEnvByToken", nullptr, RemoveEnvByToken, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"requireNodeAddonByToken", nullptr, RequireNodeAddonByToken, nullptr,
       nullptr, nullptr, napi_default, nullptr}};
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  return exports;
}
}

static napi_module lynxNodeAPIModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "lynx_napi_addon_loader",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterLynxNodeAPIModule(void) {
  napi_module_register(&lynxNodeAPIModule);
}
