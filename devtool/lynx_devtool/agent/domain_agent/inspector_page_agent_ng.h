// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_

#include <memory>
#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class InspectorPageAgentNG : public CDPDomainAgentBase {
 public:
  explicit InspectorPageAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorPageAgentNG();
  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;

 private:
  typedef void (InspectorPageAgentNG::*PageAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  void Enable(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void CanScreencast(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  void CanEmulate(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message);
  void GetResourceTree(const std::shared_ptr<MessageSender>& sender,
                       const Json::Value& message);
  void GetResourceContent(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message);
  void GetNavigationHistory(const std::shared_ptr<MessageSender>& sender,
                            const Json::Value& message);
  void SetShowViewportSizeOnResize(const std::shared_ptr<MessageSender>& sender,
                                   const Json::Value& message);
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
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_
