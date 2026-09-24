// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_

#include <map>
#include <memory>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorPageAgentNG : public CDPDomainAgentBase {
 public:
  explicit InspectorPageAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorPageAgentNG() override;
  // Temporary compatibility entry point for commands migrated in later
  // patches.
  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  // Temporary compatibility type for commands migrated in later patches.
  using LegacyPageAgentMethod = void (InspectorPageAgentNG::*)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  using PageAgentMethod = void (InspectorPageAgentNG::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(Enable);
  DECLARE_DEVTOOL_CDP_METHOD(CanScreencast);
  DECLARE_DEVTOOL_CDP_METHOD(CanEmulate);
  DECLARE_DEVTOOL_CDP_METHOD(GetResourceTree);
  DECLARE_DEVTOOL_CDP_METHOD(GetResourceContent);
  // Temporary legacy handlers retained until the remaining commands migrate.
  void StartScreencast(const std::shared_ptr<MessageSender>& sender,
                       const Json::Value& message);
  void StopScreencast(const std::shared_ptr<MessageSender>& sender,
                      const Json::Value& message);
  void ScreencastFrameAck(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message);
  void Reload(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void Navigate(const std::shared_ptr<MessageSender>& sender,
                const Json::Value& message);

  std::map<std::string, PageAgentMethod> functions_map_;
  // Temporary compatibility map removed after the remaining commands migrate.
  std::map<std::string, LegacyPageAgentMethod> legacy_functions_map_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_
