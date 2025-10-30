// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LIST_ENGINE_PROXY_H_
#define CORE_PUBLIC_LIST_ENGINE_PROXY_H_

#include <memory>
#include <utility>

namespace lynx {
namespace shell {

class ListEngineProxy {
 public:
  virtual void ScrollByListContainer(int32_t tag, float offset_x,
                                     float offset_y, float original_x,
                                     float original_y) = 0;

  virtual void ScrollToPosition(int32_t tag, int index, float offset, int align,
                                bool smooth) = 0;

  virtual void ScrollStopped(int32_t tag) = 0;
};

}  // namespace shell

}  // namespace lynx

#endif  // CORE_PUBLIC_LIST_ENGINE_PROXY_H_
