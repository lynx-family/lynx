// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_ENGINE_WRAPPER_H_
#define CORE_SHELL_LYNX_ENGINE_WRAPPER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/log/log_context.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "core/shell/layout_mediator.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/tasm_mediator.h"

namespace lynx {
namespace shell {

class LynxEngineWrapper {
 public:
  LynxEngineWrapper() = default;
  ~LynxEngineWrapper() = default;

  void SetupCore(
      const base::LogContext& log_context,
      const std::shared_ptr<lynx::shell::LynxActor<lynx::shell::LynxEngine>>&
          engine_actor_,
      const std::shared_ptr<lynx::shell::LynxActor<lynx::tasm::LayoutContext>>&
          layout_actor_,
      TasmMediator* tasm_mediator, LayoutMediator* layout_mediator);

  void BindShell(LynxShell* shell);

  void DetachEngine();

  void DestroyEngine();

  bool HasInit() { return has_init_; }

 private:
  bool has_init_{false};
  std::shared_ptr<lynx::shell::LynxActor<lynx::shell::LynxEngine>>
      engine_actor_;
  std::shared_ptr<lynx::shell::LynxActor<lynx::tasm::LayoutContext>>
      layout_actor_;
  TasmMediator* tasm_mediator_{nullptr};
  LayoutMediator* layout_mediator_{nullptr};
  base::LogContext engine_context_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_ENGINE_WRAPPER_H_
