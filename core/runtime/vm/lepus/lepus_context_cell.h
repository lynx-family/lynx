// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_LEPUS_CONTEXT_CELL_H_
#define CORE_RUNTIME_VM_LEPUS_LEPUS_CONTEXT_CELL_H_

#include "base/include/value/lynx_value_extended.h"
#include "base/include/vector.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

lynx_api_env lynx_value_api_new_env(LEPUSContext* ctx);
void lynx_value_api_delete_env(lynx_api_env env);
void lynx_value_api_detach_context_from_env(lynx_api_env env);
LEPUSContext* lynx_value_api_get_context_from_env(lynx_api_env env);

namespace lynx {
namespace lepus {

class QuickContext;

class ContextCell {
 public:
  ContextCell(lepus::QuickContext* qctx, LEPUSContext* ctx, LEPUSRuntime* rt)
      : gc_enable_(false),
        ctx_(ctx),
        rt_(rt),
        qctx_(qctx),
        env_(lynx_value_api_new_env(ctx)) {
    if (rt_) {
      gc_enable_ = LEPUS_IsGCModeRT(rt_);
    }
  };

  void DetachEnv() { lynx_value_api_detach_context_from_env(env_); }

  ~ContextCell() { lynx_value_api_delete_env(env_); }

  bool gc_enable_;
  LEPUSContext* ctx_;
  LEPUSRuntime* rt_;
  lepus::QuickContext* qctx_;
  lynx_api_env env_;
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
