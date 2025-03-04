// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_card_cache_data_manager.h"

#include <utility>

#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace shell {

void LynxCardCacheDataManager::AddCardCacheData(tasm::TemplateData data,
                                                const CacheDataType type) {
  std::lock_guard<std::mutex> lock(card_cache_data_mutex_);
  if (type == CacheDataType::RESET) {
    // when reset, cached data before is unneeded, can be cleared.
    card_cache_data_.clear();
  }
  card_cache_data_.emplace_back(CacheDataOp(std::move(data), type));
}

CacheDataOpVector LynxCardCacheDataManager::GetCardCacheData() {
  std::lock_guard<std::mutex> lock(card_cache_data_mutex_);
  CacheDataOpVector vector;
  for (const auto& data : card_cache_data_) {
    vector.emplace_back(CacheDataOp::DeepClone(data));
  }
  return vector;
}

CacheDataOpVector LynxCardCacheDataManager::ObtainCardCacheData() {
  std::lock_guard<std::mutex> lock(card_cache_data_mutex_);
  return std::move(card_cache_data_);
}

}  // namespace shell
}  // namespace lynx
