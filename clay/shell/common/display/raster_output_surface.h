// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_RASTER_OUTPUT_SURFACE_H_
#define CLAY_SHELL_COMMON_DISPLAY_RASTER_OUTPUT_SURFACE_H_
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/common/graphics/texture_budget.h"
#include "clay/flow/compositor/compositor_frame.h"
#include "clay/flow/compositor/raster_buffer_pool.h"
#include "clay/flow/surface.h"
#include "clay/gfx/paint_image.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_representation.h"
#include "clay/shell/common/display/upload_dependency.h"
namespace clay {
class OutputSurface;
class LayerTree;
class CompositorContext;
class SkityImage;
class SharedImageSink;
struct ScreenshotData;
namespace display {
class TextureCompositor;
struct RasterBuffer {
  std::shared_ptr<void> reservation;
  // Native import caches can retain storage after the pool's CPU lease ends.
  std::function<void(std::shared_ptr<void>)> retain_reservation;
  fml::RefPtr<SharedImageBacking> backing;
  fml::RefPtr<SharedImageRepresentation> representation;
  GrSurfacePtr surface;
  // Optional read-only view of the native texture, independent of drawing
  // attachments. Its manager is retained by this buffer's owning context.
  std::shared_ptr<SkityImage> shared_image;
  // Consumer-context read view. It must not retain this buffer or its record.
  std::shared_ptr<void> consumer_read_view;
  // Keep native and Canvas imports separate for a safe per-batch fallback.
  std::shared_ptr<void> native_consumer_read_view;
  uint64_t allocation_bytes = 0;
  // Native storage left after an immutable output's drawing surface is
  // released. Zero keeps the full reservation on unsupported backends.
  uint64_t retained_bytes = 0;
  ImageOrigin origin = ImageOrigin::kBottomLeft;
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
                      OutputConfig config,
                      std::shared_ptr<Surface> rendering_surface = nullptr);
  ~RasterOutputSurface() override;
  bool IsValid() override;
  std::unique_ptr<SurfaceFrame> AcquireFrame(const skity::Vec2& size) override;
  std::unique_ptr<SurfaceFrame> AcquireCompositionFrame(
      const RasterResult& result, TextureCompositor* compositor);
  bool has_native_composition() const;
  std::shared_ptr<const GPUCompletion> submission_completion() const;
  bool PrepareUploads(uint64_t submission);
  void SetSubmissionDependencies(
      std::vector<std::shared_ptr<const GPUCompletion>> dependencies) {
    submission_dependencies_ = std::move(dependencies);
  }
  skity::Vec2 AllocationSize(SegmentId segment,
                             const skity::Vec2& logical_size) const;
  struct SegmentPlan {
    skity::Vec2 size;
    bool immutable = false;
    bool whole = false;
  };
  void SetSegmentPlan(std::vector<SegmentPlan> segments, uint64_t generation);
  std::shared_ptr<const OutputTexture> CachedOutput(SegmentId segment) const;
  void ResetDamage();
  void CommitWholeDamage(const OutputTexture& texture,
                         const skity::Rect& frame_damage);
  void InvalidateUploads() { uploads_.Invalidate(); }
  void SetSegment(SegmentId segment) {
    segment_ = segment;
    output_->texture.reset();
  }
  std::shared_ptr<const OutputTexture> TakeCompleted() {
    return std::exchange(output_->texture, nullptr);
  }
  void SetConfig(OutputConfig config) {
    if (config_ != config) {
      segment_plan_.clear();
      has_segment_plan_ = false;
      for (auto& slot : slots_) slot.cache->texture.reset();
      ResetDamage();
    }
    config_ = config;
  }
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
  struct Submission;
  class Sink;
  struct Output {
    std::shared_ptr<const OutputTexture> texture;
  };
  struct Slot {
    SegmentId segment;
    std::shared_ptr<ResourceRecord> record;
    OutputConfig config;
    uint64_t generation;
    bool immutable, whole;
    // Null means that this physical buffer has no valid content history.
    std::optional<skity::Rect> damage;
    std::shared_ptr<Output> cache;
  };
  static void SealImmutableBuffer(
      const std::shared_ptr<Context>& context,
      const std::shared_ptr<ResourceRecord>& record);
  std::unique_ptr<SurfaceFrame> AcquireFrame(const skity::Vec2& size,
                                             const RasterResult* result,
                                             TextureCompositor* compositor);
  std::shared_ptr<Context> context_;
  std::shared_ptr<Submission> submission_;
  std::shared_ptr<Output> output_ = std::make_shared<Output>();
  std::shared_ptr<TextureBudget> budget_;
  OutputConfig config_;
  SegmentId segment_ = 1;
  uint64_t content_version_ = 0, plan_generation_ = 0;
  std::vector<Slot> slots_;
  std::vector<std::shared_ptr<const GPUCompletion>> submission_dependencies_;
  std::vector<SegmentPlan> segment_plan_;
  bool has_segment_plan_ = false;
  UploadDependency uploads_;
};
}  // namespace display
}  // namespace clay
#endif  // CLAY_SHELL_COMMON_DISPLAY_RASTER_OUTPUT_SURFACE_H_
