// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_RASTER_BINDING_H_
#define CLAY_COMMON_RASTER_BINDING_H_
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/flow/compositor/frame_types.h"
namespace clay::display {
struct ContentUpdate {
  int64_t image_id = 0;
  uint64_t source_generation = 0, content_version = 0;
  std::function<void()> apply;
};
class RasterThreadGroup;
// The only cross-sequence producer route. No mutable Rasterizer is exposed.
class RasterBinding : public std::enable_shared_from_this<RasterBinding> {
 public:
  enum class State { kRegistering, kReady, kClosing, kRetired };
  using TreeTask = std::function<void(uint64_t)>;
  const RasterBindingKey& key() const { return key_; }
  State state() const;
  uint64_t PostTreeUpdate(TreeTask task);
  bool NotifyContentUpdated(ContentUpdate update);
  void ForgetContentSource(int64_t id);
  void Close(std::function<void(std::function<void()>)> release,
             std::function<void()> drained);

 private:
  friend class RasterThreadGroup;
  RasterBinding(RasterBindingKey key, fml::RefPtr<fml::TaskRunner> worker,
                fml::RefPtr<fml::TaskRunner> platform,
                std::function<void(uint64_t)> announce_layout);
  void Initialize(std::function<bool()> initialize,
                  std::function<void(bool)> ready);
  void ScheduleDrainLocked();
  void Drain();
  const RasterBindingKey key_;
  const fml::RefPtr<fml::TaskRunner> worker_, platform_;
  const std::function<void(uint64_t)> announce_layout_;
  mutable std::mutex mutex_;
  State state_ = State::kRegistering;
  uint64_t layout_version_ = 0;
  TreeTask tree_;
  std::map<int64_t, ContentUpdate> contents_;
  std::map<int64_t, std::pair<uint64_t, uint64_t>> content_watermarks_;
  bool wake_pending_ = false, close_started_ = false;
  std::vector<std::function<void()>> close_callbacks_;
};
}  // namespace clay::display
#endif  // CLAY_COMMON_RASTER_BINDING_H_
