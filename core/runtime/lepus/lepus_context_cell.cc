// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/lepus_context_cell.h"

#include "core/runtime/lepusng/quick_context.h"

namespace lynx {
namespace lepus {

CellManager::~CellManager() {
  for (auto* cell : cells_) {
    delete cell;
  }
}

ContextCell* CellManager::AddCell(lepus::QuickContext* qctx) {
  LEPUSContext* ctx = qctx->context();
  auto* cell = new ContextCell(qctx, ctx, LEPUS_GetRuntime(ctx));
  cells_.emplace_back(cell);
  return cell;
}

}  // namespace lepus
}  // namespace lynx
