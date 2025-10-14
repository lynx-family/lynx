// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_RASTERIZER_H_
#define CLAY_SHELL_COMMON_RASTERIZER_H_

#include <deque>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/raster_thread_merger.h"
#include "base/include/fml/synchronization/sync_switch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "clay/common/recyclable.h"
#include "clay/common/service/service.h"
#include "clay/common/settings.h"
#include "clay/common/task_runners.h"
#include "clay/flow/compositor_context.h"
#include "clay/flow/embedded_views.h"
#include "clay/flow/frame_timings.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/surface.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/paint_image.h"
#include "clay/shell/common/screenshot_utils.h"
#include "clay/shell/common/services/compositor/compositor_service.h"
#include "clay/shell/common/services/instrumentation_service.h"
#include "clay/shell/common/services/platform_const_service.h"
#include "clay/shell/common/services/raster_frame_service.h"
#include "clay/shell/common/shell_common_rendering_backend.h"
#include "clay/ui/common/render_settings.h"

namespace clay {
class GpuResourceCache;
class ServiceManager;
}  // namespace clay

namespace clay {

using clay::GPUUnrefQueue;

//------------------------------------------------------------------------------
/// The rasterizer is a component owned by the shell that resides on the raster
/// task runner. Each shell owns exactly one instance of a rasterizer. The
/// rasterizer may only be created, used and collected on the raster task
/// runner.
///
/// The rasterizer owns the instance of the currently active on-screen render
/// surface. On this surface, it renders the contents of layer trees submitted
/// to it by the `Engine` (which lives on the UI task runner).
///
/// The primary components owned by the rasterizer are the compositor context
/// and the on-screen render surface. The compositor context has all the GPU
/// state necessary to render frames to the render surface.
///
class Rasterizer final : public Stopwatch::RefreshRateUpdater,
                         public clay::Recyclable {
 public:
  //----------------------------------------------------------------------------
  /// @brief      Creates a new instance of a rasterizer. Rasterizers may only
  ///             be created on the raster task runner. Rasterizers are
  ///             currently only created by the shell (which also sets itself up
  ///             as the rasterizer delegate).
  ///
  /// @param[in]  delegate                   The rasterizer delegate.
  /// @param[in]  gpu_image_behavior         How to handle calls to
  ///                                        MakeSkiaGpuImage.
  ///
  explicit Rasterizer(std::shared_ptr<clay::ServiceManager> service_manager);

  //----------------------------------------------------------------------------
  /// @brief      Destroys the rasterizer. This must happen on the raster task
  ///             runner. All GPU resources are collected before this call
  ///             returns. Any context set up by the embedder to hold these
  ///             resources can be immediately collected as well.
  ///
  ~Rasterizer();

  //----------------------------------------------------------------------------
  /// @brief      Rasterizers may be created well before an on-screen surface is
  ///             available for rendering. Shells usually create a rasterizer in
  ///             their constructors. Once an on-screen surface is available
  ///             however, one may be provided to the rasterizer using this
  ///             call. No rendering may occur before this call. The surface is
  ///             held till the balancing call to `Rasterizer::Teardown` is
  ///             made. Calling a setup before tearing down the previous surface
  ///             (if this is not the first time the surface has been set up) is
  ///             user error.
  ///
  /// @see        `Rasterizer::Teardown`
  ///
  /// @param[in]  surface  The on-screen render surface.
  ///
  void Setup(std::unique_ptr<Surface> surface);

  //----------------------------------------------------------------------------
  /// @brief      Releases the previously set up on-screen render surface and
  ///             collects associated resources. No more rendering may occur
  ///             till the next call to `Rasterizer::Setup` with a new render
  ///             surface. Calling a teardown without a setup is user error.
  ///
  void Teardown();

  //----------------------------------------------------------------------------
  /// @brief      Notifies the rasterizer that there is a low memory situation
  ///             and it must purge as many unnecessary resources as possible.
  ///             Currently, the Skia context associated with onscreen rendering
  ///             is told to free GPU resources.
  ///
  void NotifyLowMemoryWarning() const;

  //----------------------------------------------------------------------------
  /// @brief      Gets a weak pointer to the rasterizer. The rasterizer may only
  ///             be accessed on the raster task runner.
  ///
  /// @return     The weak pointer to the rasterizer.
  ///
  fml::WeakPtr<Rasterizer> GetWeakPtr() const;

  //----------------------------------------------------------------------------
  /// @brief      Sometimes, it may be necessary to render the same frame again
  ///             without having to wait for the framework to build a whole new
  ///             layer tree describing the same contents. One such case is when
  ///             external textures (video or camera streams for example) are
  ///             updated in an otherwise static layer tree. To support this use
  ///             case, the rasterizer holds onto the last rendered layer tree.
  ///
  /// @bug        https://github.com/flutter/flutter/issues/33939
  ///
  /// @return     A pointer to the last layer or `nullptr` if this rasterizer
  ///             has never rendered a frame.
  ///
  clay::LayerTree* GetLastLayerTree();

  //----------------------------------------------------------------------------
  /// @brief      Draws a last layer tree to the render surface. This may seem
  ///             entirely redundant at first glance. After all, on surface loss
  ///             and re-acquisition, the framework generates a new layer tree.
  ///             Otherwise, why render the same contents to the screen again?
  ///             This is used as an optimization in cases where there are
  ///             external textures (video or camera streams for example) in
  ///             referenced in the layer tree. These textures may be updated at
  ///             a cadence different from that of the Flutter application.
  ///             Flutter can re-render the layer tree with just the updated
  ///             textures instead of waiting for the framework to do the work
  ///             to generate the layer tree describing the same contents.
  ///
  RasterStatus DrawLastLayerTree(
      std::unique_ptr<FrameTimingsRecorder> frame_timings_recorder);

  std::shared_ptr<DrawableImageRegistry> GetDrawableImageRegistry();

  using LayerTreeDiscardCallback = std::function<bool(clay::LayerTree&)>;

  //----------------------------------------------------------------------------
  /// @brief      Takes the next item from the layer tree pipeline and executes
  ///             the raster thread frame workload for that pipeline item to
  ///             render a frame on the on-screen surface.
  ///
  ///             Why does the draw call take a layer tree pipeline and not the
  ///             layer tree directly?
  ///
  ///             The pipeline is the way book-keeping of frame workloads
  ///             distributed across the multiple threads is managed. The
  ///             rasterizer deals with the pipelines directly (instead of layer
  ///             trees which is what it actually renders) because the pipeline
  ///             consumer's workload must be accounted for within the pipeline
  ///             itself. If the rasterizer took the layer tree directly, it
  ///             would have to be taken out of the pipeline. That would signal
  ///             the end of the frame workload and the pipeline would be ready
  ///             for new frames. But the last frame has not been rendered by
  ///             the frame yet! On the other hand, the pipeline must own the
  ///             layer tree it renders because it keeps a reference to the last
  ///             layer tree around till a new frame is rendered. So a simple
  ///             reference wont work either. The `Rasterizer::DoDraw` method
  ///             actually performs the GPU operations within the layer tree
  ///             pipeline.
  ///
  /// @see        `Rasterizer::DoDraw`
  ///
  /// @param[in]  pipeline  The layer tree pipeline to take the next layer tree
  ///                       to render from.
  /// @param[in]  discard_callback if specified and returns true, the layer tree
  ///                             is discarded instead of being rendered
  /// @param[in]  report_instrumentation if specified and returns true, report
  ///                                    to perf collector after draw
  ///
  RasterStatus Draw(std::shared_ptr<LayerTree> layer_tree,
                    std::unique_ptr<FrameTimingsRecorder> recorder = nullptr,
                    LayerTreeDiscardCallback discard_callback = NoDiscard,
                    bool report_instrumentation = false);

  //----------------------------------------------------------------------------
  /// @brief      Screenshots the last layer tree to one of the supported
  ///             screenshot types and optionally Base 64 encodes that data for
  ///             easier transmission and packaging (usually over the service
  ///             protocol for instrumentation tools running on the host).
  ///
  /// @param[in]  type           The type of the screenshot to gather.
  /// @param[in]  base64_encode  Whether Base 64 encoding must be applied to the
  ///                            data after a screenshot has been captured.
  ///
  /// @return     A non-empty screenshot if one could be captured. A screenshot
  ///             capture may fail if there were no layer trees previously
  ///             rendered by this rasterizer, or, due to an unspecified
  ///             internal error. Internal error will be logged to the console.
  ///
  ScreenshotData ScreenshotLastLayerTree(
      ScreenshotData::ScreenshotType type, bool base64_encode,
      uint32_t background_color = Color::kTransparent().Value());

  //----------------------------------------------------------------------------
  /// @brief      Sets a callback that will be executed when the next layer tree
  ///             in rendered to the on-screen surface. This is used by
  ///             embedders to listen for one time operations like listening for
  ///             when the first frame is rendered so that they may hide splash
  ///             screens.
  ///
  ///             The callback is only executed once and dropped on the GPU
  ///             thread when executed (lambda captures must be able to deal
  ///             with the threading repercussions of this behavior).
  ///
  /// @param[in]  callback  The callback to execute when the next layer tree is
  ///                       rendered on-screen.
  ///
  void SetNextFrameCallback(const fml::closure& callback);

  //----------------------------------------------------------------------------
  /// @brief      Returns a pointer to the compositor context used by this
  ///             rasterizer. This pointer will never be `nullptr`.
  ///
  /// @return     The compositor context used by this rasterizer.
  ///
  clay::CompositorContext* compositor_context() {
    return compositor_context_.get();
  }

  //----------------------------------------------------------------------------
  /// @brief      Skia has no notion of time. To work around the performance
  ///             implications of this, it may cache GPU resources to reference
  ///             them from one frame to the next. Using this call, embedders
  ///             may set the maximum bytes cached by Skia in its caches
  ///             dedicated to on-screen rendering.
  ///
  /// @attention  This cache setting will be invalidated when the surface is
  ///             torn down via `Rasterizer::Teardown`. This call must be made
  ///             again with new limits after surface re-acquisition.
  ///
  /// @attention  This cache does not describe the entirety of GPU resources
  ///             that may be cached. The `RasterCache` also holds very large
  ///             GPU resources.
  ///
  /// @see        `RasterCache`
  ///
  /// @param[in]  max_bytes  The maximum byte size of resource that may be
  ///                        cached for GPU rendering.
  /// @param[in]  from_user  Whether this request was from user code, e.g. via
  ///                        the flutter/skia message channel, in which case
  ///                        it should not be overridden by the platform.
  ///
  void SetResourceCacheMaxBytes(size_t max_bytes, bool from_user);

  //----------------------------------------------------------------------------
  /// @brief      The current value of Skia's resource cache size, if a surface
  ///             is present.
  ///
  /// @attention  This cache does not describe the entirety of GPU resources
  ///             that may be cached. The `RasterCache` also holds very large
  ///             GPU resources.
  ///
  /// @see        `RasterCache`
  ///
  /// @return     The size of Skia's resource cache, if available.
  ///
  std::optional<size_t> GetResourceCacheMaxBytes() const;

  void set_unref_queue(fml::RefPtr<GPUUnrefQueue> queue) {
    unref_queue_ = queue;
  }

  void SetRenderSettings(fml::RefPtr<clay::RenderSettings> render_settings);

  fml::RefPtr<PaintImage> MakeRasterSnapshot(
      std::unique_ptr<LayerTree> layer_tree);

  fml::RefPtr<PaintImage> MakeRasterSnapshot(GrPicturePtr picture,
                                             skity::Vec2 size);

  const std::shared_ptr<clay::ServiceManager>& GetServiceManager() const {
    return service_manager_;
  }

  void CleanForRecycle() override;

 private:
  // |Stopwatch::Delegate|
  /// Time limit for a smooth frame.
  ///
  /// See: `DisplayManager::GetMainDisplayRefreshRate`.
  fml::Milliseconds GetFrameBudget() const override;

  RasterStatus DoDraw(
      std::unique_ptr<FrameTimingsRecorder> frame_timings_recorder,
      std::shared_ptr<clay::LayerTree> layer_tree);

  RasterStatus DrawToSurface(FrameTimingsRecorder& frame_timings_recorder,
                             clay::LayerTree& layer_tree);

  RasterStatus DrawToSurfaceUnsafe(FrameTimingsRecorder& frame_timings_recorder,
                                   clay::LayerTree& layer_tree);

  void FireNextFrameCallbackIfPresent();

  static bool NoDiscard(const clay::LayerTree& layer_tree) { return false; }

  const std::shared_ptr<clay::ServiceManager> service_manager_;
  const clay::Puppet<clay::Owner::kRaster, PlatformConstService>
      platform_const_service_;
  const clay::Puppet<clay::Owner::kRaster, RasterFrameService>
      raster_frame_service_;
  const clay::Puppet<clay::Owner::kRaster, InstrumentationService>
      instrumentation_service_;
  const clay::Puppet<clay::Owner::kRaster, CompositorService>
      compositor_service_;

  std::unique_ptr<Surface> surface_;
  std::unique_ptr<clay::CompositorContext> compositor_context_;
  // This is the last successfully rasterized layer tree.
  std::shared_ptr<clay::LayerTree> last_layer_tree_;
  fml::closure next_frame_callback_;
  bool user_override_resource_cache_bytes_;
  std::optional<size_t> max_cache_bytes_;
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  fml::RefPtr<clay::RenderSettings> render_settings_;
  bool last_memory_strategy_ = false;  // true: low memory usage, false: normal
  std::mutex frame_mutex_;
  std::unique_ptr<FrameTimingsRecorder> last_recorder_;
  const std::shared_ptr<FixedRefreshRateStopwatch> raster_time_;
  const std::shared_ptr<FixedRefreshRateStopwatch> frame_total_time_;

  // WeakPtrFactory must be the last member.
  fml::WeakPtrFactory<Rasterizer> weak_factory_;
  BASE_DISALLOW_COPY_AND_ASSIGN(Rasterizer);
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_RASTERIZER_H_
