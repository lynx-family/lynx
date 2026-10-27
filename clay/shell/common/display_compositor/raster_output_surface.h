// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_RASTER_OUTPUT_SURFACE_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_RASTER_OUTPUT_SURFACE_H_
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/flow/compositor/compositor_frame.h"
#include "clay/flow/surface.h"
#include "clay/gfx/paint_image.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_representation.h"
#include "clay/gfx/shared_image/texture_budget.h"
#include "clay/shell/common/display_compositor/upload_dependency.h"
namespace clay {
class OutputSurface;
class LayerTree;
class CompositorContext;
struct ScreenshotData;
namespace display {
struct RasterBuffer {
  std::shared_ptr<void> reservation;
  fml::RefPtr<SharedImageBacking> backing;
  fml::RefPtr<SharedImageRepresentation> representation;
  GrSurfacePtr surface;
  uint64_t allocation_bytes = 0;
};
inline SharedImageBacking* GetSharedImageBacking(const OutputTexture& texture) {
  return std::static_pointer_cast<RasterBuffer>(texture.resource()->storage())
      ->backing.get();
}
// Independent of window lifetime. Only the staged producer owns this Surface;
// the legacy Rasterizer::surface_ never points at it.
class RasterOutputSurface final : public Surface {
 public:
  RasterOutputSurface(fml::RefPtr<OutputSurface> owner,
                      fml::RefPtr<fml::TaskRunner> runner,
                      std::shared_ptr<TextureBudget> budget,
                      OutputConfig config);
  ~RasterOutputSurface() override;
  bool IsValid() override;
  std::unique_ptr<SurfaceFrame> AcquireFrame(const skity::Vec2& size) override;
  bool PrepareUploads(uint64_t submission);
  void SetSegmentPlan(std::vector<skity::Vec2> sizes) {
    segment_sizes_ = std::move(sizes);
    has_segment_plan_ = true;
  }
  void InvalidateUploads() { uploads_.Invalidate(); }
  void SetSegment(SegmentId segment) {
    segment_ = segment;
    completed_.reset();
  }
  std::shared_ptr<const OutputTexture> TakeCompleted() {
    return std::exchange(completed_, nullptr);
  }
  void SetConfig(OutputConfig config) { config_ = config; }
  bool CanReuseContext(const RasterOutputSurface& other) const;
  const OutputConfig& config() const { return config_; }
  fml::RefPtr<fml::TaskRunner> runner() const;
  std::function<void(std::function<void()>)> ContextTask() const;
  ScreenshotData ScreenshotFrame(std::shared_ptr<const CompositorFrame> frame,
                                 uint32_t background);
  std::pair<fml::RefPtr<PaintImage>, bool> SnapshotTree(
      LayerTree& tree, CompositorContext& context);
  void TrimUnusedBuffers();
  void EnterLowMemory() {
    budget_->SetLowMemory();
    TrimUnusedBuffers();
  }
  uint64_t budget_limit() const { return budget_->limit(); }
  void RetireAll(std::function<void()> drained);
  skity::Matrix GetRootTransformation() const override;
  GrContext* GetContext() override;
  GrContextPtr GetContextPtr() override;
  std::unique_ptr<GLContextResult> MakeRenderContextCurrent() override;
  bool ClearRenderContext() override;
  bool AllowsDrawingWhenGpuDisabled() const override { return false; }

 private:
  struct Context;
  struct Slot {
    SegmentId segment;
    std::shared_ptr<ResourceRecord> record;
  };
  std::shared_ptr<Context> context_;
  std::shared_ptr<TextureBudget> budget_;
  OutputConfig config_;
  SegmentId segment_ = 1;
  uint64_t content_version_ = 0;
  std::vector<Slot> slots_;
  std::vector<skity::Vec2> segment_sizes_;
  bool has_segment_plan_ = false;
  std::shared_ptr<const OutputTexture> completed_;
  UploadDependency uploads_;
};
}  // namespace display
}  // namespace clay
#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_RASTER_OUTPUT_SURFACE_H_
