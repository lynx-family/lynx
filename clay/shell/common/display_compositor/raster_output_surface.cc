// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor/raster_output_surface.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/staged_image_read_scope.h"
#include "clay/fml/logging.h"
#include "clay/gfx/paint_image_skity.h"
#include "clay/gfx/skity/skity_image.h"
#include "clay/shell/common/output_surface.h"
#include "clay/shell/common/screenshot_utils.h"

namespace clay::display {
namespace {
bool MatchesConfig(const ResourceRecord& record, const OutputConfig& config) {
  const auto& key = record.key();
  const auto& image = record.image();
  return key.device_id == config.device_id &&
         key.device_generation == config.device_generation &&
         image.format == config.format && image.alpha == config.alpha &&
         image.color == config.color;
}
// No page/binding or mutable LayerTree is retained by the retirement queue.
class ResourceRetirementQueue
    : public std::enable_shared_from_this<ResourceRetirementQueue> {
 public:
  fml::RefPtr<fml::TaskRunner> runner;
  std::vector<std::shared_ptr<ResourceRecord>> records;
  std::function<void()> drained;
  void Sweep() {
    FML_DCHECK(runner->RunsTasksOnCurrentThread());
    records.erase(std::remove_if(records.begin(), records.end(),
                                 [](auto& r) { return r->CanDestroy(); }),
                  records.end());
    if (records.empty()) {
      if (drained) drained();
      return;
    }
    runner->PostDelayedTask([self = shared_from_this()] { self->Sweep(); },
                            fml::TimeDelta::FromMilliseconds(250));
  }
};
}  // namespace
struct RasterOutputSurface::Context {
  fml::RefPtr<OutputSurface> owner;
  fml::RefPtr<fml::TaskRunner> runner;
  std::unique_ptr<Surface> surface;
};
RasterOutputSurface::RasterOutputSurface(fml::RefPtr<OutputSurface> owner,
                                         fml::RefPtr<fml::TaskRunner> runner,
                                         std::shared_ptr<TextureBudget> budget,
                                         OutputConfig config)
    : context_(std::make_shared<Context>()),
      budget_(std::move(budget)),
      config_(config) {
  context_->owner = std::move(owner);
  context_->runner = std::move(runner);
  context_->surface = context_->owner->CreateStagedProductionSurface();
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
    auto current = context->surface->MakeRenderContextCurrent();
    task();
  };
}
bool RasterOutputSurface::PrepareUploads(uint64_t submission) {
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
std::unique_ptr<SurfaceFrame> RasterOutputSurface::AcquireFrame(
    const skity::Vec2& size) {
  TRACE_EVENT("clay", "RasterOutputSurface::AcquireFrame");
  FML_DCHECK(context_->runner->RunsTasksOnCurrentThread());
  completed_.reset();
  if (!IsValid() || !config_.IsValid() || !std::isfinite(size.x) ||
      !std::isfinite(size.y) || size.x <= 0 || size.y <= 0 || size.x > 16384 ||
      size.y > 16384 || std::floor(size.x) != size.x ||
      std::floor(size.y) != size.y)
    return nullptr;
  auto current = MakeRenderContextCurrent();
  if (!current || !current->GetResult()) return nullptr;
  {
    TRACE_EVENT("clay", "RasterOutputSurface::ReclaimSlots");
    slots_.erase(
        std::remove_if(
            slots_.begin(), slots_.end(),
            [&](auto& slot) {
              const auto& image = slot.record->image();
              bool obsolete = !MatchesConfig(*slot.record, config_);
              if (slot.segment == segment_) {
                obsolete |= image.width != size.x || image.height != size.y;
              } else if (has_segment_plan_) {
                if (!slot.segment || slot.segment > segment_sizes_.size()) {
                  obsolete = true;
                } else {
                  const auto& target = segment_sizes_[slot.segment - 1];
                  obsolete |=
                      image.width != target.x || image.height != target.y;
                }
              }
              // Different segments normally have different sizes.
              // Reclaim against each segment's own current plan,
              // without retiring an older complete frame still held
              // by Mailbox or Display.
              return slot.record->CanDestroy() &&
                     (obsolete || !slot.record->CanReuse());
            }),
        slots_.end());
  }
  std::shared_ptr<ResourceRecord> record;
  size_t count = 0;
  for (auto& slot : slots_) {
    if (slot.segment != segment_) continue;
    ++count;
    const auto& image = slot.record->image();
    if (!record && image.width == size.x && image.height == size.y &&
        MatchesConfig(*slot.record, config_) && slot.record->CanReuse())
      record = slot.record;
  }
  if (!record) {
    if (count >= kMaxRasterBuffersPerSegment) {
      TRACE_EVENT("clay", "RasterOutputSurface::PoolBackpressure");
      return nullptr;
    }
    TRACE_EVENT("clay", "RasterOutputSurface::AllocateBuffer");
    uint64_t bytes = ((static_cast<uint64_t>(size.x) * 4 + 255) / 256) * 256 *
                     ((static_cast<uint64_t>(size.y) + 15) / 16) * 16;
    if (bytes > 128u * 1024u * 1024u) return nullptr;
    auto reservation = budget_->Reserve(bytes);
    if (!reservation) {
      TRACE_EVENT("clay", "RasterOutputSurface::BudgetBackpressure");
      return nullptr;
    }
    auto allocation =
        context_->owner->CreateDisplayRasterBuffer(GetContext(), size);
    if (!allocation || !allocation->surface || !allocation->backing)
      return nullptr;
    if (allocation->allocation_bytes &&
        !reservation->Resize(allocation->allocation_bytes)) {
      return nullptr;
    }
    bytes = allocation->allocation_bytes ? allocation->allocation_bytes : bytes;
    allocation->reservation = std::move(reservation);
    auto buffer = std::shared_ptr<RasterBuffer>(
        allocation.release(), [context = context_](RasterBuffer* released) {
          fml::TaskRunner::RunNowOrPostTask(
              context->runner, [context, released] {
                TRACE_EVENT("clay", "RasterOutputSurface::ReleaseBuffer");
                auto current = context->surface->MakeRenderContextCurrent();
                delete released;
              });
        });
    ImageSpec image;
    image.width = size.x;
    image.height = size.y;
    image.valid_rect = {0, 0, image.width, image.height};
    image.format = config_.format;
    image.alpha = config_.alpha;
    image.color = config_.color;
    image.origin = ImageOrigin::kBottomLeft;
    record = std::make_shared<ResourceRecord>(
        ResourceKey{config_.device_id, config_.device_generation,
                    AllocateResourceId(), 1},
        image, bytes, buffer);
    slots_.push_back({segment_, record});
  }
  if (!record->BeginWrite()) {
    return nullptr;
  }
  if (auto* scope = StagedImageReadScope::Current()) scope->Track(record);
  auto buffer = std::static_pointer_cast<RasterBuffer>(record->storage());
  // SurfaceFrame may be abandoned after recording but before Encode. Seal that
  // write as well: the external inputs and allocation still have GPU users.
  auto ended = std::make_shared<bool>(false);
  auto canvas_slot = std::make_shared<GrCanvas*>(nullptr);
  auto guard = std::shared_ptr<void>(
      record.get(),
      [context = context_, buffer, record, ended, canvas_slot](void*) {
        if (*ended) return;
        auto current = context->surface->MakeRenderContextCurrent();
        if (current && current->GetResult()) {
          if (*canvas_slot) (*canvas_slot)->Flush();
          buffer->surface->Flush();
          buffer->representation->EndWrite();
          record->EndWrite(context->owner->CreateDisplayCompletion(
              context->surface->GetContext()));
        } else {
          record->EndWrite(nullptr);
        }
        record->Retire();
      });
  SurfaceFrame::FramebufferInfo info;
  info.supports_readback = true;
  auto frame = std::make_unique<SurfaceFrame>(
      buffer->surface, info,
      [context = context_, buffer, record, guard, ended](SurfaceFrame&,
                                                         GrCanvas* canvas) {
        TRACE_EVENT("clay", "RasterOutputSurface::Encode");
        if (!canvas) {
          record->EndWrite(nullptr);
          return false;
        }
        canvas->Flush();
        buffer->surface->Flush();
        buffer->representation->EndWrite();
        auto ready = context->owner->CreateDisplayCompletion(
            context->surface->GetContext());
        record->EndWrite(ready);
        *ended = true;
        return ready != nullptr;
      },
      [this, record](const SurfaceFrame::SubmitInfo&) {
        completed_ =
            std::make_shared<const OutputTexture>(record, ++content_version_);
        return true;
      },
      size, std::move(current));
  *canvas_slot = frame->GetCanvas();
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
  canvas->Clear(background);
  for (size_t i = 0; i < frame->result->outputs.size(); ++i) {
    const auto& entry = frame->result->outputs[i];
    canvas->Save();
    canvas->Translate(entry.properties.x, entry.properties.y);
    if (entry.texture_ref->image().origin == ImageOrigin::kBottomLeft) {
      canvas->Translate(0, entry.texture_ref->image().height);
      canvas->Scale(1, -1);
    }
    const auto& rect = entry.texture_ref->image().valid_rect;
    canvas->ClipRect(
        skity::Rect::MakeXYWH(rect.x, rect.y, rect.width, rect.height));
    canvas->DrawImage(readback->imports[i]->gr_image(), 0, 0);
    canvas->Restore();
  }
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
      ImageSpec{}, bytes, storage);
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
  slots_.erase(
      std::remove_if(slots_.begin(), slots_.end(),
                     [](auto& slot) { return slot.record->CanDestroy(); }),
      slots_.end());
}
void RasterOutputSurface::RetireAll(std::function<void()> drained) {
  FML_DCHECK(context_->runner->RunsTasksOnCurrentThread());
  completed_.reset();
  auto queue = std::make_shared<ResourceRetirementQueue>();
  queue->runner = context_->runner;
  queue->drained = std::move(drained);
  for (auto& slot : slots_) {
    slot.record->Retire();
    queue->records.push_back(std::move(slot.record));
  }
  slots_.clear();
  queue->Sweep();
}
skity::Matrix RasterOutputSurface::GetRootTransformation() const { return {}; }
GrContext* RasterOutputSurface::GetContext() {
  return context_->surface->GetContext();
}
GrContextPtr RasterOutputSurface::GetContextPtr() {
  return context_->surface->GetContextPtr();
}
std::unique_ptr<GLContextResult>
RasterOutputSurface::MakeRenderContextCurrent() {
  return context_->surface ? context_->surface->MakeRenderContextCurrent()
                           : nullptr;
}
bool RasterOutputSurface::ClearRenderContext() {
  return context_->surface && context_->surface->ClearRenderContext();
}
}  // namespace clay::display
