// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_
#define CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_

#include "base/include/value/ref_counted_class.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/generated_elements_result.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class TemplateEntry;
class ElementTemplateInstanceSerializer;
class ElementTemplateInstance : public lepus::RefCounted {
 public:
  explicit ElementTemplateInstance(ElementManager* element_manager);
  ~ElementTemplateInstance() override;

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kElementTemplate;
  }

  void SetTASM(TemplateAssembler* tasm) { tasm_ = tasm; }
  void SetTemplateKey(const base::String& template_key) {
    template_key_ = template_key;
  }
  void SetBundleUrl(const base::String& bundle_url) {
    bundle_url_ = bundle_url;
  }
  void SetTypedTag(const base::String& typed_tag);
  bool IsTypedTemplate() const { return !typed_tag_.empty(); }
  void SetRootAttributes(const lepus::Value& attributes);
  void SetAttributeSlots(const lepus::Value& attribute_slots);
  void InitializeChildSlots(const lepus::Value& child_slots);
  void SetOptions(const lepus::Value& options);
  void SetUid(const lepus::Value& uid);

  fml::RefPtr<Element> GetRoot();
  lepus::Value Serialize() const;

  void SetAttributeSlot(uint32_t slot_index, const lepus::Value& value);
  void InsertNodeIntoChildSlot(uint32_t slot_index, const lepus::Value& child,
                               const lepus::Value& ref_node);
  void RemoveNodeFromChildSlot(uint32_t slot_index, const lepus::Value& child);

 private:
  friend class ElementTemplateInstanceSerializer;

  void RequestMaterializationRecursively();
  void EnsureCreateElementTreeTaskScheduled();
  base::OnceTaskRefptr<GeneratedElementsResult> CreateElementTreeTask(
      TemplateEntry* entry);
  void MaterializeRoot();
  void InitGeneratedElementTree(const lepus::Value& prepared_attribute_slots,
                                uint32_t prepared_attribute_slots_generation,
                                const lepus::Value& prepared_root_attributes,
                                uint32_t prepared_root_attributes_generation);
  void InitTypedRoot();
  bool IsMaterialized() const { return result_ != nullptr; }
  fml::RefPtr<Element> PeekMaterializedRoot() const;

  void ApplyAttributeSlotToTarget(uint32_t slot_index,
                                  const lepus::Value& previous_attribute_slots);

  lepus::Value GetOrCreateMutableChildSlot(uint32_t slot_index);
  bool EraseChildFromSlotStorage(uint32_t slot_index,
                                 const lepus::Value& child);
  void ClearLogicalChildParentLinks();

  ElementManager* element_manager_{nullptr};
  TemplateAssembler* tasm_{nullptr};
  TemplateEntry* entry_{nullptr};
  base::String template_key_;
  base::String bundle_url_;
  base::String typed_tag_;

  lepus::Value root_attributes_;
  uint32_t root_attributes_generation_{0};
  lepus::Value attribute_slots_;
  uint32_t attribute_slots_generation_{0};
  lepus::Value child_slots_;
  lepus::Value options_;
  lepus::Value uid_;
  ElementTemplateInstance* logical_parent_{nullptr};
  uint32_t logical_parent_slot_index_{0};

  bool materialization_requested_{false};
  base::OnceTaskRefptr<GeneratedElementsResult> create_element_tree_task_{
      nullptr};
  fml::RefPtr<Element> result_{nullptr};
  base::Vector<fml::RefPtr<Element>> attribute_slot_targets_;
  base::Vector<fml::RefPtr<Element>> event_attribute_slot_targets_;
  base::Vector<fml::RefPtr<Element>> static_event_targets_;
  base::Vector<ElementSlotMountPoint> element_slot_targets_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_
