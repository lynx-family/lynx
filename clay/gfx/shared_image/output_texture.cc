// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/output_texture.h"

#include <utility>
namespace clay::display {
uint64_t AllocateResourceId() {
  static std::atomic<uint64_t> next{1};
  return next.fetch_add(1);
}
bool ResourceRecord::PollLocked() {
  bool finished = !writing_ && !unresolved_write_;
  if (ready_) {
    auto status = ready_->Poll();
    if (status == CompletionStatus::kFailed) retired_.store(true);
    finished &= ready_->IsSafeToRelease();
  }
  for (auto it = reads_.begin(); it != reads_.end();) {
    auto status = it->second ? it->second->Poll() : CompletionStatus::kPending;
    if (status == CompletionStatus::kFailed) retired_.store(true);
    if (!it->second || !it->second->IsSafeToRelease()) {
      finished = false;
      ++it;
    } else {
      if (status == CompletionStatus::kFailed) retired_.store(true);
      it = reads_.erase(it);
    }
  }
  return finished;
}
bool ResourceRecord::BeginWrite() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!PollLocked() || retired_ || cpu_leases_ != 0) return false;
  writing_ = true;
  ready_.reset();
  return true;
}
void ResourceRecord::EndWrite(std::shared_ptr<const GPUCompletion> ready) {
  std::lock_guard<std::mutex> lock(mutex_);
  writing_ = false;
  unresolved_write_ = !ready;
  ready_ = std::move(ready);
  if (!ready_) retired_.store(true);
}
uint64_t ResourceRecord::BeginRead() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (retired_ || writing_ || !ready_ ||
      ready_->Poll() != CompletionStatus::kComplete)
    return 0;
  auto use = ++next_use_id_;
  reads_.emplace(use, nullptr);
  return use;
}
void ResourceRecord::EndRead(uint64_t use_id,
                             std::shared_ptr<const GPUCompletion> completion) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = reads_.find(use_id);
  if (it != reads_.end()) {
    if (!completion) retired_.store(true);
    it->second = std::move(completion);
  }
}
void ResourceRecord::CancelRead(uint64_t use_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  reads_.erase(use_id);
}
bool ResourceRecord::CanReuse() {
  std::lock_guard<std::mutex> lock(mutex_);
  return PollLocked() && !retired_ && cpu_leases_ == 0;
}
bool ResourceRecord::CanDestroy() {
  std::lock_guard<std::mutex> lock(mutex_);
  return PollLocked() && cpu_leases_ == 0;
}
std::shared_ptr<const GPUCompletion> ResourceRecord::ready() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return ready_;
}
}  // namespace clay::display
