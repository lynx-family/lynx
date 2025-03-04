// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_LYNX_ENGINE_PROXY_DARWIN_H_
#define CORE_SHELL_IOS_LYNX_ENGINE_PROXY_DARWIN_H_

#include <memory>
#include <string>

#include "core/shell/lynx_actor_specialization.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/lynx_engine_proxy_impl.h"

namespace lynx {
namespace shell {

class LynxEngineProxyDarwin : public LynxEngineProxyImpl {
 public:
  LynxEngineProxyDarwin(
      std::shared_ptr<shell::LynxActor<shell::LynxEngine>> actor)
      : LynxEngineProxyImpl(actor) {}
  virtual ~LynxEngineProxyDarwin() override = default;
  void InvokeLepusApiCallback(const int32_t callback_id,
                              const std::string& entry_name,
                              const lepus::Value& data);
};
}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_IOS_LYNX_ENGINE_PROXY_DARWIN_H_
