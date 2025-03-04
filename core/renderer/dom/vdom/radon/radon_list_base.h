// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_LIST_BASE_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_LIST_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

class RadonComponent;
class TemplateAssembler;
class RadonListBase : public ListNode, public RadonNode {
 public:
  RadonListBase(lepus::Context* context, PageProxy* page_proxy,
                TemplateAssembler* tasm, uint32_t node_index);
  RadonListBase(const RadonListBase& node, PtrLookupMap& map);
  // called by lepus function _AppendRadonListComponentInfo
  void AppendComponentInfo(std::unique_ptr<ListComponentInfo> info) override;
  void RemoveComponent(uint32_t sign) override;
  void RenderComponentAtIndex(uint32_t row, int64_t operationId = 0) override;
  void UpdateComponent(uint32_t sign, uint32_t row,
                       int64_t operationId = 0) override;
  virtual bool DisablePlatformImplementation() { return false; }

 protected:
  lepus::Context* context_;
  TemplateAssembler* tasm_;
  std::vector<std::unique_ptr<ListComponentInfo>> new_components_;
  void DispatchFirstTime() override;
  bool DiffListComponents();
  virtual void SyncComponentExtraInfo(RadonComponent* comp, uint32_t index,
                                      int64_t operation_id);
  RadonComponent* CreateComponentWithType(uint32_t index);

 private:
  bool HasComponent(const std::string& component_name,
                    const std::string& current_entry) override;
  RadonComponent* GetComponent(uint32_t sign);

  bool IsStaticComponent(const std::string& name);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_LIST_BASE_H_
