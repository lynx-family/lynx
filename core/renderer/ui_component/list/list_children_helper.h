// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_CHILDREN_HELPER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_CHILDREN_HELPER_H_

#include <functional>
#include <set>
#include <unordered_map>

#include "base/trace/native/trace_event.h"
#include "core/renderer/ui_component/list/item_holder.h"
#include "core/renderer/ui_component/list/list_orientation_helper.h"

namespace lynx {
namespace tasm {

using ItemHolderSet = std::set<ItemHolder*, ItemHolder::Compare>;

// Utility class for traversing all child nodes.
class ListChildrenHelper {
 public:
  class StickyItemHolderSet {
   public:
    bool AddItemHolder(ItemHolder* item_holder) {
      if (item_holder &&
          static_cast<int>(inline_item_holders_.size()) < capacity_ &&
          inline_item_holders_.find(item_holder) ==
              inline_item_holders_.end()) {
        inline_item_holders_.insert(item_holder);
        count_ += 1;
      }
      return count_ < capacity_;
    }

    void SetCapacity(int capacity) { capacity_ = capacity; }

    void Clear() {
      count_ = 0;
      inline_item_holders_.clear();
    }

    bool Contain(const ItemHolder* item_holder) const {
      return inline_item_holders_.find(const_cast<ItemHolder*>(item_holder)) !=
             inline_item_holders_.end();
    }

   private:
    int capacity_{1};
    int count_{0};
    ItemHolderSet inline_item_holders_;
  };

#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event);
#endif
  void AddChild(const ItemHolderSet& children, ItemHolder* item_holder);
  void AttachChild(ItemHolder* item_holder, Element* element);
  void DetachChild(ItemHolder* item_holder, Element* element);
  void ForEachChild(const std::function<bool(ItemHolder*)>& func,
                    bool reverse = false) const;
  void ForEachChild(const ItemHolderSet& children,
                    const std::function<bool(ItemHolder*)>& func,
                    bool reverse = false) const;
  const ItemHolderSet& children() const { return children_; }
  const ItemHolderSet& attached_children() const { return attached_children_; }
  const std::unordered_map<Element*, ItemHolder*>&
  attached_element_item_holder_map() const {
    return attached_element_item_holder_map_;
  }
  int GetChildCount() const { return static_cast<int>(children_.size()); }
  int GetAttachedChildCount() const {
    return static_cast<int>(children_.size());
  }
  void ClearChildren() { children_.clear(); }
  void ClearLastBindingChildren() { last_binding_children_.clear(); }
  void ClearAttachedChildren() {
    attached_children_.clear();
    attached_element_item_holder_map_.clear();
  }
  ItemHolder* GetFirstChild(
      const ItemHolderSet& children,
      const std::function<bool(const ItemHolder*)>& func) const;
  ItemHolder* GetLastChild(
      const ItemHolderSet& children,
      const std::function<bool(const ItemHolder*)>& func) const;
  ItemHolder* GetFirstChild(
      const std::function<bool(const ItemHolder*)>& func) const;
  ItemHolder* GetLastChild(
      const std::function<bool(const ItemHolder*)>& func) const;
  void UpdateOnScreenChildren(ListOrientationHelper* orientation_helper,
                              float content_offset);
  void UpdateInStickyChildren(ListOrientationHelper* orientation_helper,
                              float content_offset, float content_size,
                              float sticky_offset);
  void ClearOnScreenChildren() { on_screen_children_.clear(); }
  void ClearInPreloadChildren() { in_preload_children_.clear(); }
  void ClearInStickyChildren() { in_sticky_children_.clear(); }
  const ItemHolderSet& on_screen_children() const {
    return on_screen_children_;
  }
  const ItemHolderSet& in_preload_children() const {
    return in_preload_children_;
  }
  const ItemHolderSet& in_sticky_children() const {
    return in_sticky_children_;
  }
  const ItemHolderSet& last_binding_children() const {
    return last_binding_children_;
  }
  void EraseFromLastBindingChildren(ItemHolder* item_holder);
  void HandleLayoutOrScrollResult(
      const std::function<bool(ItemHolder*)>& insert_handler,
      const std::function<bool(ItemHolder*)>& recycle_handler,
      const std::function<bool(ItemHolder*)>& update_handler);
  void InitStickyItemHolderSet(int thread_mode);
  bool AddToStickyItemHolderSet(ItemHolder* item_holder);
  bool InStickyItemHolderSet(const ItemHolder* item_holder) const;
  void SetRecycleStickyItem(bool recycle_sticky_item) {
    recycle_item_holder_ = recycle_sticky_item;
  }
  void SetCustomStickyItemHolderCapacity(int capacity) {
    if (capacity > 0) {
      use_default_sticky_buffer_count_ = false;
      in_sticky_top_children_.SetCapacity(capacity);
      in_sticky_bottom_children_.SetCapacity(capacity);
    }
  }

 private:
  bool recycle_item_holder_{false};
  bool use_default_sticky_buffer_count_{true};
  StickyItemHolderSet in_sticky_top_children_;
  StickyItemHolderSet in_sticky_bottom_children_;
  std::unordered_map<Element*, ItemHolder*> attached_element_item_holder_map_;
  ItemHolderSet children_;
  ItemHolderSet attached_children_;
  ItemHolderSet last_binding_children_;
  ItemHolderSet on_screen_children_;
  ItemHolderSet in_preload_children_;
  ItemHolderSet in_sticky_children_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_CHILDREN_HELPER_H_
