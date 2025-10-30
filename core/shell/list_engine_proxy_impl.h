// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LIST_ENGINE_PROXY_IMPL_H_
#define CORE_SHELL_LIST_ENGINE_PROXY_IMPL_H_
#include <memory>

#include "base/include/lynx_actor.h"
#include "core/public/list_engine_proxy.h"
#include "core/shell/lynx_engine.h"

namespace lynx {
namespace shell {
class ListEngineProxyImpl : public ListEngineProxy {
 public:
  explicit ListEngineProxyImpl(
      const std::shared_ptr<LynxActor<shell::LynxEngine>>& engine_actor)
      : engine_actor_(engine_actor){};
  virtual ~ListEngineProxyImpl() = default;

  void ScrollByListContainer(int32_t tag, float offset_x, float offset_y,
                             float original_x, float original_y) override;
  void ScrollToPosition(int32_t tag, int index, float offset, int align,
                        bool smooth) override;
  void ScrollStopped(int32_t tag) override;

 private:
  std::weak_ptr<LynxActor<shell::LynxEngine>> engine_actor_;
};
}  // namespace shell

}  // namespace lynx

#endif  // CORE_SHELL_LIST_ENGINE_PROXY_IMPL_H_
