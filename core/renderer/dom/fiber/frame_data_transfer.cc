// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/frame_data_transfer.h"

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace lynx {
namespace tasm {
namespace {

using FrameDataTransferMap =
    std::unordered_map<int64_t, std::unique_ptr<lepus::Value>>;

std::atomic<int64_t>& NextFrameDataTransferHandle() {
  static std::atomic<int64_t> next_handle{1};
  return next_handle;
}

std::mutex& FrameDataTransferMutex() {
  static std::mutex mutex;
  return mutex;
}

FrameDataTransferMap& FrameDataTransfers() {
  static FrameDataTransferMap transfers;
  return transfers;
}

}  // namespace

int64_t RegisterFrameDataTransferValue(std::unique_ptr<lepus::Value> value) {
  if (!value) {
    return 0;
  }
  int64_t handle = NextFrameDataTransferHandle().fetch_add(1);
  if (handle == 0) {
    handle = NextFrameDataTransferHandle().fetch_add(1);
  }

  std::lock_guard<std::mutex> lock(FrameDataTransferMutex());
  FrameDataTransfers()[handle] = std::move(value);
  return handle;
}

std::unique_ptr<lepus::Value> ConsumeFrameDataTransferValue(int64_t handle) {
  if (handle == 0) {
    return nullptr;
  }

  std::lock_guard<std::mutex> lock(FrameDataTransferMutex());
  auto& transfers = FrameDataTransfers();
  auto iter = transfers.find(handle);
  if (iter == transfers.end()) {
    return nullptr;
  }

  auto value = std::move(iter->second);
  transfers.erase(iter);
  return value;
}

}  // namespace tasm
}  // namespace lynx
