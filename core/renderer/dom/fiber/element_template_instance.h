// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_
#define CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/vector.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/generated_elements_result.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class TemplateEntry;
class ElementTemplateInstanceSerializer;
class ElementTemplateInstance
    : public lepus::RefCounted,
      public fml::EnableWeakFromThis<ElementTemplateInstance> {
 public:
  enum class FlushPendingChildMountsResult {
    kDoNotRequeue,
    kOutOfScope,
  };

  static fml::RefPtr<ElementTemplateInstance>
  ResolveElementTemplateInstanceValue(const lepus::Value& value);
  static lepus::Value ConvertTemplateValueForStorage(const lepus::Value& value);
  static lepus::Value ConvertTemplateMapForStorage(const lepus::Value& value);
  static bool IsTemplateMapValue(const lepus::Value& value);
  // Finds the logical child's position. Returns the array length to append when
  // the reference node is absent.
  static size_t FindSlotChildIndex(const lepus::Value& slot_children,
                                   const lepus::Value& ref_node);

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
  // Typed-only attributes; compiled instances use attribute slots.
  virtual void SetAttributes(const lepus::Value& attributes);
  void SetAttributeSlots(const lepus::Value& attribute_slots);
  void InitializeChildSlots(const lepus::Value& child_slots);
  void SetOptions(const lepus::Value& options);
  void SetUid(const lepus::Value& uid);

  const base::String& GetTemplateKey() const { return template_key_; }
  const base::String& GetTypedTag() const { return typed_tag_; }
  const base::String& GetBundleUrl() const { return bundle_url_; }
  const lepus::Value& GetUid() const { return uid_; }
  ElementTemplateInstance* GetLogicalParent() const { return logical_parent_; }
  // Provides a consistent template identity for the List data source and reuse
  // pool: tag for typed templates, templateKey for compiled templates, and
  // bundleUrl:templateKey for external bundles.
  base::String GetTemplateType() const;
  lepus::Value GetListItemPlatformInfoFromOptions() const;

  fml::RefPtr<Element> GetRoot();
  // Materializes this instance on demand, then returns the slot target.
  // This does not complete pending parent mounts or layout.
  fml::RefPtr<Element> GetElementByAttrSlot(uint32_t slot_index);
  lepus::Value Serialize() const;

  void SetAttributeSlot(uint32_t slot_index, const lepus::Value& value);
  virtual void InsertNodeIntoChildSlot(uint32_t slot_index,
                                       const lepus::Value& child,
                                       const lepus::Value& ref_node);
  void RemoveNodeFromChildSlot(uint32_t slot_index, const lepus::Value& child);
  bool HasPendingChildMounts() const;
  FlushPendingChildMountsResult FlushPendingChildMounts(Element* flush_root);
  // Checks whether the materialized root is in the flush subtree without
  // triggering materialization.
  bool IsMaterializedRootInFlushScope(Element* flush_root) const;
  // Updates the item's platform information and notifies its logical parent
  // List to record the change in the current batch.
  void SetListItemPlatformInfo(const lepus::Value& platform_info);
  bool IsPendingChildMountsEnqueued() const {
    return pending_child_mounts_enqueued_;
  }
  void SetPendingChildMountsEnqueued(bool enqueued) {
    pending_child_mounts_enqueued_ = enqueued;
  }

 protected:
  // Regular ETs materialize child slots with the parent tree. Lists materialize
  // items on demand when requested by index.
  virtual bool ShouldMaterializeChildSlots() const { return true; }
  // Regular parents need no handling. Lists override this to record changes to
  // their direct child items.
  virtual void OnListItemPlatformInfoChanged(
      const lepus::Value& child, const lepus::Value& platform_info) {}

  // Reads a logical child without creating a slot or triggering
  // materialization.
  fml::RefPtr<ElementTemplateInstance> GetChildInstanceAt(
      uint32_t slot_index, uint32_t child_index) const;
  // Reads the materialized result without triggering subtree materialization.
  fml::RefPtr<Element> PeekMaterializedRoot() const;
  lepus::Value GetOrCreateMutableChildSlot(uint32_t slot_index);
  // The caller validates the arguments. Updates only the logical order and
  // parent-child links, returning the actual insertion position. Regular ETs
  // and Lists share this logic; their insertion entry points handle
  // materialization and mounting.
  size_t InsertLogicalChildIntoSlot(uint32_t slot_index,
                                    const lepus::Value& child,
                                    const lepus::Value& ref_node,
                                    ElementTemplateInstance& child_instance);
  ElementManager* element_manager() const { return element_manager_; }

 private:
  friend class ElementTemplateInstanceSerializer;

  void RequestMaterializationRecursively();
  void EnsureCreateElementTreeTaskScheduled();
  base::OnceTaskRefptr<GeneratedElementsResult> CreateElementTreeTask(
      TemplateEntry* entry);
  void MaterializeRoot();
  void InitGeneratedElementTree(const lepus::Value& prepared_attribute_slots,
                                uint32_t prepared_attribute_slots_generation);
  void InitTypedRoot();
  bool IsMaterialized() const { return result_ != nullptr; }

  void CollectCallbackAttributeSlots(const std::vector<ElementInfo>& elements);
  lepus::Value CopyAttributeSlotForStorage(uint32_t slot_index,
                                           const lepus::Value& value);
  void ApplyAttributeSlotToTarget(uint32_t slot_index,
                                  const lepus::Value& previous_attribute_slots);

  // Child materialization and physical placement.
  void MountInitialChildSlots();
  bool MountChildSlot(uint32_t slot_index, bool resolve_compiled_children);
  void MountMaterializedChildBefore(
      const ElementSlotMountPoint& mount_point,
      const fml::RefPtr<ElementTemplateInstance>& child,
      const fml::RefPtr<Element>& child_root,
      const fml::RefPtr<Element>& insertion_reference);
  void UnmountMaterializedChild(
      uint32_t slot_index, const fml::RefPtr<ElementTemplateInstance>& child);
  fml::RefPtr<Element> FindChildInsertionReference(
      uint32_t slot_index, size_t first_sibling_index) const;

  void SchedulePendingChildMounts();

  bool EraseChildFromSlotStorage(uint32_t slot_index,
                                 const lepus::Value& child);
  void ClearLogicalChildParentLinks();

  ElementManager* element_manager_{nullptr};
  TemplateAssembler* tasm_{nullptr};
  TemplateEntry* entry_{nullptr};
  base::String template_key_;
  base::String bundle_url_;
  base::String typed_tag_;

  // Typed instances store their complete attribute object in slot 0.
  lepus::Value attribute_slots_;
  std::optional<base::InlineVector<uint32_t, 1>> callback_attribute_slots_;
  base::InlineVector<uint32_t, 1> spread_attribute_slots_;
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

  // Pending first mounts owned by ElementManager's scoped weak drain.
  base::Vector<uint32_t> pending_child_mount_slots_;
  bool pending_child_mounts_enqueued_{false};
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_
