// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LYNX_LAYOUT_PROXY_H_
#define CORE_PUBLIC_LYNX_LAYOUT_PROXY_H_

#include <memory>

#include "base/include/closure.h"

namespace lynx {

namespace shell {

class LynxLayoutProxy {
 public:
  virtual ~LynxLayoutProxy() = default;

  virtual void DispatchTaskToLynxLayout(base::closure task) = 0;
  virtual void TriggerLayout() = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_LYNX_LAYOUT_PROXY_H_
