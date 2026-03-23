// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_VALUE_FORWARD_DECLARE_H_
#define CORE_RUNTIME_LEPUS_IR_VALUE_FORWARD_DECLARE_H_

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ilist.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/GenericDomTree.h"

namespace lynx {
namespace lepus {
namespace ir {
class Block;
class FuncOp;
class TypeOp;
class PrimitiveTypeOp;
class Block;
class Instruction;
class ModuleOp;
class Operation;
class Region;
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

namespace llvh {

template <>
struct ilist_alloc_traits<::lynx::lepus::ir::FuncOp> {
  static void deleteNode(::lynx::lepus::ir::FuncOp* v);
};

template <>
struct ilist_alloc_traits<::lynx::lepus::ir::TypeOp> {
  static void deleteNode(::lynx::lepus::ir::TypeOp* v);
};

template <>
struct ilist_alloc_traits<::lynx::lepus::ir::PrimitiveTypeOp> {
  static void deleteNode(::lynx::lepus::ir::PrimitiveTypeOp* v);
};

template <>
struct ilist_alloc_traits<::lynx::lepus::ir::ModuleOp> {
  static void deleteNode(::lynx::lepus::ir::ModuleOp* v);
};

template <>
struct ilist_alloc_traits<::lynx::lepus::ir::Block> {
  static void deleteNode(::lynx::lepus::ir::Block* v);
};

template <>
struct ilist_alloc_traits<::lynx::lepus::ir::Instruction> {
  static void deleteNode(::lynx::lepus::ir::Instruction* v);
};

template <>
struct ilist_alloc_traits<::lynx::lepus::ir::Operation> {
  static void deleteNode(::lynx::lepus::ir::Operation* v);
};

template <>
struct dom_trait<::lynx::lepus::ir::Block> {
  using parent_type = ::lynx::lepus::ir::Region*;
  static ::lynx::lepus::ir::Operation* cast(::lynx::lepus::ir::Operation* op) {
    return op;
  }
};
}  // namespace llvh

#endif  // CORE_RUNTIME_LEPUS_IR_VALUE_FORWARD_DECLARE_H_
