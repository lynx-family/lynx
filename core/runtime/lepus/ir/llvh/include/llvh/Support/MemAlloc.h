//===- MemAlloc.h - Memory allocation functions -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This file defines counterparts of C library allocation functions defined in
/// the namespace 'std'. The new allocation functions crash on allocation
/// failure instead of returning null pointer.
///
//===----------------------------------------------------------------------===//

#ifndef CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_MEMALLOC_H_
#define CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_MEMALLOC_H_

#include <cstdlib>

#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Compiler.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/ErrorHandling.h"

namespace llvh {

LLVM_ATTRIBUTE_RETURNS_NONNULL inline void* safe_malloc(size_t Sz) {
  void* Result = std::malloc(Sz);
  if (Result == nullptr) report_bad_alloc_error("Allocation failed");
  return Result;
}

LLVM_ATTRIBUTE_RETURNS_NONNULL inline void* safe_calloc(size_t Count,
                                                        size_t Sz) {
  void* Result = std::calloc(Count, Sz);
  if (Result == nullptr) report_bad_alloc_error("Allocation failed");
  return Result;
}

LLVM_ATTRIBUTE_RETURNS_NONNULL inline void* safe_realloc(void* Ptr, size_t Sz) {
  void* Result = std::realloc(Ptr, Sz);
  if (Result == nullptr) report_bad_alloc_error("Allocation failed");
  return Result;
}

}  // namespace llvh
#endif  // CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_MEMALLOC_H_
