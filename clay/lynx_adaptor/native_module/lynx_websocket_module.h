// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_WEBSOCKET_MODULE_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_WEBSOCKET_MODULE_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "clay/lynx_adaptor/native_module/lynx_module_base.h"

namespace lynx {

class SimpleWebSocket;

class LynxWebSocketModule
    : public LynxModuleBase,
      public std::enable_shared_from_this<LynxWebSocketModule> {
 public:
  static std::shared_ptr<LynxNativeModule> Create(
      uint32_t view_context_id, fml::RefPtr<fml::TaskRunner> task_runner) {
    return std::make_shared<LynxWebSocketModule>(view_context_id, task_runner);
  }

  static const std::string& GetName() { return name_; }

  LynxWebSocketModule(uint32_t view_context_id,
                      fml::RefPtr<fml::TaskRunner> task_runner);
  ~LynxWebSocketModule() override;

  std::unique_ptr<pub::Value> connect(std::unique_ptr<pub::Value> args,
                                      const runtime::CallbackMap& callbacks);

  std::unique_ptr<pub::Value> send(std::unique_ptr<pub::Value> args,
                                   const runtime::CallbackMap& callbacks);

  std::unique_ptr<pub::Value> ping(std::unique_ptr<pub::Value> args,
                                   const runtime::CallbackMap& callbacks);

  std::unique_ptr<pub::Value> close(std::unique_ptr<pub::Value> args,
                                    const runtime::CallbackMap& callbacks);

 private:
  static const std::string name_;
  std::unordered_map<int, SimpleWebSocket*> sockets_;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_WEBSOCKET_MODULE_H_
