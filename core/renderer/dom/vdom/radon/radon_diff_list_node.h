// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_DIFF_LIST_NODE_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_DIFF_LIST_NODE_H_

#include <memory>

#include "core/renderer/dom/vdom/radon/radon_list_base.h"

namespace lynx {
namespace tasm {

class RadonComponent;
class TemplateAssembler;
class RadonDiffListNode : public RadonListBase {
 public:
  // called by lepus function _CreateVirtualListNode
  RadonDiffListNode(lepus::Context* context, PageProxy* page_proxy,
                    TemplateAssembler* tasm, uint32_t node_index);

 protected:
  void DispatchFirstTime() override;
  void RadonDiffChildren(const std::unique_ptr<RadonBase>& old_radon_child,
                         const DispatchOption& option) override;
  bool ShouldFlush(const std::unique_ptr<RadonBase>& old_radon_child,
                   const DispatchOption& option) override;

 private:
  void SyncComponentExtraInfo(RadonComponent* comp, uint32_t index,
                              int64_t operation_id) override;
  // Option Handler
  // The databinding process of list sub-component is triggered by platform
  // list, hence we need to store some dispatch_option in the
  // list_component_info when we update the list. After the platform notify
  // radon to update the sub-component, we can reuse these dispatchOptions.
  void TransmitDispatchOptionFromOldComponentToNewComponent(
      const ListComponentInfo& old_component, ListComponentInfo& new_component);
  void TransmitDispatchOptionFromListNodeToListComponent(
      const DispatchOption& option);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_DIFF_LIST_NODE_H_
