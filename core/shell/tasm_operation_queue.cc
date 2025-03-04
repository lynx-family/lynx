// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/tasm_operation_queue.h"

#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace shell {

// TODO(heshan):support base::OperationQueue, which can be used by
// TASMOperationQueue, UIOperationQueue, cached_tasks_ of LynxRuntime, etc.
void TASMOperationQueue::EnqueueOperation(TASMOperation operation) {
  operations_.emplace_back(std::move(operation));
}

void TASMOperationQueue::EnqueueTrivialOperation(TASMOperation operation) {
  operations_.emplace_back(std::move(operation), true);
}

bool TASMOperationQueue::Flush() {
  bool result = false;

  if (operations_.empty()) {
    return result;
  }

  auto operations = std::move(operations_);
  operations_.reserve(kOperationArrayReserveSize);

  for (auto& [operation, is_trivial] : operations) {
    operation();
    result |= (!is_trivial);
  }
  return result;
}

}  // namespace shell
}  // namespace lynx
