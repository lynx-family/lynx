// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_SETTING_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_SETTING_AGENT_H_

#include <map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"

namespace lynx {
namespace devtool {

struct LynxSettingRequest;

class InspectorLynxSettingAgent : public CDPDomainAgentBase {
 public:
  InspectorLynxSettingAgent();

  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;

 private:
  using LynxSettingAgentMethod = void (InspectorLynxSettingAgent::*)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);

  void GetValues(const std::shared_ptr<MessageSender>& sender,
                 const Json::Value& message);
  void GetLayeredValues(const std::shared_ptr<MessageSender>& sender,
                        const Json::Value& message);
  void GetValue(const std::shared_ptr<MessageSender>& sender,
                const Json::Value& message);
  void SetMockValue(const std::shared_ptr<MessageSender>& sender,
                    const Json::Value& message);
  void RemoveMockValue(const std::shared_ptr<MessageSender>& sender,
                       const Json::Value& message);
  void ClearMockValues(const std::shared_ptr<MessageSender>& sender,
                       const Json::Value& message);
  void GetFetchInfo(const std::shared_ptr<MessageSender>& sender,
                    const Json::Value& message);
  void FetchLatest(const std::shared_ptr<MessageSender>& sender,
                   const Json::Value& message);

  void HandleRequest(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message, LynxSettingRequest request);
  void HandleKeyRequest(const std::shared_ptr<MessageSender>& sender,
                        const Json::Value& message, const std::string& method,
                        bool requires_value);

  std::map<std::string, LynxSettingAgentMethod> functions_map_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_SETTING_AGENT_H_
