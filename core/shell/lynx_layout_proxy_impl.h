// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_LAYOUT_PROXY_IMPL_H_
#define CORE_SHELL_LYNX_LAYOUT_PROXY_IMPL_H_

#include <memory>

#include "base/include/closure.h"
#include "base/include/lynx_actor.h"
#include "core/public/lynx_layout_proxy.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"

namespace lynx {
namespace shell {
class LynxLayoutProxyImpl : public LynxLayoutProxy {
 public:
  explicit LynxLayoutProxyImpl(
      const std::shared_ptr<shell::LynxActor<tasm::LayoutContext>>& actor)
      : layout_actor_(actor) {}
  ~LynxLayoutProxyImpl() override = default;

  void DispatchTaskToLynxLayout(base::closure task) override;
  void TriggerLayout() override;

 private:
  std::shared_ptr<shell::LynxActor<tasm::LayoutContext>> layout_actor_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_LAYOUT_PROXY_IMPL_H_
