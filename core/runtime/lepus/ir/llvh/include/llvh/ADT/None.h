//===-- None.h - Simple null value for implicit construction ------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file provides None, an enumerator for use in implicit constructors
//  of various (usually templated) types to make such construction more
//  terse.
//
//===----------------------------------------------------------------------===//

#ifndef CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_ADT_NONE_H_
#define CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_ADT_NONE_H_

namespace llvh {
/// A simple null object to allow implicit construction of Optional<T>
/// and similar types without having to spell out the specialization's name.
// (constant value 1 in an attempt to workaround MSVC build issue... )
enum class NoneType { None = 1 };
const NoneType None = NoneType::None;
}  // namespace llvh

#endif  // CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_ADT_NONE_H_
