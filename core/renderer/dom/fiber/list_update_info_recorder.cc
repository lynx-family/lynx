// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/list_update_info_recorder.h"

#include <cstddef>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/element_template_instance.h"

namespace lynx {
namespace tasm {

namespace {

static constexpr const char kListItemPosition[] = "position";
static constexpr const char kListItemType[] = "type";
static constexpr const char kListItemFrom[] = "from";
static constexpr const char kListItemTo[] = "to";
static constexpr const char kListItemFlush[] = "flush";
static constexpr const char kListInsertAction[] = "insertAction";
static constexpr const char kListRemoveAction[] = "removeAction";
static constexpr const char kListUpdateAction[] = "updateAction";
static constexpr const char kListUpdateInfo[] = "update-list-info";

}  // namespace

std::string ListUpdateInfoRecorder::GetListItemType(
    const ElementTemplateInstance& instance) {
  // Use the same template identity for the data source and the reuse pool.
  return instance.GetTemplateType().str();
}

std::string ListUpdateInfoRecorder::GetListItemTypeUID(
    const ElementTemplateInstance& instance) {
  return GetListItemType(instance) + "#" + instance.GetUid().ToString();
}

ListUpdateInfoRecorder::ListUpdateInfoRecorder(
    const lepus::Value& old_children) {
  // A non-array old_children leaves the snapshot empty; the recorder remains
  // valid.
  if (!old_children.IsArrayOrJSArray()) {
    return;
  }
  const auto child_count = static_cast<size_t>(old_children.GetLength());
  old_children_.reserve(child_count);
  for (size_t index = 0; index < child_count; ++index) {
    // Keep strong references to the ET instances in their original order.
    old_children_.emplace_back(
        ElementTemplateInstance::ResolveElementTemplateInstanceValue(
            old_children.GetProperty(static_cast<uint32_t>(index))));
  }
}

void ListUpdateInfoRecorder::OnInsertBefore(const lepus::Value& child,
                                            const lepus::Value& ref_child) {
  auto child_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(child);
  if (!child_instance) {
    return;
  }
  // Match RL3's parentNode check: moving an attached item first records a
  // removal from its old position.
  if (child_instance->GetLogicalParent() != nullptr) {
    remove_child_.insert(child_instance);
  }
  auto ref_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(ref_child);
  if (!ref_instance) {
    // Without an anchor, record an append to the end of the list.
    //    LOGI("[ListET] ListUpdateInfoRecorder::OnInsertBefore: append child="
    //         << GetListItemTypeUID(*child_instance));
    append_child_.push_back(child_instance);
    return;
  }
  //  LOGI("[ListET] ListUpdateInfoRecorder::OnInsertBefore: child="
  //       << GetListItemTypeUID(*child_instance)
  //       << " before=" << GetListItemTypeUID(*ref_instance));
  // Record items before the same anchor in call order.
  ElementTemplateInstance* ref_instance_ptr = ref_instance.get();
  if (insert_before_.find(ref_instance_ptr) == insert_before_.end()) {
    insert_before_.emplace(ref_instance_ptr,
                           InsertBeforeGroup{.ref_child_ = ref_instance});
  }
  insert_before_[ref_instance_ptr].children_.push_back(child_instance);
}

void ListUpdateInfoRecorder::OnRemoveChild(const lepus::Value& child) {
  auto child_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(child);
  if (!child_instance) {
    return;
  }
  //  LOGI("[ListET] ListUpdateInfoRecorder::OnRemoveChild: child="
  //       << GetListItemTypeUID(*child_instance));
  remove_child_.insert(child_instance);
}

void ListUpdateInfoRecorder::OnSetAttribute(const lepus::Value& child,
                                            const lepus::Value& platform_info) {
  auto child_instance =
      ElementTemplateInstance::ResolveElementTemplateInstanceValue(child);
  if (!child_instance) {
    return;
  }
  // Match RL3's platformInfoUpdate.set: keep only the latest platform info for
  // each item.
  platform_info_update_[child_instance] =
      platform_info.IsObject()
          ? ElementTemplateInstance::ConvertTemplateMapForStorage(platform_info)
          : lepus::Value(lepus::Dictionary::Create());
}

bool ListUpdateInfoRecorder::Flush(Element* list_element) {
  if (!list_element || !list_element->is_list()) {
    return false;
  }
  if (insert_before_.empty() && append_child_.empty() &&
      remove_child_.empty() && platform_info_update_.empty()) {
    return true;
  }

  auto insert_actions = lepus::CArray::Create();
  auto remove_actions = lepus::CArray::Create();
  auto update_actions = lepus::CArray::Create();

  const auto append_insertion_action =
      [&insert_actions](const fml::RefPtr<ElementTemplateInstance>& child,
                        int32_t position) {
        if (!child) {
          return false;
        }
        // Read the latest platform info for new items directly; no separate
        // updateAction is needed.
        auto list_item_platform_info =
            child->GetListItemPlatformInfoFromOptions();
        auto insert_action =
            list_item_platform_info.IsObject()
                ? ElementTemplateInstance::ConvertTemplateMapForStorage(
                      list_item_platform_info)
                : lepus::Value(lepus::Dictionary::Create());
        // Set the insertion position.
        insert_action.SetProperty(BASE_STATIC_STRING(kListItemPosition),
                                  lepus::Value(position));
        // Set the template type.
        insert_action.SetProperty(BASE_STATIC_STRING(kListItemType),
                                  lepus::Value(GetListItemType(*child)));
        insert_actions->emplace_back(std::move(insert_action));
        return true;
      };

  // Walk the old_children_ snapshot.
  // i is the item's old index, used for removals.
  // j is the next position after preceding old items, used for insertions and
  // updates. Increment j for each insertion to account for the added item.
  // Decrement j when removing an old item to cancel the loop's final increment.
  // The loop increments j to count each retained old item.
  int32_t j = 0;
  for (int32_t i = 0; i < static_cast<int32_t>(old_children_.size());
       ++i, ++j) {
    const auto& child = old_children_[i];
    if (!child) {
      return false;
    }
    ElementTemplateInstance* child_instance_ptr = child.get();
    const auto insert_before_it = insert_before_.find(child_instance_ptr);
    const auto updated_it = platform_info_update_.find(child);
    const int32_t insert_before_children_size =
        insert_before_it == insert_before_.end()
            ? 0
            : static_cast<int32_t>(insert_before_it->second.children_.size());

    // 1. Generate update actions.
    // Old list: [A, B]. In the same batch, update A's platform info and move A
    // to the end; the expected list is [B, A].
    // Skip the separate update for A because its insertion carries the latest
    // info. Otherwise, Flush would produce:
    // removeAction = [0];
    // insertAction = [{ position: 1, 'item-key': 'A', /* latest platform info
    // */ }]; updateAction = [{ from: 0, to: 0, 'item-key': 'A', flush: false
    // }];
    if (updated_it != platform_info_update_.end() &&
        remove_child_.find(child) == remove_child_.end()) {
      auto update_action = updated_it->second;
      // Set the source index.
      update_action.SetProperty(BASE_STATIC_STRING(kListItemFrom),
                                lepus::Value(j + insert_before_children_size));
      // Set the destination index.
      update_action.SetProperty(BASE_STATIC_STRING(kListItemTo),
                                lepus::Value(j + insert_before_children_size));
      // Disable flushing for the platform info update.
      update_action.SetProperty(BASE_STATIC_STRING(kListItemFlush),
                                lepus::Value(false));
      // Set the template type.
      update_action.SetProperty(BASE_STATIC_STRING(kListItemType),
                                lepus::Value(GetListItemType(*child)));
      update_actions->emplace_back(std::move(update_action));
    }

    // 2. Generate insert actions.
    if (insert_before_children_size) {
      for (const auto& inserted_child : insert_before_it->second.children_) {
        if (!append_insertion_action(inserted_child, j)) {
          return false;
        }
        ++j;
      }
    }

    // 3. Generate remove actions.
    if (remove_child_.find(child) != remove_child_.end()) {
      // Removal positions always refer to the old sequence. Keep the records
      // intact so a failed submission can be retried.
      remove_actions->emplace_back(static_cast<int32_t>(i));
      --j;
    }
  }

  // 4. Generate insert actions for items appended to the end.
  for (const auto& child : append_child_) {
    if (!append_insertion_action(child, j)) {
      return false;
    }
    ++j;
  }

  // Actions above follow the old sequence and the call order within each group.
  auto update_info = lepus::Dictionary::Create();
  // Set insertAction.
  update_info->SetValue(BASE_STATIC_STRING(kListInsertAction),
                        std::move(insert_actions));
  // Set removeAction.
  update_info->SetValue(BASE_STATIC_STRING(kListRemoveAction),
                        std::move(remove_actions));
  // Set updateAction.
  update_info->SetValue(BASE_STATIC_STRING(kListUpdateAction),
                        std::move(update_actions));
  lepus::Value update_info_value(std::move(update_info));
  // Set the attribute on the real ListElement, bypassing ET attribute slots.
  // This only queues the update for Element::FlushActions to consume later.
  list_element->SetAttribute(BASE_STATIC_STRING(kListUpdateInfo),
                             update_info_value);
  return true;
}

}  // namespace tasm
}  // namespace lynx
