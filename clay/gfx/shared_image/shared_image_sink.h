// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_SINK_H_
#define CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_SINK_H_

#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>

#include "base/include/closure.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/time/time_delta.h"
#include "skity/geometry/vector.hpp"

namespace clay {

class SharedImageBacking;
class SharedImageBackingUnmanaged;
class FenceSync;

class SharedImageSink : public fml::RefCountedThreadSafe<SharedImageSink> {
 public:
  enum BufferMode {
    // No buffer is needed, which means the user would always acquire a new back
    // backing
    kNoBuffer = 0,
    // In single buffer mode, the user must call ReleaseFront after read
    kSingleBuffer = 1,
    kDoubleBuffer = 2,
    kTripleBuffer = 3,
    kMultiBuffer = 4,
  };

  enum class UpdateFrontMode {
    // Sequential update, display every frame in order
    kSequence,
    // Only update to the latest frame, skip intermediate frames
    kLatest,
  };

  enum class AcquireBackStatus {
    // Acquire succedded.
    kSuccess,
    // Acquire failed. Buffer is full and view is attached.
    kFullAttach,
    // Acquire failed. Buffer is full and view is detached.
    kFullDetach,
    // When this status is not needed.
    kUnkown,
  };

  using GraphicsMemoryHandle = void*;

  explicit SharedImageSink(
      UpdateFrontMode update_front_mode = UpdateFrontMode::kSequence);
  virtual ~SharedImageSink() = default;

  virtual void SetFrameAvailableCallback(const fml::closure& callback) = 0;

  /// Move the current front buffer to next.
  ///
  /// This is a non-blocking call and will return nullptr if no front buffer
  /// available
  /// The caller should receive a notification in `FrameAvailableCallback`
  /// and determine if need to call the function before
  [[nodiscard]] virtual fml::RefPtr<SharedImageBacking> UpdateFront(
      std::unique_ptr<FenceSync> produced_fence_sync) = 0;

  /// Move the current front buffer to latest buffer, if no.
  ///
  /// This is a non-blocking call and will return current back buffer if no
  /// front buffer available.
  [[nodiscard]] virtual fml::RefPtr<SharedImageBacking> UpdateFrontToLatest(
      std::unique_ptr<FenceSync> produced_fence_sync) = 0;

  // Releases the front buffer. This is needed in single buffered mode to
  // allow the producer to take ownership of the buffer.
  virtual void ReleaseFront(std::unique_ptr<FenceSync> produced_fence_sync) = 0;

  /// Acquire the latest back buffer.
  /// The second return value is the buffer age, kinda like
  /// `EGL_BUFFER_AGE_EXT`, 0 means a new buffer without swap history
  ///
  /// This is a blocking call, if no backing buffer available and the buffer
  /// queue is full, it will wait until front buffer swap to back
  [[nodiscard]] virtual std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
  AcquireBack(skity::Vec2 size,
              std::optional<GraphicsMemoryHandle> gfx_handle = {}) = 0;

  /// Try acquire the latest the back buffer.
  ///
  /// This is a non-blocking call, when no backing buffer available, it will
  /// return immediately when null.
  [[nodiscard]] virtual std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t,
                                   AcquireBackStatus>
  TryAcquireBack(skity::Vec2 size,
                 std::optional<GraphicsMemoryHandle> gfx_handle = {}) = 0;

  /// Acquire or steal the latest back buffer, only valid for non-single buffer
  /// mode sink.
  ///
  /// This is a non-blocking call, it will always return the non-current oldest
  /// back buffer.
  /// The reader should pay attention if using frame-count buffer consuming,
  /// since the already available frame may soon be stealed.
  [[nodiscard]] virtual std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
  AcquireBackForced(skity::Vec2 size,
                    std::optional<GraphicsMemoryHandle> gfx_handle = {}) = 0;

  /// Swap the current back buffer to pending front
  virtual bool SwapBack(std::unique_ptr<FenceSync> fence_sync) = 0;

  virtual uint32_t Capacity() const = 0;

  UpdateFrontMode update_front_mode() const { return update_front_mode_; }

  virtual void ClearBackFences() {}

 private:
  UpdateFrontMode update_front_mode_;
};

/// SharedImageSinkManaged owns all internal buffers.
/// It manages the buffer queue and controls the swap of buffers.
class SharedImageSinkManaged final : public SharedImageSink {
 public:
  using SharedImageFactory = std::function<fml::RefPtr<SharedImageBacking>(
      skity::Vec2, std::optional<GraphicsMemoryHandle>)>;
  using UpdateFrontMode = SharedImageSink::UpdateFrontMode;

  SharedImageSinkManaged(
      BufferMode buffer_mode, const SharedImageFactory& shared_image_factory,
      UpdateFrontMode update_front_mode = UpdateFrontMode::kSequence);
  ~SharedImageSinkManaged();

  void SetFrameAvailableCallback(const fml::closure& callback) override;

  [[nodiscard]] fml::RefPtr<SharedImageBacking> UpdateFront(
      std::unique_ptr<FenceSync> produced_fence_sync) override;

  [[nodiscard]] fml::RefPtr<SharedImageBacking> UpdateFrontToLatest(
      std::unique_ptr<FenceSync> produced_fence_sync) override;

  void ReleaseFront(std::unique_ptr<FenceSync> produced_fence_sync) override;

  [[nodiscard]] std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
  AcquireBack(skity::Vec2 size,
              std::optional<GraphicsMemoryHandle> gfx_handle = {}) override;

  [[nodiscard]] std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t,
                           AcquireBackStatus>
  TryAcquireBack(skity::Vec2 size,
                 std::optional<GraphicsMemoryHandle> gfx_handle = {}) override;

  [[nodiscard]] std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
  AcquireBackForced(
      skity::Vec2 size,
      std::optional<GraphicsMemoryHandle> gfx_handle = {}) override;

  bool SwapBack(std::unique_ptr<FenceSync> fence_sync) override;

  uint32_t Capacity() const override { return capacity_; }

  void ClearBackFences() override;

 private:
  void InternalReleaseCurrentFront(
      std::unique_ptr<FenceSync> produced_fence_sync);

  struct SharedImageSlot {
    std::optional<uint64_t> swap_id;
    fml::RefPtr<SharedImageBacking> shared_image;
    bool is_current = false;  // is current front or current back
  };

  uint32_t used_ = 0;
  const uint32_t capacity_;
  SharedImageFactory shared_image_factory_;

  fml::closure frame_available_callback_;

  std::list<SharedImageSlot> front_backings_;
  std::list<SharedImageSlot> back_backings_;
  std::mutex mutex_;
  std::condition_variable backings_cond_;
  std::condition_variable front_cond_;
  uint64_t swap_counter_ = 0;
  const fml::Milliseconds timeout_{500};
};

/// SharedImageSinkUnmanaged doesn't own any internal buffers.
/// The buffer queue is implemented by platforms, like SurfaceTexture on
/// Android and NativeImage on Harmony.
class SharedImageSinkUnmanaged final : public SharedImageSink {
 public:
  using UpdateFrontMode = SharedImageSink::UpdateFrontMode;
  explicit SharedImageSinkUnmanaged(
      fml::RefPtr<SharedImageBackingUnmanaged> buffer_unmanaged,
      UpdateFrontMode update_front_mode = UpdateFrontMode::kSequence);

  ~SharedImageSinkUnmanaged();

  void SetFrameAvailableCallback(const fml::closure& callback) override;

  [[nodiscard]] fml::RefPtr<SharedImageBacking> UpdateFront(
      std::unique_ptr<FenceSync> produced_fence_sync) override;

  [[nodiscard]] fml::RefPtr<SharedImageBacking> UpdateFrontToLatest(
      std::unique_ptr<FenceSync> produced_fence_sync) override;

  void ReleaseFront(std::unique_ptr<FenceSync> produced_fence_sync) override;

  [[nodiscard]] std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
  AcquireBack(skity::Vec2 size,
              std::optional<GraphicsMemoryHandle> gfx_handle = {}) override;

  [[nodiscard]] std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t,
                           AcquireBackStatus>
  TryAcquireBack(skity::Vec2 size,
                 std::optional<GraphicsMemoryHandle> gfx_handle = {}) override;

  [[nodiscard]] std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t>
  AcquireBackForced(
      skity::Vec2 size,
      std::optional<GraphicsMemoryHandle> gfx_handle = {}) override;

  bool SwapBack(std::unique_ptr<FenceSync> fence_sync) override;

  uint32_t Capacity() const override;

 private:
  fml::RefPtr<SharedImageBackingUnmanaged> buffer_unmanaged_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SHARED_IMAGE_SINK_H_
