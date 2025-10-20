

// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LIST_CONTAINER_PROXY_H_
#define CORE_SHELL_LIST_CONTAINER_PROXY_H_

#include "core/shell/lynx_engine.h"
#include "base/include/lynx_actor.h"
#include "core/shell/list_engine_proxy.h"

namespace lynx {
 namespace shell { 

class ListContainerProxy {
 public:
  explicit ListContainerProxy(const std::shared_ptr<ListEngineProxy> & list_engine_proxy): list_engine_proxy_(list_engine_proxy){};

    void ScrollByListContainer(int32_t tag, float offset_x, float offset_y,
                               float original_x, float original_y);

    void ScrollToPosition(int32_t tag, int index, float offset, int align,
                          bool smooth);

    void ScrollStopped(int32_t tag); 
  
    // ListEngineProxy shared_ptr
    std::shared_ptr<ListEngineProxy> GetListEngineProxy() const {
        return list_engine_proxy_;
    }

private:
    std::shared_ptr<ListEngineProxy> list_engine_proxy_;

};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LIST_CONTAINER_PROXY_H_
