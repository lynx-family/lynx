// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_UTILS_BLOCK_UTILS_H_
#define CORE_RUNTIME_LEPUS_IR_UTILS_BLOCK_UTILS_H_

namespace lynx {
namespace lepus {
namespace ir {
class OpBuilder;
class Block;
class FuncOp;

void InsertAfterPhi(OpBuilder* builder, Block* block);

bool DeleteUnreachableBlocks(FuncOp* f);

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_UTILS_BLOCK_UTILS_H_
