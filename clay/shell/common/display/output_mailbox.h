// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_OUTPUT_MAILBOX_H_
#define CLAY_SHELL_COMMON_DISPLAY_OUTPUT_MAILBOX_H_

#include <memory>
#include <mutex>
#include <optional>
#include <utility>

#include "clay/common/graphics/output_texture.h"

namespace clay::display {
// Only unconsumed composed outputs live here. Taking an output transfers its
// CPU lease to the host; the host retains its current frame and GPU read uses.
class OutputMailbox {
 public:
  struct Acquisition {
    std::shared_ptr<const OutputTexture> texture;
    uint64_t generation;
    uint64_t size_generation;
    bool open;
    bool legacy_output;
  };

  uint64_t Open() {
    std::shared_ptr<const OutputTexture> previous;
    uint64_t generation;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      previous = std::move(output_);
      last_content_version_ = 0;
      open_ = true;
      legacy_output_ = false;
      generation = ++generation_;
    }
    return generation;
  }
  // A Hybrid page publishes its background through the existing SharedImage
  // sink. A closed staged window alone does not enable that consumer path.
  void UseLegacyOutput() {
    std::shared_ptr<const OutputTexture> previous;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      open_ = false;
      legacy_output_ = true;
      ++generation_;
      previous = std::move(output_);
    }
  }
  void Close(uint64_t generation) {
    std::shared_ptr<const OutputTexture> previous;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (generation != generation_) return;
      open_ = false;
      previous = std::move(output_);
    }
  }
  void Resize(uint32_t width, uint32_t height) {
    std::shared_ptr<const OutputTexture> previous;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (width_ == width && height_ == height) return;
      ++size_generation_;
      width_ = width;
      height_ = height;
      previous = std::move(output_);
    }
  }
  uint64_t size_generation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_generation_;
  }
  bool Publish(uint64_t generation, std::shared_ptr<const OutputTexture> output,
               std::optional<uint64_t> size_generation = std::nullopt) {
    std::shared_ptr<const OutputTexture> previous;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!open_ || generation != generation_ || !output || !Fits(*output))
        return false;
      if (size_generation && *size_generation != size_generation_) return false;
      if (!output->ready()) return false;
      if (output->content_version() <= last_content_version_) return false;
      last_content_version_ = output->content_version();
      previous = std::exchange(output_, std::move(output));
    }
    // The replacement is already visible before releasing the old CPU lease.
    // Resource destruction must not run while holding the transfer lock.
    return true;
  }
  Acquisition TakeLatest() {
    std::lock_guard<std::mutex> lock(mutex_);
    return {std::exchange(output_, {}), generation_, size_generation_, open_,
            legacy_output_};
  }

 private:
  bool Fits(const OutputTexture& output) const {
    return (!width_ || output.image().width == width_) &&
           (!height_ || output.image().height == height_);
  }
  mutable std::mutex mutex_;
  uint64_t generation_ = 0, size_generation_ = 0;
  uint64_t last_content_version_ = 0;
  uint32_t width_ = 0, height_ = 0;
  bool open_ = false;
  bool legacy_output_ = false;
  std::shared_ptr<const OutputTexture> output_;
};
}  // namespace clay::display
#endif  // CLAY_SHELL_COMMON_DISPLAY_OUTPUT_MAILBOX_H_
