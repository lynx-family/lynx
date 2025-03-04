// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/testing/utils.h"

namespace lynx {
namespace tasm {
namespace list {

int64_t GenerateOperationId(int32_t list_id) {
  static int32_t base_operation_id = 0;
  return (static_cast<int64_t>(list_id) << 32) + base_operation_id++;
}

}  // namespace list
}  // namespace tasm
}  // namespace lynx
