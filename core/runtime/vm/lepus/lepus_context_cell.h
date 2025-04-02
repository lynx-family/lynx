// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_LEPUS_CONTEXT_CELL_H_
#define CORE_RUNTIME_VM_LEPUS_LEPUS_CONTEXT_CELL_H_

#include "base/include/vector.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

namespace lynx {
namespace lepus {

class QuickContext;

class ContextCell {
 public:
  ContextCell(lepus::QuickContext* qctx, LEPUSContext* ctx, LEPUSRuntime* rt)
      : gc_enable_(false), ctx_(ctx), rt_(rt), qctx_(qctx) {
    if (rt_) {
      gc_enable_ = LEPUS_IsGCModeRT(rt_);
    }
  };
  bool gc_enable_;
  LEPUSContext* ctx_;
  LEPUSRuntime* rt_;
  lepus::QuickContext* qctx_;
};

class CellManager {
 public:
  CellManager() : cells_(){};
  ~CellManager();
  ContextCell* AddCell(lepus::QuickContext* qctx);

 private:
  base::InlineVector<ContextCell*, 16> cells_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_LEPUS_CONTEXT_CELL_H_
