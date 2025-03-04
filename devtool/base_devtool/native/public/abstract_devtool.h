// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_ABSTRACT_DEVTOOL_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_ABSTRACT_DEVTOOL_H_
#include <cstdint>
#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/base_devtool_export.h"
#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"

namespace lynx {
namespace devtool {

/**
 * Entry of devtool, A view instance has an AbstractDevTool instance
 *
 * You can inherit this class to implement your own devtool.
 * AbstractDevTool provides default messaging capabilities based on DebugRouter,
 * including the distribution global debug messages (message's session-id = -1)
 * and view-specific debug messages(message's session-id > 0) from
 * debug-platform, as well as sending messages to debug-platform via
 * DebugRouter.
 *
 */
class BASE_DEVTOOL_EXPORT AbstractDevTool {
 public:
  AbstractDevTool();
  virtual ~AbstractDevTool();

  /**
   * notify devtool that view will start
   */
  virtual int32_t Attach(const std::string& url);

  /**
   * notify devtool that view destroyed
   */
  void Detach();

  // In most cases, you don't need to directly use this method. Instead,
  // you have a custom sender to retrieve the execution results of debugging
  // messages.
  void DispatchMessage(const std::shared_ptr<MessageSender>& sender,
                       const std::string& type, const std::string& msg);

  // This interface can subscribe to all messages, including CDP and non-CDP.
  // The following two interfaces are only used internally, while this interface
  // is used for external tools to listen to devtool messages.
  void SubscribeMessage(const std::string& type,
                        std::unique_ptr<DevToolMessageHandler>&& handler);

  void UnSubscribeMessage(const std::string& type);

  // Get message sender, which will be used during dispatching message.
  std::shared_ptr<MessageSender> GetCurrentSender() const;

 protected:
  // for CDP domain agent
  void RegisterAgent(const std::string& agent_name,
                     std::unique_ptr<CDPDomainAgentBase>&& agent);

  // for handling Non-CDP message
  void RegisterMessageHandler(const std::string& type,
                              std::unique_ptr<DevToolMessageHandler>&& handler);
  /**
   *  global_message_dispatcher is for handling global messages from debugrouter
   */
  static DevToolMessageDispatcher& GetGlobalMessageDispatcherInstance();

  // Get CDPdomain agent
  CDPDomainAgentBase* GetAgent(const std::string& agent_name) const;

 private:
  class Impl;
  Impl* impl_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_ABSTRACT_DEVTOOL_H_
