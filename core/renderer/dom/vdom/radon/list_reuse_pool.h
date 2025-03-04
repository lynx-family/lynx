// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_LIST_REUSE_POOL_H_
#define CORE_RENDERER_DOM_VDOM_RADON_LIST_REUSE_POOL_H_

#include <unordered_map>

#include "base/include/linked_hash_map.h"
#include "base/include/value/base_string.h"

namespace lynx {
namespace tasm {

class RadonComponent;

using ListKeyComponentMap = std::unordered_map<base::String, RadonComponent*>;

class ListReusePool {
 public:
  struct Action {
    enum class Type : int32_t {
      CREATE,
      REUSE,
      UPDATE,
    };
    Type type_;
    base::String key_to_reuse_;
  };
  void Enqueue(const base::String& item_key,
               const base::String& reuse_identifier);

  Action Dequeue(const base::String& item_key,
                 const base::String& reuse_identifier,
                 RadonComponent* component);

  void InsertIntoListKeyComponentMap(const base::String& item_key,
                                     RadonComponent* val);
  RadonComponent* GetComponentFromListKeyComponentMap(
      const base::String& item_key);

  void Remove(const base::String& item_key,
              const base::String& reuse_identifier);

 private:
  using Pool =
      std::unordered_map<base::String,
                         base::LinkedHashMap<base::String, base::String>>;
  // This pool is a map from ReuseIdentifier to ItemKey LinkedHashMap.
  // The LinkedHashMap (actually it's a set) includes all of the item_key_
  // whose component can be reused.
  Pool pool_{};

  // this map includes all of the component which has been created before.
  ListKeyComponentMap key_component_map_;

  void Invalidate(const base::String& reuse_identifier,
                  const base::String& item_key);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_LIST_REUSE_POOL_H_
