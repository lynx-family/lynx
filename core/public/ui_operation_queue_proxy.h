// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_UI_OPERATION_QUEUE_PROXY_H_
#define CORE_PUBLIC_UI_OPERATION_QUEUE_PROXY_H_

// TODO(chenyouhui): Remove this interface after migrating all Enqueue logic to
// PaintingContext

#include "base/include/closure.h"

namespace lynx {
namespace shell {

class UIOperationQueueProxy {
 public:
  virtual ~UIOperationQueueProxy() = default;
  virtual void EnqueueUIOperation(base::closure operation) = 0;
  virtual void Flush() = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_UI_OPERATION_QUEUE_PROXY_H_
