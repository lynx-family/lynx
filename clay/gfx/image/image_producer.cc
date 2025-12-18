// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_producer.h"

#include <algorithm>
#include <future>
#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/trace/native/trace_event.h"
#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/image/codec.h"
#include "clay/gfx/image/graphics_image.h"
#include "clay/gfx/image/image_descriptor.h"
#include "clay/gfx/image/skimage_holder.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/skity_to_skia_utils.h"

#define CACHE_IMAGE_BY_DRAWING_SIZE 1

#ifdef ENABLE_SKITY
#define IMAGE_DIMENSION(image) image
#else
#define IMAGE_DIMENSION(image) \
  SkISize { (int32_t) image.x, (int32_t)image.y }
#endif

namespace clay {
namespace {
static const size_t kMaxMemoryLimitPerFrame = 24 * 1024 * 1024;

class PromiseImageContext
    : public fml::RefCountedThreadSafe<PromiseImageContext> {
 public:
  fml::RefPtr<GPUUnrefQueue> unref_queue;
  fml::RefPtr<fml::TaskRunner> raster_runner;
  std::future<fml::RefPtr<GraphicsImage>> image_future;
  fml::RefPtr<GraphicsImage> texture_image;
};

template <class T>
class SafePromise {
 public:
  SafePromise() = default;

  ~SafePromise() {
    if (!has_set_) {
      promise_.set_value(nullptr);
    }
  }

  void SetValue(T value) {
    promise_.set_value(value);
    has_set_ = true;
  }

  std::future<T> GetFuture() { return promise_.get_future(); }

 private:
  std::promise<T> promise_;
  bool has_set_ = false;
};

#ifndef ENABLE_SKITY
sk_sp<SkPromiseImageTexture> GetPromiseImage(void* context) {
  TRACE_EVENT("clay", "GetPromiseImage");
  PromiseImageContext* image_context =
      static_cast<PromiseImageContext*>(context);

  auto decode_image = image_context->image_future.get();
  if (!decode_image) {
    return nullptr;
  }

  image_context->texture_image = decode_image->makeTextureImage(
      image_context->unref_queue->GetContext().get());
  if (!image_context->texture_image ||
      !image_context->texture_image->gr_image()) {
    return nullptr;
  }
  GrBackendTexture backend_texture;
  if (!SkImages::GetBackendTextureFromImage(
          image_context->texture_image->gr_image(), &backend_texture, false)) {
    FML_DLOG(ERROR) << "PromiseImage get backend texture failed";
    return nullptr;
  }

  if (!backend_texture.isValid()) {
    FML_DLOG(ERROR) << "PromiseImage backend texture is invalid";
    return nullptr;
  }
  return SkPromiseImageTexture::Make(backend_texture);
}
#else
std::shared_ptr<skity::Texture> GetPromiseImage(void* context) {
  TRACE_EVENT("clay", "GetPromiseImage");
  PromiseImageContext* image_context =
      static_cast<PromiseImageContext*>(context);

  auto decode_image = image_context->image_future.get();
  if (!decode_image) {
    return nullptr;
  }
  if (decode_image->isTextureBacked()) {
    return *(decode_image->gr_image()->GetTexture());
  }

  image_context->texture_image = decode_image->makeTextureImage(
      image_context->unref_queue->GetContext().get());
  if (!image_context->texture_image ||
      !image_context->texture_image->gr_image()) {
    return nullptr;
  }
  return *(image_context->texture_image->gr_image()->GetTexture());
}
#endif  // ENABLE_SKITY

void ReleasePromiseImage(void* context) {
  TRACE_EVENT("clay", "ReleasePromiseImage");
  PromiseImageContext* image_context =
      static_cast<PromiseImageContext*>(context);
  // In most cases, this function is called in Raster Thread.
  // However, currently `skottie::Animation` holds `sk_sp<SkImage>` in UI
  // thread, makes it possible to release SkImage in UI thread, we must post the
  // release task to raster thread in this case.
  fml::TaskRunner::RunNowOrPostTask(image_context->raster_runner,
                                    [image_context] {
                                      image_context->texture_image->Release();
                                      image_context->Release();
                                    });
}
}  // namespace

std::shared_ptr<ImageProducer> ImageProducer::CreateImageProducer(
    bool is_svg, GrDataPtr data, fml::RefPtr<ImageDescriptor> descriptor,
    const ImageProduceContext& produce_context,
    const std::shared_ptr<std::recursive_mutex>& mutex,
    bool use_texture_backend, bool enable_low_quality_image) {
  auto ptr = new ImageProducer(is_svg, data, descriptor, produce_context, mutex,
                               use_texture_backend, enable_low_quality_image);
  return std::shared_ptr<ImageProducer>(ptr);
}

ImageProducer::ImageProducer(bool is_svg, GrDataPtr data,
                             fml::RefPtr<ImageDescriptor> descriptor,
                             const ImageProduceContext& produce_context,
                             const std::shared_ptr<std::recursive_mutex>& mutex,
                             bool use_texture_backend,
                             bool enable_low_quality_image)
    : is_svg_(is_svg),
      raw_data_(data),
      descriptor_(descriptor),
      produce_context_(produce_context),
      mutex_(mutex),
      use_texture_backend_(use_texture_backend) {
  if (!use_texture_backend_) {
    // In fact, "PromiseImage" is available only when we pretend to use
    // texture-backend image.
    produce_context_.use_promise = false;
  }
  if (!is_svg) {
    enable_low_quality_image_ = enable_low_quality_image;
    if (!descriptor_ && raw_data_) {
      descriptor_ =
          ImageDescriptor::Create(raw_data_, enable_low_quality_image);
    }
    if (descriptor_) {
      orig_info_ = descriptor_->image_info();
    }
  } else if (is_svg) {
    // Just a (1, 1) placeholder for EMPTY ImageInfo check. Because `orig_info_`
    // is meaningless for SVG. We should never use `orig_info_` to do
    // anything.
#ifndef ENABLE_SKITY
    orig_info_ =
        SkImageInfo::Make(1, 1, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
#else
    orig_info_ = ImageInfo::makeWH(1, 1);
#endif  // ENABLE_SKITY
  }
}

ImageProducer::~ImageProducer() { Reset(); }

bool ImageProducer::IsSingleFrameImage() const {
  if (!descriptor_) {
    return true;
  }
  return descriptor_->IsSingleFrame();
}

void ImageProducer::SetData(bool is_svg, GrDataPtr data) {
  Reset();

  if (!data) {
    return;
  }
  raw_data_ = data;
  if (!is_svg) {
    EnsureDescriptorInitialized();
  } else {
    // Just a (1, 1) placeholder for EMPTY ImageInfo check. Because `orig_info_`
    // is meaningless for SVG. We should never use `orig_info_` to do
    // anything.
#ifndef ENABLE_SKITY
    orig_info_ =
        SkImageInfo::Make(1, 1, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
#else
    orig_info_ = ImageInfo::makeWH(1, 1);
#endif  // ENABLE_SKITY
  }
}

#if defined(ENABLE_SVG)
void ImageProducer::DecodeSVG() {
  if (!raw_data_) {
    return;
  }

  if (!svg_frame_) {
    svg_frame_ = fml::MakeRefCounted<SVGImageHolder>();
  }

  svg_frame_->CreateSVGDOM(raw_data_);
  if (use_texture_backend_) {
    FetchHardwareSVGImage();
  } else {
    FetchSoftwareSVGImage();
  }
}

#ifndef ENABLE_SKITY
static void RenderSVG(SkCanvas* canvas, SkSVGDOM* dom,
                      const SkImageInfo& image_info) {
  canvas->clear(SK_AlphaTRANSPARENT);
  auto root = dom->getRoot();
  auto svg_height = root->getHeight(), svg_width = root->getWidth();
  if (svg_height.unit() != SkSVGLength::Unit::kPercentage &&
      svg_width.unit() != SkSVGLength::Unit::kPercentage) {
    canvas->scale(image_info.width() / root->getWidth().value(),
                  image_info.height() / root->getHeight().value());
  } else {
    dom->setContainerSize(
        SkSize::Make(image_info.width(), image_info.height()));
  }
  dom->render(canvas);
}
#endif  // ENABLE_SKITY

void ImageProducer::FetchHardwareSVGImage() {
  if (render_info_.isEmpty()) {
    FML_DLOG(WARNING) << "The size of SVG is empty";
    return;
  }
  auto graphics_image = svg_frame_->GetGraphicsImage();
  if (!graphics_image || graphics_image->width() < render_info_.width() ||
      graphics_image->height() < render_info_.height()) {
    fml::TaskRunner::RunNowOrPostTask(
        produce_context_.raster_task_runner,
        [weak = weak_from_this(), unref_queue = produce_context_.unref_queue,
         render_info = render_info_, svg_frame = svg_frame_,
         callback_runner = produce_context_.is_deferred
                               ? produce_context_.raster_task_runner
                               : produce_context_.ui_task_runner] {
          if (auto self = weak.lock()) {
            auto svg_dom = svg_frame->GetSVGDOM();
            if (!svg_dom) {
              FML_LOG(WARNING) << "SVGDOM is null, cannot decode an empty svg.";
              return;
            }
#ifndef ENABLE_SKITY
            // Ensure that the Context for the unref_queue has been
            // properly set.
            FML_DCHECK(unref_queue->GetContext());
            sk_sp<SkSurface> surface =
                SkSurface::MakeRenderTarget(unref_queue->GetContext().get(),
                                            skgpu::Budgeted::kYes, render_info);
            if (!surface) {
              FML_LOG(ERROR) << "Failed to create SkSurface of SVG";
              return;
            }
            RenderSVG(surface->getCanvas(), svg_dom.get(), render_info);

            auto gpu_object = GPUObject(
                GraphicsImage::Make(surface->makeImageSnapshot()), unref_queue);
#else
            auto data =
                svg_dom->Render(render_info.width(), render_info.height());
            if (!data) {
              FML_LOG(ERROR) << "Failed to render SVG";
              return;
            }
            std::shared_ptr<skity::Pixmap> skity_pixmap =
                std::make_shared<skity::Pixmap>(
                    std::move(data),
                    render_info.width() * render_info.bytesPerPixel(),
                    render_info.width(), render_info.height(),
                    ConvertToSkityAlphaType(render_info.alphaType()),
                    ConvertToSkityColorType(render_info.colorType()));
            auto image =
                GraphicsImage::Make(skity::Image::MakeImage(skity_pixmap));
            auto gpu_object = GPUObject(
                image->makeTextureImage(unref_queue->GetContext().get()),
                unref_queue);
#endif  // ENABLE_SKITY
            svg_frame->SetGraphicsImage(
                fml::MakeRefCounted<GpuGraphicsImageWrapper>(
                    std::move(gpu_object)));
            fml::TaskRunner::RunNowOrPostTask(callback_runner, [weak, svg_frame,
                                                                render_info] {
              if (auto self = weak.lock()) {
                // For deferred images, modifications to the frame can occur on
                // either the UI thread or the raster thread, so locking is
                // necessary to ensure safe access. For the ImageProducer, all
                // public methods are accessed through the Image, and the Image
                // already locks when accessing the ImageProducer. Therefore,
                // most methods do not require additional locking. However, this
                // piece of code is called by the ImageProducer's own member
                // function via a post task, so it requires locking.
                std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
                if (self->produce_context_.is_deferred) {
                  lock =
                      std::make_unique<std::lock_guard<std::recursive_mutex>>(
                          *self->mutex_);
                }

                if (self->produce_context_.decode_callback) {
                  self->produce_context_.decode_callback(true);
                }
                if (self->produce_context_.decode_with_priority &&
                    self->produce_context_.upload_callback) {
                  self->produce_context_.upload_callback(true);
                }
              }
            });
          }
        });
  }
}

void ImageProducer::FetchSoftwareSVGImage() {
  if (render_info_.isEmpty()) {
    FML_DLOG(WARNING) << "The size of SVG is empty";
    return;
  }
  auto graphics_image = svg_frame_->GetGraphicsImage();
  if (!graphics_image || graphics_image->width() < render_info_.width() ||
      graphics_image->height() < render_info_.height()) {
    GraphicsIsolate::Instance().GetConcurrentWorkerTaskRunner()->PostTask(
        [weak = weak_from_this(), render_info = render_info_,
         svg_frame = svg_frame_,
         callback_runner = produce_context_.is_deferred
                               ? produce_context_.raster_task_runner
                               : produce_context_.ui_task_runner]() {
          if (auto self = weak.lock()) {
            auto svg_dom = svg_frame->GetSVGDOM();
            if (!svg_dom) {
              FML_LOG(WARNING) << "SVGDOM is null, cannot decode an empty svg.";
              return;
            }
#ifndef ENABLE_SKITY
            const size_t min_row_bytes =
                render_info.minRowBytes();  // bytes used by one bitmap row
            const size_t size = render_info.computeMinByteSize();
            // bytes used by all rows
            SkColor* pixels = new SkColor[size];
            auto canvas =
                SkCanvas::MakeRasterDirect(render_info, pixels, min_row_bytes);
            if (!canvas) {
              FML_LOG(ERROR) << "Failed to create SkCanvas of SVG";
              delete[] pixels;
              return;
            }
            RenderSVG(canvas.get(), svg_dom.get(), render_info);

            auto sk_data = SkData::MakeWithProc(
                pixels, size * sizeof(SkColor),
                [](const void* ptr, void* context) {
                  delete[] static_cast<const SkColor*>(context);
                },
                pixels);
            auto image = GraphicsImage::MakeRasterData(render_info, sk_data,
                                                       min_row_bytes);
#else
            auto data =
                svg_dom->Render(render_info.width(), render_info.height());
            if (!data) {
              FML_LOG(ERROR) << "Failed to render SVG";
              return;
            }
            std::shared_ptr<skity::Pixmap> skity_pixmap =
                std::make_shared<skity::Pixmap>(
                    std::move(data),
                    render_info.width() * render_info.bytesPerPixel(),
                    render_info.width(), render_info.height(),
                    ConvertToSkityAlphaType(render_info.alphaType()),
                    ConvertToSkityColorType(render_info.colorType()));
            auto image =
                GraphicsImage::Make(skity::Image::MakeImage(skity_pixmap));

#endif  // ENABLE_SKITY
            svg_frame->SetGraphicsImage(
                fml::MakeRefCounted<CpuGraphicsImageWrapper>(image));
            fml::TaskRunner::RunNowOrPostTask(callback_runner, [weak, svg_frame,
                                                                render_info] {
              if (auto self = weak.lock()) {
                // For deferred images, modifications to the frame can occur on
                // either the UI thread or the raster thread, so locking is
                // necessary to ensure safe access. For the ImageProducer, all
                // public methods are accessed through the Image, and the Image
                // already locks when accessing the ImageProducer. Therefore,
                // most methods do not require additional locking. However, this
                // piece of code is called by the ImageProducer's own member
                // function via a post task, so it requires locking.
                std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
                if (self->produce_context_.is_deferred) {
                  lock =
                      std::make_unique<std::lock_guard<std::recursive_mutex>>(
                          *self->mutex_);
                }

                if (self->produce_context_.decode_callback) {
                  self->produce_context_.decode_callback(true);
                }
              }
            });
          }
        });
  }
}

fml::RefPtr<SVGImageHolder> ImageProducer::GetSVGFrame() const {
  return svg_frame_;
}
#endif

bool ImageProducer::SVGFrameReady() const {
#if defined(ENABLE_SVG)
  if (svg_frame_ && svg_frame_->GetSVGDOM() && svg_frame_->GetGraphicsImage() &&
      IMAGE_DIMENSION(svg_frame_->GetGraphicsImage()->dimensions()) ==
          render_info_.dimensions()) {
    return true;
  }
#endif
  return false;
}

void ImageProducer::Reset(bool reuse) {
#if defined(ENABLE_SVG)
  svg_frame_ = nullptr;
#endif
  decoding_ = false;
  codec_ = nullptr;
  current_frame_index_ = -1;
  next_frame_ = nullptr;
  if (!reuse) {
    frame_count_ = 1;
    should_use_cache_ = true;
    descriptor_ = nullptr;
    orig_info_.reset();
    render_info_.reset();
    ReleaseAllFrames();
  }
}

void ImageProducer::EnsureDescriptorInitialized() {
  if (is_svg_) {
    return;
  }
  if (!raw_data_) {
    FML_LOG(ERROR) << "The data is null, cannot create ImageDescriptor";
    return;
  }
  if (descriptor_) {
    return;
  }

  descriptor_ = ImageDescriptor::Create(raw_data_, enable_low_quality_image_);
  if (!descriptor_) {
    FML_LOG(ERROR) << "Failed to create ImageDescriptor";
    return;
  }
  orig_info_ = descriptor_->image_info();
}

bool ImageProducer::PrepareCodec() {
  EnsureDescriptorInitialized();
  if (!descriptor_ || orig_info_.isEmpty()) {
    return false;
  }
  if (render_info_.isEmpty() || !descriptor_->IsSingleFrame()) {
    // SetExpectSizeCalculator not be called. so use orig_info_.
    render_info_ = orig_info_;
  }

  if (!codec_) {
    codec_ = descriptor_->InstantiateCodec(render_info_.width(),
                                           render_info_.height());
    frame_count_ = codec_->FrameCount();
    if (frame_count_ > 1) {
      should_use_cache_ = false;
    }
  }

  return !!codec_;
}

void ImageProducer::DecodeNextFrame() {
  // For single-frame image, if it have already been decoded, there is no
  // need to trigger decoding for the next frame. This return statement prevents
  // a second decoding from being triggered, ensuring that each single-frame
  // image is decoded only once.
  if (CurrentFrameReady() && frame_count_ == 1) {
    return;
  }

  if (use_texture_backend_) {
    AsyncDecodeNextFrame();
  } else {
    SyncDecodeNextFrame();
  }
}

void ImageProducer::AsyncDecodeNextFrame() {
  if (!codec_) {
    if (!PrepareCodec()) {
      FML_LOG(ERROR) << "Image codec must not be null";
      return;
    }
  }

  if (decoding_) {
    return;
  }
  decoding_ = true;

  auto next_frame =
      fml::MakeRefCounted<SkImageHolder>(produce_context_.decode_with_priority);
  next_frame->MarkLoadStart();
  if (produce_context_.use_promise) {
    DecodePromiseFrame(next_frame);
  } else {
    if (produce_context_.decode_with_priority) {
      DecodeBackendTextureFrameWithPriority(next_frame);
    } else {
      DecodeBackendTextureFrame(next_frame);
    }
  }
}

void ImageProducer::DecodeBackendTextureFrame(
    const fml::RefPtr<clay::SkImageHolder>& frame) {
  codec_->NextFrame([weak = weak_from_this(), next_frame = frame,
                     unref_queue = produce_context_.unref_queue,
                     raster_runner = produce_context_.raster_task_runner,
                     callback_runner = produce_context_.is_deferred
                                           ? produce_context_.raster_task_runner
                                           : produce_context_.ui_task_runner](
                        FrameInfo frame_info) {
    TRACE_EVENT("clay", "DecodeFinish");
    if (!weak.lock()) {
      return;
    }

    if (!frame_info.image) {
      callback_runner->PostTask([weak, next_frame]() {
        if (auto self = weak.lock()) {
          self->OnFrameDecoded(next_frame, false, true);
        }
      });
      return;
    }

    raster_runner->PostTask([weak, next_frame, unref_queue, callback_runner,
                             frame_info] {
      FrameInfo new_frame_info = frame_info;
      TRACE_EVENT("clay", "makeTextureImage");
      // Ensure that the Context for the unref_queue has been
      // properly set.
      FML_DCHECK(unref_queue->GetContext());
      new_frame_info.image = new_frame_info.image->makeTextureImage(
          unref_queue->GetContext().get());
      if (new_frame_info.image) {
        new_frame_info.image->flushAndSubmit(unref_queue->GetContext().get());
        callback_runner->PostTask(
            [weak, next_frame, unref_queue, new_frame_info] {
              // Create GPUObject with UnrefQueue before 'if' to
              // make sure it can be destroyed in UI thread safely
              auto gpu_object = GPUObject(new_frame_info.image, unref_queue);
              if (auto self = weak.lock()) {
                next_frame->CacheFrame(new_frame_info.duration,
                                       std::move(gpu_object));
                self->OnFrameDecoded(next_frame, true, true);
              }
            });
      } else {
        callback_runner->PostTask([weak, next_frame] {
          if (auto self = weak.lock()) {
            self->OnFrameDecoded(next_frame, false, true);
          }
        });
      }
    });
  });
}

void ImageProducer::DecodePromiseFrame(
    const fml::RefPtr<clay::SkImageHolder>& frame) {
  fml::RefPtr<PromiseImageContext> promise_context =
      fml::MakeRefCounted<PromiseImageContext>();
  promise_context->unref_queue = produce_context_.unref_queue;
  std::shared_ptr<SafePromise<fml::RefPtr<GraphicsImage>>> promise_image =
      std::make_shared<SafePromise<fml::RefPtr<GraphicsImage>>>();
  promise_context->image_future = promise_image->GetFuture();
  promise_context->raster_runner = produce_context_.raster_task_runner;

  codec_->NextFrame([weak = weak_from_this(), next_frame = frame,
                     promise_image = std::move(promise_image),
                     unref_queue = produce_context_.unref_queue,
                     raster_runner = produce_context_.raster_task_runner,
                     callback_runner = produce_context_.is_deferred
                                           ? produce_context_.raster_task_runner
                                           : produce_context_.ui_task_runner](
                        FrameInfo frame_info) {
    TRACE_EVENT("clay", "DecodeFinish");
    if (!weak.lock()) {
      return;
    }
    if (!frame_info.image) {
      callback_runner->PostTask([weak, next_frame]() {
        if (auto self = weak.lock()) {
          self->OnFrameDecoded(next_frame, false, true);
        }
      });
      return;
    }
    promise_image->SetValue(frame_info.image);
    callback_runner->PostTask([unref_queue, frame_info, next_frame, weak] {
      if (auto self = weak.lock()) {
        next_frame->CacheFrame(frame_info.duration,
                               {frame_info.image, unref_queue}, true);
        self->OnFrameDecoded(next_frame, true, true);
      }
    });
  });

  // Ensure that the Context for the unref_queue has been
  // properly set.
  FML_DCHECK(produce_context_.unref_queue->GetContext());
  auto info = GetInfo();
#ifndef ENABLE_SKITY
  sk_sp<GrContextThreadSafeProxy> gpu_context_proxy =
      produce_context_.unref_queue->GetContext()->threadSafeProxy();
  GrBackendFormat backend_format = gpu_context_proxy->defaultBackendFormat(
      info.colorType(), GrRenderable::kYes);
  promise_context->AddRef();
  frame->SetPromiseImage(
      {GraphicsImage::MakePromiseTexture(
           gpu_context_proxy, backend_format, {info.width(), info.height()},
           GrMipmapped::kNo, kTopLeft_GrSurfaceOrigin, info.colorType(),
           info.alphaType(), info.refColorSpace(), GetPromiseImage,
           ReleasePromiseImage, promise_context.get()),
       produce_context_.unref_queue});
#else
  promise_context->AddRef();
  frame->SetPromiseImage(
      {GraphicsImage::MakePromiseTexture(
           skity::TextureFormat::kRGBA, info.width(), info.height(),
           ConvertToSkityAlphaType(info.alphaType()), GetPromiseImage,
           ReleasePromiseImage, promise_context.get()),
       produce_context_.unref_queue});
#endif  // ENABLE_SKITY
  frame->MarkLoadResult(true);
  next_frame_ = frame;
}

void ImageProducer::DecodeBackendTextureFrameWithPriority(
    const fml::RefPtr<clay::SkImageHolder>& frame) {
  codec_->NextFrame([weak = weak_from_this(), next_frame = frame,
                     callback_runner = produce_context_.is_deferred
                                           ? produce_context_.raster_task_runner
                                           : produce_context_.ui_task_runner](
                        FrameInfo frame_info) {
    TRACE_EVENT("clay", "DecodeFinish");
    if (!weak.lock()) {
      return;
    }

    if (!frame_info.image) {
      callback_runner->PostTask([weak, next_frame]() {
        if (auto self = weak.lock()) {
          self->OnFrameDecoded(next_frame, false, true);
        }
      });
      return;
    }
    callback_runner->PostTask([weak, next_frame, frame_info] {
      if (auto self = weak.lock()) {
        self->RegisterUploadTask(next_frame, frame_info);
        self->OnFrameDecoded(next_frame, true, true);
      }
    });
  });
}

void ImageProducer::SyncDecodeNextFrame() {
  if (!codec_) {
    if (!PrepareCodec()) {
      FML_LOG(ERROR) << "Image codec must not be null";
      return;
    }
  }

  auto next_frame = fml::MakeRefCounted<SkImageHolder>();

  std::promise<fml::RefPtr<SkImageHolder>> next_frame_promise;
  std::future<fml::RefPtr<SkImageHolder>> next_frame_future =
      next_frame_promise.get_future();
  next_frame->MarkLoadStart();
  codec_->NextFrame([weak = weak_from_this(), next_frame = next_frame,
                     unref_queue = produce_context_.unref_queue,
                     &next_frame_promise](FrameInfo frame_info) {
    TRACE_EVENT("clay", "DecodeFinish");
    if (!weak.lock()) {
      next_frame_promise.set_value(nullptr);
      return;
    }
    if (frame_info.image) {
      next_frame->CacheFrame(frame_info.duration,
                             {frame_info.image, unref_queue});
    }
    next_frame_promise.set_value(next_frame);
  });
  auto next_frame_result = next_frame_future.get();
  if (!next_frame_result) {
    return;
  }
  OnFrameDecoded(next_frame_result, next_frame_result->FrameIsReady());
}

void ImageProducer::UpdateCurrentFrame() {
  current_frame_index_ = (current_frame_index_ + 1) % FrameCount();

  if (next_frame_ && next_frame_ != current_frame_ &&
      next_frame_->GetGraphicsImage()) {
    if (current_frame_) {
      current_frame_->ReleaseResource();
    }
    current_frame_ = next_frame_;
  }
  next_frame_ = nullptr;
}

void ImageProducer::RegisterUploadTask(
    const fml::RefPtr<clay::SkImageHolder>& frame,
    const FrameInfo& frame_info) {
  FML_DCHECK(!produce_context_.is_deferred);

  auto upload_task = [weak = weak_from_this(), next_frame = frame, frame_info,
                      unref_queue = produce_context_.unref_queue,
                      callback_runner = produce_context_.ui_task_runner] {
    FrameInfo new_frame_info = frame_info;
    TRACE_EVENT("clay", "makeTextureImage");
    // Ensure that the Context for the unref_queue has been
    // properly set.
    FML_DCHECK(unref_queue->GetContext());
    new_frame_info.image =
        new_frame_info.image->makeTextureImage(unref_queue->GetContext().get());
    if (new_frame_info.image) {
      new_frame_info.image->flushAndSubmit(unref_queue->GetContext().get());
      callback_runner->PostTask(
          [weak, next_frame, unref_queue, new_frame_info] {
            // Create GPUObject with UnrefQueue before 'if' to
            // make sure it can be destroyed in UI thread safely
            auto gpu_object = GPUObject(new_frame_info.image, unref_queue);
            if (auto self = weak.lock()) {
              next_frame->CacheFrame(new_frame_info.duration,
                                     std::move(gpu_object));
              self->OnFrameUploaded(next_frame, true);
            }
          });
    } else {
      callback_runner->PostTask([weak, next_frame] {
        if (auto self = weak.lock()) {
          self->OnFrameUploaded(next_frame, false);
        }
      });
    }
  };

  if (produce_context_.register_upload_callback) {
    produce_context_.register_upload_callback(upload_task);
  }
}

fml::RefPtr<SkImageHolder> ImageProducer::GetCurrentFrame() const {
  return current_frame_;
}

bool ImageProducer::NextFrameReady() const {
  return next_frame_ && next_frame_->FrameIsReady() &&
         IMAGE_DIMENSION(next_frame_->GetGraphicsImage()->dimensions()) ==
             render_info_.dimensions();
}

bool ImageProducer::CurrentFrameReady() const {
  return current_frame_ && current_frame_->FrameIsReady() &&
         IMAGE_DIMENSION(current_frame_->GetGraphicsImage()->dimensions()) ==
             render_info_.dimensions();
}

void ImageProducer::TryDestroyCodec() {
  if (FrameCount() == 1) {
    codec_ = nullptr;
  }
}

const GrImageInfo& ImageProducer::GetInfo() {
  EnsureDescriptorInitialized();
  if (!descriptor_ && !is_svg_) {
    render_info_.reset();
    return render_info_;
  }
  if (!render_info_.isEmpty()) {
    return render_info_;
  }
  return orig_info_;
}

bool ImageProducer::ShouldCacheImage(const fml::RefPtr<SkImageHolder>& holder) {
  return should_use_cache_ &&
         holder->GetAllocationSize() < kMaxMemoryLimitPerFrame;
}

void ImageProducer::OnFrameUploaded(const fml::RefPtr<SkImageHolder>& holder,
                                    bool result) {
  // holder can't be null
  FML_DCHECK(holder);
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (produce_context_.is_deferred) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  holder->MarkUploadResult(result);

  if (!produce_context_.use_promise) {
    next_frame_ = holder;
  }

  if (produce_context_.upload_callback) {
    produce_context_.upload_callback(result);
  }
}

void ImageProducer::OnFrameDecoded(const fml::RefPtr<SkImageHolder>& holder,
                                   bool result, bool is_async) {
  // holder can't be null
  FML_DCHECK(holder);

  // For deferred image, modifications to the frame can occur on either the ui
  // thread or the raster thread, so locking is necessary to ensure safe access.
  // For the ImageProducer, all public methods are accessed through the Image,
  // and the Image already locks when accessing the ImageProducer. Therefore,
  // most methods do not require additional locking. However, the OnFrameDecoded
  // method is called by the ImageProducer's own member function via a post
  // task, so it requires locking.
  std::unique_ptr<std::lock_guard<std::recursive_mutex>> lock;
  if (produce_context_.is_deferred && is_async) {
    lock = std::make_unique<std::lock_guard<std::recursive_mutex>>(*mutex_);
  }

  if (use_texture_backend_) {
    decoding_ = false;
    // If it is a multi frame image, there might be more images needing decode.
    if (IsSingleFrameImage()) {
      decoded_ = true;
    }
  }

  TryDestroyCodec();

  holder->MarkLoadResult(result);

  if (use_texture_backend_) {
    // Won't use GPUResourceCache to manager image texture resource to avoid
    // image blink or other problems. if (FrameCount() == 1) {
    //   holder->SetUseCache(ShouldCacheImage(holder));
    //   GraphicsIsolate::Instance().CacheStoreImage(holder);
    // }
  }

  if (!produce_context_.use_promise && !produce_context_.decode_with_priority) {
    next_frame_ = holder;
  }

  if (produce_context_.decode_callback) {
    produce_context_.decode_callback(result);
  }
}

void ImageProducer::ReleaseAllFrames() {
  current_frame_ = nullptr;
  next_frame_ = nullptr;
#if defined(ENABLE_SVG)
  svg_frame_ = nullptr;
#endif
}

size_t ImageProducer::GetAllocationSize() const {
  if (is_svg_) {
#if defined(ENABLE_SVG)
    return svg_frame_->GetAllocationSize();
#else
    return 0;
#endif
  }
  if (current_frame_ && current_frame_->FrameIsReady()) {
    return current_frame_->GetAllocationSize();
  }
  if (next_frame_ && next_frame_->FrameIsReady()) {
    return next_frame_->GetAllocationSize();
  }
  return 0;
}

bool ImageProducer::WillNextFrameSizeChange() const {
  if (is_svg_) {
    return true;
  }
  if (!next_frame_ || !next_frame_->FrameIsReady()) {
    return false;
  }
  if (!current_frame_ || !current_frame_->FrameIsReady()) {
    return true;
  }
  return current_frame_->GetGraphicsImage()->dimensions() !=
         next_frame_->GetGraphicsImage()->dimensions();
}

void ImageProducer::SetExpectSizeCalculator(
    std::function<skity::Vec2(skity::Vec2)> calculator,
    bool force_use_original_size) {
  EnsureDescriptorInitialized();
  if (!descriptor_ && !is_svg_) {
    return;
  }

  if (descriptor_ && !descriptor_->IsSingleFrame()) {
    return;
  }

  // orig_info_.isEmpty() means raw data is broken.
  if (orig_info_.isEmpty()) {
    return;
  }

  if (orig_info_ == render_info_ && !is_svg_) {
    // Already max, ignore expect size to avoid recursively fallback.
    return;
  }

#if CACHE_IMAGE_BY_DRAWING_SIZE
#ifndef ENABLE_SKITY
  skity::Vec2 expect_size = calculator(skity::Vec2(
      orig_info_.dimensions().width(), orig_info_.dimensions().height()));
#else
  skity::Vec2 expect_size =
      calculator(skity::Vec2(orig_info_.width(), orig_info_.height()));
#endif  // ENABLE_SKITY
  // if origin_size can contain expect_size, use expect_size, else origin_size.
  auto chooser = [this, &expect_size, force_use_original_size]() {
    if (expect_size.x > orig_info_.width() ||
        expect_size.y > orig_info_.height() || force_use_original_size) {
      if (is_svg_) {
        render_info_ = orig_info_.makeWH(
            std::max(static_cast<int>(expect_size.x), render_info_.width()),
            std::max(static_cast<int>(expect_size.y), render_info_.height()));
      } else {
        render_info_ = orig_info_;
      }
    } else {
#ifndef ENABLE_SKITY
      render_info_ = orig_info_.makeDimensions(
          SkISize::Make(expect_size.x, expect_size.y));
#else
      render_info_ = ImageInfo::makeWH(expect_size.x, expect_size.y);
#endif  // ENABLE_SKITY
    }
  };

  // If hasn't set, just choose a proper size.
  if (render_info_.isEmpty()) {
    chooser();
    return;
  }

  // If expect_size enlarged, choose a new proper size.
  if (expect_size.x > render_info_.width() ||
      expect_size.y > render_info_.height() || force_use_original_size) {
    int old_width = render_info_.width();
    int old_height = render_info_.height();
    chooser();
    // If size changed, release all frames.
    if (old_width != render_info_.width() ||
        old_height != render_info_.height()) {
      ReleaseAllFrames();
    }
  }
#endif  // CACHE_IMAGE_BY_DRAWING_SIZE
}

}  // namespace clay
