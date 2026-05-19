// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_

#include <memory>
#include <optional>
#include <utility>

#include "core/base/thread/once_task.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/generated_elements_result.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class TemplateEntry;

class TemplateElement : public FiberElement {
 public:
  explicit TemplateElement(ElementManager* element_manager = nullptr);
  ~TemplateElement() override;

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new TemplateElement(*this, clone_resolved_props));
  }

  bool is_template() const override { return true; }
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
  void SetAttributeSlots(const lepus::Value& attribute_slots) {
    attribute_slots_ = attribute_slots;
  }
  void SetElementSlots(const lepus::Value& element_slots);
  void SetUid(const lepus::Value& uid) { uid_ = uid; }

  void PrepareAsyncCreateElementTree();
  fml::RefPtr<FiberElement> GetRoot();
  lepus::Value Serialize() const;
  void SetAttributeSlot(uint32_t slot_index, const lepus::Value& value);
  void InsertElementSlotChild(uint32_t slot_index,
                              const fml::RefPtr<FiberElement>& child,
                              const fml::RefPtr<FiberElement>& ref_node);
  void RemoveElementSlotChild(uint32_t slot_index,
                              const fml::RefPtr<FiberElement>& child);

 private:
  struct PendingOperation {
    enum class Type {
      kSetAttributeSlot,
      kInsertElementSlotChild,
      kRemoveElementSlotChild,
    };

    PendingOperation(Type type, uint32_t slot_index, lepus::Value value)
        : type_(type), slot_index_(slot_index), value_(std::move(value)) {}
    PendingOperation(Type type, uint32_t slot_index,
                     fml::RefPtr<FiberElement> child,
                     fml::RefPtr<FiberElement> ref_node = nullptr)
        : type_(type),
          slot_index_(slot_index),
          child_(std::move(child)),
          ref_node_(std::move(ref_node)) {}

    Type type_{Type::kSetAttributeSlot};
    uint32_t slot_index_{0};
    lepus::Value value_;
    fml::RefPtr<FiberElement> child_;
    fml::RefPtr<FiberElement> ref_node_;
  };

  struct PreparedCachedTemplateElementTree {
    GeneratedElementsResult generated_;
    lepus::Value applied_attribute_slots_;
  };

  TemplateElement(const TemplateElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props),
        tasm_(element.tasm_),
        entry_(element.entry_),
        template_key_(element.template_key_),
        bundle_url_(element.bundle_url_),
        typed_tag_(element.typed_tag_),
        root_attributes_(element.root_attributes_),
        attribute_slots_(element.attribute_slots_),
        element_slots_(element.element_slots_),
        uid_(element.uid_) {}

  // Builds a once task that may run either on the concurrent loop or on the
  // GetRoot caller. The task returns detached data; GetRoot is the only place
  // that moves the result back onto TemplateElement and initializes the tree.
  base::OnceTaskRefptr<GeneratedElementsResult>
  CreateAsyncCreateElementTreeTask(TemplateEntry* entry);
  void ResolveGeneratedElements();
  void InitGeneratedElementTree();
  void ApplyAttributeSlotToTarget(uint32_t slot_index,
                                  const lepus::Value& previous_attribute_slots);
  void InitTypedRoot();
  bool IsPageTemplate() const;
  void MarkInTemplateTreeAndPrepare();
  void MarkInTemplateTreeAndPrepareRecursively();
  void MarkTemplateChildrenInElementSlotsInTree();
  bool TryPrepareFromCache();
  void ConsumePreparedCachedTree();
  bool MoveElementTreeToCacheIfNeeded();
  void DetachElementSlotChildrenForCacheRecursively();
  void DetachAndMaybeCacheElementSlotChild(
      const ElementSlotMountPoint& mount_point,
      const fml::RefPtr<FiberElement>& child);
  void ApplyAttributeSlotsFromPrevious(
      const lepus::Value& previous_attribute_slots);
  void ApplyRootAttributes(const lepus::Value& previous_root_attributes);
  void ApplyInitialElementSlots();
  void ApplyPendingOperations();
  void InsertInitialElementSlotChild(const ElementSlotMountPoint& mount_point,
                                     const fml::RefPtr<FiberElement>& child);
  void MountElementSlotChild(const ElementSlotMountPoint& mount_point,
                             const fml::RefPtr<FiberElement>& child,
                             const fml::RefPtr<FiberElement>& ref_node);
  void UnmountElementSlotChild(const ElementSlotMountPoint& mount_point,
                               const fml::RefPtr<FiberElement>& child);
  lepus::Value GetOrCreateElementSlotChildren(uint32_t slot_index);
  void RemoveElementSlotChildFromSlot(uint32_t slot_index, FiberElement* child);
  lepus::Value SerializeElementSlots() const;
  lepus::Value SerializeRootAttributes() const;
  lepus::Value SerializeTypedTemplate() const;
  lepus::Value SerializeCompiledTemplate() const;
  lepus::Value SerializeElementSlotChildren(
      const lepus::Value& slot_children) const;
  lepus::Value SerializeElementSlotChild(const lepus::Value& child) const;

  TemplateAssembler* tasm_{nullptr};
  TemplateEntry* entry_{nullptr};
  base::String template_key_;
  base::String bundle_url_;
  base::String typed_tag_;
  lepus::Value root_attributes_;
  lepus::Value attribute_slots_;
  lepus::Value element_slots_;
  lepus::Value uid_;
  fml::RefPtr<FiberElement> result_{nullptr};
  base::Vector<fml::RefPtr<FiberElement>> attribute_slot_targets_;
  base::Vector<fml::RefPtr<FiberElement>> static_event_targets_;
  base::Vector<ElementSlotMountPoint> element_slot_targets_;
  base::Vector<PreparedElementSlotInsertion> prepared_element_slot_insertions_;
  base::Vector<PendingOperation> pending_operations_;
  std::optional<PreparedCachedTemplateElementTree> prepared_cached_tree_;
  base::OnceTaskRefptr<GeneratedElementsResult> async_create_task_{nullptr};
  bool is_in_template_tree_{false};
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_
