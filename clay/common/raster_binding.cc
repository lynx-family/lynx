// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/raster_binding.h"

#include <utility>

#include "clay/fml/logging.h"
namespace clay::display {
RasterBinding::RasterBinding(RasterBindingKey key,
                             fml::RefPtr<fml::TaskRunner> worker,
                             fml::RefPtr<fml::TaskRunner> platform,
                             std::function<void(uint64_t)> announce)
    : key_(key),
      worker_(std::move(worker)),
      platform_(std::move(platform)),
      announce_layout_(std::move(announce)) {}
RasterBinding::State RasterBinding::state() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_;
}
void RasterBinding::Initialize(std::function<bool()> initialize,
                               std::function<void(bool)> ready) {
  worker_->PostTask([self = shared_from_this(),
                     initialize = std::move(initialize),
                     ready = std::move(ready)] {
    if (self->state() != State::kRegistering) return;
    bool success = initialize();
    {
      std::lock_guard<std::mutex> lock(self->mutex_);
      if (self->state_ != State::kRegistering) return;
      self->state_ = success ? State::kReady : State::kClosing;
      if (success) self->ScheduleDrainLocked();
    }
    self->platform_->PostTask([self, ready, success] {
      bool notify;
      {
        std::lock_guard<std::mutex> lock(self->mutex_);
        notify = !self->close_started_ &&
                 self->state_ == (success ? State::kReady : State::kClosing);
      }
      if (notify) ready(success);
    });
  });
}
uint64_t RasterBinding::PostTreeUpdate(TreeTask task) {
  TreeTask discarded;
  uint64_t version;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == State::kClosing || state_ == State::kRetired) return 0;
    version = ++layout_version_;
    discarded = std::move(tree_);
    tree_ = std::move(task);
    // Announcement is queued before Worker can publish that layout's outputs.
    platform_->PostTask([self = shared_from_this(), version] {
      if (self->state() != State::kClosing && self->state() != State::kRetired)
        self->announce_layout_(version);
    });
    ScheduleDrainLocked();
  }
  return version;
}
bool RasterBinding::NotifyContentUpdated(ContentUpdate update) {
  ContentUpdate discarded;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == State::kClosing || state_ == State::kRetired ||
        !update.apply || !update.source_generation)
      return false;
    auto key = std::make_pair(update.source_generation, update.content_version);
    auto found = content_watermarks_.find(update.image_id);
    if (found != content_watermarks_.end() && key <= found->second)
      return false;
    if (found == content_watermarks_.end() && content_watermarks_.size() >= 64)
      return false;
    content_watermarks_[update.image_id] = key;
    discarded = std::move(contents_[update.image_id]);
    contents_[update.image_id] = std::move(update);
    ScheduleDrainLocked();
  }
  return true;
}
void RasterBinding::ForgetContentSource(int64_t id) {
  ContentUpdate discarded;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = contents_.find(id);
    if (it != contents_.end()) {
      discarded = std::move(it->second);
      contents_.erase(it);
    }
    content_watermarks_.erase(id);
  }
}
void RasterBinding::ScheduleDrainLocked() {
  if (wake_pending_ || state_ != State::kReady || (!tree_ && contents_.empty()))
    return;
  wake_pending_ = true;
  worker_->PostTask([self = shared_from_this()] { self->Drain(); });
}
void RasterBinding::Drain() {
  FML_DCHECK(worker_->RunsTasksOnCurrentThread());
  TreeTask tree;
  std::map<int64_t, ContentUpdate> contents;
  uint64_t version;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    wake_pending_ = false;
    if (state_ != State::kReady) return;
    tree = std::move(tree_);
    contents.swap(contents_);
    version = layout_version_;
  }
  if (tree) tree(version);
  for (auto& item : contents) {
    if (state() != State::kReady) break;
    item.second.apply();
  }
}
void RasterBinding::Close(std::function<void(std::function<void()>)> release,
                          std::function<void()> drained) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == State::kRetired) {
      if (drained) platform_->PostTask(std::move(drained));
      return;
    }
    if (drained) close_callbacks_.push_back(std::move(drained));
    if (close_started_) return;
    close_started_ = true;
    state_ = State::kClosing;
  }
  worker_->PostTask([self = shared_from_this(), release = std::move(release)] {
    TreeTask discarded;
    std::map<int64_t, ContentUpdate> contents;
    {
      std::lock_guard<std::mutex> lock(self->mutex_);
      discarded = std::move(self->tree_);
      contents.swap(self->contents_);
    }
    release([self] {
      self->platform_->PostTask([self] {
        std::vector<std::function<void()>> callbacks;
        {
          std::lock_guard<std::mutex> lock(self->mutex_);
          self->state_ = State::kRetired;
          callbacks.swap(self->close_callbacks_);
        }
        for (auto& callback : callbacks) callback();
      });
    });
  });
}
}  // namespace clay::display
