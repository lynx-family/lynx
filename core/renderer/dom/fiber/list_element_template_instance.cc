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
#include "core/renderer/utils/base/tasm_constants.h"
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

// Enumerates native logical children without materializing their roots.
template <typename Callback>
void ForEachLogicalChild(const ElementTemplateInstance& instance,
                         Callback&& callback) {
  const auto& child_slots = instance.GetChildSlots();
  if (!child_slots.IsArray()) {
    return;
  }
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(child_slots.GetLength());
       ++slot_index) {
    auto children = child_slots.GetProperty(static_cast<uint32_t>(slot_index));
    if (!children.IsArray()) {
      continue;
    }
    for (size_t child_index = 0;
         child_index < static_cast<size_t>(children.GetLength());
         ++child_index) {
      auto child = ElementTemplateInstance::ResolveElementTemplateInstanceValue(
          children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child) {
        callback(child);
      }
    }
  }
}

void VisitElementTemplateInstance(ElementTemplateInstance& instance, void* rt,
                                  void* mark_func, uint64_t trace_tool);

void VisitLogicalChildren(const ElementTemplateInstance& instance, void* rt,
                          void* mark_func, uint64_t trace_tool) {
  ForEachLogicalChild(
      instance, [&](const fml::RefPtr<ElementTemplateInstance>& child) {
        VisitElementTemplateInstance(*child, rt, mark_func, trace_tool);
      });
}

void VisitElementTemplateInstance(ElementTemplateInstance& instance, void* rt,
                                  void* mark_func, uint64_t trace_tool) {
  instance.visitor(rt, mark_func, trace_tool);
  // Typed Lists are ListElementTemplateInstance objects. Their visitor already
  // traverses logical children and retained items, so do not descend twice.
  if (instance.IsTypedTemplate() &&
      instance.GetTypedTag().IsEqual(kElementListTag)) {
    return;
  }
  // Ordinary ETs inherit the empty visitor, but may contain nested Lists.
  VisitLogicalChildren(instance, rt, mark_func, trace_tool);
}

void VisitRetainedItems(const ListElementTemplateReusePool& pool,
                        const ElementTemplateInstance& list_instance, void* rt,
                        void* mark_func, uint64_t trace_tool) {
  pool.ForEachRetainedInstance(
      [&](const fml::RefPtr<ElementTemplateInstance>& instance) {
        // Direct children are visited through the logical tree below.
        if (instance->GetLogicalParent() != &list_instance) {
          VisitElementTemplateInstance(*instance, rt, mark_func, trace_tool);
        }
      });
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
  VisitRetainedItems(reuse_pool_, *this, rt, mark_func, trace_tool);
  VisitLogicalChildren(*this, rt, mark_func, trace_tool);
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

void ListElementTemplateInstance::RemoveNodeFromChildSlot(
    uint32_t slot_index, const lepus::Value& child_node) {
  if (slot_index != kTypedTemplateRootChildSlotIndex) {
    return;
  }
  auto child_instance = ResolveElementTemplateInstanceValue(child_node);
  if (!child_instance || child_instance->GetLogicalParent() != this) {
    return;
  }
  auto slot_children = GetOrCreateMutableChildSlot(slot_index);
  if (FindSlotChildIndex(slot_children, child_node) >=
      static_cast<size_t>(slot_children.GetLength())) {
    return;
  }
  // Capture the old order before removing the first item in this batch.
  if (!list_update_info_recorder_) {
    list_update_info_recorder_ =
        std::make_unique<ListUpdateInfoRecorder>(slot_children);
  }
  list_update_info_recorder_->OnRemoveChild(child_node);

  // Preserve logical removal and detachment of an already materialized child.
  ElementTemplateInstance::RemoveNodeFromChildSlot(slot_index, child_node);
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
    LOGE(
        "[ListET] ComponentAtIndex: get list item element template instance "
        "failed with index="
        << index);
    return list::kInvalidIndex;
  }

  fml::RefPtr<Element> list_item_root = nullptr;
  const auto action = reuse_pool_.Dequeue(list_item_instance);
  switch (action.type_) {
    case ListElementTemplateReusePool::Action::Type::kUseExisting:
      list_item_root = action.reusable_root_;
      break;
    case ListElementTemplateReusePool::Action::Type::kReuse: {
      auto reusable_instance = action.reusable_instance_;
      // TODO: impl reuse element in the next mr.
      // bool reuse_from_element =
      // list_item_instance->ReuseElementsFrom(*reusable_instance);
      bool reuse_from_element = false;
      if (reuse_from_element) {
        // TODO: impl reuse notify
      } else {
        reuse_pool_.CancelDequeue(action);
        list_item_root = list_item_instance->GetRoot();
      }
      break;
    }
    case ListElementTemplateReusePool::Action::Type::kCreate:
      list_item_root = list_item_instance->GetRoot();
      break;
    case ListElementTemplateReusePool::Action::Type::kUnavailable:
    default:
      return list::kInvalidIndex;
  }

  if (!list_item_root) {
    return list::kInvalidIndex;
  } else if (list_item_root->parent() == nullptr) {
    // New nodes are mounted on first use. A node taken back by its own instance
    // usually retains its parent, so it is not appended again. Only update the
    // physical tree and list-item flag here; do not change logical slots or
    // record another insertion.
    list_root->InsertNode(list_item_root);
  }

  const auto list_item_sign = list_item_root->impl_id();
  // Note: Register the sign before Flush. Native code may enqueue again during
  // Flush, so the sign must already resolve to the ET that owns this physical
  // tree rather than being registered after the request returns.
  if (!reuse_pool_.RegisterNewBinding(list_item_sign, list_item_instance)) {
    LOGE("[ListET] ComponentAtIndex: register list item binding failed, sign="
         << list_item_sign);
    return list::kInvalidIndex;
  }

  LOGI("[ListET] ComponentAtIndex submitted, list.id="
       << list_root->impl_id() << " index=" << index
       << " operation_id=" << operation_id << " item.id=" << list_item_sign);
  // Submit even previously materialized and mounted items so this operation
  // receives a completion notification.
  FlushElementTree(tasm, list_element, list_item_root.get(), operation_id);
  return list_item_sign;
}

void ListElementTemplateInstance::EnqueueComponent(int32_t sign) {
  reuse_pool_.Enqueue(sign);
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
