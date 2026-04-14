// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_host_gl.h"

#include <string>

#include "base/trace/native/trace_event.h"
#include "build/build_config.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/shell/platform/embedder/embedder_struct_macros.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"

namespace clay {

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateHostGL(
    ClayHeadlessEngine* engine, const ClayOpenGLRendererConfig& config) {
  const ClayOpenGLRendererConfig* config_ptr = &config;
  if (SAFE_ACCESS(config_ptr, enable_shared_image_sink, false)) {
    ClaySharedImageSinkBufferMode buffer_mode =
        SAFE_ACCESS(config_ptr, shared_image_sink_buffer_mode,
                    kClaySharedImageSinkBufferModeDoubleBuffer);
    return std::make_unique<ClayHeadlessRendererSharedImageHostGL>(
        engine, config, buffer_mode);

  } else {
    return std::make_unique<ClayHeadlessRendererHostGL>(engine, config);
  }
}

ClayHeadlessRendererHostGL::ClayHeadlessRendererHostGL(
    ClayHeadlessEngine* engine, const ClayOpenGLRendererConfig& renderer_config)
    : ClayHeadlessRendererGL(engine), config_(renderer_config) {
  FML_LOG(ERROR) << "Starting Clay in [Host GL] mode. "
                    "Components using external textures will NOT work";
}

GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererHostGL::GetGLProcResolver() const {
  return [this](const char* name) -> void* {
    return const_cast<ClayHeadlessRendererHostGL*>(this)->ResolveProc(name);
  };
}

bool ClayHeadlessRendererHostGL::MakeCurrent() {
  return config_.make_current(engine_->UserData());
}

bool ClayHeadlessRendererHostGL::ClearCurrent() {
  return config_.clear_current(engine_->UserData());
}

bool ClayHeadlessRendererHostGL::Present() {
  return config_.present(engine_->UserData());
}

int64_t ClayHeadlessRendererHostGL::FBO(const ClayFrameInfo& frame_info) {
  return config_.fbo_callback(engine_->UserData(), &frame_info);
}

void* ClayHeadlessRendererHostGL::ResolveProc(const char* name) {
  return config_.gl_proc_resolver(engine_->UserData(), name);
}

void ClayHeadlessRendererHostGL::CleanupGPUResources() {}

ClayHeadlessRendererSharedImageHostGL::ClayHeadlessRendererSharedImageHostGL(
    ClayHeadlessEngine* engine, const ClayOpenGLRendererConfig& renderer_config,
    ClaySharedImageSinkBufferMode buffer_mode)
    : ClayHeadlessRenderer(engine),
      host_gl_thread_("clay.headless.host-gl"),
      config_(renderer_config) {
  FML_LOG(ERROR) << "Starting Clay in [Host GL+SharedImage] mode. "
                    "Maybe slow in large views";

  host_gl_thread_.GetTaskRunner()->PostTask([&] {
#ifdef ENABLE_SKITY
    host_gl_surface_ = std::make_unique<GPUSurfaceGLSkity>(this, nullptr);
#else
    host_gl_surface_ = std::make_unique<GPUSurfaceGLSkia>(this, true);
#endif
  });

  ClayHeadlessRendererConfig hardware_config;

  ClaySharedImageBackingType image_backing_type;

#if OS_MACOSX
  image_backing_type = kClaySharedImageBackingTypeIOSurface;
  hardware_config.type = kClayRendererTypeMetal;
#elif OS_WIN
  image_backing_type = kClaySharedImageBackingTypeD3DTexture;
  hardware_config.type = kClayRendererTypeOpenGL;
#elif OS_LINUX
  image_backing_type = kClaySharedImageBackingTypeShmImage;
  hardware_config.type = kClayRendererTypeOpenGL;
#elif OS_HARMONY
  image_backing_type = kClaySharedImageBackingTypeNativeImage;
  hardware_config.type = kClayRendererTypeOpenGL;
#else
  FML_DCHECK(false) << "Shared Image Renderer not supported on this platform";
  return;
#endif

  ClaySharedImageSinkRef sink_ref =
      ClayCreateSharedImageSink(buffer_mode, image_backing_type,
                                kClaySharedImageBackingPixelFormatNative8888);
  shared_image_sink_ = fml::RefPtr<clay::SharedImageSink>(
      reinterpret_cast<clay::SharedImageSink*>(sink_ref));

  shared_image_sink_->SetFrameAvailableCallback([this] {
    // The callback is triggered in Clay Raster thread
    host_gl_thread_.GetTaskRunner()->PostTask([this] { Draw(); });
  });

  hardware_config.hardware.struct_size = sizeof(hardware_config.hardware);
  hardware_config.hardware.sink_ref = sink_ref;
  renderer_ = ClayHeadlessRenderer::Create(engine, hardware_config);

  FML_CHECK(renderer_);

  // sink_ref is owned by shared_image_sink_
  ClayReleaseSharedImageSink(sink_ref);
}

// |ClayHeadlessRenderer|
EmbedderSurfaceSoftwareDelegate*
ClayHeadlessRendererSharedImageHostGL::GetSoftwareRendererDelegate() {
  return renderer_->GetSoftwareRendererDelegate();
}
#ifdef SHELL_ENABLE_GL
// |ClayHeadlessRenderer|
GPUSurfaceGLDelegate*
ClayHeadlessRendererSharedImageHostGL::GetGLRendererDelegate() {
  return renderer_->GetGLRendererDelegate();
}
#endif
#ifdef SHELL_ENABLE_METAL
// |ClayHeadlessRenderer|
EmbedderSurfaceMetalDelegate*
ClayHeadlessRendererSharedImageHostGL::GetMetalRendererDelegate() {
  return renderer_->GetMetalRendererDelegate();
}
#endif

ClayHeadlessRenderer*
ClayHeadlessRendererSharedImageHostGL::GetEngineRenderer() {
  return renderer_.get();
}

ClayHeadlessRendererSharedImageHostGL::
    ~ClayHeadlessRendererSharedImageHostGL() {
  {
    std::lock_guard<std::mutex> lock(shared_image_sink_mutex_);
    // shared_image_sink internally keeps ref to D3D device and mutex,
    // which means it should be reset before destroy renderer
    shared_image_sink_->SetFrameAvailableCallback(nullptr);
    shared_image_sink_ = nullptr;
  }
  renderer_.reset();
  fml::AutoResetWaitableEvent latch;
  host_gl_thread_.GetTaskRunner()->PostTask([&] {
    host_gl_surface_.reset();
    latch.Signal();
  });
  latch.Wait();
}

void ClayHeadlessRendererSharedImageHostGL::CleanupGPUResources() {
  renderer_->CleanupGPUResources();
}

// |GPUSurfaceGLDelegate|
std::unique_ptr<GLContextResult>
ClayHeadlessRendererSharedImageHostGL::GLContextMakeCurrent() {
  return std::make_unique<GLContextDefaultResult>(
      config_.make_current(engine_->UserData()));
}

// |GPUSurfaceGLDelegate|
bool ClayHeadlessRendererSharedImageHostGL::GLContextClearCurrent() {
  return config_.clear_current(engine_->UserData());
}

// |GPUSurfaceGLDelegate|
bool ClayHeadlessRendererSharedImageHostGL::GLContextPresent(
    const GLPresentInfo& present_info) {
  return config_.present(engine_->UserData());
}

// |GPUSurfaceGLDelegate|
bool ClayHeadlessRendererSharedImageHostGL::GLContextFBOResetAfterPresent()
    const {
  return true;
}

// |GPUSurfaceGLDelegate|
GLFBOInfo ClayHeadlessRendererSharedImageHostGL::GLContextFBO(
    GLFrameInfo frame_info) const {
  ClayFrameInfo clay_frame_info{};
  clay_frame_info.struct_size = sizeof(clay_frame_info);
  clay_frame_info.width = frame_info.width;
  clay_frame_info.height = frame_info.height;
  return {.fbo_id = config_.fbo_callback(engine_->UserData(), &clay_frame_info),
          .existing_damage = {}};
}

// |GPUSurfaceGLDelegate|
GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererSharedImageHostGL::GetGLProcResolver() const {
  return [gl_proc_resolver = config_.gl_proc_resolver,
          user_data = engine_->UserData()](const char* name) -> void* {
    return gl_proc_resolver(user_data, name);
  };
}

void ClayHeadlessRendererSharedImageHostGL::Draw() {
#ifdef ENABLE_SKITY
  FML_UNIMPLEMENTED();
#else
  TRACE_EVENT("clay", __FUNCTION__);
  std::lock_guard<std::mutex> lock(shared_image_sink_mutex_);
  if (!host_gl_surface_ || !shared_image_sink_) {
    return;
  }

  SkMatrix transformation;
  SkBitmap bitmap;
  {
    fml::RefPtr<clay::SharedImageBacking> backing =
        shared_image_sink_->UpdateFront(nullptr);
    if (!backing) {
      FML_LOG(ERROR) << "No front buffer";
      return;
    }

    // We don't need to hold the front, so we always release it
    struct AutoReleaseSink {
      explicit AutoReleaseSink(clay::SharedImageSink& sink) : sink_(sink) {}

      ~AutoReleaseSink() { sink_.ReleaseFront(nullptr); }

      clay::SharedImageSink& sink_;
    };

    AutoReleaseSink auto_release_sink(*shared_image_sink_);

    if (backing->GetPixelFormat() !=
        clay::SharedImageBacking::PixelFormat::kNative8888) {
      FML_LOG(ERROR) << "PixelFormat not supported: "
                     << static_cast<uint32_t>(backing->GetPixelFormat());
      return;
    }

    // Currently, kNative8888 equals BGRA8888
    auto image_info = SkImageInfo::Make(
        SkISize::Make(backing->GetSize().x, backing->GetSize().y),
        kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    bitmap.allocPixels(image_info, 0);

    if (std::unique_ptr<clay::FenceSync> fence_sync = backing->GetFenceSync()) {
      if (!fence_sync->ClientWait()) {
        FML_LOG(ERROR) << "Failed to wait sync";
        return;
      }
    }

    {
      TRACE_EVENT("clay", "SharedImageBacking::ReadbackToMemory");

      // temp trace
      fml::TimePoint start = fml::TimePoint::Now();

      if (!backing->ReadbackToMemory(&bitmap.pixmap(), 1)) {
        FML_LOG(ERROR) << "Failed to ReadbackToMemory";
        return;
      }

      bitmap.setImmutable();

      transformation =
          clay::ConvertSkityMatrixToSkMatrix(backing->GetTransformation());

      [[maybe_unused]] fml::TimeDelta duration = fml::TimePoint::Now() - start;

      // Remove these lines after tracing working
      // FML_LOG(ERROR) << "!!! ReadbackToMemory from ["
      //                << backing->GetSize().width() << "x"
      //                << backing->GetSize().height()
      //                << "], cost: " << duration.ToMilliseconds() << "ms";
    }
  }

  std::unique_ptr<SurfaceFrame> frame = host_gl_surface_->AcquireFrame(
      {bitmap.dimensions().fWidth, bitmap.dimensions().fHeight});

  if (!frame) {
    FML_LOG(ERROR) << "Failed to AcquireFrame";
    return;
  }

  SkCanvas* canvas = frame->GetCanvas();
  canvas->clear(SK_ColorTRANSPARENT);

  SkAutoCanvasRestore autoRestore(canvas, true);

  sk_sp<SkImage> sk_image = bitmap.asImage();

  SkIRect bounds = sk_image->bounds();

  // The incoming texture is vertically flipped, so we flip it
  // back.
  // Maybe it's better to use SurfaceOrigin in AdoptTexture,
  // but on Electron this method doesn't work.
  SkMatrix flip_y_mat =
      SkMatrix::MakeAll(1, 0, 0, 0, -1, bounds.height(), 0, 0, 1);

  canvas->concat(flip_y_mat);

  if (!transformation.isIdentity()) {
    sk_sp<SkShader> shader =
        sk_image->makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat,
                             SkSamplingOptions(), transformation);

    SkPaint paintWithShader;
    paintWithShader.setShader(shader);
    canvas->drawRect(SkRect::Make(sk_image->bounds()), paintWithShader);
  } else {
    canvas->drawImage(sk_image, 0, 0, SkSamplingOptions());
  }
  frame->Submit();

  host_gl_surface_->GetContext()->performDeferredCleanup(
      std::chrono::milliseconds(0));
#endif
}

}  // namespace clay
