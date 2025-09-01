// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_COMMON_DEBUGGER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_COMMON_DEBUGGER_EMBEDDER_H_

#include <string>
#include <unordered_map>

#include "devtool/embedder/core/debug_bridge_embedder.h"

namespace lynx {
namespace devtool {

// TODO(zuojinglong.9): We may want to come up with a more distinctive name
// for the "Devtools". It feels like we are mixing up several concepts like
// devtool, inspector, debugger, debugbridge, and debugrouter. A more specific
// and descriptive name would help to clarify the purpose and functionality of
// the "Devtools".
class DebuggerEmbedder {
 public:
  static bool ConnectDevtools(
      const std::string& url,
      const std::unordered_map<std::string, std::string>& options);

  static void SetOpenCardCallback(DevtoolsOpenCardCallback callback);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_COMMON_DEBUGGER_EMBEDDER_H_
