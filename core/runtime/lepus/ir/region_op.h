// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_REGION_OP_H_
#define CORE_RUNTIME_LEPUS_IR_REGION_OP_H_

#include <string>

#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ilist.h"
#include "core/runtime/lepus/ir/operation.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {
class IRContext;
class Region;
class Value;
class FuncOp;
class Block;
class Operation;

class Region {
  NON_COPYABLE(Region);

 public:
  using BlockListType = llvh::iplist<Block>;
  using RegionListType = llvh::SmallVector<Region*, 4>;
  using iterator = BlockListType::iterator;
  using const_iterator = BlockListType::const_iterator;
  using region_iterator = RegionListType::iterator;
  using const_region_iterator = RegionListType::const_iterator;
  using block_range_iterator = llvh::iterator_range<iterator>;
  using block_const_range_iterator = llvh::iterator_range<const const_iterator>;

  Region(Operation* parent, const llvh::StringRef name = {});

  IRContext* GetIRCtx();
  Block* GetUniqueBlock();
  void AddBlock(Block* block);
  bool RemoveBlock(Block* block);
  Block* GetBlock(uint32_t idx);
  BlockListType& GetBlocks() { return blocks_; }
  uint32_t GetBlockSize() const { return blocks_.size(); }
  Block* GetEntryBlock();
  bool IsEntryBlock(const Block* block) { return GetEntryBlock() == block; }
  Operation* GetParent() const {
    if (LEPUS_UNLIKELY(!parent_)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Region::GetParent called with nullptr parent "
          "operation");
    }
    return parent_;
  }
  std::string GetName() const {
    if (name_.empty()) return "region." + std::to_string(uuid_);
    return "region." + name_.str();
  }

  std::string GetPureName() const {
    if (name_.empty()) return std::to_string(uuid_);
    return name_.str();
  }

  FuncOp* GetFunction() const;
  Operation* GetNearestFuncOrClassOrModule() const;
  Operation* GetTopLevelOperation() const;
  // ------ iterator -----------
  const_iterator begin() const { return blocks_.begin(); }

  const_iterator end() const { return blocks_.end(); }

  block_range_iterator BlockRange() {
    return block_range_iterator(begin(), end());
  }

  block_const_range_iterator BlockRange() const {
    return block_const_range_iterator(begin(), end());
  }

  iterator begin() { return blocks_.begin(); }

  iterator end() { return blocks_.end(); }

  Block& Front();

  void SetUUID();
  uint32_t GetUUID() const { return uuid_; }

 private:
  BlockListType blocks_;
  Operation* parent_ = nullptr;
  uint32_t uuid_ = -1;
  llvh::StringRef name_;
};
}  // namespace ir

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_REGION_OP_H_
