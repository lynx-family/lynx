//==- llvm/Support/RecyclingAllocator.h - Recycling Allocator ----*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the RecyclingAllocator class.  See the doxygen comment for
// RecyclingAllocator for more details on the implementation.
//
//===----------------------------------------------------------------------===//

#ifndef CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_RECYCLINGALLOCATOR_H_
#define CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_RECYCLINGALLOCATOR_H_

#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Recycler.h"

namespace llvh {

/// RecyclingAllocator - This class wraps an Allocator, adding the
/// functionality of recycling deleted objects.
///
template <class AllocatorType, class T, size_t Size = sizeof(T),
          size_t Align = alignof(T)>
class RecyclingAllocator {
 private:
  /// Base - Implementation details.
  ///
  Recycler<T, Size, Align> Base;

  /// Allocator - The wrapped allocator.
  ///
  AllocatorType Allocator;

 public:
  ~RecyclingAllocator() { Base.clear(Allocator); }

  /// Allocate - Return a pointer to storage for an object of type
  /// SubClass. The storage may be either newly allocated or recycled.
  ///
  template <class SubClass>
  SubClass* Allocate() {
    return Base.template Allocate<SubClass>(Allocator);
  }

  T* Allocate() { return Base.Allocate(Allocator); }

  /// Deallocate - Release storage for the pointed-to object. The
  /// storage will be kept track of and may be recycled.
  ///
  template <class SubClass>
  void Deallocate(SubClass* E) {
    return Base.Deallocate(Allocator, E);
  }

  void PrintStats() {
    Allocator.PrintStats();
    Base.PrintStats();
  }
};

}  // namespace llvh

template <class AllocatorType, class T, size_t Size, size_t Align>
inline void* operator new(
    size_t size,
    llvh::RecyclingAllocator<AllocatorType, T, Size, Align>& Allocator) {
  assert(size <= Size && "allocation size exceeded");
  return Allocator.Allocate();
}

template <class AllocatorType, class T, size_t Size, size_t Align>
inline void operator delete(
    void* E, llvh::RecyclingAllocator<AllocatorType, T, Size, Align>& A) {
  A.Deallocate(E);
}

#endif  // CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_RECYCLINGALLOCATOR_H_
