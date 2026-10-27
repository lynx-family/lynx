// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display/raster_output_surface.h"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/staged_image_read_scope.h"
#include "clay/fml/logging.h"
#include "clay/gfx/paint_image_skity.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/skity/skity_image.h"
#include "clay/shell/common/display/display_compositor.h"
#include "clay/shell/common/display/resource_retirement_queue.h"
#include "clay/shell/common/display/texture_compositor.h"
#include "clay/shell/common/output_surface.h"
#include "clay/shell/common/screenshot_utils.h"

namespace clay::display {
namespace {
RasterBufferSize PixelSize(const skity::Vec2& size) {
  if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0 ||
      size.y <= 0 || size.x > kMaxRasterBufferDimension ||
      size.y > kMaxRasterBufferDimension || std::floor(size.x) != size.x ||
      std::floor(size.y) != size.y)
    return {};
  return {static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)};
}
bool MatchesConfig(const ResourceRecord& record,
                   const OutputConfig& allocation_config,
                   const OutputConfig& config) {
  const auto& key = record.key();
  const auto& image = record.image();
  return allocation_config.SameContentConfig(config) &&
         key.device_id == config.device_id &&
         key.device_generation == config.device_generation &&
         image.format == config.format && image.alpha == config.alpha &&
         image.color == config.color;
}
}  // namespace
struct RasterOutputSurface::Context {
  // Allocation storage may retain this owner. Keep it separate from the
  // renderer, whose accessor retains the sink and its representation provider.
  fml::RefPtr<OutputSurface> owner, renderer;
  fml::RefPtr<fml::TaskRunner> runner;
  fml::RefPtr<Sink> sink;
  GrContextPtr gpu;
  std::shared_ptr<Surface> surface;
  void RecreateProducer();
  std::unique_ptr<GLContextResult> MakeCurrent() {
    return surface ? surface->MakeRenderContextCurrent() : owner->MakeCurrent();
  }
};
struct RasterOutputSurface::Submission {
  std::weak_ptr<Context> context;
  // A failed frame may outlive replacement of the producer delegate.
  fml::RefPtr<OutputSurface> renderer;
  fml::RefPtr<Sink> sink;
  std::shared_ptr<Surface> surface;
  std::shared_ptr<ResourceRecord> record;
  DependentGPUCompletion::Dependencies dependencies;
  TextureComposition composition;
  std::shared_ptr<const GPUCompletion> completion;
  std::shared_ptr<const OutputTexture> output;
  PixelRect crop;
  uint64_t version = 0;
  bool native = false, attempted = false, begun = false, ended = false;
  bool discard = false;

  bool Finish(std::unique_ptr<FenceSync> fence) {
    if (ended || !begun) return false;
    ended = true;
    if (!completion) {
      if (auto owner = context.lock())
        completion = owner->owner->CreateDisplayCompletion(
            surface->GetContext(), std::move(fence));
    }
    const bool proven = completion != nullptr;
    completion =
        DependentGPUCompletion::Wrap(std::move(completion), dependencies);
    record->EndWrite(completion);
    if (discard || !proven) {
      record->Retire();
    } else {
      output = OutputTexture::WithValidRect(record, version, crop);
    }
    // SwapBack acknowledges a consumed write, including a discarded frame.
    // Returning false here would make the delegate retry EndWrite/fencing.
    return true;
  }
};
class RasterOutputSurface::Sink final : public SharedImageSink {
 public:
  void SetSubmission(const std::shared_ptr<Submission>& submission) {
    current_ = submission;
  }
  fml::RefPtr<SharedImageRepresentation> GetProducerRepresentation(
      SharedImageBacking* backing) override {
    auto submission = current_.lock();
    if (!submission || submission->ended) return nullptr;
    auto buffer =
        std::static_pointer_cast<RasterBuffer>(submission->record->storage());
    return buffer->backing.get() == backing ? buffer->representation : nullptr;
  }
  void SetFrameAvailableCallback(const fml::closure&) override {}
  fml::RefPtr<SharedImageBacking> UpdateFront(
      std::unique_ptr<FenceSync>) override {
    return nullptr;
  }
  fml::RefPtr<SharedImageBacking> UpdateFrontToLatest(
      std::unique_ptr<FenceSync>) override {
    return nullptr;
  }
  void ReleaseFront(std::unique_ptr<FenceSync>) override {}
  std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t> AcquireBack(
      skity::Vec2 size, std::optional<GraphicsMemoryHandle> = {}) override {
    auto submission = current_.lock();
    if (!submission || submission->ended) return {nullptr, 0};
    auto buffer =
        std::static_pointer_cast<RasterBuffer>(submission->record->storage());
    if (buffer->backing->GetSize() != size ||
        (!submission->begun && !submission->record->BeginWrite()))
      return {nullptr, 0};
    submission->begun = true;
    return {buffer->backing, 0};
  }
  std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t, AcquireBackStatus>
  TryAcquireBack(skity::Vec2 size,
                 std::optional<GraphicsMemoryHandle> handle = {}) override {
    auto [backing, age] = AcquireBack(size, handle);
    auto status =
        backing ? AcquireBackStatus::kSuccess : AcquireBackStatus::kFullAttach;
    return {std::move(backing), age, status};
  }
  std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t> AcquireBackForced(
      skity::Vec2 size,
      std::optional<GraphicsMemoryHandle> handle = {}) override {
    return AcquireBack(size, handle);
  }
  bool SwapBack(std::unique_ptr<FenceSync> fence) override {
    auto submission = current_.lock();
    current_.reset();
    return submission && submission->Finish(std::move(fence));
  }
  uint32_t Capacity() const override { return kMaxRasterBuffersPerSegment; }

 private:
  // The backend accessor owns this sink. A strong write lease would form a
  // cycle through ResourceRecord's context-bound storage deleter.
  std::weak_ptr<Submission> current_;
};
void RasterOutputSurface::Context::RecreateProducer() {
  sink = fml::MakeRefCounted<Sink>();
  renderer = owner->CreateSharedImageOutput(sink);
  surface = renderer ? renderer->CreateGPUSurface(gpu) : nullptr;
  if (!gpu && surface) gpu = surface->GetContextPtr();
}
RasterOutputSurface::RasterOutputSurface(
    fml::RefPtr<OutputSurface> owner, fml::RefPtr<fml::TaskRunner> runner,
    std::shared_ptr<TextureBudget> budget, OutputConfig config,
    std::shared_ptr<Surface> rendering_surface)
    : context_(std::make_shared<Context>()),
      budget_(std::move(budget)),
      config_(config) {
  context_->owner = std::move(owner);
  context_->runner = std::move(runner);
  context_->gpu =
      rendering_surface ? rendering_surface->GetContextPtr() : nullptr;
  context_->RecreateProducer();
}
RasterOutputSurface::~RasterOutputSurface() { RetireAll(nullptr); }
bool RasterOutputSurface::CanReuseContext(
    const RasterOutputSurface& other) const {
  return context_->owner == other.context_->owner &&
         config_.device_id == other.config_.device_id &&
         config_.device_generation == other.config_.device_generation;
}
fml::RefPtr<fml::TaskRunner> RasterOutputSurface::runner() const {
  return context_->runner;
}
std::function<void(std::function<void()>)> RasterOutputSurface::ContextTask()
    const {
  return [context = context_](std::function<void()> task) {
    auto current = context->MakeCurrent();
    task();
  };
}
bool RasterOutputSurface::PrepareUploads(uint64_t submission) {
  TRACE_EVENT("clay", "RasterOutputSurface::PrepareUploads");
  auto main_context = context_->owner->GetMainGrContext();
  // Uploads and production on the same context are already ordered on its
  // owning sequence and GPU queue, including uploads deferred until drawing.
  if (main_context && context_->surface && GetContext() == main_context.get())
    return true;
  return uploads_.Prepare(
      submission,
      [this]() -> std::shared_ptr<GPUCompletion> {
        auto current = context_->owner->MakeCurrent();
        if (!current || !current->GetResult()) return nullptr;
        auto gpu = context_->owner->GetMainGrContext();
        return gpu ? context_->owner->CreateDisplayCompletion(gpu.get())
                   : nullptr;
      },
      [this](const auto& completion) {
        auto current = MakeRenderContextCurrent();
        return current && current->GetResult() &&
               context_->owner->EnqueueStagedUploadWait(GetContext(),
                                                        completion);
      });
}
bool RasterOutputSurface::IsValid() {
  return context_->surface && context_->surface->IsValid();
}
skity::Vec2 RasterOutputSurface::AllocationSize(
    SegmentId segment, const skity::Vec2& logical_size) const {
  const auto logical = PixelSize(logical_size);
  auto capacity = RasterBufferAllocationSize(logical, config_.height);
  if (!capacity.width) return {};
  for (const auto& slot : slots_) {
    const auto& image = slot.record->image();
    if (slot.segment == segment &&
        MatchesConfig(*slot.record, slot.config, config_) &&
        CanUseRasterBuffer({image.width, image.height}, logical,
                           config_.height))
      capacity.height = std::max(capacity.height, image.height);
  }
  return {static_cast<float>(capacity.width),
          static_cast<float>(capacity.height)};
}
void RasterOutputSurface::SetSegmentPlan(std::vector<SegmentPlan> segments,
                                         uint64_t generation) {
  segment_plan_ = std::move(segments);
  plan_generation_ = generation;
  has_segment_plan_ = true;
  for (auto& slot : slots_) {
    if (slot.generation != generation) slot.cache->texture.reset();
  }
}
std::shared_ptr<const OutputTexture> RasterOutputSurface::CachedOutput(
    SegmentId segment) const {
  for (const auto& slot : slots_) {
    if (slot.segment == segment && slot.generation == plan_generation_ &&
        slot.immutable && slot.cache->texture && !slot.record->IsRetired())
      return slot.cache->texture;
  }
  return nullptr;
}
void RasterOutputSurface::ResetDamage() {
  for (auto& slot : slots_) slot.damage.reset();
}
void RasterOutputSurface::CommitWholeDamage(const OutputTexture& texture,
                                            const skity::Rect& frame_damage) {
  for (auto& slot : slots_) {
    if (!slot.whole) continue;
    if (slot.record == texture.resource()) {
      slot.damage = skity::Rect::MakeEmpty();
    } else if (slot.damage) {
      slot.damage->Join(frame_damage);
    }
  }
}
void RasterOutputSurface::SealImmutableBuffer(
    const std::shared_ptr<Context>& context,
    const std::shared_ptr<ResourceRecord>& record) {
  auto buffer = std::static_pointer_cast<RasterBuffer>(record->storage());
  auto ready = record->ready();
  if (!buffer->retained_bytes || !ready) return;
  struct Seal : std::enable_shared_from_this<Seal> {
    std::shared_ptr<Context> context;
    std::shared_ptr<RasterBuffer> buffer;
    std::shared_ptr<const GPUCompletion> ready;
    void Poll() {
      // SurfaceFrame and any other drawing aliases must release their
      // attachments before accounting can shrink to the retained image.
      if (!ready->IsSafeToRelease() || buffer->surface.use_count() > 1) {
        context->runner->PostDelayedTask(
            [self = shared_from_this()] { self->Poll(); },
            fml::TimeDelta::FromMilliseconds(8));
        return;
      }
      auto current = context->MakeCurrent();
      if (!current || !current->GetResult()) return;
      buffer->surface.reset();
      std::static_pointer_cast<TextureBudget::Reservation>(buffer->reservation)
          ->Resize(buffer->retained_bytes);
    }
  };
  auto seal = std::make_shared<Seal>();
  seal->context = context;
  seal->buffer = std::move(buffer);
  seal->ready = std::move(ready);
  // Always post: the submitting SurfaceFrame still owns the drawing surface.
  context->runner->PostTask([seal] { seal->Poll(); });
}
std::unique_ptr<SurfaceFrame> RasterOutputSurface::AcquireFrame(
    const skity::Vec2& size) {
  return AcquireFrame(size, nullptr, nullptr);
}
std::unique_ptr<SurfaceFrame> RasterOutputSurface::AcquireCompositionFrame(
    const RasterResult& result, TextureCompositor* compositor) {
  return AcquireFrame({result.config.width, result.config.height}, &result,
                      compositor);
}
bool RasterOutputSurface::has_native_composition() const {
  return submission_ && submission_->native;
}
std::shared_ptr<const GPUCompletion>
RasterOutputSurface::submission_completion() const {
  return submission_ ? submission_->completion : nullptr;
}
std::unique_ptr<SurfaceFrame> RasterOutputSurface::AcquireFrame(
    const skity::Vec2& size, const RasterResult* result,
    TextureCompositor* compositor) {
  TRACE_EVENT("clay", "RasterOutputSurface::AcquireFrame");
  FML_DCHECK(context_->runner->RunsTasksOnCurrentThread());
  output_->texture.reset();
  submission_ = std::make_shared<Submission>();
  auto submission = submission_;
  const auto logical = PixelSize(size);
  if (!IsValid() || !config_.IsValid() || !logical.width) return nullptr;
  const auto allocation_size = AllocationSize(segment_, size);
  if (allocation_size.x <= 0 || allocation_size.y <= 0) return nullptr;
  const SegmentPlan plan =
      has_segment_plan_ && segment_ && segment_ <= segment_plan_.size()
          ? segment_plan_[segment_ - 1]
          : SegmentPlan{size};
  auto current = MakeRenderContextCurrent();
  if (!current || !current->GetResult()) return nullptr;
  slots_.erase(
      std::remove_if(
          slots_.begin(), slots_.end(),
          [&](auto& slot) {
            const auto& image = slot.record->image();
            bool obsolete = !MatchesConfig(*slot.record, slot.config, config_);
            if (slot.segment == segment_) {
              obsolete |= !CanUseRasterBuffer({image.width, image.height},
                                              logical, config_.height);
            } else if (has_segment_plan_) {
              if (!slot.segment || slot.segment > segment_plan_.size()) {
                obsolete = true;
              } else {
                const auto& target = segment_plan_[slot.segment - 1].size;
                obsolete |=
                    !CanUseRasterBuffer({image.width, image.height},
                                        PixelSize(target), config_.height);
              }
            }
            if (has_segment_plan_ && slot.segment &&
                slot.segment <= segment_plan_.size()) {
              const auto& target = segment_plan_[slot.segment - 1];
              obsolete |=
                  slot.immutable != target.immutable ||
                  slot.whole != target.whole ||
                  (slot.immutable && slot.generation != plan_generation_);
            }
            // Different segments normally have different sizes.
            // Reclaim against each segment's own current plan,
            // without retiring an older complete frame still held
            // by Mailbox or Display.
            return slot.record->CanDestroy() &&
                   (obsolete || !slot.record->CanReuse());
          }),
      slots_.end());
  std::shared_ptr<ResourceRecord> record;
  std::optional<skity::Rect> existing_damage;
  size_t count = 0;
  for (auto& slot : slots_) {
    if (slot.segment != segment_ || slot.immutable != plan.immutable ||
        slot.whole != plan.whole ||
        (slot.immutable && slot.generation != plan_generation_))
      continue;
    ++count;
    const auto& image = slot.record->image();
    if ((!record || image.height < record->image().height) &&
        CanUseRasterBuffer({image.width, image.height}, logical,
                           config_.height) &&
        MatchesConfig(*slot.record, slot.config, config_) &&
        slot.record->CanReuse()) {
      record = slot.record;
      existing_damage = slot.damage;
    }
  }
  if (!record) {
    if (count >= (plan.immutable ? 1 : kMaxRasterBuffersPerSegment)) {
      TRACE_EVENT("clay", "RasterOutputSurface::PoolBackpressure");
      return nullptr;
    }
    const uint64_t rgba_bytes =
        ((static_cast<uint64_t>(allocation_size.x) * 4 + 255) / 256) * 256 *
        ((static_cast<uint64_t>(allocation_size.y) + 15) / 16) * 16;
    if (rgba_bytes > 128u * 1024u * 1024u) return nullptr;
    // Account for the color storage, draw-texture intermediate and attachment
    // bytes before allocating. The backend supplies the final physical cost.
    uint64_t bytes = static_cast<uint64_t>(allocation_size.x) *
                     static_cast<uint64_t>(allocation_size.y) * 12;
    auto reservation = budget_->Reserve(bytes);
    if (!reservation) {
      TRACE_EVENT("clay", "RasterOutputSurface::BudgetBackpressure");
      return nullptr;
    }
    auto allocation = context_->owner->CreateDisplayRasterBuffer(
        GetContext(), allocation_size);
    if (!allocation || !allocation->backing) return nullptr;
    if (allocation->allocation_bytes &&
        !reservation->Resize(allocation->allocation_bytes)) {
      return nullptr;
    }
    allocation->reservation = std::move(reservation);
    if (allocation->retain_reservation) {
      auto retain = std::move(allocation->retain_reservation);
      retain(allocation->reservation);
    }
    auto buffer = std::shared_ptr<RasterBuffer>(
        allocation.release(), [context = context_](RasterBuffer* released) {
          fml::TaskRunner::RunNowOrPostTask(
              context->runner, [context, released] {
                auto current = context->MakeCurrent();
                delete released;
              });
        });
    ImageSpec image;
    image.width = allocation_size.x;
    image.height = allocation_size.y;
    image.valid_rect = {0, 0, image.width, image.height};
    image.format = config_.format;
    image.alpha = config_.alpha;
    image.color = config_.color;
    image.origin = buffer->origin;
    record = std::make_shared<ResourceRecord>(
        ResourceKey{config_.device_id, config_.device_generation,
                    AllocateResourceId(), 1},
        image, buffer);
    slots_.push_back({segment_, record, config_, plan_generation_,
                      plan.immutable, plan.whole, std::nullopt,
                      std::make_shared<Output>()});
  } else {
    TRACE_EVENT("clay", "RasterOutputSurface::PoolHit");
  }
  auto buffer = std::static_pointer_cast<RasterBuffer>(record->storage());
  submission->context = context_;
  submission->renderer = context_->renderer;
  submission->surface = context_->surface;
  submission->sink = context_->sink;
  submission->record = record;
  const auto& image = record->image();
  submission->crop =
      RasterBufferValidRect({image.width, image.height}, logical, image.origin);
  submission->version = ++content_version_;
  if (!submission_dependencies_.empty())
    submission->dependencies = std::make_shared<
        const std::vector<std::shared_ptr<const GPUCompletion>>>(
        std::move(submission_dependencies_));
  submission_dependencies_.clear();
  auto sink = context_->sink;
  sink->SetSubmission(submission);
  if (!std::get<0>(sink->AcquireBack(allocation_size))) return nullptr;
  // An abandoned or failed write must never retain the old content history.
  std::weak_ptr<Output> cache;
  for (auto& slot : slots_) {
    if (slot.record == record) {
      slot.damage.reset();
      cache = slot.cache;
    }
  }
  if (auto* scope = StagedImageReadScope::Current()) scope->Track(record);
  if (result && compositor) {
    submission->composition =
        compositor->Prepare(*buffer, *result, *context_->owner);
    submission->native = bool(submission->composition.encode);
  }
  std::shared_ptr<SurfaceFrame> delegated;
  if (!submission->native) {
    delegated = context_->surface->AcquireFrame(allocation_size);
    if (delegated) buffer->surface = delegated->GetSurface();
  }
  // The same delegate that acquired the write also releases its FBO/texture.
  // Late frame callbacks own the lease and context, never the pool object.
  auto abandon = [context = context_, sink, buffer, submission, delegated] {
    if (submission->ended) return;
    submission->discard = true;
    submission->composition = {};
    auto current = submission->surface->MakeRenderContextCurrent();
    if (current && current->GetResult()) {
      if (delegated && !submission->attempted) {
        submission->attempted = true;
        delegated->Submit();
      } else if (submission->native && !submission->attempted) {
        submission->attempted = true;
        buffer->representation->EndWrite();
        sink->SwapBack(buffer->representation->ProduceFence());
      }
      if (!submission->ended) {
        sink->SwapBack(buffer->backing->GetFenceSync());
        if (context->surface == submission->surface)
          context->RecreateProducer();
      }
    } else {
      // No GL/Metal commands may be issued without its producer context.
      submission->record->EndWrite(nullptr);
      submission->record->Retire();
      submission->ended = true;
      sink->SetSubmission(nullptr);
      if (context->surface == submission->surface) context->RecreateProducer();
    }
  };
  if (!submission->native && !delegated) {
    abandon();
    return nullptr;
  }
  auto guard =
      std::shared_ptr<void>(record.get(), [abandon](void*) { abandon(); });
  SurfaceFrame::FramebufferInfo info;
  info.supports_readback = true;
  info.supports_partial_repaint = plan.whole;
  info.existing_damage = existing_damage;
  auto frame = std::make_unique<SurfaceFrame>(
      submission->native ? nullptr : buffer->surface, info,
      [context = context_, sink, buffer, delegated, submission, guard](
          SurfaceFrame& frame, GrCanvas*) {
        TRACE_EVENT("clay", "RasterOutputSurface::Encode");
        if (submission->attempted) return false;
        submission->attempted = true;
        bool encoded;
        if (submission->native) {
          auto result = submission->composition.encode();
          encoded = result.success && result.completion;
          submission->completion = std::move(result.completion);
          encoded = buffer->representation->EndWrite() && encoded;
          submission->discard = !encoded;
          sink->SwapBack(nullptr);
        } else {
          delegated->set_submit_info(frame.submit_info());
          // Android presents during Encode; iOS wraps its own Submit there.
          // Submit exactly once here so both reach their existing SwapBack.
          encoded = delegated->Submit();
          if (!encoded) {
            submission->discard = true;
            submission->output.reset();
            submission->record->Retire();
            if (!submission->ended) {
              sink->SwapBack(buffer->backing->GetFenceSync());
              if (context->surface == submission->surface)
                context->RecreateProducer();
            }
          }
        }
        return encoded && submission->ended && submission->output;
      },
      [context = context_, output = std::weak_ptr<Output>(output_), cache,
       submission, immutable = plan.immutable](const auto&) {
        auto published = output.lock();
        if (!published || submission->discard || !submission->output)
          return false;
        published->texture = submission->output;
        if (immutable) {
          if (auto retained = cache.lock())
            retained->texture = submission->output;
          SealImmutableBuffer(context, submission->record);
        }
        return true;
      },
      size, std::move(current), delegated ? delegated->GetCanvas() : nullptr);
  if (delegated) {
    frame->SetPreparedCallback([delegated](std::optional<skity::Rect> damage) {
      delegated->Prepare(damage);
    });
    if (!plan.whole && frame->GetCanvas())
      frame->GetCanvas()->Clear(skity::Color_TRANSPARENT);
  }
  return frame;
}

ScreenshotData RasterOutputSurface::ScreenshotFrame(
    std::shared_ptr<const CompositorFrame> frame, uint32_t background) {
  TRACE_EVENT("clay", "RasterOutputSurface::ScreenshotFrame");
  FML_DCHECK(context_->runner->RunsTasksOnCurrentThread());
  if (!frame || frame->result->config.device_id != config_.device_id ||
      frame->result->config.device_generation != config_.device_generation)
    return {};
  auto current = MakeRenderContextCurrent();
  if (!current || !current->GetResult()) return {};
  for (const auto& entry : frame->result->outputs) {
    if (!entry.texture_ref || !entry.texture_ref->ready() ||
        !entry.texture_ref->ready()->EnqueueWait())
      return {};
  }
  const auto& config = frame->result->config;
  auto reservation =
      budget_->Reserve(static_cast<uint64_t>(config.width) * config.height * 8);
  if (!reservation) return {};
  struct Readback : std::enable_shared_from_this<Readback> {
    fml::RefPtr<fml::TaskRunner> runner;
    std::function<void(std::function<void()>)> context;
    std::shared_ptr<const CompositorFrame> frame;
    std::vector<std::pair<std::shared_ptr<ResourceRecord>, uint64_t>> uses;
    std::shared_ptr<const GPUCompletion> done;
    std::shared_ptr<void> reservation;
    std::vector<std::shared_ptr<SkityImage>> imports;
    std::shared_ptr<skity::Image> snapshot;
    void Poll() {
      if (!done || !done->IsSafeToRelease()) {
        runner->PostDelayedTask([self = shared_from_this()] { self->Poll(); },
                                fml::TimeDelta::FromMilliseconds(8));
        return;
      }
      context([&] {
        imports.clear();
        snapshot.reset();
        uses.clear();
        frame.reset();
        reservation.reset();
      });
    }
  };
  auto readback = std::make_shared<Readback>();
  readback->runner = context_->runner;
  readback->context = ContextTask();
  readback->frame = frame;
  readback->reservation = std::move(reservation);
  for (const auto& entry : frame->result->outputs) {
    auto repr = GetSharedImageBacking(*entry.texture_ref)
                    ->CreateSkityRepresentation(GetContext());
    auto image = repr ? repr->GetSkityImage() : nullptr;
    if (!image) return {};
    readback->imports.push_back(std::move(image));
  }
  skity::GPURenderTargetDescriptor desc;
  desc.width = config.width;
  desc.height = config.height;
  desc.sample_count = 1;
  auto target = GetContext()->CreateRenderTarget(desc);
  if (!target) return {};
  for (const auto& entry : frame->result->outputs) {
    auto record = entry.texture_ref->resource();
    auto id = record->BeginRead();
    if (!id) {
      for (auto& use : readback->uses) use.first->CancelRead(use.second);
      return {};
    }
    readback->uses.emplace_back(record, id);
  }
  auto* canvas = target->GetCanvas();
  DrawDisplayInputs(*canvas, frame->result->outputs, readback->imports,
                    background);
  canvas->Flush();
  readback->snapshot = GetContext()->MakeSnapshot(std::move(target));
  auto pixmap = readback->snapshot
                    ? readback->snapshot->ReadPixels(GetContext())
                    : nullptr;
  readback->done = context_->owner->CreateDisplayCompletion(GetContext());
  for (auto& use : readback->uses)
    use.first->EndRead(use.second, readback->done);
  ScreenshotData result;
  if (pixmap)
    result = {skity::Data::MakeWithCopy(pixmap->Addr(),
                                        pixmap->RowBytes() * pixmap->Height()),
              skity::Vec2(config.width, config.height)};
  readback->Poll();
  return result;
}
std::pair<fml::RefPtr<PaintImage>, bool> RasterOutputSurface::SnapshotTree(
    LayerTree& tree, CompositorContext& compositor) {
  auto size = tree.frame_size();
  if (tree.HasPlatformViewLayer() || !std::isfinite(size.x) ||
      !std::isfinite(size.y) || size.x <= 0 || size.y <= 0 || size.x > 16384 ||
      size.y > 16384)
    return {nullptr, false};
  if (!PrepareUploads(0)) return {nullptr, true};
  auto current = MakeRenderContextCurrent();
  if (!current || !current->GetResult()) return {nullptr, false};
  uint64_t bytes = static_cast<uint64_t>(std::ceil(size.x)) *
                   static_cast<uint64_t>(std::ceil(size.y)) * 8;
  auto reservation = budget_->Reserve(bytes);
  if (!reservation) return {nullptr, true};
  skity::GPURenderTargetDescriptor desc;
  desc.width = static_cast<uint32_t>(std::ceil(size.x));
  desc.height = static_cast<uint32_t>(std::ceil(size.y));
  desc.sample_count = 1;
  auto target = GetContext()->CreateRenderTarget(desc);
  if (!target) return {nullptr, false};
  struct SnapshotStorage {
    std::shared_ptr<void> reservation;
    std::shared_ptr<skity::Image> image;
  };
  auto storage = std::make_shared<SnapshotStorage>();
  storage->reservation = std::move(reservation);
  // The snapshot is a separate consumer of external inputs. Keep its budget
  // and GPU image and source accessors until sampling actually finishes,
  // including failed readbacks.
  StagedImageReadScope inputs(context_->runner, ContextTask());
  auto write = std::make_shared<ResourceRecord>(
      ResourceKey{config_.device_id, config_.device_generation,
                  AllocateResourceId(), 1},
      ImageSpec{}, storage);
  write->BeginWrite();
  inputs.Track(write);
  auto* canvas = target->GetCanvas();
  auto frame = compositor.AcquireFrame(GetContext(), canvas, nullptr,
                                       skity::Matrix{}, false, true);
  canvas->Clear(skity::Color_TRANSPARENT);
  auto status = frame->Raster(tree, true, nullptr);
  canvas->Flush();
  frame.reset();
  storage->image = GetContext()->MakeSnapshot(std::move(target));
  auto pixmap =
      storage->image ? storage->image->ReadPixels(GetContext()) : nullptr;
  write->EndWrite(context_->owner->CreateDisplayCompletion(GetContext()));
  if (inputs.blocked()) return {nullptr, true};
  return {pixmap && status == RasterStatus::kSuccess
              ? PaintImageSkity::Make(skity::Image::MakeImage(pixmap))
              : nullptr,
          false};
}
void RasterOutputSurface::TrimUnusedBuffers() {
  FML_DCHECK(context_->runner->RunsTasksOnCurrentThread());
  for (auto& slot : slots_) {
    slot.cache->texture.reset();
    if (slot.immutable) slot.generation = 0;
  }
  slots_.erase(
      std::remove_if(slots_.begin(), slots_.end(),
                     [](auto& slot) { return slot.record->CanDestroy(); }),
      slots_.end());
}
void RasterOutputSurface::RetireAll(std::function<void()> drained) {
  FML_DCHECK(context_->runner->RunsTasksOnCurrentThread());
  output_->texture.reset();
  std::vector<std::shared_ptr<ResourceRecord>> records;
  records.reserve(slots_.size());
  for (auto& slot : slots_) {
    records.push_back(std::move(slot.record));
  }
  slots_.clear();
  ResourceRetirementQueue::Retire(context_->runner, std::move(records),
                                  std::move(drained));
}
skity::Matrix RasterOutputSurface::GetRootTransformation() const { return {}; }
GrContext* RasterOutputSurface::GetContext() { return context_->gpu.get(); }
GrContextPtr RasterOutputSurface::GetContextPtr() { return context_->gpu; }
std::unique_ptr<GLContextResult>
RasterOutputSurface::MakeRenderContextCurrent() {
  TRACE_EVENT("clay", "RasterOutputSurface::MakeCurrent");
  return context_->surface ? context_->surface->MakeRenderContextCurrent()
                           : nullptr;
}
bool RasterOutputSurface::ClearRenderContext() {
  return context_->surface && context_->surface->ClearRenderContext();
}
}  // namespace clay::display
