// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_CARD_CACHE_DATA_OP_H_
#define CORE_SHELL_LYNX_CARD_CACHE_DATA_OP_H_

#include <string>
#include <utility>

#include "core/renderer/data/template_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace shell {
enum class CacheDataType {
  UPDATE = 0,
  RESET,
};

class CacheDataOp {
 public:
  static CacheDataOp DeepClone(const CacheDataOp& other) {
    return CacheDataOp(tasm::TemplateData::DeepClone(other.data_), other.type_);
  }

  explicit CacheDataOp(tasm::TemplateData data,
                       CacheDataType type = CacheDataType::UPDATE)
      : data_(std::move(data)), type_(type){};

  // make CacheDataOp move only.
  CacheDataOp(const CacheDataOp& other) = delete;
  CacheDataOp& operator=(const CacheDataOp& other) = delete;
  CacheDataOp(CacheDataOp&& other) = default;
  CacheDataOp& operator=(CacheDataOp&& other) = default;

  const lepus::Value& GetValue() const { return data_.GetValue(); }
  const std::string& ProcessorName() const { return data_.PreprocessorName(); }

  CacheDataType GetType() const { return type_; }

  friend bool operator==(const CacheDataOp& left, const CacheDataOp& right) {
    return left.data_.GetValue() == right.data_.GetValue() &&
           left.type_ == right.type_;
  }

 private:
  tasm::TemplateData data_;
  CacheDataType type_;
};
}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_CARD_CACHE_DATA_OP_H_
