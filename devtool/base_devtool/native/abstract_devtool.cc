// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/public/abstract_devtool.h"

#include <utility>

#include "base/include/no_destructor.h"
#include "devtool/base_devtool/native/global_message_dispatcher.h"
#include "devtool/base_devtool/native/view_message_dispatcher.h"

namespace lynx {
namespace devtool {

DevToolMessageDispatcher&
AbstractDevTool::GetGlobalMessageDispatcherInstance() {
  static lynx::base::NoDestructor<std::shared_ptr<DevToolMessageDispatcher>>
      global_message_dispatcher(GlobalMessageDispatcher::Create());
  return **global_message_dispatcher;
}

class AbstractDevTool::Impl {
 public:
  /**
   *  slot_agent_ is for handling view-messages based on debugrouter
   */
  std::shared_ptr<ViewMessageDispatcher> slot_agent_;
};

AbstractDevTool::AbstractDevTool() : impl_(new Impl()) {
  impl_->slot_agent_ = ViewMessageDispatcher::Create();
}

AbstractDevTool::~AbstractDevTool() { delete impl_; }
int32_t AbstractDevTool::Attach(const std::string& url) {
  return impl_->slot_agent_->Attach(url);
}
void AbstractDevTool::Detach() { impl_->slot_agent_->Detach(); }

void AbstractDevTool::RegisterAgent(
    const std::string& agent_name,
    std::unique_ptr<CDPDomainAgentBase>&& agent) {
  impl_->slot_agent_->RegisterAgent(agent_name, std::move(agent));
}

void AbstractDevTool::RegisterMessageHandler(
    const std::string& type, std::unique_ptr<DevToolMessageHandler>&& handler) {
  impl_->slot_agent_->RegisterMessageHandler(type, std::move(handler));
}

void AbstractDevTool::DispatchMessage(
    const std::shared_ptr<MessageSender>& sender, const std::string& type,
    const std::string& msg) {
  impl_->slot_agent_->DispatchMessage(sender, type, msg);
}

CDPDomainAgentBase* AbstractDevTool::GetAgent(
    const std::string& agent_name) const {
  return impl_->slot_agent_->GetAgent(agent_name);
}

void AbstractDevTool::SubscribeMessage(
    const std::string& type, std::unique_ptr<DevToolMessageHandler>&& handler) {
  impl_->slot_agent_->SubscribeMessage(type, std::move(handler));
}
void AbstractDevTool::UnSubscribeMessage(const std::string& type) {
  impl_->slot_agent_->UnSubscribeMessage(type);
}

std::shared_ptr<MessageSender> AbstractDevTool::GetCurrentSender() const {
  return impl_->slot_agent_->GetSender();
}

}  // namespace devtool
}  // namespace lynx
