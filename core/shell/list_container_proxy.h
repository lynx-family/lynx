

// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LIST_CONTAINER_PROXY_H_
#define CORE_SHELL_LIST_CONTAINER_PROXY_H_

#include "core/shell/lynx_engine.h"
#include "base/include/lynx_actor.h"

namespace lynx {
 namespace shell { 

class ListContainerProxy {
 public:
  explicit ListContainerProxy(const std::shared_ptr<LynxActor<shell::LynxEngine>> & engine_actor):engine_actor_(engine_actor){};

  void ScrollByListContainer(int32_t tag, float offset_x, float offset_y,
                             float original_x, float original_y);

  void ScrollToPosition(int32_t tag, int index, float offset, int align,
                        bool smooth);

  void ScrollStopped(int32_t tag);
 
  private:
    std::weak_ptr<LynxActor<shell::LynxEngine>> engine_actor_;

};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LIST_CONTAINER_PROXY_H_
