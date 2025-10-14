// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_H_
#define CLAY_GFX_IMAGE_IMAGE_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/time/timer.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class ImageDescriptor;
class ImageProducer;
class SkImageHolder;

// Image represents a image, which can be a single frame image (such as png,
// jpg, etc.) or a multi-frame image (such as GIF).
class Image : public std::enable_shared_from_this<Image> {
 public:
  class Notifier {
   public:
    virtual ~Notifier() = default;
    virtual void ImageHasNoAccessor(const Image* image) = 0;
    virtual fml::RefPtr<GPUUnrefQueue> GetUnrefQueue() = 0;
  };

  static std::shared_ptr<Image> CreateImage(
      const std::string& url, GrDataPtr data,
      fml::RefPtr<ImageDescriptor> descriptor,
      std::weak_ptr<Notifier> weak_notifier,
      fml::RefPtr<fml::TaskRunner> ui_runner,
      fml::RefPtr<fml::TaskRunner> raster_runner, bool is_svg,
      bool use_texture_backend, bool is_promise = false,
      bool enable_low_quality_image = false, bool is_deferred = false,
      bool decode_with_priority = false);

  static std::shared_ptr<Image> CloneImage(std::shared_ptr<Image> image);

  ~Image();

  std::string GetUrl() const { return url_; }
  void SetRawData(GrDataPtr data);

  int GetWidth();
  int GetHeight();
  void SetGraphicsImage(fml::RefPtr<GraphicsImage> graphics_image);
  fml::RefPtr<GraphicsImage> GetGraphicsImage(
      DecodePriority priority = DecodePriority::kImmediate);
  // Try to get GraphicsImage without triggering decode
  fml::RefPtr<GraphicsImage> TryGetGraphicsImage() const;
  size_t GetGraphicsImageAllocSize() const;

  std::unique_ptr<ImageResource> GetAccessor(bool from_raster = false);
  size_t GetUIAccessorCount() const { return ui_accessors_.size(); }

#if defined(ENABLE_SVG)
  void SetContentHash(std::string hash_string) {
    content_hash_ = std::move(hash_string);
  }
  const std::string& GetContentHash() const { return content_hash_; }
#endif

  // Support SVG.
  bool IsSVG() const { return is_svg_; }

  bool IsSingleFrameImage() const;

  void AccessorCreated(ImageResource* accessor);
  void AccessorDestroyed(ImageResource* accessor);

  void SetExpectSizeCalculator(
      std::function<skity::Vec2(skity::Vec2)> calculator,
      bool force_use_original_size);

  bool MaybeAnimated() const;

  bool UseTextureBackend() const { return use_texture_backend_; }
  bool UsePromise() const { return is_promise_; }
  bool IsDeferred() const { return is_deferred_; }
  bool DecodeWithPriority() const { return decode_with_priority_; }

  void SetAutoPlay(bool auto_play);
  void SetLoopCount(int loop_count);

  void StartAnimate();
  void StopAnimation();
  void PauseAnimation();
  void ResumeAnimation();

  void SetIsActive(bool is_active);
  bool IsActive() const { return is_active_; }

  bool NeedDecode() const;
  void CancelUpload();

 private:
  enum RunningState {
    kUndefined = 0,
    kRunning = 1,
    kPause = 2,
  };
  enum Operation {
    kStartAnimate = 0,
    kStopAnimation,
    kPauseAnimation,
    kResumeAnimation,
  };
  Image(const std::string& url, GrDataPtr data,
        std::weak_ptr<Notifier> weak_notifier,
        fml::RefPtr<fml::TaskRunner> ui_runner,
        fml::RefPtr<fml::TaskRunner> raster_runner, bool is_svg,
        bool is_promise, bool enable_low_quality_image,
        bool use_texture_backend, bool is_deferred, bool decode_with_priority);

  static int NextUniqueId();

  bool IsAnimationFinished() const;
  void ResetAnimation(bool reuse = false);

  size_t FrameCount() const;
  int RepetitionCount() const;
  void StartAnimation();
  bool IsAnimating() const;
  bool IsFirstFrame() const;
  bool HasCurrentFrameDurationPassed(const int64_t timestamp) const;
  size_t CurrentFrame() const { return current_frame_; }

  // Animation.
  void ScheduleNewFrame();
  void AdvanceAnimation();
  bool ShouldPauseAnimation() const;
  void AnimationAdvanced();
  void DecodeFinish(bool success);
  void UploadFinish(bool success);
  void WillRegisterUploadTask(const std::function<void()>& task);
  void OnStartPlay();
  void OnCurrentLoopComplete();
  void OnFinalLoopComplete();
  void PushOperation(Operation operation);
  void HandlePendingOperations();

  // The index of the current frame of animation.
  int current_frame_ = -1;
  // How many total animation loops we should do.
  int loop_count_ = 0;
  // How many repetitions we've finished.
  int repetitions_complete_ = 0;
  // Whether or not we've completed the entire animation.
  bool animation_finished_ = false;
  // Whether or not the animation is running. It is controlled by front-end.
  RunningState running_state_ = kUndefined;

  bool auto_play_ = true;

  int64_t current_frame_start_time_ = 0;
  int64_t current_frame_duration_ = 0;

  const std::string url_;
  GrDataPtr raw_data_;
  // Support SVG.
  const bool is_svg_ = false;
#if defined(ENABLE_SVG)
  // content_hash_ stores the hash value of the SVG image content. This hash is
  // primarily used to cache the content of SVG images within the ImageManager.
  std::string content_hash_;
#endif
  const bool is_promise_ = false;
  std::weak_ptr<Notifier> weak_notifier_;
  std::unique_ptr<fml::OneshotTimer> frame_timer_;
  std::shared_ptr<ImageProducer> image_producer_;
  std::unordered_set<ImageResource*> ui_accessors_;
  std::unordered_set<ImageResource*> raster_accessors_;
  fml::RefPtr<fml::TaskRunner> ui_runner_;
  fml::RefPtr<fml::TaskRunner> raster_runner_;
  std::list<Operation> pending_operations_;

  const bool use_texture_backend_ = true;
  // TODO: support deferred decode for svg.
  const bool is_deferred_ = false;
  const bool decode_with_priority_ = false;

  std::shared_ptr<std::recursive_mutex> mutex_ =
      std::make_shared<std::recursive_mutex>();

  // is_active_ refer to whether the image is still bound with any views that
  // are currently attached to the view tree.
  bool is_active_ = true;
  mutable bool force_decode_ = false;
  int image_id_;
};

}  // namespace clay
#endif  // CLAY_GFX_IMAGE_IMAGE_H_
