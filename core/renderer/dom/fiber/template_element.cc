// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/template_element.h"

#include "base/include/value/base_value.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"

namespace lynx {
namespace tasm {

SlotElement::SlotElement(ElementManager* element_manager)
    : FiberElement(element_manager, "view") {}

SlotElement::~SlotElement() {}

void SlotElement::HandleInsert(const fml::RefPtr<FiberElement>& node,
                               const fml::RefPtr<FiberElement>& ref) {
  if (node == nullptr) {
    return;
  }

  if (dangling_parent_ == nullptr) {
    dangling_parent_ = static_cast<FiberElement*>(parent());
    parent()->RemoveNode(fml::RefPtr<FiberElement>(this));
  }

  if (ref == nullptr) {
    if (after_ == nullptr) {
      dangling_parent_->InsertNode(node);
    } else {
      dangling_parent_->InsertNodeBefore(node,
                                         fml::RefPtr<FiberElement>(after_));
    }
    return;
  }

  dangling_parent_->InsertNodeBefore(node, ref);
}

void SlotElement::HandleRemove(const fml::RefPtr<FiberElement>& node) {
  if (node == nullptr) {
    return;
  }

  if (dangling_parent_ == nullptr) {
    dangling_parent_ = static_cast<FiberElement*>(parent());
    parent()->RemoveNode(fml::RefPtr<FiberElement>(this));
  }

  dangling_parent_->RemoveNode(node);
}

TemplateElement::TemplateElement(ElementManager* element_manager)
    : FiberElement(element_manager, "template") {}

TemplateElement::~TemplateElement() = default;

void TemplateElement::AsyncCreateElementTree() {
  entry_ = tasm_->FindEntry(bundle_url_.str()).get();
  async_task_ = entry_->GenerateElements(template_key_.str(),
                                         element_manager_->root()->impl_id(),
                                         element_manager());
}

fml::RefPtr<FiberElement> TemplateElement::GetRoot() {
  if (async_task_ != nullptr) {
    async_task_->Run();

    result_ = async_task_->GetFuture().get();
    root_ = result_[0];
    TreeResolver::InitElementTree(root_, element_manager()->root()->impl_id(),
                                  element_manager(),
                                  entry_->GetStyleSheetManager());

    if (!op_codes_.IsEmpty()) {
      for (int32_t index = 0; index < op_codes_.GetLength();) {
        auto op = op_codes_.GetProperty(index++).Number();
        auto part_id = op_codes_.GetProperty(index++).Number();
        if (op == 1) {
          auto key = op_codes_.GetProperty(index++).String();
          result_[part_id + 1]->SetAttribute(key,
                                             op_codes_.GetProperty(index++));
        } else if (op == 2) {
          auto ref = op_codes_.GetProperty(index++);
          auto node = op_codes_.GetProperty(index++);
          static_cast<SlotElement*>(result_[part_id + 1].get())
              ->HandleInsert(
                  node.IsRefCounted() ? fml::static_ref_ptr_cast<FiberElement>(
                                            node.RefCounted())
                                      : nullptr,
                  ref.IsReference()
                      ? fml::static_ref_ptr_cast<FiberElement>(ref.RefCounted())
                      : nullptr);
        } else if (op == 3) {
          auto node = op_codes_.GetProperty(index++);
          static_cast<SlotElement*>(result_[part_id + 1].get())
              ->HandleRemove(node.IsRefCounted()
                                 ? fml::static_ref_ptr_cast<FiberElement>(
                                       node.RefCounted())
                                 : nullptr);
        } else if (op == 4) {
          auto map = op_codes_.GetProperty(index++);
          tasm::ForEachLepusValue(
              map, [element = result_[part_id + 1].get()](
                       const auto& key, const auto& attrValue) {
                element->SetAttribute(key.String(), attrValue);
              });
        }
      }
    }

    // Call manager->PrepareNodeForInspector to init inspector attr for the
    // element tree.
    EXEC_EXPR_FOR_INSPECTOR(
        auto* manager = element_manager();
        if (manager->GetDevToolFlag() && manager->IsDomTreeEnabled()) {
          std::function<void(FiberElement*)> prepare_node_f =
              [manager, &prepare_node_f](const auto& element) {
                manager->PrepareNodeForInspector(element);
                for (const auto& child : element->children()) {
                  prepare_node_f(child.get());
                }
              };
          prepare_node_f(root_.get());
        });

    async_task_ = nullptr;
  }
  return root_;
}

}  // namespace tasm
}  // namespace lynx
