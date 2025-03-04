// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shared_data/lynx_white_board.h"

#include <algorithm>
#include <string>
#include <utility>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

WhiteBoard::WhiteBoard() {
  data_center_lock_ =
      std::unique_ptr<fml::SharedMutex>(fml::SharedMutex::Create());
  listener_lock_.emplace(
      WhiteBoardStorageType::TYPE_CLIENT,
      std::unique_ptr<fml::SharedMutex>(fml::SharedMutex::Create()));
  listener_lock_.emplace(
      WhiteBoardStorageType::TYPE_JS,
      std::unique_ptr<fml::SharedMutex>(fml::SharedMutex::Create()));
  listener_lock_.emplace(
      WhiteBoardStorageType::TYPE_LEPUS,
      std::unique_ptr<fml::SharedMutex>(fml::SharedMutex::Create()));
}

void WhiteBoard::SetGlobalSharedData(const std::string& key,
                                     const std::shared_ptr<pub::Value>& value) {
  {
    fml::UniqueLock lock(*data_center_lock_);
    data_center_[key] = value;
  }

  TriggerListener(WhiteBoardStorageType::TYPE_LEPUS, key, *value);
  TriggerListener(WhiteBoardStorageType::TYPE_CLIENT, key, *value);
  TriggerListener(WhiteBoardStorageType::TYPE_JS, key, *value);
}

std::shared_ptr<pub::Value> WhiteBoard::GetGlobalSharedData(
    const std::string& key) {
  fml::SharedLock lock(*data_center_lock_);
  auto iter = data_center_.find(key);
  if (iter != data_center_.end()) {
    return iter->second;
  }
  return nullptr;
}

void WhiteBoard::TriggerListener(const WhiteBoardStorageType& type,
                                 const std::string& key,
                                 const pub::Value& value) {
  fml::SharedLock lock(*listener_lock_[type]);
  auto& listener_map = listener_map_[type];
  auto listener_iter = listener_map.find(key);
  if (listener_iter != listener_map.end()) {
    // iterator over listeners and trigger callbacks;
    auto& listener = listener_iter->second;
    for (auto& listener : listener) {
      listener.trigger_callback(value);
    }
  }
}

// subscribe operation.
void WhiteBoard::RegisterSharedDataListener(const WhiteBoardStorageType& type,
                                            const std::string& key,
                                            WhiteBoardListener listener) {
  fml::UniqueLock lock(*listener_lock_[type]);
  auto& listener_map = listener_map_[type];
  auto pair = listener_map.emplace(key, std::vector<WhiteBoardListener>());
  pair.first->second.emplace_back(std::move(listener));
}

void WhiteBoard::RemoveSharedDataListener(const WhiteBoardStorageType& type,
                                          const std::string& key,
                                          int32_t listener_id) {
  fml::SharedLock lock(*listener_lock_[type]);
  auto& listener_map = listener_map_[type];
  auto listener_iter = listener_map.find(key);
  if (listener_iter != listener_map.end()) {
    auto& listeners = listener_iter->second;
    for (auto iter = listeners.begin(); iter != listeners.end(); iter++) {
      if (iter->callback_id == listener_id) {
        iter->remove_callback();
        listeners.erase(iter);
        break;
      }
    }
  }
}

// Unsubscribe Operation End
}  // namespace tasm
}  // namespace lynx
