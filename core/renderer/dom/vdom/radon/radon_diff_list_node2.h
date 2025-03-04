// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_DIFF_LIST_NODE2_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_DIFF_LIST_NODE2_H_

#include <memory>
#include <optional>
#include <vector>

#include "core/renderer/dom/vdom/radon/radon_list_base.h"

namespace lynx {
namespace tasm {

class RadonComponent;
class TemplateAssembler;
class ListReusePool;

class RadonDiffListNode2 : public RadonListBase {
 public:
  // called by lepus function _CreateVirtualListNode
  RadonDiffListNode2(lepus::Context* context, PageProxy* page_proxy,
                     TemplateAssembler* tasm, uint32_t node_index);

  int32_t ComponentAtIndex(uint32_t index, int64_t operationId,
                           bool enable_reuse_notification) final;
  void EnqueueComponent(int32_t sign) final;
  bool DisablePlatformImplementation() override;

 protected:
  void DispatchFirstTime() override;
  void RadonDiffChildren(const std::unique_ptr<RadonBase>& old_radon_child,
                         const DispatchOption& option) override;
  bool ShouldFlush(const std::unique_ptr<RadonBase>& old_radon_child,
                   const DispatchOption& option) override;

 private:
  void SyncComponentExtraInfo(RadonComponent* comp, uint32_t index,
                              int64_t operation_id) override;
  // Handle a new created component, update the component and then render
  // recursively.
  void UpdateAndRenderNewComponent(RadonComponent* component,
                                   const lepus::Value& incoming_property,
                                   const lepus::Value& incoming_data);
  // Handle a component which created before, handle lifecycle but not element.
  void UpdateOldComponent(RadonComponent* component,
                          ListComponentInfo& component_info);
  // Do extra steps to ensure that any item-key isn't empty or duplicate
  void FilterComponents(
      std::vector<std::unique_ptr<ListComponentInfo>>& components,
      TemplateAssembler* tasm) override;
  void CheckItemKeys(
      std::vector<std::unique_ptr<ListComponentInfo>>& components);

  void SetupListInfo(bool list_updated);

  // New Arch
  std::unique_ptr<ListReusePool> reuse_pool_;

  // Option Handler
  // The databinding process of list sub-component is triggered by platform
  // list, hence we need to store some dispatch_option in the
  // list_component_info when we update the list. After the platform notify
  // radon to update the sub-component, we can reuse these dispatchOptions.
  void TransmitDispatchOptionFromOldComponentToNewComponent(
      const ListComponentInfo& old_component, ListComponentInfo& new_component);
  void TransmitDispatchOptionFromListNodeToListComponent(
      const DispatchOption& option);
  std::optional<bool> disable_platform_implementation_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_DIFF_LIST_NODE2_H_
