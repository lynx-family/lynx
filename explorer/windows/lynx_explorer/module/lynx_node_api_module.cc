// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/windows/lynx_explorer/module/lynx_node_api_module.h"

#include <cstdint>
#include <string>

#include "explorer/cpp/LynxNodeAPI.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace {

napi_value RequireNodeAddon(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  void* data = nullptr;
  napi_get_cb_info(env, info, &argc, argv, nullptr, &data);
  if (argc < 1 || data == nullptr) {
    return 0;
  }

  char addon_name[512] = {0};
  napi_get_value_string_utf8(env, argv[0], addon_name, sizeof(addon_name),
                             nullptr);
  uint64_t token_id = reinterpret_cast<uint64_t>(data);
  lynx::explorer::LynxNodeAPI::GetInstance().RequireNodeAddonByToken(
      token_id, std::string(addon_name));
  return 0;
}

}  // namespace

napi_value LynxNodeAPIModuleCreator(napi_env env, napi_value exports,
                                    const char* module_name, void* opaque) {
  napi_value func;
  napi_create_function(env, "requireNodeAddon", NAPI_AUTO_LENGTH,
                       &RequireNodeAddon, opaque, &func);
  napi_set_named_property(env, exports, "requireNodeAddon", func);
  return exports;
}
