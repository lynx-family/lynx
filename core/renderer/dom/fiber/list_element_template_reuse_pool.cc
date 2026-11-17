// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/list_element_template_reuse_pool.h"

namespace lynx {
namespace tasm {
namespace {
static constexpr const char kReuseIdentifier[] = "reuse-identifier";
}  // namespace

base::String ListElementTemplateReusePool::GetReuseKey(
    const ElementTemplateInstance& instance) {
  const auto template_type = instance.GetTemplateType();
  const auto platform_info = instance.GetListItemPlatformInfoFromOptions();
  const auto reuse_identifier =
      platform_info.GetProperty(BASE_STATIC_STRING(kReuseIdentifier));
  if (!reuse_identifier.IsString()) {
    return template_type;
  }
  return base::String(template_type.str() + ":" +
                      reuse_identifier.String().str());
}

fml::RefPtr<ElementTemplateInstance> ListElementTemplateReusePool::GetInstance(
    int32_t sign) const {
  auto owner_it = sign_et_instance_map_.find(sign);
  if (owner_it == sign_et_instance_map_.end()) {
    return nullptr;
  }
  return owner_it->second;
}

fml::RefPtr<ElementTemplateInstance> ListElementTemplateReusePool::Enqueue(
    int32_t sign) {
  auto list_item_instance = GetInstance(sign);
  if (!list_item_instance) {
    return nullptr;
  }
  auto list_item_root = list_item_instance->PeekMaterializedRoot();
  if (!list_item_root || list_item_root->impl_id() != sign) {
    return nullptr;
  }
  // Duplicate enqueues retain FIFO order and preserve reservations.
  const auto reuse_key = GetReuseKey(*list_item_instance);
  const auto enqueue_result =
      recycled_pool_[reuse_key].insert_if_absent(sign, true);
  if (!enqueue_result.first->second) {
    return nullptr;
  }
  return list_item_instance;
}

// This function manages registrations and candidate eligibility. new_instance
// must already own the physical root identified by sign.
// For a transfer from A to B, the caller must finish the transfer before
// registering the root to B.
bool ListElementTemplateReusePool::RegisterNewBinding(
    int32_t sign, const fml::RefPtr<ElementTemplateInstance>& new_instance) {
  if (!new_instance) {
    return false;
  }
  auto list_item_root = new_instance->PeekMaterializedRoot();
  if (!list_item_root || list_item_root->impl_id() != sign) {
    return false;
  }
  auto old_instance = GetInstance(sign);
  if (old_instance) {
    const auto old_reuse_key = GetReuseKey(*old_instance);
    if (old_instance.get() == new_instance.get() &&
        IsReserved(old_reuse_key, sign)) {
      // After B reserves A's root but before transfer, sign still maps to A.
      // Re-registering A must preserve the reservation; otherwise, a later
      // Enqueue could make the same source available again.
      return false;
    }
    // Revoke eligibility under the old owner's reuse key first; update the
    // registration separately below.
    Invalidate(old_reuse_key, sign);
  }
  sign_et_instance_map_.insert_or_assign(sign, new_instance);
  return true;
}

ListElementTemplateReusePool::Action ListElementTemplateReusePool::Dequeue(
    const fml::RefPtr<ElementTemplateInstance>& instance) {
  Action action;
  if (!instance) {
    action.type_ = Action::Type::kUnavailable;
    return action;
  }

  const auto reuse_key = GetReuseKey(*instance);
  auto list_item_root = instance->PeekMaterializedRoot();
  // Prefer the target's existing root, checking this sign's registration and
  // reservation state in the pool.
  if (list_item_root) {
    auto sign = list_item_root->impl_id();
    auto registered_instance = GetInstance(sign);
    if (registered_instance && (registered_instance.get() != instance.get() ||
                                IsReserved(reuse_key, sign))) {
      // Reject a root registered to another instance or a reserved sign of the
      // current registered instance. Leave existing registrations and candidate
      // queues unchanged.
      action.type_ = Action::Type::kUnavailable;
      return action;
    }
    // Taking the target's own root removes its candidate entry; it must be
    // enqueued again before future recycling.
    Invalidate(reuse_key, sign);
    // An unregistered existing root is also usable. Return the instance and
    // root here; the caller registers them afterward.
    action.type_ = Action::Type::kUseExisting;
    action.reusable_instance_ = instance;
    action.reusable_root_ = list_item_root;
    return action;
  }

  // Read the identifier from logical metadata without materializing the target.
  auto pool_it = recycled_pool_.find(reuse_key);
  if (pool_it == recycled_pool_.end()) {
    // No queue for this reuse key: keep kCreate and let the caller
    // materialize the target.
    return action;
  }

  auto& recycled_signs = pool_it->second;
  for (auto sign_it = recycled_signs.begin();
       sign_it != recycled_signs.end();) {
    if (!sign_it->second) {
      // Keep reserved entries in place and look for a later available entry.
      // Do not validate or remove the reserved entry here.
      ++sign_it;
      continue;
    }

    const auto sign = sign_it->first;
    auto reusable_instance = GetInstance(sign);
    if (!reusable_instance || GetReuseKey(*reusable_instance) != reuse_key) {
      // If registration is missing or the source's current key differs, remove
      // only this candidate entry without changing the registration table.
      sign_it = recycled_signs.erase(sign_it);
      continue;
    }

    auto reusable_root = reusable_instance->PeekMaterializedRoot();
    if (!reusable_root || reusable_root->impl_id() != sign) {
      // If the source no longer owns the root for this sign, remove only its
      // candidate eligibility.
      sign_it = recycled_signs.erase(sign_it);
      continue;
    }

    // Keep the source's root and registration; reserve only the candidate.
    // The caller decides whether to transfer the physical tree.
    sign_it->second = false;
    action.type_ = Action::Type::kReuse;
    action.reusable_instance_ = reusable_instance;
    action.reusable_root_ = reusable_root;
    return action;
  }

  // Remove an empty bucket. Keep buckets containing only reserved entries;
  // both cases return the default kCreate.
  if (recycled_signs.empty()) {
    recycled_pool_.erase(pool_it);
  }
  return action;
}

void ListElementTemplateReusePool::CancelDequeue(const Action& action) {
  if (action.type_ != Action::Type::kReuse || !action.reusable_instance_ ||
      !action.reusable_root_) {
    return;
  }
  // The caller must cancel only this request's valid reservation. Locate the
  // candidate using its source's stable reuse key and the root sign.
  auto pool_it = recycled_pool_.find(GetReuseKey(*action.reusable_instance_));
  if (pool_it == recycled_pool_.end()) {
    return;
  }
  auto sign_it = pool_it->second.find(action.reusable_root_->impl_id());
  if (sign_it != pool_it->second.end()) {
    // Restore only the existing entry found here, preserving its position.
    // Do not reinsert a removed sign.
    sign_it->second = true;
  }
}

void ListElementTemplateReusePool::Clear() {
  recycled_pool_.clear();
  sign_et_instance_map_.clear();
}

bool ListElementTemplateReusePool::IsReserved(const base::String& reuse_key,
                                              int32_t sign) const {
  auto pool_it = recycled_pool_.find(reuse_key);
  if (pool_it == recycled_pool_.end()) {
    return false;
  }
  auto sign_it = pool_it->second.find(sign);
  return sign_it != pool_it->second.end() && !sign_it->second;
}

void ListElementTemplateReusePool::Invalidate(const base::String& reuse_key,
                                              int32_t sign) {
  auto pool_it = recycled_pool_.find(reuse_key);
  if (pool_it == recycled_pool_.end()) {
    return;
  }
  pool_it->second.erase(sign);
  if (pool_it->second.empty()) {
    recycled_pool_.erase(pool_it);
  }
}

}  // namespace tasm
}  // namespace lynx
