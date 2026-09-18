// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

// A global CDP message bridge to an externally owned runtime.
class InspectorLynxAgent : public CDPDomainAgentBase {
 public:
  // Called on the CDP dispatch thread. The handler must enqueue work on the
  // runtime's thread and return whether the message was accepted.
  using MessageHandler = std::function<bool(const std::string&)>;

  explicit InspectorLynxAgent(std::shared_ptr<MessageSender> event_sender);

  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

  // Bind the existing runtime here; pass an empty handler to unbind it.
  // Replacing the handler does not cancel a call already in progress.
  void SetMessageHandler(MessageHandler handler);

  // Publish an unsolicited runtime message through the existing global CDP
  // channel. No preceding sendMessage command is required.
  void SendMessageReceived(const std::string& message);

 private:
  using AgentMethod = void (InspectorLynxAgent::*)(
      const std::shared_ptr<CDPResponder>&, const Json::Value&);

  DECLARE_DEVTOOL_CDP_METHOD(SendMessage);

  std::map<std::string, AgentMethod> functions_map_;
  const std::shared_ptr<MessageSender> event_sender_;
  std::mutex handler_mutex_;
  MessageHandler message_handler_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_H_
