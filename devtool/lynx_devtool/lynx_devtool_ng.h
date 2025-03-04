// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_LYNX_DEVTOOL_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_LYNX_DEVTOOL_NG_H_

#include <memory>
#include <string>

#include "core/inspector/console_message_postman.h"
#include "devtool/base_devtool/native/public/abstract_devtool.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class LynxDevToolNG : public lynx::devtool::AbstractDevTool,
                      public std::enable_shared_from_this<LynxDevToolNG> {
 public:
  LynxDevToolNG();
  ~LynxDevToolNG() override;

  int32_t Attach(const std::string& url) override;

  void SendMessageToDebugPlatform(const std::string& type,
                                  const std::string& message);

  void OnTasmCreated(intptr_t shell_ptr);

  void SetDevToolPlatformFacade(
      const std::shared_ptr<DevToolPlatformFacade>& platform_facade);

  std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>
  OnBackgroundRuntimeCreated(const std::string& group_thread_name);

  virtual std::shared_ptr<MessageSender> GetMessageSender() const;

 protected:
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;

 private:
  static void RegisterGlobalDomainAgents(
      DevToolMessageDispatcher& global_dispatcher);
  static void RegisterGlobalDomainAgents(
      DevToolMessageDispatcher& global_dispatcher,
      const std::string& domain_key);
  void RegisterInstanceDomainAgents();
  void RegisterInstanceDomainAgents(const std::string& domain_key);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_LYNX_DEVTOOL_NG_H_
