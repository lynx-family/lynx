// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_
#define CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_

#include <cstddef>
#include <utility>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/generated_elements_result.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class TemplateEntry;
class ElementTemplateInstanceSerializer;
class ElementTemplateInstance;

class ElementTemplateInstance
    : public lepus::RefCounted,
      public fml::EnableWeakFromThis<ElementTemplateInstance> {
 public:
  explicit ElementTemplateInstance(ElementManager* element_manager);
  ~ElementTemplateInstance() override;

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kElementTemplate;
  }

  // Template identity and logical inputs.
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

  // Materialized root access and logical-state serialization.
  fml::RefPtr<Element> GetRoot();
  lepus::Value Serialize() const;

  // Incremental logical VDOM mutations.
  void SetAttributeSlot(uint32_t slot_index, const lepus::Value& value);
  void InsertNodeIntoChildSlot(uint32_t slot_index, const lepus::Value& child,
                               const lepus::Value& ref_node);
  void RemoveNodeFromChildSlot(uint32_t slot_index, const lepus::Value& child);

 private:
  friend class ElementManager;
  friend class ElementTemplateInstanceSerializer;

  // Materialization lifecycle.
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

  // Initial-state application.
  void ApplyInitialRootAttributesAfterAttach(
      const lepus::Value& prepared_root_attributes,
      uint32_t prepared_root_attributes_generation);
  void ApplyAttributeSlotToTarget(uint32_t slot_index,
                                  const lepus::Value& previous_attribute_slots);

  // Child materialization and physical placement.
  void MountInitialChildSlots();
  bool MountChildSlot(uint32_t slot_index, bool resolve_compiled_children);
  void MountMaterializedChildBefore(
      const ChildSlotMountPoint& mount_point,
      const fml::RefPtr<ElementTemplateInstance>& child,
      const fml::RefPtr<Element>& child_root,
      const fml::RefPtr<Element>& insertion_reference);
  void UnmountMaterializedChild(
      uint32_t slot_index, const fml::RefPtr<ElementTemplateInstance>& child);
  fml::RefPtr<Element> FindChildInsertionReference(
      uint32_t slot_index, size_t first_sibling_index) const;

  // Pending first-mount scheduling for unresolved compiled children.
  enum class FlushPendingChildMountsResult {
    kDoNotRequeue,
    kOutOfScope,
  };
  bool HasPendingChildMounts() const;
  void SchedulePendingChildMounts();
  FlushPendingChildMountsResult FlushPendingChildMounts(Element* flush_root);
  bool IsMaterializedRootInFlushScope(Element* flush_root) const;

  // Logical child-slot storage and ownership.
  lepus::Value GetOrCreateMutableChildSlot(uint32_t slot_index);
  bool EraseChildFromSlotStorage(uint32_t slot_index,
                                 const lepus::Value& child);
  void ClearLogicalChildParentLinks();

  // Template identity and external context.
  ElementManager* element_manager_{nullptr};
  TemplateAssembler* tasm_{nullptr};
  TemplateEntry* entry_{nullptr};
  base::String template_key_;
  base::String bundle_url_;
  base::String typed_tag_;

  // Current logical VDOM state and ownership.
  lepus::Value root_attributes_;
  uint32_t root_attributes_generation_{0};
  lepus::Value attribute_slots_;
  uint32_t attribute_slots_generation_{0};
  lepus::Value child_slots_;
  lepus::Value options_;
  lepus::Value uid_;
  ElementTemplateInstance* logical_parent_{nullptr};
  uint32_t logical_parent_slot_index_{0};

  // Materialization lifecycle and generated-tree topology.
  bool materialization_requested_{false};
  base::OnceTaskRefptr<GeneratedElementsResult> create_element_tree_task_{
      nullptr};
  fml::RefPtr<Element> result_{nullptr};
  base::Vector<fml::RefPtr<Element>> attribute_slot_targets_;
  base::Vector<fml::RefPtr<Element>> event_attribute_slot_targets_;
  base::Vector<fml::RefPtr<Element>> static_event_targets_;
  base::Vector<ChildSlotMountPoint> child_slot_targets_;

  // Pending first mounts owned by ElementManager's scoped weak drain.
  base::Vector<uint32_t> pending_child_mount_slots_;
  bool pending_child_mounts_enqueued_{false};
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_
