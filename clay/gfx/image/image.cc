// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "base/include/compiler_specific.h"
#include "base/include/fml/make_copyable.h"
#include "base/include/fml/message_loop.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "base/include/string/string_utils.h"
#include "clay/common/thread_host.h"
#include "clay/fml/logging.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/image/image_data_cache.h"
#include "clay/gfx/image/image_producer.h"
#include "clay/gfx/image/image_upload_manager.h"
#include "clay/gfx/image/skimage_holder.h"
#include "clay/shell/common/one_shot_callback.h"

#if defined(ENABLE_SVG)
#include "clay/gfx/image/svg_image_holder.h"
#endif

namespace clay {

Image::Image(const std::string& url, GrDataPtr data,
             std::weak_ptr<Image::Notifier> weak_notifier,
             fml::RefPtr<fml::TaskRunner> ui_runner,
             fml::RefPtr<fml::TaskRunner> raster_runner, bool is_svg,
             bool is_promise, bool enable_low_quality_image,
             bool use_texture_backend, bool is_deferred,
             bool decode_with_priority)
    : url_(url),
      raw_data_(data),
      is_svg_(lynx::base::EndsWith(url, ".svg") || is_svg),
      is_promise_(is_promise),
      weak_notifier_(weak_notifier),
      ui_runner_(ui_runner),
      raster_runner_(raster_runner),
      use_texture_backend_(use_texture_backend),
      is_deferred_(is_deferred),
      decode_with_priority_(decode_with_priority),
      image_id_(NextUniqueId()) {
  if (data) {
    ImageDataCache::GetInstance().CacheImageData(url, data);
  }
}

std::shared_ptr<Image> Image::CreateImage(
    const std::string& url, GrDataPtr data,
    fml::RefPtr<ImageDescriptor> descriptor,
    std::weak_ptr<Notifier> weak_notifier,
    fml::RefPtr<fml::TaskRunner> ui_runner,
    fml::RefPtr<fml::TaskRunner> raster_runner, bool is_svg,
    bool use_texture_backend, bool is_promise, bool enable_low_quality_image,
    bool is_deferred, bool decode_with_priority) {
  // The resource context is set up in the raster thread. For non-promise
  // images, texture creation also occurs in the raster thread. Therefore, it is
  // sufficient to check the context only when the texture needs to be created.
  // However, for non-deferred promise images, decoding is triggered on the UI
  // thread. If the texture is used, it will be accessed from the context's
  // threadSafeProxy on the UI thread. Hence, it is necessary to perform a check
  // here first.
  auto notifier = weak_notifier.lock();
  if (!notifier || !notifier->GetUnrefQueue() ||
      (is_promise && !is_deferred &&
       !notifier->GetUnrefQueue()->GetContext())) {
    use_texture_backend = false;
  }

  auto image =
      new Image(url, data, weak_notifier, ui_runner, raster_runner, is_svg,
                is_promise, enable_low_quality_image, use_texture_backend,
                is_deferred, decode_with_priority);
  std::shared_ptr<Image> ptr(image);

  if (!notifier) {
    image->image_producer_ = ImageProducer::CreateImageProducer(
        image->is_svg_, data, descriptor, ImageProduceContext(), image->mutex_,
        use_texture_backend, enable_low_quality_image);
  } else {
    std::function<void(bool)> decode_callback =
        [weak = image->weak_from_this()](bool success) {
          if (auto self = weak.lock()) {
            self->DecodeFinish(success);
            if (!self->IsSVG() && !self->decode_with_priority_) {
              self->StartAnimation();
            }
          }
        };
    std::function<void(bool)> upload_callback = nullptr;
    if (decode_with_priority) {
      upload_callback = [weak = image->weak_from_this()](bool success) {
        if (auto self = weak.lock()) {
          self->UploadFinish(success);
          if (!self->IsSVG()) {
            self->StartAnimation();
          }
        }
      };
    }
    std::function<void(const std::function<void()>&)> register_upload_callback =
        [weak = image->weak_from_this()](const std::function<void()>& f) {
          if (auto self = weak.lock()) {
            self->WillRegisterUploadTask(std::move(f));
          }
        };
    ImageProduceContext produce_context = {
        .use_promise = is_promise,
        .is_deferred = image->is_deferred_,
        .decode_with_priority = image->decode_with_priority_,
        .decode_callback = decode_callback,
        .upload_callback = upload_callback,
        .register_upload_callback = register_upload_callback,
        .unref_queue = notifier->GetUnrefQueue(),
        .ui_task_runner = ui_runner,
        .raster_task_runner = raster_runner};
    image->image_producer_ = ImageProducer::CreateImageProducer(
        image->is_svg_, data, descriptor, produce_context, image->mutex_,
        use_texture_backend, enable_low_quality_image);
  }
  return ptr;
}

std::shared_ptr<Image> Image::CloneImage(std::shared_ptr<Image> image) {
  if (!image) {
    return nullptr;
  }
  return Image::CreateImage(image->url_, image->image_producer_->GetData(),
#ifndef ENABLE_SKITY
                            nullptr,
#else
                            image->image_producer_->GetDescriptor(),
#endif  // ENABLE_SKITY
                            image->weak_notifier_, image->ui_runner_,
                            image->raster_runner_, image->is_svg_,
                            image->use_texture_backend_, image->is_promise_,
                            image->image_producer_->EnableLowQualityImage(),
                            image->is_deferred_, image->decode_with_priority_);
}

Image::~Image() {
  if (raw_data_) {
    ImageDataCache::GetInstance().ReleaseOrArchiveImageData(url_);
  }

  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  ResetAnimation();
}

// If fetching an image by FetchPromiseImage, who has not been downloaded, an
// ImageResource without image data will be return immediately, and a
// asynchronous loading task will be post, then the image data will be put into
// the ImageResource on UI thread when finishing the download.
void Image::SetRawData(GrDataPtr data) {
  FML_DCHECK(ui_runner_->RunsTasksOnCurrentThread());

  // Image already has data(the image maybe already created by another
  // Page).
  if (raw_data_) {
    // Equals with old data, do nothing.
    if (memcmp(DATA_GET_DATA(raw_data_), DATA_GET_DATA(data),
               DATA_GET_SIZE(data)) == 0) {
      return;
    } else {
      // Data changed, remove old data.
      ImageDataCache::GetInstance().RemoveImageDataIfExist(url_);
    }
  }

  raw_data_ = data;
  // Ref new data.
  if (data) {
    ImageDataCache::GetInstance().CacheImageData(url_, data);
  }

  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  ResetAnimation();
  image_producer_->SetData(is_svg_, data);
  for (auto accessor : ui_accessors_) {
    accessor->AnimationAdvanced();
  }
}

std::unique_ptr<ImageResource> Image::GetAccessor(bool from_raster) {
  if (!from_raster && !IsSingleFrameImage()) {
    FML_DCHECK(ui_accessors_.empty());
  }
  return std::make_unique<ImageResource>(shared_from_this(), from_raster);
}

void Image::SetExpectSizeCalculator(
    std::function<skity::Vec2(skity::Vec2)> calculator,
    bool force_use_original_size) {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  image_producer_->SetExpectSizeCalculator(calculator, force_use_original_size);
}

void Image::AccessorCreated(ImageResource* accessor) {
  if (accessor->OwnedByRaster()) {
    if (raster_runner_) {
      FML_DCHECK(raster_runner_->RunsTasksOnCurrentThread());
    }
    raster_accessors_.insert(accessor);
  } else {
    if (ui_runner_) {
      FML_DCHECK(ui_runner_->RunsTasksOnCurrentThread());
    }
    ui_accessors_.insert(accessor);
  }
}

void Image::AccessorDestroyed(ImageResource* accessor) {
  if (accessor->OwnedByRaster()) {
    if (raster_runner_) {
      FML_DCHECK(raster_runner_->RunsTasksOnCurrentThread());
    }
    raster_accessors_.erase(accessor);
  } else {
    if (ui_runner_) {
      FML_DCHECK(ui_runner_->RunsTasksOnCurrentThread());
    }
    ui_accessors_.erase(accessor);
    if (ui_accessors_.empty()) {
      // for multi frame image, need to reset animation when all ui accessors
      // are destroyed, as we may cache the image.
      if (!IsSingleFrameImage()) {
        std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
        if (is_deferred_) {
          lock =
              std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
        }
        ResetAnimation(true);
      }

      if (ui_runner_) {
        // Image will be destructed if not cached.
        // Sometimes an imageview is assigned a different url, and previous url
        // will be set to another imageview. If release resource immediately,
        // the other imageview need decode and make texture again, which is
        // obviously redundant. To avoid that, just release resource in next
        // runloop.
        ui_runner_->PostTask([weak_self = weak_from_this()] {
          auto self = weak_self.lock();
          if (self && self->ui_accessors_.empty()) {
            auto notifier = self->weak_notifier_.lock();
            if (notifier) {
              notifier->ImageHasNoAccessor(self.get());
            }
          }
        });
      }
    }
  }
}

fml::RefPtr<GraphicsImage> Image::GetGraphicsImage(DecodePriority priority) {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  if (decode_with_priority_) {
    auto graphics_image = TryGetGraphicsImage();
    if (graphics_image) {
      return graphics_image;
    }

    switch (priority) {
      case DecodePriority::kPending:
        // Do not decode when priority is pending.
        return nullptr;
      case DecodePriority::kDeferred: {
        // If image is multi frame, do not decode it in deferred priority.
        if (!IsSingleFrameImage()) {
          return nullptr;
        }
        auto deferred_decode_callback =
            [](std::weak_ptr<Image> weak, fml::RefPtr<fml::TaskRunner> runner) {
              runner->PostDelayedTask(
                  [weak]() {
                    if (auto image = weak.lock()) {
                      if (!image->IsActive()) {
                        return;
                      }
                      if (!image->NeedDecode()) {
                        return;
                      }
                      // Check if this image is still in kDeferred/kImmediate
                      // decode priority, otherwise there is no need to decode.
                      // Only needs to check ui_accessors.
                      bool should_decode = false;
                      for (auto accessor : image->ui_accessors_) {
                        for (auto client : accessor->GetClients()) {
                          DecodePriority prior = client->GetDecodePriority();
                          if (prior == DecodePriority::kImmediate ||
                              prior == DecodePriority::kDeferred) {
                            should_decode = true;
                            break;
                          }
                        }
                        if (should_decode) {
                          break;
                        }
                      }
                      if (should_decode) {
                        image->GetGraphicsImage(DecodePriority::kImmediate);
                      }
                    }
                  },
                  fml::TimeDelta::FromSeconds(2));
            };
        deferred_decode_callback(weak_from_this(), ui_runner_);
        return nullptr;
      }
      case DecodePriority::kImmediate:
        // Continue to decode.
        break;
    }
  }

#if defined(ENABLE_SVG)
  if (IsSVG()) {
    if (image_producer_->SVGFrameReady()) {
      return image_producer_->GetSVGFrame()->GetGraphicsImage();
    } else {
      // Async decode SVG frame.
      image_producer_->DecodeSVG();
      return nullptr;
    }
  }
#endif

  FML_DCHECK(image_producer_);
  StartAnimation();

  return image_producer_->CurrentFrameReady()
             ? image_producer_->GetCurrentFrame()->GetGraphicsImage()
             : nullptr;
}

// static
int Image::NextUniqueId() {
  static std::atomic<int> id = 0;
  int current = id.fetch_add(1);
  return current % (std::numeric_limits<int>::max() / 2);
}

void Image::CancelUpload() {
#ifndef ENABLE_SKITY
  // Cancel upload task, will affect only when ImageDecodeWithPriority is
  // enabled.
  ImageUploadManager::GetInstance().RemoveImageUploadTaskByImageId(image_id_);
#endif  // ENABLE_SKITY
}

void Image::SetIsActive(bool is_active) {
  is_active_ = is_active;
  if (!is_active_) {
    // If image is decoded but not uploaded, the upload tasks may be removed
    // from ImageUploadManager. We need make sure when image is active again, it
    // will trigger a new upload task.
    if (!TryGetGraphicsImage()) {
      force_decode_ = true;
    }
    // Also cancel current upload.
    CancelUpload();
  }
}

bool Image::NeedDecode() const {
  // If TryGetGraphicsImage returns a valid image, it means the image has
  // already been decoded and uploaded.
  if (TryGetGraphicsImage()) {
    return false;
  }

  if (force_decode_) {
    force_decode_ = false;
    return true;
  }

  return image_producer_->NeedDecode();
}

fml::RefPtr<GraphicsImage> Image::TryGetGraphicsImage() const {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }
  // If there are pending operations, we may use new graphics image.
  if (!pending_operations_.empty()) {
    return nullptr;
  }

#if defined(ENABLE_SVG)
  if (IsSVG()) {
    return image_producer_->SVGFrameReady()
               ? image_producer_->GetSVGFrame()->GetGraphicsImage()
               : nullptr;
  }
#endif

  FML_DCHECK(image_producer_);

  return image_producer_->CurrentFrameReady()
             ? image_producer_->GetCurrentFrame()->GetGraphicsImage()
             : nullptr;
}

size_t Image::GetGraphicsImageAllocSize() const {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }
#if defined(ENABLE_SVG)
  if (IsSVG()) {
    return image_producer_->SVGFrameReady()
               ? image_producer_->GetSVGFrame()->GetAllocationSize()
               : 0;
  }
#endif

  FML_DCHECK(image_producer_);
  return image_producer_->CurrentFrameReady()
             ? image_producer_->GetCurrentFrame()->GetAllocationSize()
             : 0;
}

int Image::GetWidth() {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  return image_producer_->GetInfo().width();
}

int Image::GetHeight() {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  return image_producer_->GetInfo().height();
}

size_t Image::FrameCount() const { return image_producer_->FrameCount(); }

void Image::SetLoopCount(int loop_count) {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  loop_count_ = loop_count;
}

void Image::SetAutoPlay(bool auto_play) {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  auto_play_ = auto_play;
}

void Image::ResetAnimation(bool reuse) {
  if (frame_timer_) {
    if (IsDeferred() && raster_runner_) {
      raster_runner_->PostTask(fml::MakeCopyable(
          [timer = std::move(frame_timer_)]() mutable { timer.reset(); }));
    } else {
      frame_timer_.reset();
    }
  }

  current_frame_ = -1;
  repetitions_complete_ = 0;
  animation_finished_ = false;
  running_state_ = kUndefined;
  current_frame_start_time_ = 0;
  current_frame_duration_ = 0;
  // For extremely large animations, when the animation is reset, we just throw
  // everything away.
  image_producer_->Reset(reuse);
}

bool Image::IsAnimationFinished() const {
  return animation_finished_ || IsSingleFrameImage();
}

bool Image::MaybeAnimated() const {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  return running_state_ == kRunning && !IsAnimationFinished();
}

bool Image::IsFirstFrame() const { return current_frame_ == -1; }

bool Image::IsSingleFrameImage() const {
  if (!image_producer_ || is_svg_) {
    return true;
  }
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  return image_producer_->IsSingleFrameImage();
}

bool Image::HasCurrentFrameDurationPassed(const int64_t timestamp) const {
  return timestamp - current_frame_start_time_ >= current_frame_duration_;
}

// called from StartAnimation, maybe already locked mutex_;
void Image::ScheduleNewFrame() { image_producer_->DecodeNextFrame(); }

void Image::StartAnimation() {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  HandlePendingOperations();

  // TODO(yudingqian): This is a workaround for images whose graphics resource
  // might be release by GPUResourceCache(e.g. when up to cache limit) but
  // haven't been notified.
  if (!image_producer_->CurrentFrameReady()) {
    if (image_producer_->NextFrameReady()) {
      AdvanceAnimation();
    } else {
      ScheduleNewFrame();
      if (is_promise_ && image_producer_->NextFrameReady()) {
        image_producer_->UpdateCurrentFrame();
      }
    }
    return;
  }

  if (UNLIKELY(running_state_ == kUndefined)) {
    if (auto_play_) {
      running_state_ = kRunning;
      OnStartPlay();
    } else {
      running_state_ = kPause;
    }
  }

  // Image is already animating
  if (frame_timer_ && !frame_timer_->Stopped()) {
    return;
  }

  // Animation has finished
  if (IsAnimationFinished()) {
    return;
  }

  // Animation has been paused by front-end
  if (running_state_ == kPause) {
    return;
  }

  // All accessors are invisible
  if (ShouldPauseAnimation()) {
    return;
  }

  const int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  if (IsFirstFrame() || HasCurrentFrameDurationPassed(now)) {
    // In `use_texture_backend_` mode, we always prepare next frame in advance.
    // So here we can get next frame unless the decoding is not finished yet.
    //
    // In `software rendering` mode, we will not start to decode next frame
    // until current frame duration is passed. So if the duration passed, we
    // `ScheduleNewFrame` now.
    if (image_producer_->NextFrameReady()) {
      AdvanceAnimation();
    } else {
      ScheduleNewFrame();
    }
    return;
  }

  // The animation painting progress has caught up with the animated image. So
  // wait for next frame and then advance.
  if (!frame_timer_) {
    frame_timer_ = std::make_unique<fml::OneshotTimer>(
        IsDeferred() ? raster_runner_ : ui_runner_);
  }

  frame_timer_->Start(
      fml::TimeDelta::FromMilliseconds(current_frame_duration_ -
                                       (now - current_frame_start_time_)),
      [weak = weak_from_this()] {
        if (auto self = weak.lock()) {
          self->StartAnimation();
        }
      });
}

// called from StartAnimation, maybe already locked mutex_;
void Image::AdvanceAnimation() {
  if (!image_producer_->NextFrameReady()) {
    return;
  }

  image_producer_->UpdateCurrentFrame();

  ++current_frame_;
  if (current_frame_ == static_cast<int>(FrameCount()) - 1) {
    ++repetitions_complete_;
    OnCurrentLoopComplete();
    if (loop_count_ > 0 && repetitions_complete_ >= loop_count_) {
      animation_finished_ = true;
      OnFinalLoopComplete();
    }
  } else if (current_frame_ > static_cast<int>(FrameCount()) - 1) {
    current_frame_ = 0;
  }

  current_frame_start_time_ =
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  current_frame_duration_ = image_producer_->GetCurrentFrame()->FrameDuration();

  AnimationAdvanced();
}

// called from StartAnimation, maybe already locked mutex_;
bool Image::ShouldPauseAnimation() const {
  if (IsDeferred()) {
    for (auto accessor : raster_accessors_) {
      if (!accessor->ShouldPauseAnimation()) {
        return false;
      }
    }
  } else {
    for (auto accessor : ui_accessors_) {
      if (!accessor->ShouldPauseAnimation()) {
        return false;
      }
    }
  }
  return true;
}

// called from decode_callback, maybe already locked mutex_ by ImageProducer.;
void Image::DecodeFinish(bool success) {
  if (IsDeferred()) {
    // If the `render_info_` of `ImageProducer` has been changed, we need to
    // recreate the displaylist to make sure `drawImageRect` method has the
    // correct `SrcRect` parameter. So notify ui accessors to `MarkNeedsPaint`
    // here.
    if (success && image_producer_->WillNextFrameSizeChange()) {
      ui_runner_->PostTask([weak_self = weak_from_this()] {
        auto self = weak_self.lock();
        if (self) {
          for (auto accessor : self->ui_accessors_) {
            accessor->DecodeFinish(true);
          }
        }
      });
    }

    for (auto accessor : raster_accessors_) {
      accessor->DecodeFinish(success);
    }
  } else {
    for (auto accessor : ui_accessors_) {
      accessor->DecodeFinish(success);
    }
  }
}

void Image::UploadFinish(bool success) {
  for (auto accessor : ui_accessors_) {
    accessor->UploadFinish(success);
  }
}

void Image::WillRegisterUploadTask(const std::function<void()>& task) {
  OneShotCallback one_shot_task = OneShotCallback(task);
  for (auto accessor : ui_accessors_) {
    accessor->RegisterUploadTask(std::move(one_shot_task), image_id_);
  }
}

// called from StartAnimation, maybe already locked mutex_;
void Image::AnimationAdvanced() {
  if (IsDeferred()) {
    for (auto accessor : raster_accessors_) {
      accessor->AnimationAdvanced();
    }
  } else {
    for (auto accessor : ui_accessors_) {
      accessor->AnimationAdvanced();
    }
  }

  // If `use_texture_backend_`, we can start decoding next frame in advance,
  // cause the decoding progress is asynchronous.
  // But the decoding progress is synchronous in `software rendering` mode. So
  // here we just call `StartAnimation` and start a timer to wait next frame.
  if (use_texture_backend_) {
    ScheduleNewFrame();
  } else {
    StartAnimation();
  }
}

void Image::StartAnimate() { PushOperation(kStartAnimate); }
void Image::StopAnimation() { PushOperation(kStopAnimation); }
void Image::PauseAnimation() { PushOperation(kPauseAnimation); }
void Image::ResumeAnimation() { PushOperation(kResumeAnimation); }

void Image::PushOperation(Operation operation) {
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (is_deferred_) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  if (!pending_operations_.empty() && pending_operations_.back() == operation) {
    return;
  }
  pending_operations_.push_back(operation);
}

// called from StartAnimation, maybe already locked mutex_;
void Image::HandlePendingOperations() {
  for (const auto operation : pending_operations_) {
    switch (operation) {
      case kStartAnimate:
        ResetAnimation(true);
        running_state_ = kRunning;
        OnStartPlay();
        break;
      case kStopAnimation:
        ResetAnimation(true);
        running_state_ = kPause;
        break;
      case kPauseAnimation:
        running_state_ = kPause;
        break;
      case kResumeAnimation:
        if (running_state_ == kRunning) {
          break;
        }
        running_state_ = kRunning;
        OnStartPlay();
        break;
      default:
        FML_LOG(ERROR) << "Unrecognized operation";
    }
  }
  pending_operations_.clear();
}

void Image::OnStartPlay() {
  fml::TaskRunner::RunNowOrPostTask(ui_runner_, [weak_self = weak_from_this()] {
    auto self = weak_self.lock();
    if (self) {
      for (auto accessor : self->ui_accessors_) {
        accessor->OnStartPlay();
      }
    }
  });
}

void Image::OnCurrentLoopComplete() {
  fml::TaskRunner::RunNowOrPostTask(ui_runner_, [weak_self = weak_from_this()] {
    auto self = weak_self.lock();
    if (self) {
      for (auto accessor : self->ui_accessors_) {
        accessor->OnCurrentLoopComplete();
      }
    }
  });
}

void Image::OnFinalLoopComplete() {
  fml::TaskRunner::RunNowOrPostTask(ui_runner_, [weak_self = weak_from_this()] {
    auto self = weak_self.lock();
    if (self) {
      for (auto accessor : self->ui_accessors_) {
        accessor->OnFinalLoopComplete();
      }
    }
  });
}

}  // namespace clay
