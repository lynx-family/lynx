// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_DOM_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_DOM_AGENT_NG_H_

#include <memory>
#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class DevToolAgentNG;

class InspectorDOMAgentNG : public CDPDomainAgentBase {
 public:
  explicit InspectorDOMAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorDOMAgentNG();
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  typedef void (InspectorDOMAgentNG::*DOMAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& params);

  void Enable(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void Disable(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);
  void EnableDomTree(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  void DisableDomTree(const std::shared_ptr<MessageSender>& sender,
                      const Json::Value& message);
  void GetDocument(const std::shared_ptr<MessageSender>& sender,
                   const Json::Value& message);
  void GetDocumentWithBoxModel(const std::shared_ptr<MessageSender>& sender,
                               const Json::Value& message);
  void RequestChildNodes(const std::shared_ptr<MessageSender>& sender,
                         const Json::Value& message);
  void GetBoxModel(const std::shared_ptr<MessageSender>& sender,
                   const Json::Value& message);
  void SetAttributesAsText(const std::shared_ptr<MessageSender>& sender,
                           const Json::Value& message);
  void MarkUndoableState(const std::shared_ptr<MessageSender>& sender,
                         const Json::Value& message);
  void GetNodeForLocation(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message);
  void PushNodesByBackendIdsToFrontend(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  void RemoveNode(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message);
  void CopyTo(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void MoveTo(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void GetOuterHTML(const std::shared_ptr<MessageSender>& sender,
                    const Json::Value& message);
  void SetOuterHTML(const std::shared_ptr<MessageSender>& sender,
                    const Json::Value& message);
  void SetInspectedNode(const std::shared_ptr<MessageSender>& sender,
                        const Json::Value& message);
  void QuerySelector(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  void QuerySelectorAll(const std::shared_ptr<MessageSender>& sender,
                        const Json::Value& message);
  void InnerText(const std::shared_ptr<MessageSender>& sender,
                 const Json::Value& message);
  void GetAttributes(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  Json::Value GetAttributesImpl(const std::shared_ptr<MessageSender>& sender,
                                size_t node_id);
  void PerformSearch(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  void GetSearchResults(const std::shared_ptr<MessageSender>& sender,
                        const Json::Value& message);
  void DiscardSearchResults(const std::shared_ptr<MessageSender>& sender,
                            const Json::Value& message);
  void ScrollIntoViewIfNeeded(const std::shared_ptr<MessageSender>& sender,
                              const Json::Value& message);
  void GetOriginalNodeIndex(const std::shared_ptr<MessageSender>& sender,
                            const Json::Value& message);

  std::unordered_map<uint64_t, std::vector<int>> search_results_;
  std::map<std::string, DOMAgentMethod> functions_map_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_DOM_AGENT_NG_H_
