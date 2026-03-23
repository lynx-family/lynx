// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_OWNING_FOLDING_SET_H_
#define CORE_RUNTIME_LEPUS_IR_OWNING_FOLDING_SET_H_

#include <memory>
#include <utility>

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/FoldingSet.h"

namespace lynx {
namespace lepus {
namespace ir {  /// A FoldingSet that owns its entries and deletes them when it
                /// is destroyed.
template <typename T, class Deleter = std::default_delete<T>>
class OwningFoldingSet : Deleter {
  llvh::FoldingSet<T> set_;

 public:
  explicit OwningFoldingSet(unsigned log2_init_size = 6)
      : set_(log2_init_size) {}
  OwningFoldingSet(OwningFoldingSet&& arg) = default;
  OwningFoldingSet& operator=(OwningFoldingSet&& rhs) = default;

  ~OwningFoldingSet() {
    // The folding set is intrusive, so elements cannot be destroyed while they
    // are still inserted in the set. At the same time, removal of individual
    // elements from the set is rather slow as it involves iterating over
    // the bucket to find the previous node. So, instead we just save all nodes,
    // clear the set, and then delete them.
    llvh::SmallVector<T*, 8> toDelete{};
    toDelete.reserve(set_.size());
    for (T& entry : set_) toDelete.push_back(&entry);
    set_.clear();
    for (T* entry : toDelete) Deleter::operator()(entry);
  }

  /// FindNodeOrInsertPos - Look up the node specified by id.  If it exists,
  /// return it.  If not, return the insertion token that will make insertion
  /// faster.
  T* FindNodeOrInsertPos(const llvh::FoldingSetNodeID& id, void*& insert_pos) {
    return set_.FindNodeOrInsertPos(id, insert_pos);
  }

  /// InsertNode - Insert the specified node into the folding set, knowing that
  /// it is not already in the folding set.  insert_pos must be obtained from
  /// FindNodeOrInsertPos.
  T* InsertNode(std::unique_ptr<T, Deleter> N, void* insert_pos) {
    set_.InsertNode(N.get(), insert_pos);
    return N.release();
  }

  /// A helper method wrapping FindNodeOrInsertPos() and InsertNode()
  /// for types where the constructor parameters can be passed to a static
  /// method \c T::Profile() in the same order.
  /// The new node is created using
  /// <code>new T(std::forward<Args>(args)...)</code>.
  ///
  /// \return a pair of the pointer to the node and a boolean indicating whether
  ///     a new node was inserted.
  template <typename... Args>
  std::pair<T*, bool> GetOrEmplaceWithNew(Args&&... args) {
    llvh::FoldingSetNodeID id;
    T::Profile(id, std::forward<Args>(args)...);
    void* insert_pos = nullptr;
    if (auto* v = FindNodeOrInsertPos(id, insert_pos)) return {v, false};
    return {InsertNode(
                std::unique_ptr<T, Deleter>(new T(std::forward<Args>(args)...)),
                insert_pos),
            true};
  }
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_OWNING_FOLDING_SET_H_
