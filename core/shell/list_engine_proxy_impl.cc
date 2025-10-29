// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/list_engine_proxy_impl.h"

namespace lynx {
namespace shell {
void ListEngineProxyImpl::ScrollByListContainer(int32_t tag, float offset_x,
                                                float offset_y,
                                                float original_x,
                                                float original_y) {
  if (auto engine_actor = engine_actor_.lock()) {
    engine_actor->Act(
        [tag, offset_x, offset_y, original_x, original_y](auto& engine) {
          engine->ScrollByListContainer(tag, offset_x, offset_y, original_x,
                                        original_y);
        });
  }
}

void ListEngineProxyImpl::ScrollToPosition(int32_t tag, int index, float offset,
                                           int align, bool smooth) {
  if (auto engine_actor = engine_actor_.lock()) {
    engine_actor->Act([tag, index, offset, align, smooth](auto& engine) {
      engine->ScrollToPosition(tag, index, offset, align, smooth);
    });
  }
}

void ListEngineProxyImpl::ScrollStopped(int32_t tag) {
  if (auto engine_actor = engine_actor_.lock()) {
    engine_actor->Act([tag](auto& engine) { engine->ScrollStopped(tag); });
  }
}
}  // namespace shell

}  // namespace lynx
