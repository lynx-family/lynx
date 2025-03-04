// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_children_helper.h"

#include <string>

#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace tasm {

// Insert ItemHolder to children_ set. It will be invoked by
// ListAdapter::UpdateItemHolderToLatest() when list's data source has new inset
// child.
void ListChildrenHelper::AddChild(const ItemHolderSet& children,
                                  ItemHolder* item_holder) {
  if (!item_holder) {
    return;
  }
  const_cast<ItemHolderSet&>(children).insert(item_holder);
}

// Insert ItemHolder to attached_children_ set. It will be invoked by
// ListContainer::OnComponentFinished() when the ItemHolder is bound with
// element.
void ListChildrenHelper::AttachChild(ItemHolder* item_holder,
                                     Element* element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListChildrenHelper::AttachChild", "index",
              std::to_string(item_holder ? item_holder->index() : -1),
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!item_holder) {
    return;
  }
  attached_children_.insert(item_holder);
  if (element) {
    attached_element_item_holder_map_[element] = item_holder;
  }
}

// Delete ItemHolder from attached_children_ set. It will be invoked by
// ListContainer::RecycleChild() when the ItemHolder is recycled.
void ListChildrenHelper::DetachChild(ItemHolder* item_holder,
                                     Element* element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListChildrenHelper::DetachChild", "index",
              std::to_string(item_holder ? item_holder->index() : -1),
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!item_holder) {
    return;
  }
  if (!attached_children_.erase(item_holder)) {
    NLIST_LOGE("Fail to erase item holder at pos = " << item_holder->index());
  }
  if (element) {
    attached_element_item_holder_map_.erase(element);
  }
}

// Traverse child nodes. When reverse == true, traverse in backward order
void ListChildrenHelper::ForEachChild(
    const ItemHolderSet& children, const std::function<bool(ItemHolder*)>& func,
    bool reverse) const {
  if (!reverse) {
    for (auto* item_holder : children) {
      if (item_holder && func(item_holder)) {
        return;
      }
    }
  } else {
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
      if (*it && func(*it)) {
        return;
      }
    }
  }
}

// Traverse child nodes. When reverse == true, traverse in backward order
void ListChildrenHelper::ForEachChild(
    const std::function<bool(ItemHolder*)>& func, bool reverse) const {
  ForEachChild(children_, func, reverse);
}

ItemHolder* ListChildrenHelper::GetFirstChild(
    const std::function<bool(const ItemHolder*)>& func) const {
  return GetFirstChild(children_, func);
}

ItemHolder* ListChildrenHelper::GetLastChild(
    const std::function<bool(const ItemHolder*)>& func) const {
  return GetLastChild(children_, func);
}

//(TODO)fangzhou.fz: After implementing preload, this first child is incorrect.
ItemHolder* ListChildrenHelper::GetFirstChild(
    const ItemHolderSet& children,
    const std::function<bool(const ItemHolder*)>& func) const {
  ItemHolder* res = nullptr;
  ForEachChild(children, [&res, &func](ItemHolder* item_holder) {
    if (item_holder && func(item_holder)) {
      res = item_holder;
      return true;
    }
    return false;
  });
  return res;
}

ItemHolder* ListChildrenHelper::GetLastChild(
    const ItemHolderSet& children,
    const std::function<bool(const ItemHolder*)>& func) const {
  ItemHolder* res = nullptr;
  ForEachChild(
      children,
      [&res, &func](ItemHolder* item_holder) {
        if (item_holder && func(item_holder)) {
          res = item_holder;
          return true;
        }
        return false;
      },
      true);
  return res;
}

void ListChildrenHelper::UpdateOnScreenChildren(
    ListOrientationHelper* orientation_helper, float content_offset) {
  if (!orientation_helper) {
    return;
  }
  on_screen_children_.clear();
  ForEachChild(
      [this, content_offset, &orientation_helper](ItemHolder* item_holder) {
        if (item_holder &&
            item_holder->VisibleInList(orientation_helper, content_offset)) {
          on_screen_children_.insert(item_holder);
        }
        return false;
      });
  // This trace event is used to output the debug info.
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListChildrenHelper::UpdateOnScreenChildren",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
}

void ListChildrenHelper::UpdateInStickyChildren(
    ListOrientationHelper* orientation_helper, float content_offset,
    float content_size, float sticky_offset) {
  if (!orientation_helper || !orientation_helper->IsVertical()) {
    // Not support sticky in horizontal direction.
    return;
  }
  in_sticky_children_.clear();
  ForEachChild([this, orientation_helper, content_offset, content_size,
                sticky_offset](ItemHolder* item_holder) {
    if (item_holder &&
        item_holder->IsAtStickyPosition(
            content_offset, orientation_helper->GetMeasurement(), content_size,
            sticky_offset, orientation_helper->GetDecoratedStart(item_holder),
            orientation_helper->GetDecoratedEnd(item_holder))) {
      in_sticky_children_.insert(item_holder);
    }
    return false;
  });
}

void ListChildrenHelper::HandleLayoutOrScrollResult(
    const std::function<bool(ItemHolder*)>& insert_handler,
    const std::function<bool(ItemHolder*)>& recycle_handler,
    const std::function<bool(ItemHolder*)>& update_handler) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ListChildrenHelper::HandleLayoutOrScrollResult");
  ItemHolderSet new_binding_children;
  ItemHolderSet new_added_children;
  // Merge all need binding children from on_screen_children_ /
  // in_preload_children_ / in_sticky_children_.
  ForEachChild(attached_children_, [this, &new_binding_children](
                                       ItemHolder* item_holder) {
    if (on_screen_children_.find(item_holder) != on_screen_children_.end() ||
        in_preload_children_.find(item_holder) != in_preload_children_.end() ||
        in_sticky_children_.find(item_holder) != in_sticky_children_.end()) {
      new_binding_children.insert(item_holder);
    }
    return false;
  });
  // Update new added ItemHolders.
  ForEachChild(new_binding_children,
               [this, &new_added_children](ItemHolder* item_holder) {
                 if (last_binding_children_.find(item_holder) ==
                     last_binding_children_.end()) {
                   new_added_children.insert(item_holder);
                 }
                 return false;
               });
  // Update new removed ItemHolders.
  ForEachChild(new_binding_children, [this](ItemHolder* item_holder) {
    last_binding_children_.erase(item_holder);
    return false;
  });
  // Handle insert.
  ForEachChild(new_added_children, insert_handler);
  // Handle recycle.
  ForEachChild(last_binding_children_, recycle_handler);
  // Update layout info to platform.
  last_binding_children_.clear();
  last_binding_children_.insert(new_binding_children.begin(),
                                new_binding_children.end());
  ForEachChild(last_binding_children_, update_handler);
}

#if ENABLE_TRACE_PERFETTO
void ListChildrenHelper::UpdateTraceDebugInfo(TraceEvent* event) {
  // attached children
  auto* attached_children = event->add_debug_annotations();
  attached_children->set_name("attached_children");
  std::string attached_children_str = "";
  int index = 0;
  for (const auto* item_holder : attached_children_) {
    attached_children_str += "(" + std::to_string(index++) + ") ";
    if (item_holder) {
      attached_children_str += "[" + std::to_string(item_holder->index()) +
                               ", " + item_holder->item_key() + "]\n";
    } else {
      attached_children_str += "[-1, nullptr]\n";
    }
  }
  attached_children->set_string_value(attached_children_str);

  // on screen children
  auto* on_screen_children = event->add_debug_annotations();
  on_screen_children->set_name("on_screen_children");
  std::string on_screen_children_str = "";
  index = 0;
  for (const auto* item_holder : on_screen_children_) {
    on_screen_children_str += "(" + std::to_string(index++) + ") ";
    if (item_holder) {
      on_screen_children_str += "[" + std::to_string(item_holder->index()) +
                                ", " + item_holder->item_key() + "]\n";
    } else {
      on_screen_children_str += "[-1, nullptr]\n";
    }
  }
  on_screen_children->set_string_value(on_screen_children_str);

  // children
  auto* children = event->add_debug_annotations();
  children->set_name("children");
  std::string children_str = "";
  index = 0;
  for (const auto* item_holder : children_) {
    children_str += "(" + std::to_string(index++) + ") ";
    if (item_holder) {
      children_str += "[" + std::to_string(item_holder->index()) + ", " +
                      item_holder->item_key() + "]\n";
    } else {
      children_str += "[-1, nullptr]\n";
    }
  }
  children->set_string_value(children_str);
}
#endif

}  // namespace tasm
}  // namespace lynx
