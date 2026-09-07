// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/ui_delegate_clay.h"

namespace lynx::tasm {

std::string UIDelegateClay::GetLynxUITree() { return {}; }

std::string UIDelegateClay::GetUINodeInfo(int) { return {}; }

int UIDelegateClay::SetUIStyle(int, const std::string&, const std::string&) {
  return 0;
}

}  // namespace lynx::tasm
