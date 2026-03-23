// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_IR_VISITOR_H_
#define CORE_RUNTIME_LEPUS_IR_IR_VISITOR_H_

#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {
/// IRVisitorBase - This is a simple visitor class for Lepus IR nodes, allowing
/// clients to walk over entire IR functions, blocks, or instructions.
template <typename ImplClass, typename ValueRetTy = void>
class IRVisitorBase {
 public:
  ImplClass& AsImpl() { return static_cast<ImplClass&>(*this); }

  /// Top level visitor.
  ValueRetTy VisitValue(const Value& v) { return ValueRetTy(); }

/// Define default IR implementations, chaining to parent nodes.
/// Use ValueKinds.def to automatically generate them.
#define DEF_VALUE(CLASS, PARENT) \
  ValueRetTy Visit##CLASS(const CLASS& i) { return AsImpl().Visit##PARENT(i); }
#define MARK_FIRST(CLASS, PARENT) DEF_VALUE(CLASS, PARENT)
#define BEGIN_VALUE(CLASS, PARENT) DEF_VALUE(CLASS, PARENT)
#define DEF_TAG(NAME, PARENT) \
  ValueRetTy Visit##NAME(const PARENT& i) { return AsImpl().Visit##PARENT(i); }
#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/value_kinds.def"
#undef ENABLE_MIR_INSTR
};

/// IRVisitor - This is a simple visitor class for Lepus IR nodes.
// ir allowing clients to walk over IRNodes of different types.
template <typename ImplClass, typename ValueRetTy = void>
class IRVisitor : public IRVisitorBase<ImplClass, ValueRetTy> {
 public:
  ImplClass& AsImpl() { return static_cast<ImplClass&>(*this); }

  /// Visit IR nodes.
  ValueRetTy Visit(const Value& v) {
    switch (v.GetKind()) {
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: IRVisitor::Visit encountered invalid ValueKind");
#define DEF_VALUE(CLASS, PARENT) \
  case ValueKind::CLASS##Kind:   \
    return AsImpl().Visit##CLASS(*llvh::cast<CLASS>(&v));
#define DEF_TAG(NAME, PARENT) \
  case ValueKind::NAME##Kind: \
    return AsImpl().Visit##PARENT(*llvh::cast<PARENT>(&v));

#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/value_kinds.def"
#undef ENABLE_MIR_INSTR
    }
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRVisitor::Visit fell through unexpectedly");
  }
};

template <typename ImplClass>
class IRWalker {
  using SelfType = IRWalker<ImplClass>;

 public:
  ImplClass& AsImpl() { return static_cast<ImplClass&>(*this); }

  virtual ~IRWalker() = default;

  /// Top level visitor.
  bool VisitValue(const Value& v) { return false; }
  virtual void VisitRegion(const Region& region, bool skip_vars) {
    VisitChildren(region);
  }

  void VisitChildren(const Region& region) {
    llvh::for_each(region, [&](const Block& block) { Visit(block); });
  }
  void VisitChildren(const Instruction& inst) {
    VisitChildren(*llvh::cast<Operation>(&inst));
  }
  void VisitChildren(const Operation& op) {
    std::for_each(op.RegionBegin(), op.RegionEnd(),
                  [&](const Region* region) { VisitRegion(*region, false); });
  }
  void VisitChildren(const Value& val) {
    if (auto* inst = llvh::dyn_cast<Instruction>(&val))
      return VisitChildren(*inst);

    if (auto* block = llvh::dyn_cast<Block>(&val)) {
      llvh::for_each(*block, [&](const Operation& op) { Visit(op); });
      return;
    }
  }

#define SPECIAL_HOLD_FUNC_OP_VALUE(CLASS, PARENT)                            \
  bool Visit##CLASS(const CLASS& inst) {                                     \
    bool skip_children = AsImpl().Visit##PARENT(*llvh::cast<PARENT>(&inst)); \
    for (uint32_t idx = 0; idx < inst.GetNumOperands(); idx++) {             \
      auto* op = inst.GetOperand(idx);                                       \
      if (auto func = llvh::dyn_cast<FuncOp>(op)) {                          \
        Visit(*func);                                                        \
      }                                                                      \
    }                                                                        \
    if (!skip_children) {                                                    \
      VisitChildren(inst);                                                   \
    }                                                                        \
    return false;                                                            \
  }
#define DEF_VALUE(CLASS, PARENT)                                             \
  bool Visit##CLASS(const CLASS& inst) {                                     \
    bool skip_children = AsImpl().Visit##PARENT(*llvh::cast<PARENT>(&inst)); \
    if (!skip_children) {                                                    \
      VisitChildren(inst);                                                   \
    }                                                                        \
    return false;                                                            \
  }
#define MARK_FIRST(CLASS, PARENT) DEF_VALUE(CLASS, PARENT)
#define BEGIN_VALUE(CLASS, PARENT) DEF_VALUE(CLASS, PARENT)
#define DEF_TAG(NAME, PARENT)                                                \
  bool Visit##NAME(const PARENT& inst) {                                     \
    bool skip_children = AsImpl().Visit##PARENT(*llvh::cast<PARENT>(&inst)); \
    if (!skip_children) {                                                    \
      VisitChildren(inst);                                                   \
    }                                                                        \
    return false;                                                            \
  }
#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/value_kinds.def"
#undef ENABLE_MIR_INSTR

  /// Visit IR nodes.
  bool Visit(const Value& v) {
    switch (v.GetKind()) {
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: IRWalker::Visit encountered invalid ValueKind");
#define DEF_VALUE(CLASS, PARENT) \
  case ValueKind::CLASS##Kind:   \
    return AsImpl().Visit##CLASS(*llvh::cast<CLASS>(&v));
#define DEF_TAG(NAME, PARENT) \
  case ValueKind::NAME##Kind: \
    return AsImpl().Visit##PARENT(*llvh::cast<PARENT>(&v));

#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/value_kinds.def"
#undef ENABLE_MIR_INSTR
    }
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRWalker::Visit fell through unexpectedly");
  }
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_IR_VISITOR_H_
