// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_DIALECT_H_
#define CORE_RUNTIME_LEPUS_IR_DIALECT_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallSet.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

class Dialect {
 public:
  static constexpr ValueKind DIALECT_GUARD_OP = ValueKind::First_ValueKind;

  Dialect() = default;
  virtual ~Dialect() = default;
  virtual void* GetId() = 0;
  virtual std::string GetName() const = 0;

  template <ValueKind... kind>
  void AddOperations() {
    (void)std::initializer_list<ValueKind>{
        Dialect::DIALECT_GUARD_OP,
        (InsertOp(kind), Dialect::DIALECT_GUARD_OP)...};
  }

  void InsertOp(ValueKind kind) {
    if (LEPUS_UNLIKELY(kind == Dialect::DIALECT_GUARD_OP)) return;
    ops_.insert(kind);
  }

  bool BelongTo(ValueKind kind) {
    return llvh::find_if(ops_, [kind](ValueKind k) { return k == kind; }) !=
           ops_.end();
  }

  bool IsMIRDialect() const;

  virtual void RegisterOperation() = 0;

  uint32_t GetOpsSize() const { return ops_.size(); }

  const llvh::SmallSet<ValueKind, 16>& GetOps() const { return ops_; }

 private:
  llvh::SmallSet<ValueKind, 16> ops_;
};

class DialectRegistry {
 private:
  DialectRegistry();
  ~DialectRegistry() = default;

 public:
  static DialectRegistry* SharedInstance() {
    static auto* registry = new DialectRegistry();
    if (!registry) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: DialectRegistry::SharedInstance returned nullptr");
    }
    return registry;
  }

  Dialect* GetDialect(void* id) const {
    auto iter = id_to_dialect_.find(id);
    if (LEPUS_LIKELY(iter != id_to_dialect_.end())) return iter->second;
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: dialect not found for id");
  }

  const Dialect* GetDialect(ValueKind kind) const {
    if (LEPUS_UNLIKELY(kind == ValueKind::Last_ValueKind)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: invalid ValueKind::Last_ValueKind");
    }

    for (auto& iter : id_to_dialect_) {
      auto& dialect = iter.second;
      if (!dialect) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: DialectRegistry contains nullptr dialect");
      }
      if (LEPUS_LIKELY(dialect->BelongTo(kind))) return dialect;
    }
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: no dialect matches this operation kind");
  }

  template <typename... Args>
  void RegisterDialect(Args&&... args) {
    auto dialect = std::make_unique<Dialect>(std::forward(args)...);
    id_to_dialect_[dialect->getID()] = std::move(dialect);
  }

 private:
  std::map<void*, Dialect*> id_to_dialect_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_DIALECT_H_
