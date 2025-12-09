// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_GROUP_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_GROUP_PRIV_H_

#include <string>
#include <vector>

#include "platform/embedder/public/capi/lynx_group_capi.h"

struct lynx_group_t {
  std::string name;
  std::string id;
  std::vector<std::string> preload_js_paths;
  bool enable_js_group_thread = false;
};

#endif  // PLATFORM_EMBEDDER_LYNX_GROUP_PRIV_H_
