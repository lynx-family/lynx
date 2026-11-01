// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_TEMPLATE_REUSE_POOL_H_
#define CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_TEMPLATE_REUSE_POOL_H_

#include <cstdint>
#include <unordered_map>

#include "base/include/linked_hash_map.h"
#include "core/renderer/dom/fiber/element_template_instance.h"

namespace lynx {
namespace tasm {

// Each List owns a pool that records ET instances by the physical root's
// impl_id (sign). Candidates are grouped by GetTemplateType(), preserving
// enqueue order and reservation state. This class does not create or transfer
// physical trees, or modify ET UIDs, attributes, or logical children.
class ListElementTemplateReusePool {
 public:
  struct Action {
    enum class Type {
      // The target has no root and no available same-type source; the caller
      // materializes the target.
      kCreate,
      // The target's root passes this pool's binding and reservation checks.
      kUseExisting,
      // A same-type source was selected for the rootless target and reserved.
      kReuse,
      // The target is null, or its root fails binding or reservation checks.
      kUnavailable
    };
    Type type_{Type::kCreate};
    // kUseExisting holds the target; kReuse holds the candidate source.
    // Other results are null.
    fml::RefPtr<ElementTemplateInstance> reusable_instance_{nullptr};
    // kUseExisting and kReuse hold the selected physical root; others are null.
    fml::RefPtr<Element> reusable_root_{nullptr};
  };

  ListElementTemplateReusePool() = default;
  ListElementTemplateReusePool(const ListElementTemplateReusePool&) = delete;
  ListElementTemplateReusePool& operator=(const ListElementTemplateReusePool&) =
      delete;
  ListElementTemplateReusePool(ListElementTemplateReusePool&&) = default;
  ListElementTemplateReusePool& operator=(ListElementTemplateReusePool&&) =
      default;

  // Returns a strong reference copy of the owner registered for sign, or null.
  fml::RefPtr<ElementTemplateInstance> GetInstance(int32_t sign) const;

  // Adds sign to its current type's candidate queue if its registered instance
  // still owns the corresponding root.
  void Enqueue(int32_t sign);

  // Registers sign to new_instance, which must already own its physical root.
  // Returns false if validation fails. On success, removes the old owner's
  // candidate entry under its current type, then updates the registration.
  bool RegisterNewBinding(
      int32_t sign, const fml::RefPtr<ElementTemplateInstance>& new_instance);

  // Uses an existing target root first; otherwise selects a valid, available
  // same-type source in enqueue order.
  Action Dequeue(const fml::RefPtr<ElementTemplateInstance>& instance);

  void CancelDequeue(const Action& action);

  // Enumerates all registered owners, including retained old items.
  template <typename Callback>
  void ForEachRetainedInstance(Callback&& callback) const {
    for (const auto& entry : sign_et_instance_map_) {
      callback(entry.second);
    }
  }

  void Clear();

 private:
  bool IsReserved(const base::String& type, int32_t sign) const;

  void Invalidate(const base::String& type, int32_t sign);

 private:
  // Strong sign-to-owner mapping. Registration does not imply reusability.
  // Removing candidate eligibility leaves this registration intact, so the
  // table may retain instances that have left the logical children.
  std::unordered_map<int32_t, fml::RefPtr<ElementTemplateInstance>>
      sign_et_instance_map_;

  // Candidate sign queues grouped by template type, storing no extra ET refs.
  // LinkedHashMap preserves enqueue order: true allows validation and
  // selection; false means reserved. Selecting a source with Dequeue or
  // cancelling with CancelDequeue changes only the flag, keeping its position.
  std::unordered_map<base::String, base::LinkedHashMap<int32_t, bool>>
      recycled_pool_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_TEMPLATE_REUSE_POOL_H_
