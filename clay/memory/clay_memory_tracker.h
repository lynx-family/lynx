// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_MEMORY_CLAY_MEMORY_TRACKER_H_
#define CLAY_MEMORY_CLAY_MEMORY_TRACKER_H_

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace clay {

enum class ImageMemoryType { kEncoded, kDecoded };

struct ClayPageMemorySample {
  // Bytes attributed to this page after deduplicating backing allocations.
  int64_t accounted_ui_bytes{0};
  struct Details {
    int64_t image_bytes{0};
  } details;
};

// Created once for a sampled Clay view. The reporter and tracked CPU data can
// retain the tracker without retaining the view.
class ClayMemoryTracker
    : public std::enable_shared_from_this<ClayMemoryTracker> {
 public:
  // Takes a fresh, unpublished allocation. Share the returned pointer so all
  // references to the payload keep the same accounting lifetime.
  template <typename T>
  std::shared_ptr<T> TrackImageData(ImageMemoryType type,
                                    std::shared_ptr<T>&& allocation,
                                    size_t bytes) {
    if (!allocation || bytes == 0) {
      return std::move(allocation);
    }
    auto* data = allocation.get();
    return std::shared_ptr<T>(
        std::make_shared<DataAllocation>(shared_from_this(), type,
                                         std::move(allocation), bytes),
        data);
  }

  // The backing can outlive the Skity texture object through a release
  // queue. Track its owner identity without extending that lifetime.
  void TrackImageTexture(const std::shared_ptr<const void>& texture,
                         size_t bytes) {
    if (!texture || bytes == 0) {
      return;
    }
    std::lock_guard<std::mutex> lock(texture_mutex_);
    const bool inserted = textures_.try_emplace(texture, bytes).second;
    if (inserted && --textures_until_prune_ == 0) {
      PruneAndSumTextureBytes();
    }
  }

  ClayPageMemorySample GetMemorySample() {
    std::lock_guard<std::mutex> lock(texture_mutex_);
    const int64_t encoded_bytes =
        data_bytes_[static_cast<size_t>(ImageMemoryType::kEncoded)].load(
            std::memory_order_relaxed);
    const int64_t decoded_bytes =
        data_bytes_[static_cast<size_t>(ImageMemoryType::kDecoded)].load(
            std::memory_order_relaxed);
    const int64_t image_bytes =
        encoded_bytes + decoded_bytes + PruneAndSumTextureBytes();
    return {image_bytes, {image_bytes}};
  }

  // Use the frozen sample so serializing details never touches live counters.
  template <typename JsonValue>
  static void BuildClayMemoryDetails(
      const ClayPageMemorySample::Details& details, JsonValue& object) {
    object["imageBytes"] =
        static_cast<typename JsonValue::Int64>(details.image_bytes);
  }

 private:
  class DataAllocation {
   public:
    DataAllocation(std::shared_ptr<ClayMemoryTracker> tracker,
                   ImageMemoryType type, std::shared_ptr<const void> allocation,
                   size_t bytes)
        : tracker_(std::move(tracker)),
          allocation_(std::move(allocation)),
          counter_(tracker_->data_bytes_[static_cast<size_t>(type)]),
          bytes_(static_cast<int64_t>(bytes)) {
      counter_.fetch_add(bytes_, std::memory_order_relaxed);
    }

    ~DataAllocation() {
      allocation_.reset();
      counter_.fetch_sub(bytes_, std::memory_order_relaxed);
    }

   private:
    const std::shared_ptr<ClayMemoryTracker> tracker_;
    std::shared_ptr<const void> allocation_;
    std::atomic<int64_t>& counter_;
    const int64_t bytes_;
  };

  // Requires texture_mutex_. Also prunes in the writer path so animated
  // images cannot grow the list indefinitely between reporter snapshots.
  int64_t PruneAndSumTextureBytes() {
    int64_t bytes = 0;
    for (auto it = textures_.begin(); it != textures_.end();) {
      if (it->first.expired()) {
        it = textures_.erase(it);
      } else {
        bytes += static_cast<int64_t>(it->second);
        ++it;
      }
    }
    textures_until_prune_ =
        std::max(kMinTexturePruneInterval, textures_.size());
    return bytes;
  }

  static constexpr size_t kMinTexturePruneInterval = 64;
  std::array<std::atomic<int64_t>, 2> data_bytes_{0, 0};
  std::mutex texture_mutex_;
  std::map<std::weak_ptr<const void>, size_t, std::owner_less<>> textures_;
  size_t textures_until_prune_ = kMinTexturePruneInterval;
};

}  // namespace clay

#endif  // CLAY_MEMORY_CLAY_MEMORY_TRACKER_H_
