// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/shared_image_sink.h"

#include <string>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

SharedImageSink::SharedImageSink(UpdateFrontMode update_front_mode)
    : update_front_mode_(update_front_mode) {}

SharedImageSinkManaged::SharedImageSinkManaged(
    BufferMode buffer_mode, const SharedImageFactory& shared_image_factory,
    UpdateFrontMode update_front_mode)
    : SharedImageSink(update_front_mode),
      capacity_(buffer_mode),
      shared_image_factory_(std::move(shared_image_factory)) {}

SharedImageSinkManaged::~SharedImageSinkManaged() = default;

void SharedImageSinkManaged::SetFrameAvailableCallback(
    const fml::closure& callback) {
  std::lock_guard<std::mutex> l(mutex_);

  frame_available_callback_ = callback;

  backings_cond_.notify_one();
}

void SharedImageSinkManaged::ClearBackFences() {
  std::lock_guard<std::mutex> l(mutex_);

  for (auto& backing : back_backings_) {
    std::unique_ptr<FenceSync> fence = backing.shared_image->GetFenceSync();
  }
}

fml::RefPtr<SharedImageBacking> SharedImageSinkManaged::UpdateFront(
    std::unique_ptr<FenceSync> produced_fence_sync) {
  std::unique_lock<std::mutex> l(mutex_);

  InternalReleaseCurrentFront(std::move(produced_fence_sync));

  if (front_backings_.empty()) {
    if (capacity_ == BufferMode::kSingleBuffer && !back_backings_.empty()) {
      // Single Buffer Synchronous Mode
      if (back_backings_.front().is_current) {
        // Wait for back releasing
        if (!front_cond_.wait_for(l, timeout_,
                                  [&] { return !front_backings_.empty(); })) {
          FML_LOG(ERROR) << "timeout to wait front available";
          return nullptr;
        }
      } else {
        // Steal the backing directly
        front_backings_.swap(back_backings_);
      }
    } else {
      FML_LOG(ERROR) << "no pending front available";
      return nullptr;
    }
  }

  auto& curr_front = front_backings_.front();

  curr_front.is_current = true;

  return curr_front.shared_image;
}

fml::RefPtr<SharedImageBacking> SharedImageSinkManaged::UpdateFrontToLatest(
    std::unique_ptr<FenceSync> produced_fence_sync) {
  std::unique_lock<std::mutex> l(mutex_);
  if (front_backings_.empty()) {
    return nullptr;
  }
  while (front_backings_.size() > 1) {
    front_backings_.front().is_current = true;
    // The fence sync is in fact only moved once for the head of front backings
    InternalReleaseCurrentFront(std::move(produced_fence_sync));
  }
  auto& curr_front = front_backings_.front();

  curr_front.is_current = true;

  return curr_front.shared_image;
}

void SharedImageSinkManaged::ReleaseFront(
    std::unique_ptr<FenceSync> produced_fence_sync) {
  std::lock_guard<std::mutex> l(mutex_);

  InternalReleaseCurrentFront(std::move(produced_fence_sync));
}

std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
SharedImageSinkManaged::AcquireBack(
    skity::Vec2 size, std::optional<GraphicsMemoryHandle> gfx_handle) {
  std::unique_lock<std::mutex> l(mutex_);

  if (capacity_ == BufferMode::kNoBuffer) {
    // When capacity equals to ZERO, always create a new back backing.
    SharedImageSlot slot = {.shared_image =
                                shared_image_factory_(size, gfx_handle)};
    if (back_backings_.empty()) {
      back_backings_.emplace_back(slot);
    } else {
      back_backings_.front() = slot;
    }
  } else if (capacity_ == BufferMode::kMultiBuffer) {
    if (back_backings_.empty()) {
      SharedImageSlot slot = {.shared_image =
                                  shared_image_factory_(size, gfx_handle)};
      back_backings_.emplace_back(slot);
    } else {
      if (!frame_available_callback_) {
        return std::make_tuple(nullptr, 0);
      }
      if (back_backings_.front().shared_image->GetSize() != size ||
          (gfx_handle.has_value() && gfx_handle.value())) {
        // maybe resized, discard it
        back_backings_.front() = SharedImageSlot{
            .shared_image = shared_image_factory_(size, gfx_handle)};
      }
    }
  } else {
    if (back_backings_.empty() && used_ < capacity_) {
      SharedImageSlot slot = {.shared_image =
                                  shared_image_factory_(size, gfx_handle)};
      back_backings_.emplace_back(slot);
      used_++;
    } else {
      if (!backings_cond_.wait_for(l, timeout_, [&] {
            return !back_backings_.empty() || !frame_available_callback_;
          })) {
        FML_LOG(ERROR) << "timeout to wait back available";
        return std::make_tuple(nullptr, 0);
      }
      // No `frame_available_callback_` means the sink front is detached,
      // return null directly to prevent deadlock.
      if (!frame_available_callback_) {
        return std::make_tuple(nullptr, 0);
      }

      if (back_backings_.front().shared_image->GetSize() != size ||
          gfx_handle.has_value()) {
        // maybe resized, discard it
        back_backings_.front() = SharedImageSlot{
            .shared_image = shared_image_factory_(size, gfx_handle)};
      }
    }
  }

  auto& curr_backing = back_backings_.front();
  curr_backing.is_current = true;

  uint32_t buffer_age = 0;

  if (curr_backing.swap_id) {
    buffer_age = swap_counter_ - curr_backing.swap_id.value();
  }
  return std::make_tuple(curr_backing.shared_image, buffer_age);
}

std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t,
           SharedImageSink::AcquireBackStatus>
SharedImageSinkManaged::TryAcquireBack(
    skity::Vec2 size, std::optional<GraphicsMemoryHandle> gfx_handle) {
  std::unique_lock<std::mutex> l(mutex_);

  if (capacity_ == BufferMode::kNoBuffer) {
    // When capacity equals to ZERO, always create a new back backing.
    SharedImageSlot slot = {.shared_image =
                                shared_image_factory_(size, gfx_handle)};
    if (back_backings_.empty()) {
      back_backings_.emplace_back(slot);
    } else {
      back_backings_.front() = slot;
    }
  } else {
    if (back_backings_.empty()) {
      if (used_ < capacity_) {
        SharedImageSlot slot = {.shared_image =
                                    shared_image_factory_(size, gfx_handle)};
        back_backings_.emplace_back(slot);
        used_++;
      } else {
        auto status = frame_available_callback_
                          ? AcquireBackStatus::kFullAttach
                          : AcquireBackStatus::kFullDetach;
        return std::make_tuple(nullptr, 0, status);
      }
    } else {
      if (back_backings_.front().shared_image->GetSize() != size ||
          gfx_handle.has_value()) {
        // maybe resized, discard it
        back_backings_.front() = SharedImageSlot{
            .shared_image = shared_image_factory_(size, gfx_handle)};
      }
    }
  }

  auto& curr_backing = back_backings_.front();
  curr_backing.is_current = true;

  uint32_t buffer_age = 0;

  if (curr_backing.swap_id) {
    buffer_age = swap_counter_ - curr_backing.swap_id.value();
  }
  return std::make_tuple(curr_backing.shared_image, buffer_age,
                         AcquireBackStatus::kSuccess);
}

std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
SharedImageSinkManaged::AcquireBackForced(
    skity::Vec2 size, std::optional<GraphicsMemoryHandle> gfx_handle) {
  std::unique_lock<std::mutex> l(mutex_);

  if (capacity_ == BufferMode::kNoBuffer) {
    // When capacity equals to ZERO, always create a new back backing.
    SharedImageSlot slot = {.shared_image =
                                shared_image_factory_(size, gfx_handle)};
    if (back_backings_.empty()) {
      back_backings_.emplace_back(slot);
    } else {
      back_backings_.front() = slot;
    }
  } else {
    if (back_backings_.empty() && used_ < capacity_) {
      SharedImageSlot slot = {.shared_image =
                                  shared_image_factory_(size, gfx_handle)};
      back_backings_.emplace_back(slot);
      used_++;
    } else {
      // No `frame_available_callback_` means the sink front is detached,
      // return null directly to prevent deadlock.
      if (!frame_available_callback_) {
        return std::make_tuple(nullptr, 0);
      }

      if (back_backings_.empty()) {
        if (front_backings_.size() == 0 ||
            (front_backings_.size() == 1 &&
             front_backings_.front().is_current)) {
          return std::make_tuple(nullptr, 0);
        }
        FML_DCHECK(!front_backings_.back().is_current);
        // Steal the unconsumed front backing directly
        back_backings_.splice(back_backings_.begin(), front_backings_,
                              --front_backings_.end());
      }

      if (back_backings_.front().shared_image->GetSize() != size ||
          gfx_handle.has_value()) {
        // maybe resized, discard it
        back_backings_.front() = SharedImageSlot{
            .shared_image = shared_image_factory_(size, gfx_handle)};
      }
    }
  }

  auto& curr_backing = back_backings_.front();
  curr_backing.is_current = true;

  uint32_t buffer_age = 0;

  if (curr_backing.swap_id) {
    buffer_age = swap_counter_ - curr_backing.swap_id.value();
  }
  return std::make_tuple(curr_backing.shared_image, buffer_age);
}

bool SharedImageSinkManaged::SwapBack(std::unique_ptr<FenceSync> fence_sync) {
  std::lock_guard<std::mutex> l(mutex_);
  if (!frame_available_callback_) {
    return false;
  }

  if (back_backings_.empty() || !back_backings_.front().is_current) {
    FML_LOG(ERROR) << "no current backing to swap";
    return false;
  }

  auto& curr_backing = back_backings_.front();

  curr_backing.shared_image->SetFenceSync(std::move(fence_sync));
  curr_backing.swap_id = swap_counter_++;
  curr_backing.is_current = false;

#if 0
  {
    std::string path =
        "Sink" + std::to_string(reinterpret_cast<intptr_t>(this)) + "_swap_" +
        std::to_string(*curr_backing.swap_id) + ".png";
    curr_backing.shared_image->DumpToPng(path);
  }
#endif

  front_backings_.splice(front_backings_.end(), back_backings_,
                         back_backings_.begin(), ++back_backings_.begin());
  if (capacity_ == 1) {
    // Single Buffer Synchronous Mode, we need to notify the front
    front_cond_.notify_one();
  }

  if (frame_available_callback_) {
    frame_available_callback_();
  }

  return true;
}

void SharedImageSinkManaged::InternalReleaseCurrentFront(
    std::unique_ptr<FenceSync> fence_sync) {
  if (front_backings_.empty() || !front_backings_.front().is_current) {
    return;
  }

  if (capacity_ == BufferMode::kNoBuffer) {
    // When capacity equals to ZERO, back would always push back new backings
    // so there is no need to reuse backing here.
    front_backings_.pop_front();
  } else {
    auto& curr_front = front_backings_.front();
    curr_front.is_current = false;
    curr_front.shared_image->SetFenceSync(std::move(fence_sync));
    back_backings_.splice(back_backings_.end(), front_backings_,
                          front_backings_.begin(), ++front_backings_.begin());
  }
  backings_cond_.notify_one();
}

SharedImageSinkUnmanaged::SharedImageSinkUnmanaged(
    fml::RefPtr<SharedImageBackingUnmanaged> buffer_unmanaged,
    UpdateFrontMode update_front_mode)
    : SharedImageSink(update_front_mode),
      buffer_unmanaged_(std::move(buffer_unmanaged)) {}

SharedImageSinkUnmanaged::~SharedImageSinkUnmanaged() = default;

void SharedImageSinkUnmanaged::SetFrameAvailableCallback(
    const fml::closure& callback) {
  buffer_unmanaged_->SetFrameAvailableCallback(callback);
}

fml::RefPtr<SharedImageBacking> SharedImageSinkUnmanaged::UpdateFront(
    std::unique_ptr<FenceSync> produced_fence_sync) {
  if (produced_fence_sync) {
    FML_LOG(WARNING) << "Unmanaged shared image sink does not support fence";
  }
  if (!buffer_unmanaged_->UpdateFront()) {
    FML_LOG(ERROR) << "Failed to UpdateFront";
    return nullptr;
  }
  return buffer_unmanaged_;
}

fml::RefPtr<SharedImageBacking> SharedImageSinkUnmanaged::UpdateFrontToLatest(
    std::unique_ptr<FenceSync> produced_fence_sync) {
  return UpdateFront(std::move(produced_fence_sync));
}

void SharedImageSinkUnmanaged::ReleaseFront(
    std::unique_ptr<FenceSync> produced_fence_sync) {
  if (produced_fence_sync) {
    FML_LOG(WARNING) << "Unmanaged shared image sink does not support fence";
  }
  buffer_unmanaged_->ReleaseFront();
}

std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
SharedImageSinkUnmanaged::AcquireBack(
    skity::Vec2 size, std::optional<GraphicsMemoryHandle> gfx_handle) {
  if (gfx_handle.has_value()) {
    FML_LOG(ERROR) << "SharedImageSinkUnmanaged::AcquireBack can not "
                      "retain gfx_handle";
  }
  if (size.x > 0 & size.y > 0 && !buffer_unmanaged_->SetSize(size)) {
    FML_LOG(ERROR)
        << "SharedImageSinkUnmanaged::AcquireBack can not "
           "AcquireBack by size. Please set the internal buffer's size";
  }
  return std::make_tuple(buffer_unmanaged_, buffer_unmanaged_->AcquireBack());
}

bool SharedImageSinkUnmanaged::SwapBack(std::unique_ptr<FenceSync> fence_sync) {
  return buffer_unmanaged_->SwapBack();
}

uint32_t SharedImageSinkUnmanaged::Capacity() const {
  return buffer_unmanaged_->Capacity();
}

std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t,
           SharedImageSink::AcquireBackStatus>
SharedImageSinkUnmanaged::TryAcquireBack(
    skity::Vec2 size, std::optional<GraphicsMemoryHandle> gfx_handle) {
  auto acquire_result = AcquireBack(size, gfx_handle);
  auto status = std::make_tuple(AcquireBackStatus::kUnkown);
  auto res = std::tuple_cat(acquire_result, status);
  return res;
}

std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
SharedImageSinkUnmanaged::AcquireBackForced(
    skity::Vec2 size, std::optional<GraphicsMemoryHandle> gfx_handle) {
  return AcquireBack(size, gfx_handle);
}

}  // namespace clay
