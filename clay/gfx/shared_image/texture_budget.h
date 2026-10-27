// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_TEXTURE_BUDGET_H_
#define CLAY_GFX_SHARED_IMAGE_TEXTURE_BUDGET_H_
#include <atomic>
#include <cstdint>
#include <memory>
#include <utility>

#include "clay/flow/compositor/frame_types.h"
namespace clay::display {
// Device-domain accounting survives windows, pages, and worker registrations.
class TextureBudget : public std::enable_shared_from_this<TextureBudget> {
 public:
  // Estimates the maximum slot population without reserving storage. Actual
  // allocations still use Reserve/Resize against the same device-domain cap.
  static uint64_t EstimatePoolBytes(const OutputConfig& config,
                                    uint64_t output_pixels,
                                    uint64_t output_bytes_per_pixel,
                                    uint64_t display_bytes_per_pixel,
                                    uint64_t window_bytes_per_pixel) {
    const uint64_t pixels = static_cast<uint64_t>(config.width) * config.height;
    const uint64_t window_bytes =
        config.uses_window_attachments
            ? window_bytes_per_pixel * kMaxDisplayFramesInFlight
            : 0;
    return output_pixels * output_bytes_per_pixel *
               kMaxRasterBuffersPerSegment +
           pixels * (display_bytes_per_pixel * kMaxDisplayOutputBuffers +
                     window_bytes);
  }
  class Reservation {
   public:
    Reservation(std::shared_ptr<TextureBudget> owner, uint64_t bytes)
        : owner_(std::move(owner)), bytes_(bytes) {}
    ~Reservation() { owner_->used_.fetch_sub(bytes_); }
    bool Resize(uint64_t bytes) {
      if (bytes > bytes_ && !owner_->Acquire(bytes - bytes_)) return false;
      if (bytes < bytes_) owner_->used_.fetch_sub(bytes_ - bytes);
      bytes_ = bytes;
      return true;
    }

   private:
    std::shared_ptr<TextureBudget> owner_;
    uint64_t bytes_;
  };
  // Covers both three-slot pools, native window attachments and explicit
  // capture headroom. Storage is allocated lazily, never all at registration.
  explicit TextureBudget(uint64_t limit = 256u * 1024u * 1024u)
      : limit_(limit) {}
  std::shared_ptr<Reservation> Reserve(uint64_t bytes) {
    if (!bytes || !Acquire(bytes)) return nullptr;
    return std::make_shared<Reservation>(shared_from_this(), bytes);
  }
  uint64_t used() const { return used_.load(); }
  uint64_t limit() const { return limit_.load(); }
  void SetLowMemory() { limit_.store(128u * 1024u * 1024u); }

 private:
  bool Acquire(uint64_t bytes) {
    uint64_t used = used_.load();
    do {
      auto limit = limit_.load();
      if (bytes > limit || used > limit - bytes) return false;
    } while (!used_.compare_exchange_weak(used, used + bytes));
    return true;
  }
  std::atomic<uint64_t> limit_, used_{0};
};
}  // namespace clay::display
#endif  // CLAY_GFX_SHARED_IMAGE_TEXTURE_BUDGET_H_
