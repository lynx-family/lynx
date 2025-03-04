// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHARED_DATA_LYNX_WHITE_BOARD_H_
#define CORE_SHARED_DATA_LYNX_WHITE_BOARD_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/synchronization/shared_mutex.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {

/**
 WhiteBoard is a DataCenter that can be shared and operated by multi LynxViews,
 it is not thread-safe, should be operated only on TASM thread Now. users can
 `set` `get` `registerListener` to whiteboard for sharing data between multiple
 lynxViews.
 */
enum class WhiteBoardStorageType : uint8_t { TYPE_LEPUS, TYPE_JS, TYPE_CLIENT };

struct WhiteBoardListener {
  double callback_id;
  // invoked while new data is received.
  base::MoveOnlyClosure<void, const pub::Value&> trigger_callback;
  // invoked while being removed.
  base::MoveOnlyClosure<void> remove_callback;
};

class WhiteBoardDelegate;
class WhiteBoard final {
 public:
  WhiteBoard();
  WhiteBoard(const WhiteBoard&) = delete;
  WhiteBoard& operator=(const WhiteBoard&) = delete;
  WhiteBoard(WhiteBoard&&) = delete;
  WhiteBoard& operator=(WhiteBoard&&) = delete;

  // set & get operation
  void SetGlobalSharedData(const std::string& key,
                           const std::shared_ptr<pub::Value>& value);
  std::shared_ptr<pub::Value> GetGlobalSharedData(const std::string& key);

  // subscribe & unsubscribe operation
  void RegisterSharedDataListener(const WhiteBoardStorageType& type,
                                  const std::string& key,
                                  WhiteBoardListener listener);
  void RemoveSharedDataListener(const WhiteBoardStorageType& type,
                                const std::string& key, int32_t listener_id);

  ~WhiteBoard() = default;

 private:
  using LynxWhiteBoardMap =
      std::unordered_map<std::string, std::shared_ptr<pub::Value>>;
  using WhiteBoardListenerMap =
      std::unordered_map<std::string, std::vector<WhiteBoardListener>>;

  void TriggerListener(const WhiteBoardStorageType& type,
                       const std::string& key, const pub::Value& value);

  LynxWhiteBoardMap data_center_;
  std::unique_ptr<fml::SharedMutex> data_center_lock_;
  std::unordered_map<WhiteBoardStorageType, std::unique_ptr<fml::SharedMutex>>
      listener_lock_;
  std::unordered_map<WhiteBoardStorageType, WhiteBoardListenerMap>
      listener_map_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SHARED_DATA_LYNX_WHITE_BOARD_H_
