// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_TEMPLATE_INSTANCE_H_
#define CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_TEMPLATE_INSTANCE_H_

#include <cstdint>
#include <memory>

#include "core/renderer/dom/fiber/element_template_instance.h"
#include "core/renderer/dom/fiber/list_element.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class ListUpdateInfoRecorder;

class ListElementTemplateInstance : public ElementTemplateInstance {
 public:
  explicit ListElementTemplateInstance(ElementManager* element_manager);

  ~ListElementTemplateInstance() override;

  // Let the JS GC visit the three callbacks held by this List ET.
  // These member references keep callbacks alive only while the GC can reach
  // this List ET from a live object.
  void visitor(void* rt, void* mark_func, uint64_t trace_tool) override;

  // Store callbacks in the List ET and let the base class apply the remaining
  // attributes to the materialized Element.
  void SetAttributes(const lepus::Value& attributes) override;

  void InsertNodeIntoChildSlot(uint32_t slot_index, const lepus::Value& child,
                               const lepus::Value& ref_node) override;

  // Submits pending updates and releases the recorder on success.
  // Logs failures and retains the recorder for a later submission.
  void FlushListUpdates();

  // Synchronously materialize and mount the requested item for initial
  // rendering, submit the operation, and return the materialized Element ID.
  int32_t ComponentAtIndex(TemplateAssembler* tasm, ListElement* list_element,
                           uint32_t index, int64_t operation_id,
                           bool enable_reuse_notification);

  // Expose pending state while keeping recorder ownership in the List instance.
  bool HasPendingListUpdates() const {
    return list_update_info_recorder_ != nullptr;
  }

  bool IsPendingListUpdatesEnqueued() const {
    return pending_list_updates_enqueued_;
  }
  void SetPendingListUpdatesEnqueued(bool enqueued) {
    pending_list_updates_enqueued_ = enqueued;
  }

 protected:
  bool ShouldMaterializeChildSlots() const override;

  void OnListItemPlatformInfoChanged(
      const lepus::Value& child, const lepus::Value& platform_info) override;

 private:
  // Submit a mounted item, trigger layout, and complete its List operation.
  void FlushElementTree(TemplateAssembler* tasm, ListElement* list_element,
                        Element* list_item_root, int64_t operation_id);

 private:
  // Managed by ElementManager to avoid enqueuing the same insertion batch
  // twice.
  bool pending_list_updates_enqueued_{false};
  // Created on the first insertion or platform info update in a batch and
  // released after a successful submission.
  std::unique_ptr<ListUpdateInfoRecorder> list_update_info_recorder_;
  // Follow ListElement's callback reference handling and participate in JS GC
  // through visitor().
  lepus::Value component_at_index_{};
  lepus::Value enqueue_component_{};
  lepus::Value component_at_indexes_{};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_TEMPLATE_INSTANCE_H_
