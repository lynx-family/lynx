// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_group_priv.h"

namespace {
std::string GenerateID() {
  static int32_t id = 0;
  return std::to_string(id++);
}
}  // namespace

LYNX_EXTERN_C lynx_group_t* lynx_group_create(const char* name) {
  lynx_group_t* group = new lynx_group_t();
  group->name = name ? name : "";
  group->id = GenerateID();
  return group;
}

LYNX_EXTERN_C lynx_group_t* lynx_group_create_with_id(const char* name,
                                                      const char* id) {
  lynx_group_t* group = new lynx_group_t();
  group->name = name ? name : "";
  group->id = id ? id : GenerateID();
  return group;
}

LYNX_EXTERN_C void lynx_group_set_preload_js_paths(lynx_group_t* group,
                                                   const char* paths[],
                                                   size_t size) {
  group->preload_js_paths.clear();
  for (size_t i = 0; i < size; i++) {
    if (!paths[i]) {
      continue;
    }
    group->preload_js_paths.push_back(paths[i]);
  }
}

LYNX_EXTERN_C void lynx_group_set_enable_js_group_thread(lynx_group_t* group,
                                                         int enable) {
  group->enable_js_group_thread = enable;
}

LYNX_EXTERN_C void lynx_group_release(lynx_group_t* group) { delete group; }
