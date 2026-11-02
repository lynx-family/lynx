// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor/staged_rasterizer.h"

#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/staged_image_read_scope.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/shell/common/screenshot_utils.h"

namespace clay::display {
namespace {
bool HasPendingSourceInTree(const Layer& layer,
                            DrawableImageRegistry& registry) {
  if (layer.is_empty()) return false;
  if (auto* image_layer = layer.as_drawable_image_layer()) {
    auto image = registry.GetDrawableImage(image_layer->image_id());
    return image && image->IsStagedReadPending();
  }
  if (auto* container = layer.as_container_layer()) {
    for (const auto& child : container->layers()) {
      if (HasPendingSourceInTree(*child, registry)) return true;
    }
  }
  return false;
}
}  // namespace
StagedRasterizer::StagedRasterizer(
    std::unique_ptr<RasterOutputSurface> surface,
    Stopwatch::RefreshRateUpdater& updater,
    std::shared_ptr<DrawableImageRegistry> registry, RasterBindingKey binding,
    std::shared_ptr<FrameMailbox> mailbox, std::function<void()> wake)
    : surface_(std::move(surface)),
      context_(updater),
      binding_(binding),
      mailbox_(std::move(mailbox)),
      wake_(std::move(wake)) {
  context_.SetDrawableImageRegistry(std::move(registry));
  if (surface_ && surface_->IsValid()) {
    auto current = surface_->MakeRenderContextCurrent();
    if (current && current->GetResult()) {
      context_.OnGrContextCreated();
      context_initialized_ = true;
    }
  }
}
StagedRasterizer::~StagedRasterizer() {
  if (surface_) Release(nullptr);
}
bool StagedRasterizer::IsValid() const {
  return context_initialized_ && surface_ && surface_->IsValid();
}
void StagedRasterizer::SetOutputConfig(OutputConfig config) {
  if (!surface_ || config == surface_->config()) return;
  CancelPendingProduction();
  surface_->SetConfig(config);
  ClearOutputPlan();
  surface_->TrimUnusedBuffers();
}
ProductionOutcome StagedRasterizer::RasterizeToOutputs(
    std::shared_ptr<LayerTree> tree, int64_t frame_time_ms) {
  TRACE_EVENT("clay", "StagedRasterizer::RasterizeToOutputs");
  if (!IsValid() || !tree || !tree->root_layer())
    return ProductionOutcome::kFailed;
  if (tree->StagedBinding() != binding_) return ProductionOutcome::kFailed;
  TRACE_EVENT("clay", tree->HasScrollUpdates() ? "StagedRasterizer::ScrollFrame"
                                               : "StagedRasterizer::IdleFrame");
  if (tree->HasPlatformViewLayer()) {
    if (mailbox_->ReportFailure(PipelineError::kUnsupportedContent)) wake_();
    return ProductionOutcome::kFailed;
  }
  const auto& config = surface_->config();
  if (tree->frame_size().x != config.width ||
      tree->frame_size().y != config.height ||
      tree->device_pixel_ratio() !=
          static_cast<float>(config.device_pixel_ratio))
    return ProductionOutcome::kFailed;
  pending_tree_frame_time_ms_ = frame_time_ms;
  // Keep the newest request retryable until uploads, context acquisition and
  // the scene plan have all succeeded, including reattach without a UI frame.
  pending_tree_ = tree;
  if (!surface_->PrepareUploads(tree->StagedLayoutVersion()))
    return DeferProduction();
  if (tree != tree_) {
    tree->MergeAnimations(tree_.get());
    if (tree_) tree_->ResetServiceManagerForAnimation();
    tree_ = std::move(tree);
  }
  needs_animation_frame_ =
      tree_->HasAnimations() && !tree_->DoAnimations(frame_time_ms);
  auto current = surface_->MakeRenderContextCurrent();
  if (!current || !current->GetResult()) return DeferProduction();
  auto state = std::make_unique<CompositorState>(tree_->frame_size());
  auto preroll =
      context_.AcquireFrame(surface_->GetContext(), nullptr, state.get(),
                            skity::Matrix(), false, true);
  bool readback = tree_->Preroll(*preroll, true);
  split_ = !tree_->HasScrollUpdates() && !readback && !tree_->HasAnimations() &&
           SegmentPlanner::CanSplit(*tree_->root_layer());
  segments_.clear();
  // Keep the producer's old leases until source preflight succeeds. A new
  // plan always requests full production, so these cannot be reused as new
  // scene content and are dropped together before acquiring output writes.
  // Retain content dirtiness until the replacement plan is published.
  ++plan_version_;
  if (split_) {
    {
      TRACE_EVENT("clay", "StagedRasterizer::RecordSegments");
      SegmentRecorder recorder(tree_->frame_size());
      auto frame =
          context_.AcquireFrame(surface_->GetContext(), recorder.canvas(),
                                state.get(), skity::Matrix(), false, true);
      tree_->SetSegmentRecorder(&recorder);
      tree_->Paint(*frame, true);
      tree_->SetSegmentRecorder(nullptr);
      segments_ = recorder.Finish();
    }
    constexpr uint64_t output_bytes_per_pixel = 12,
                       display_bytes_per_pixel = 12, window_bytes_per_pixel = 8;
    uint64_t output_pixels = 0;
    for (size_t i = 0; i < segments_.size(); ++i) {
      const auto& segment = segments_[i];
      const auto allocation = surface_->AllocationSize(
          i + 1, {segment.bounds.Width(), segment.bounds.Height()});
      output_pixels += static_cast<uint64_t>(allocation.x) *
                       static_cast<uint64_t>(allocation.y);
    }
    // Reserve both read/pending/write pools and the attachments actually used
    // by this host. Explicit screenshots reserve their readback separately.
    if (TextureBudget::EstimatePoolBytes(
            config, output_pixels, output_bytes_per_pixel,
            display_bytes_per_pixel,
            window_bytes_per_pixel) > surface_->budget_limit()) {
      split_ = false;
      segments_.clear();
      segments_.emplace_back();
    }
  } else {
    segments_.emplace_back();
  }
  if (!split_) segments_[0].bounds = skity::Rect::MakeSize(tree_->frame_size());
  {
    TRACE_EVENT("clay", "StagedRasterizer::BuildScene");
    auto scene = std::make_shared<PageSceneSnapshot>();
    scene->binding = binding_;
    scene->layout_version = tree_->StagedLayoutVersion();
    scene->segment_plan_version = plan_version_;
    scene->config = config;
    image_segments_.clear();
    for (size_t i = 0; i < segments_.size(); ++i) {
      if (segments_[i].dynamic && !segments_[i].frozen)
        image_segments_[segments_[i].image_id].push_back(i);
      scene->ordered_segments.push_back(
          {i + 1,
           {static_cast<int32_t>(segments_[i].bounds.Left()),
            static_cast<int32_t>(segments_[i].bounds.Top())}});
    }
    scene_ = std::move(scene);
    std::vector<skity::Vec2> segment_sizes;
    segment_sizes.reserve(segments_.size());
    for (const auto& segment : segments_)
      segment_sizes.emplace_back(segment.bounds.Width(),
                                 segment.bounds.Height());
    surface_->SetSegmentPlan(std::move(segment_sizes));
  }
  pending_tree_.reset();
  pending_tree_frame_time_ms_ = 0;
  return Produce(true);
}
bool StagedRasterizer::RequestRetainedFrame() {
  if (!pending_tree_ && tree_) {
    pending_tree_ = tree_;
    pending_tree_frame_time_ms_ =
        fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  }
  return pending_tree_ != nullptr;
}
bool StagedRasterizer::MarkDynamicSegmentsDirty(int64_t id) {
  if (tree_ && scene_ && split_ &&
      image_segments_.find(id) == image_segments_.end())
    return HasPendingProduction();
  dirty_.insert(id);
  return true;
}
ProductionOutcome StagedRasterizer::ProducePendingContent() {
  if (pending_tree_) {
    auto tree = std::move(pending_tree_);
    const int64_t frame_time_ms = pending_tree_frame_time_ms_;
    auto outcome = RasterizeToOutputs(tree, frame_time_ms);
    if (outcome != ProductionOutcome::kPublished && !pending_tree_) {
      pending_tree_ = std::move(tree);
      pending_tree_frame_time_ms_ = frame_time_ms;
    }
    return outcome;
  }
  if (dirty_.empty() && !full_production_pending_)
    return ProductionOutcome::kNoWork;
  // The first valid UI tree will consume these content updates. Until then,
  // keep them pending without scheduling empty production opportunities.
  if (!tree_ || !scene_) return ProductionOutcome::kNoWork;
  return Produce(full_production_pending_ || !split_);
}
bool StagedRasterizer::CanReuseSegment(size_t index, bool all) const {
  const auto& segment = segments_[index];
  return !all && index < outputs_.size() && outputs_[index].texture_ref &&
         (!segment.dynamic || segment.frozen ||
          !dirty_.count(segment.image_id));
}
bool StagedRasterizer::HasPendingSourceReads(bool all) {
  auto registry = context_.drawable_image_registry();
  if (!registry) return false;
  if (!split_) {
    // Grouped effects and other fallback content do not have a split plan.
    // Conservatively inspect nonempty preroll subtrees before any GPU write.
    return tree_->root_layer() &&
           HasPendingSourceInTree(*tree_->root_layer(), *registry);
  }
  for (size_t i = 0; i < segments_.size(); ++i) {
    const auto& segment = segments_[i];
    if (!segment.dynamic || CanReuseSegment(i, all)) continue;
    auto image = registry->GetDrawableImage(segment.image_id);
    if (image && image->IsStagedReadPending()) return true;
  }
  return false;
}
ProductionOutcome StagedRasterizer::Produce(bool all) {
  TRACE_EVENT("clay", "StagedRasterizer::Produce", "full_scene", all);
  if (!tree_ || !scene_ || !IsValid()) return ProductionOutcome::kFailed;
  full_production_pending_ |= all;
  all = full_production_pending_;
  // Validate every source before dropping cached outputs or acquiring a write.
  // A source still read by an older batch is normal backpressure, not a failed
  // output that needs sealing and retirement. Dirty/latest work stays pending.
  if (HasPendingSourceReads(all)) {
    TRACE_EVENT("clay", "StagedRasterizer::SourceReadPending");
    return DeferProduction();
  }
  timing_.tree_submission_id = tree_->StagedLayoutVersion();
  timing_.producer_start_ns =
      fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
  {
    TRACE_EVENT("clay", all ? "StagedRasterizer::FullProduction"
                            : "StagedRasterizer::PartialProduction");
  }
  StagedImageReadScope inputs(surface_->runner(), surface_->ContextTask());
  outputs_.resize(segments_.size());
  std::vector<TextureEntry> next(segments_.size());
  for (size_t i = 0; i < segments_.size(); ++i) {
    if (CanReuseSegment(i, all)) {
      TRACE_EVENT("clay", "StagedRasterizer::ReuseSegment");
      next[i] = outputs_[i];
    } else {
      // Drop only the producer's cache for outputs being replaced. Mailbox and
      // Display retain the last complete publication until its replacement is
      // submitted; another local copy here can pin every writable slot.
      outputs_[i] = {};
    }
  }
  for (size_t i = 0; i < segments_.size(); ++i) {
    if (next[i].texture_ref) continue;
    auto& segment = segments_[i];
    surface_->SetSegment(i + 1);
    auto frame = surface_->AcquireFrame(
        {segment.bounds.Width(), segment.bounds.Height()});
    if (!frame) {
      return DeferProduction();
    }
    auto* canvas = frame->GetCanvas();
    if (!canvas) return DeferProduction();
    {
      // Skity reuses the surface canvas across LockCanvas calls. Restore the
      // crop, transforms and clips before Submit, including early failure.
      GrAutoCanvasRestore restore(canvas, true);
      canvas->Clear(skity::Color_TRANSPARENT);
      if (split_)
        canvas->Translate(-segment.bounds.Left(), -segment.bounds.Top());
      if (!split_) {
        TRACE_EVENT("clay", "StagedRasterizer::RasterWholeScene");
        CompositorState state(tree_->frame_size());
        auto scope =
            context_.AcquireFrame(surface_->GetContext(), canvas, &state,
                                  skity::Matrix(), false, true);
        if (scope->Raster(*tree_, true, nullptr) != RasterStatus::kSuccess)
          return DeferProduction();
      } else if (segment.dynamic) {
        TRACE_EVENT("clay", "StagedRasterizer::PaintDynamicSegment");
        segment.repaint(canvas, surface_->GetContext());
      } else {
        TRACE_EVENT("clay", "StagedRasterizer::PaintStaticSegment");
        segment.recording->Draw(canvas);
      }
      if (inputs.blocked()) return DeferProduction();
    }
    if (!frame->Submit()) return DeferProduction();
    auto texture = surface_->TakeCompleted();
    if (!texture) return DeferProduction();
    next[i] = {i + 1, texture->content_version(), std::move(texture),
               scene_->ordered_segments[i].properties, std::nullopt};
  }
  outputs_ = std::move(next);
  auto outcome = Publish();
  if (outcome == ProductionOutcome::kPublished) {
    first_failure_ = fml::TimePoint();
    dirty_.clear();
    full_production_pending_ = false;
  }
  return outcome;
}
ProductionOutcome StagedRasterizer::DeferProduction() {
  auto now = fml::TimePoint::Now();
  if (first_failure_ == fml::TimePoint()) first_failure_ = now;
  if ((now - first_failure_).ToMilliseconds() > 2000) {
    if (mailbox_->ReportFailure(PipelineError::kProducerFailure)) wake_();
    return ProductionOutcome::kFailed;
  }
  return ProductionOutcome::kDeferred;
}
ProductionOutcome StagedRasterizer::Publish() {
  auto result = std::make_shared<RasterResult>();
  result->binding = binding_;
  result->render_version = ++render_version_;
  result->layout_version = scene_->layout_version;
  result->segment_plan_version = scene_->segment_plan_version;
  result->config = scene_->config;
  result->outputs = outputs_;
  timing_.render_id = render_version_;
  timing_.producer_submit_ns =
      fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
  result->timing = timing_;
  TRACE_EVENT("clay", "StagedRasterizer::Publish", "render_id", render_version_,
              "layout_version", scene_->layout_version);
  // All segment commands have been submitted. Transfer the complete snapshot
  // with its dependencies; consumers order their GPU reads without CPU waits.
  auto status = mailbox_->Publish({scene_, std::move(result)});
  if (status == FrameMailbox::PublishResult::kRejected)
    return ProductionOutcome::kFailed;
  TRACE_EVENT("clay", split_ ? "StagedRasterizer::PublishedSplit"
                             : "StagedRasterizer::PublishedWhole");
  if (status == FrameMailbox::PublishResult::kWake) wake_();
  return ProductionOutcome::kPublished;
}
void StagedRasterizer::CancelPendingProduction() {
  pending_tree_.reset();
  pending_tree_frame_time_ms_ = 0;
}
ScreenshotData StagedRasterizer::Screenshot(
    std::shared_ptr<const CompositorFrame> frame, uint32_t background) {
  return surface_ && frame && frame->result->binding == binding_
             ? surface_->ScreenshotFrame(std::move(frame), background)
             : ScreenshotData{};
}
void StagedRasterizer::OnFrameSubmitted(uint64_t layout) {
  if (tree_ && tree_->StagedLayoutVersion() == layout)
    tree_->AppendFrameTimings({}, true);
}
void StagedRasterizer::Trim() {
  if (surface_) {
    outputs_.clear();
    full_production_pending_ = scene_ != nullptr;
    context_.raster_cache().Clear();
    surface_->EnterLowMemory();
  }
}
void StagedRasterizer::ClearScene() {
  CancelPendingProduction();
  ClearOutputPlan();
  if (tree_) tree_->ResetServiceManagerForAnimation();
  tree_.reset();
}
void StagedRasterizer::ClearOutputPlan() {
  outputs_.clear();
  segments_.clear();
  image_segments_.clear();
  dirty_.clear();
  full_production_pending_ = false;
  scene_.reset();
}
std::unique_ptr<RasterOutputSurface> StagedRasterizer::Suspend(
    std::function<void()> drained) {
  ClearScene();
  if (surface_) {
    surface_->InvalidateUploads();
    surface_->RetireAll(std::move(drained));
  } else if (drained) {
    drained();
  }
  // UI image caches may still reference textures created by this context.
  // Hiding a page releases its output pools, not its image context.
  return std::move(surface_);
}
void StagedRasterizer::Release(std::function<void()> drained) {
  ClearScene();
  if (surface_) {
    auto current = surface_->MakeRenderContextCurrent();
    if (current && current->GetResult()) context_.OnGrContextDestroyed();
    surface_->RetireAll(std::move(drained));
    surface_.reset();
  } else if (drained)
    drained();
}
}  // namespace clay::display
