// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/native_module/native_module_record_manager.h"

#include <utility>

#include "core/runtime/lepus/json_parser.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {
namespace {

// CDP transport field names for the LynxNativeModule domain. This part keeps
// them local; the full record-builder key set lives in the follow-up data part
// (native_module_record_keys.h).
constexpr char kSequenceKey[] = "sequence";
constexpr char kRecordsKey[] = "records";
constexpr char kLatestSequenceKey[] = "latestSequence";
constexpr char kRecordKey[] = "record";
constexpr char kRecordAddedMethod[] = "LynxNativeModule.recordAdded";

// Mirrors the lepus->Json path in InspectorTASMExecutor. Null on parse failure.
Json::Value RecordToJson(const lepus::Value& record) {
  Json::Value json;
  Json::Reader reader;
  reader.parse(lepus::lepusValueToString(record), json, false);
  return json;
}

}  // namespace

NativeModuleRecordManager::NativeModuleRecordManager(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_wp_(devtool_mediator) {}

void NativeModuleRecordManager::EnqueueRecordOnJSThread(lepus::Value record) {
  // Serialize before locking so the mutex only guards the queue push.
  Json::Value json = RecordToJson(record);

  {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    // Drop-new when full so the JS thread never blocks.
    if (pending_queue_.size() >= kMaxPendingCount) {
      ++dropped_count_;
      return;
    }
    pending_queue_.push_back(std::move(json));
    if (drain_scheduled_) {
      return;
    }
    drain_scheduled_ = true;
  }

  if (auto devtool_mediator = devtool_mediator_wp_.lock();
      devtool_mediator != nullptr) {
    devtool_mediator->RunOnDevToolThread(
        [self = shared_from_this()] { self->DrainPendingQueue(); });
  }
}

void NativeModuleRecordManager::DrainPendingQueue() {
  std::deque<Json::Value> drained;
  {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    drained.swap(pending_queue_);
    drain_scheduled_ = false;
  }
  for (auto& record : drained) {
    AddRecord(std::move(record));
  }
}

void NativeModuleRecordManager::AddRecord(Json::Value record) {
  // Sequence in arrival order is the sole ordering key the frontend relies on.
  ++latest_sequence_;
  record[kSequenceKey] = static_cast<Json::Int64>(latest_sequence_);

  // History grows regardless; live events only fire while enabled.
  if (enable_) {
    PostRecordAdded(record);
  }
  history_.push_back(std::move(record));
  EnforceHistoryBounds();
}

void NativeModuleRecordManager::Enable() { enable_ = true; }

void NativeModuleRecordManager::Disable() { enable_ = false; }

void NativeModuleRecordManager::GetRecords(
    const std::shared_ptr<MessageSender>& sender, int64_t id) {
  Json::Value records(Json::ValueType::arrayValue);
  for (const auto& entry : history_) {
    records.append(entry);
  }
  Json::Value result;
  result[kRecordsKey] = std::move(records);
  result[kLatestSequenceKey] = static_cast<Json::Int64>(latest_sequence_);

  Json::Value response;
  response["id"] = id;
  response["result"] = std::move(result);
  sender->SendMessage("CDP", response);
}

void NativeModuleRecordManager::EnforceHistoryBounds() {
  while (history_.size() > kMaxHistoryCount) {
    history_.pop_front();
  }
}

void NativeModuleRecordManager::PostRecordAdded(const Json::Value& record) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  if (devtool_mediator == nullptr) {
    return;
  }
  Json::Value params;
  params[kRecordKey] = record;
  Json::Value content;
  content["method"] = kRecordAddedMethod;
  content["params"] = std::move(params);
  devtool_mediator->SendCDPEvent(content);
}

}  // namespace devtool
}  // namespace lynx
