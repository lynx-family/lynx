// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/instrs.h"

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"

namespace lynx {
namespace lepus {
namespace ir {

unsigned TerminatorInst::GetNumSuccessors() const {
#undef TERMINATOR
#define TERMINATOR(CLASS, PARENT) \
  if (auto i = llvh::dyn_cast<CLASS>(this)) return i->GetNumSuccessorsImpl();
#define BEGIN_TERMINATOR(NAME, PARENT) TERMINATOR(NAME, PARENT)
#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/instrs.def"
#undef ENABLE_MIR_INSTR
  throw ::lynx::lepus::CompileException(
      "Lepus IR error: TerminatorInst::GetNumSuccessors called on "
      "non-terminator instruction");
}

Block* TerminatorInst::GetSuccessor(unsigned idx) const {
#undef TERMINATOR
#define TERMINATOR(CLASS, PARENT) \
  if (auto i = llvh::dyn_cast<CLASS>(this)) return i->GetSuccessorImpl(idx);
#define BEGIN_TERMINATOR(NAME, PARENT) TERMINATOR(NAME, PARENT)
#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/instrs.def"
#undef ENABLE_MIR_INSTR
  throw ::lynx::lepus::CompileException(
      "Lepus IR error: TerminatorInst::GetSuccessor called on non-terminator "
      "instruction");
}

#undef TERMINATOR
void TerminatorInst::SetSuccessor(unsigned idx, Block* b) {
#define TERMINATOR(CLASS, PARENT) \
  if (auto i = llvh::dyn_cast<CLASS>(this)) return i->SetSuccessorImpl(idx, b);
#define BEGIN_TERMINATOR(NAME, PARENT) TERMINATOR(NAME, PARENT)
#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/instrs.def"
#undef ENABLE_MIR_INSTR
  throw ::lynx::lepus::CompileException(
      "Lepus IR error: TerminatorInst::SetSuccessor called on non-terminator "
      "instruction");
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
