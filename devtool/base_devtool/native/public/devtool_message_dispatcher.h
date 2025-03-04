// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_MESSAGE_DISPATCHER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_MESSAGE_DISPATCHER_H_
#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/base_devtool/native/public/base_devtool_export.h"
#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/base_devtool/native/public/devtool_message_handler.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {
static const char kDomainDot = '.';

/*
 * DevToolMessageDispatcher is the base class for handling devtool message.
 * It can be used to register CDP domain agents and message handlers.
 * It can dispatch messages to CDP domain agents or message handlers.
 * In most cases, you don't need to directly use this class. Instead,
 * you can use the two subclasses already implemented in basedevtool.
 * However, if you have specific dispatching rules or require custom
 * ways of receiving and sending messages, you can inherit from this
 * class to implement your own DevToolMessageDispatcher.
 */
class BASE_DEVTOOL_EXPORT DevToolMessageDispatcher
    : public std::enable_shared_from_this<DevToolMessageDispatcher> {
 public:
  DevToolMessageDispatcher() = default;
  virtual ~DevToolMessageDispatcher() = default;

  // When a message arrives, this method can parse the message and dispatch it
  // to the corresponding agent or handler.
  virtual void DispatchMessage(const std::shared_ptr<MessageSender>& sender,
                               const std::string& type, const std::string& msg);
  // for CDP domain agent
  void RegisterAgent(const std::string& agent_name,
                     std::unique_ptr<CDPDomainAgentBase>&& agent);
  // for handling Non-CDP message
  void RegisterMessageHandler(const std::string& type,
                              std::unique_ptr<DevToolMessageHandler>&& handler);

  CDPDomainAgentBase* GetAgent(const std::string& agent_name);

 protected:
  virtual void DispatchJsonMessage(const std::shared_ptr<MessageSender>& sender,
                                   const std::string& type,
                                   const Json::Value& msg);

  virtual void DispatchCDPMessage(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& msg);

  std::unordered_map<std::string, std::unique_ptr<CDPDomainAgentBase>>
      agent_map_;
  std::unordered_map<std::string, std::unique_ptr<DevToolMessageHandler>>
      handler_map_;
};

}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_MESSAGE_DISPATCHER_H_
