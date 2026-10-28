// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/list_element_template_instance.h"

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/list/decoupled_list_types.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/list_update_info_recorder.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/services/timing_handler/timing.h"

namespace lynx {
namespace tasm {

namespace {

static constexpr uint32_t kTypedTemplateRootChildSlotIndex = 0;
static constexpr const char kComponentAtIndexAttribute[] = "component-at-index";
static constexpr const char kEnqueueComponentAttribute[] = "enqueue-component";
static constexpr const char kComponentAtIndexesAttribute[] =
    "component-at-indexes";

void SetListTemplateCallbackAttribute(lepus::Value* target,
                                      const lepus::Value& value) {
  if (!target) {
    return;
  }
  if (value.IsNil() || value.IsUndefined() || !value.IsCallable()) {
    *target = lepus::Value();
    return;
  }
  target->CopyWeakValue(value);
}

bool IsListTemplateCallbackAttribute(const base::String& key) {
  return key.IsEqual(kComponentAtIndexAttribute) ||
         key.IsEqual(kEnqueueComponentAttribute) ||
         key.IsEqual(kComponentAtIndexesAttribute);
}

}  // namespace

ListElementTemplateInstance::ListElementTemplateInstance(
    ElementManager* element_manager)
    : ElementTemplateInstance(element_manager) {}

ListElementTemplateInstance::~ListElementTemplateInstance() = default;

void ListElementTemplateInstance::visitor(void* rt, void* mark_func,
                                          uint64_t trace_tool) {
  auto* runtime = reinterpret_cast<LEPUSRuntime*>(rt);
  auto* mark = reinterpret_cast<LEPUS_MarkFunc*>(mark_func);
  auto mark_callback = [runtime, mark,
                        trace_tool](const lepus::Value& callback) {
    if (callback.IsJSValue()) {
      mark(runtime, WRAP_AS_JS_VALUE(callback.value()), trace_tool);
    }
  };
  // Mark the three JS functions referenced by this List ET. The GC does not
  // automatically scan C++ member fields, and CopyWeakValue() does not create
  // an independent strong reference to the callbacks.
  mark_callback(component_at_index_);
  mark_callback(enqueue_component_);
  mark_callback(component_at_indexes_);
  ElementTemplateInstance::visitor(rt, mark_func, trace_tool);
}

void ListElementTemplateInstance::SetAttributes(
    const lepus::Value& attributes) {
  // SetAttributes is called only for typed element template instances, not for
  // regular compiled element templates.
  if (!IsTemplateMapValue(attributes) && !attributes.IsNil() &&
      !attributes.IsUndefined()) {
    ElementTemplateInstance::SetAttributes(attributes);
    return;
  }

  // Preserve callbacks omitted from attribute updates. The helper clears a
  // callback only when explicitly given null, undefined, or a non-callable
  // value.
  if (attributes.Contains(BASE_STATIC_STRING(kComponentAtIndexAttribute))) {
    SetListTemplateCallbackAttribute(
        &component_at_index_,
        attributes.GetProperty(BASE_STATIC_STRING(kComponentAtIndexAttribute)));
  }
  if (attributes.Contains(BASE_STATIC_STRING(kEnqueueComponentAttribute))) {
    SetListTemplateCallbackAttribute(
        &enqueue_component_,
        attributes.GetProperty(BASE_STATIC_STRING(kEnqueueComponentAttribute)));
  }
  if (attributes.Contains(BASE_STATIC_STRING(kComponentAtIndexesAttribute))) {
    SetListTemplateCallbackAttribute(&component_at_indexes_,
                                     attributes.GetProperty(BASE_STATIC_STRING(
                                         kComponentAtIndexesAttribute)));
  }

  // Exclude callbacks from ordinary attributes so the base class does not store
  // them again or forward them to ListElement.
  auto stored_attributes = lepus::Dictionary::Create();
  lepus::Value::ForEachLepusValue(
      attributes,
      [&stored_attributes](const lepus::Value& key, const lepus::Value& value) {
        if (!key.IsString() || IsListTemplateCallbackAttribute(key.String())) {
          return;
        }
        // Values may still be JS values after callback attributes are filtered.
        stored_attributes->SetValue(key.String(), value);
      });
  // The base SetAttributes converts JS values in stored_attributes to Lepus
  // values.
  ElementTemplateInstance::SetAttributes(lepus::Value(stored_attributes));
}

void ListElementTemplateInstance::InsertNodeIntoChildSlot(
    uint32_t slot_index, const lepus::Value& child_node,
    const lepus::Value& ref_node) {
  /*
   1. Validate the slot, child, and anchor. List supports insertion only into
   slot 0 and rejects all other slot indices.
   List.child_slots_ = [
     [itemA, itemB, itemC, itemD, itemE]
     └──────── slot 0 ────────┘
   ]
   */
  if (slot_index != kTypedTemplateRootChildSlotIndex) {
    return;
  }
  auto child_instance = ResolveElementTemplateInstanceValue(child_node);
  if (!child_instance || child_node.IsEqual(ref_node)) {
    return;
  }

  // GetOrCreateMutableChildSlot(0) returns the children array for slot 0:
  // [itemA, itemB, itemC, itemD]
  auto slot_children = GetOrCreateMutableChildSlot(slot_index);
  auto ref_index = FindSlotChildIndex(slot_children, ref_node);
  // Match regular ET insertion semantics: append when the anchor is not in the
  // current slot.
  auto effective_ref_node = lepus::Value();
  if (ref_index < static_cast<size_t>(slot_children.GetLength())) {
    effective_ref_node =
        slot_children.GetProperty(static_cast<uint32_t>(ref_index));
  }

  // Initialize list_update_info_recorder_ if needed.
  if (!list_update_info_recorder_) {
    // Snapshot the logical children before modifying them. Subsequent
    // insertions reuse the same recorder.
    list_update_info_recorder_ =
        std::make_unique<ListUpdateInfoRecorder>(slot_children);
  }
  list_update_info_recorder_->OnInsertBefore(child_node, effective_ref_node);

  // List maintains only logical items, without invoking the full base insertion
  // path or materializing items. The recorder has already recorded the removal
  // side of a move; update the logical order here.
  InsertLogicalChildIntoSlot(slot_index, child_node, effective_ref_node,
                             *child_instance);

  // Enqueue every valid insertion and let the Manager deduplicate entries, even
  // before the List is materialized. Generate the diff at the shared submission
  // point instead of setting attributes on the materialized Element here.
  element_manager()->EnqueuePendingElementTemplateListUpdates(*this);
}

void ListElementTemplateInstance::OnListItemPlatformInfoChanged(
    const lepus::Value& child, const lepus::Value& platform_info) {
  if (list_update_info_recorder_ == nullptr) {
    // Attribute-only updates also need the old order so Flush can compute
    // updateAction's from/to indices.
    list_update_info_recorder_ = std::make_unique<ListUpdateInfoRecorder>(
        GetOrCreateMutableChildSlot(kTypedTemplateRootChildSlotIndex));
  }
  list_update_info_recorder_->OnSetAttribute(child, platform_info);
  element_manager()->EnqueuePendingElementTemplateListUpdates(*this);
}

void ListElementTemplateInstance::FlushListUpdates() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              LIST_ELEMENT_TEMPLATE_INSTANCE_FLUSH_LIST_UPDATES);
  if (!list_update_info_recorder_) {
    return;
  }
  // Inspect the existing root without triggering materialization.
  auto root = PeekMaterializedRoot();
  if (!root) {
    LOGE("[ListET] FlushListUpdates root is not materialized, type="
         << GetTemplateType().str() << " uid=" << GetUid().ToString());
    return;
  }
  if (!root->is_list()) {
    LOGE("[ListET] FlushListUpdates target is not a list, type="
         << GetTemplateType().str() << " uid=" << GetUid().ToString());
    return;
  }
  auto* list_element = static_cast<ListElement*>(root.get());
  // Establish the association before publishing the diff so subsequent index
  // requests can find the logical List.
  list_element->MarkListElementTemplateInstance(*this);
  if (!list_update_info_recorder_->Flush(list_element)) {
    LOGE("[ListET] FlushListUpdates recorder submission failed, type="
         << GetTemplateType().str() << " uid=" << GetUid().ToString());
    return;
  }
  // Release this batch after a successful submission to avoid submitting it
  // twice.
  list_update_info_recorder_.reset();
}

int32_t ListElementTemplateInstance::ComponentAtIndex(
    TemplateAssembler* tasm, ListElement* list_element, uint32_t index,
    int64_t operation_id, bool enable_reuse_notification) {
  auto list_root = PeekMaterializedRoot();
  if (!tasm || !list_root || list_root.get() != list_element) {
    return list::kInvalidIndex;
  }
  // The index refers to the full logical item sequence and matches insertAction
  // positions during initial rendering.
  auto list_item_instance =
      GetChildInstanceAt(kTypedTemplateRootChildSlotIndex, index);
  if (!list_item_instance) {
    LOGE("[ListET] ComponentAtIndex: get list item instance failed, index="
         << index);
    return list::kInvalidIndex;
  }

  // Materialize the item only when requested. An already materialized instance
  // returns its existing root.
  auto list_item_root = list_item_instance->GetRoot();
  if (!list_item_root) {
    LOGE("[ListET] ComponentAtIndex: list item materialization failed, index="
         << index);
    return list::kInvalidIndex;
  }

  if (list_item_root->parent() == nullptr) {
    // Mount the materialized node and set the list-item flag without modifying
    // the logical slot or collecting the diff again.
    list_root->InsertNode(list_item_root);
  } else if (list_item_root->parent() != list_root.get()) {
    // The initial rendering prototype does not support moves across Lists yet.
    return list::kInvalidIndex;
  }
  const auto list_item_sign = list_item_root->impl_id();
  // Submit even previously materialized and mounted items so this operation
  // receives a completion notification.
  FlushElementTree(tasm, list_element, list_item_root.get(), operation_id);

  return list_item_sign;
}

void ListElementTemplateInstance::FlushElementTree(TemplateAssembler* tasm,
                                                   ListElement* list_element,
                                                   Element* list_item_element,
                                                   int64_t operation_id) {
  std::shared_ptr<PipelineOptions> current_option = nullptr;
  if (!list_item_element && tasm->GetCurrentPipelineContext()) {
    current_option = tasm->GetCurrentPipelineContext()->GetOptions();
  } else {
    current_option = std::make_shared<PipelineOptions>();
  }

  // Match FiberFlushElementTree's single-item parameters using the materialized
  // Element ID directly.
  current_option->trigger_layout_ = true;
  current_option->operation_id = operation_id;
  current_option->list_comp_id_ = list_item_element->impl_id();
  current_option->list_id_ = list_element->impl_id();

  if (current_option->enable_unified_pixel_pipeline) {
    current_option->resolve_requested = true;
    current_option->target_node = list_element->impl_id();
  } else {
    element_manager()->OnPatchFinish(current_option, list_item_element);
  }
  EXEC_EXPR_FOR_INSPECTOR({
    auto* observer = element_manager()->inspector_element_observer();
    if (observer != nullptr) {
      observer->OnFiberFlushElementTree();
    }
  });
}

bool ListElementTemplateInstance::ShouldMaterializeChildSlots() const {
  // ComponentAtIndex materializes logical List items on demand.
  return false;
}

}  // namespace tasm
}  // namespace lynx
