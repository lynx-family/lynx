// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_NATIVE_MODULE_NATIVE_MODULE_RECORD_MANAGER_H_
#define DEVTOOL_LYNX_DEVTOOL_NATIVE_MODULE_NATIVE_MODULE_RECORD_MANAGER_H_

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include "base/include/value/base_value.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "third_party/jsoncpp/include/json/value.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

// Per-instance recorder for NativeModule records: carries records from the JS
// thread to the DevTool thread over a bounded, non-blocking queue, stamps an
// increasing `sequence`, keeps a bounded FIFO history for `getRecords`, and
// pushes `recordAdded` events while the CDP session is enabled.
//
// Threading: `EnqueueRecordOnJSThread` is the only method callable off the
// DevTool thread; everything else is DevTool-only.
class NativeModuleRecordManager
    : public std::enable_shared_from_this<NativeModuleRecordManager> {
 public:
  // History is capped by record count; the oldest record is evicted first.
  // Per-record payload size is bounded by the record producer, not here.
  static constexpr size_t kMaxHistoryCount = 200;
  // Pending-queue burst bound; the newest record is dropped when full.
  static constexpr size_t kMaxPendingCount = 1024;

  explicit NativeModuleRecordManager(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~NativeModuleRecordManager() = default;

  // --- JS thread ---
  // Serializes `record`, enqueues it, and (on the empty -> pending edge)
  // schedules a drain on the DevTool thread, so callers need not know about the
  // pending queue at all.
  void EnqueueRecordOnJSThread(lepus::Value record);

  // --- DevTool thread; methods below run serially, no locking ---
  // Enable/disable live `recordAdded` reporting. Neither touches history, which
  // the frontend always fetches via `getRecords`.
  void Enable();
  void Disable();

  // Writes the history snapshot and `latestSequence` into a CDP response.
  void GetRecords(const std::shared_ptr<MessageSender>& sender, int64_t id);

  int64_t latest_sequence() const { return latest_sequence_; }

  // Records dropped because the queue was full. Thread-safe.
  int64_t dropped_count() const {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    return dropped_count_;
  }

 private:
  void DrainPendingQueue();

  // Stamps `sequence`, appends to history, and emits `recordAdded` if enabled.
  void AddRecord(Json::Value record);
  void EnforceHistoryBounds();
  void PostRecordAdded(const Json::Value& record);

  // DevTool-thread-only state.
  bool enable_ = false;
  int64_t latest_sequence_ = 0;
  std::deque<Json::Value> history_;
  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;

  // Cross-thread pending queue state, guarded by `pending_mutex_`. Holds JSON
  // records so nothing lepus-typed crosses the thread boundary.
  mutable std::mutex pending_mutex_;
  std::deque<Json::Value> pending_queue_;
  int64_t dropped_count_ = 0;
  bool drain_scheduled_ = false;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_NATIVE_MODULE_NATIVE_MODULE_RECORD_MANAGER_H_
