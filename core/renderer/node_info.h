// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_NODE_INFO_H_
#define CORE_RENDERER_NODE_INFO_H_

#include <cstdint>
#include <string>

namespace lynx {
namespace tasm {

struct NodeInfo {
  int32_t sign{0};
  // The template entry associated with the node. DEFAULT_ENTRY_NAME maps to
  // the main page template.
  std::string entry_url;
  // The debug metadata URL resolved from the matched template config.
  std::string debug_metadata_url;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_NODE_INFO_H_
