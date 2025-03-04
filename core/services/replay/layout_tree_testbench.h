// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_LAYOUT_TREE_TESTBENCH_H_
#define CORE_SERVICES_REPLAY_LAYOUT_TREE_TESTBENCH_H_

#include <string>

#include "core/renderer/starlight/layout/layout_object.h"

namespace lynx {
namespace tasm {
namespace replay {

class LayoutTreeTestBench {
 public:
  static std::string GetLayoutTree(SLNode* slnode);
};

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_LAYOUT_TREE_TESTBENCH_H_
