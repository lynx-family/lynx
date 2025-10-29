// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/public/list_container_proxy.h"
namespace lynx {
namespace shell {
void ListContainerProxy::ScrollByListContainer(int32_t tag, float offset_x,
                                               float offset_y, float original_x,
                                               float original_y) {
  if (list_engine_proxy_) {
    list_engine_proxy_->ScrollByListContainer(tag, offset_x, offset_y,
                                              original_x, original_y);
  }
}

void ListContainerProxy::ScrollToPosition(int32_t tag, int index, float offset,
                                          int align, bool smooth) {
  if (list_engine_proxy_) {
    list_engine_proxy_->ScrollToPosition(tag, index, offset, align, smooth);
  }
}

void ListContainerProxy::ScrollStopped(int32_t tag) {
  if (list_engine_proxy_) {
    list_engine_proxy_->ScrollStopped(tag);
  }
}

}  // namespace shell

}  // namespace lynx
