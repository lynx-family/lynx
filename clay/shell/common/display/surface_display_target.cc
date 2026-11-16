// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display/surface_display_target.h"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/gfx/skity/skity_image.h"
#include "clay/shell/common/display/output_mailbox.h"
#include "clay/shell/common/display/raster_output_surface.h"
#include "clay/shell/common/display/texture_compositor.h"
#include "clay/shell/common/output_surface.h"

namespace clay::display {
namespace {
class UnresolvedSubmission final : public GPUCompletion {
 public:
  CompletionStatus Poll() const override { return CompletionStatus::kFailed; }
};
class SurfaceDisplayTarget final : public DisplayTarget {
  struct ReadView {
    std::weak_ptr<ResourceRecord> record;
    const Surface* consumer;
    // Native imports can outlive the buffer while their consumer is draining.
    std::shared_ptr<void> reservation;
    fml::RefPtr<SkityImageRepresentation> representation;
    std::shared_ptr<SkityImage> image;
  };

 public:
  SurfaceDisplayTarget(fml::RefPtr<OutputSurface> output,
                       std::shared_ptr<TextureBudget> budget,
                       fml::RefPtr<fml::TaskRunner> runner,
                       std::shared_ptr<OutputMailbox> mailbox,
                       std::function<void()> available)
      : output_(std::move(output)),
        budget_(std::move(budget)),
        runner_(std::move(runner)),
        surface_(output_->CreateGPUSurface()),
        mailbox_(std::move(mailbox)),
        output_available_(std::move(available)) {
    if (IsValid())
      texture_compositor_ = CreateTextureCompositor(surface_->GetContextPtr());
    if (mailbox_) generation_ = mailbox_->Open();
  }
  ~SurfaceDisplayTarget() override { Detach(); }
  bool IsValid() const { return surface_ && surface_->IsValid(); }
  void Detach() override {
    if (detached_) return;
    detached_ = true;
    if (surface_) surface_->DetachPresentation();
    if (mailbox_) {
      mailbox_->Close(generation_);
      if (output_available_) output_available_();
    }
    auto current = surface_ ? surface_->MakeRenderContextCurrent() : nullptr;
    texture_compositor_.reset();
    for (auto& cached : read_views_) {
      if (auto view = cached.lock()) {
        if (auto record = view->record.lock()) {
          auto buffer =
              std::static_pointer_cast<RasterBuffer>(record->storage());
          if (buffer->consumer_read_view == view)
            buffer->consumer_read_view.reset();
        }
      }
    }
    read_views_.clear();
    pool_.reset();
  }
  DisplaySubmission Present(const CompositorFrame& frame) override {
    TRACE_EVENT("clay", "SurfaceDisplayTarget::Present");
    if (detached_ || !IsValid()) return {};
    const auto& result = *frame.result;
    const bool texture_output =
        !mailbox_ && surface_->SupportsTexturePresentation();
    auto current = surface_->MakeRenderContextCurrent();
    if (!current || !current->GetResult()) return {};
    read_views_.erase(
        std::remove_if(read_views_.begin(), read_views_.end(),
                       [](const auto& view) { return view.expired(); }),
        read_views_.end());
    if (texture_output && CanForwardWholeOutput(result)) {
      auto output = result.outputs.front().texture_ref;
      if (auto completion = surface_->PresentSharedImage(output)) {
        bool presented = completion->Poll() != CompletionStatus::kFailed;
        return {std::move(completion),
                surface_,
                presented,
                false,
                std::move(output),
                DisplaySubmission::OutputMode::kForwarded};
      }
    }
    auto window_reservation =
        !mailbox_ && !texture_output && result.config.uses_window_attachments
            ? budget_->Reserve(static_cast<uint64_t>(result.config.width) *
                               result.config.height * 8)
            : nullptr;
    if (!mailbox_ && !texture_output && result.config.uses_window_attachments &&
        !window_reservation)
      return {nullptr, nullptr, false, true};
    if ((mailbox_ || texture_output) && !pool_)
      pool_ = std::make_shared<RasterOutputSurface>(output_, runner_, budget_,
                                                    result.config, surface_);
    std::vector<std::shared_ptr<const GPUCompletion>> dependencies;
    dependencies.reserve(result.outputs.size());
    for (const auto& entry : result.outputs) {
      if (!entry.texture_ref || !entry.texture_ref->ready() ||
          !entry.texture_ref->ready()->EnqueueWait())
        return {};
      dependencies.push_back(entry.texture_ref->ready());
    }
    if (pool_) {
      pool_->SetConfig(result.config);
      pool_->SetSubmissionDependencies(dependencies);
    }
    auto size_generation = mailbox_ ? mailbox_->size_generation() : 0;
    std::shared_ptr<SurfaceFrame> drawing =
        pool_
            ? pool_->AcquireCompositionFrame(result, texture_compositor_.get())
            : surface_->AcquireFrame(
                  {result.config.width, result.config.height});
    if (!drawing) return {nullptr, nullptr, false, true};
    struct Resources {
      std::shared_ptr<TextureBudget::Reservation> window_reservation;
      fml::RefPtr<OutputSurface> output;
      std::shared_ptr<Surface> surface;
      std::shared_ptr<RasterOutputSurface> pool;
      std::shared_ptr<SurfaceFrame> drawing;
      std::vector<std::shared_ptr<SkityImage>> images;
      std::vector<std::shared_ptr<ReadView>> reads;
      std::vector<std::shared_ptr<void>> input_storage;
      std::vector<std::function<void()>> read_finished;
      std::shared_ptr<const GPUCompletion> completion;
      bool submitted_reads = false;
      ~Resources() {
        // In-flight resources drain only after the existing submission proves
        // safety. Before drawing, an abandoned import has no GPU reader.
        if (!submitted_reads || (completion && completion->IsSafeToRelease()))
          for (auto& finished : read_finished) finished();
      }
    };
    auto resources = std::shared_ptr<Resources>(
        new Resources{std::move(window_reservation),
                      output_,
                      surface_,
                      pool_,
                      std::move(drawing),
                      {},
                      {},
                      {},
                      {},
                      nullptr,
                      false},
        [runner = runner_](Resources* resources) {
          fml::TaskRunner::RunNowOrPostTask(runner, [resources] {
            auto current = resources->surface->MakeRenderContextCurrent();
            delete resources;
          });
        });
    for (const auto& entry : result.outputs) {
      resources->input_storage.push_back(
          entry.texture_ref->resource()->storage());
    }
    resources->drawing->Prepare(std::nullopt);
    const bool native = pool_ && pool_->has_native_composition();
    if (!native) {
      for (const auto& entry : result.outputs) {
        auto view = GetReadView(entry.texture_ref->resource());
        if (!view) return {};
        if (auto finished = view->representation->TrackRead(true))
          resources->read_finished.push_back(std::move(finished));
        resources->images.push_back(view->image);
        resources->reads.push_back(std::move(view));
      }
      auto* canvas = resources->drawing->GetCanvas();
      if (!canvas) return {};
      DrawDisplayInputs(*canvas, result.outputs, resources->images);
    }
    resources->submitted_reads = true;
    bool presented = resources->drawing->Submit();
    auto output = pool_ ? pool_->TakeCompleted() : nullptr;
    auto completion = output  ? output->ready()
                      : pool_ ? pool_->submission_completion()
                              : nullptr;
    if (!pool_) {
      completion = output_->CreateDisplayCompletion(surface_->GetContext());
      completion = DependentGPUCompletion::Wrap(
          std::move(completion),
          std::make_shared<
              const std::vector<std::shared_ptr<const GPUCompletion>>>(
              std::move(dependencies)));
    }
    // Drawing may already have submitted reads even if presentation failed.
    // An absent fence cannot cancel those reads or authorize pool reuse.
    if (!completion) completion = std::make_shared<UnresolvedSubmission>();
    resources->completion = completion;
    if (output && presented && mailbox_) {
      presented = mailbox_->Publish(generation_, output, size_generation);
      if (presented && output_available_) output_available_();
    } else if (output && presented && texture_output) {
      auto read = output->resource()->BeginRead();
      auto presentation = read ? surface_->PresentSharedImage(output) : nullptr;
      presented = presentation != nullptr;
      if (presentation) {
        output->resource()->EndRead(read, presentation);
        completion = std::move(presentation);
        presented = completion->Poll() != CompletionStatus::kFailed;
      } else if (read) {
        output->resource()->CancelRead(read);
      }
    }
    return {std::move(completion),
            resources,
            presented,
            false,
            std::move(output),
            pool_ ? DisplaySubmission::OutputMode::kComposed
                  : DisplaySubmission::OutputMode::kNativeWindow};
  }

 private:
  std::shared_ptr<ReadView> GetReadView(
      const std::shared_ptr<ResourceRecord>& record) {
    auto buffer = std::static_pointer_cast<RasterBuffer>(record->storage());
    auto view = std::static_pointer_cast<ReadView>(buffer->consumer_read_view);
    if (view && view->consumer == surface_.get()) return view;
    auto representation =
        buffer->backing->CreateSkityRepresentation(surface_->GetContext());
    auto image = representation ? representation->GetSkityImageForTrackedRead()
                                : nullptr;
    if (!image) return nullptr;
    view = std::shared_ptr<ReadView>(
        new ReadView{record, surface_.get(), buffer->reservation,
                     std::move(representation), std::move(image)},
        [runner = runner_, surface = surface_,
         output = output_](ReadView* view) {
          fml::TaskRunner::RunNowOrPostTask(runner, [surface, output, view] {
            auto current = surface->MakeRenderContextCurrent();
            delete view;
          });
        });
    buffer->consumer_read_view = view;
    read_views_.push_back(view);
    return view;
  }
  fml::RefPtr<OutputSurface> output_;
  std::shared_ptr<TextureBudget> budget_;
  fml::RefPtr<fml::TaskRunner> runner_;
  std::shared_ptr<Surface> surface_;
  std::unique_ptr<TextureCompositor> texture_compositor_;
  std::shared_ptr<OutputMailbox> mailbox_;
  std::function<void()> output_available_;
  std::shared_ptr<RasterOutputSurface> pool_;
  std::vector<std::weak_ptr<ReadView>> read_views_;
  uint64_t generation_ = 0;
  bool detached_ = false;
};
}  // namespace
std::unique_ptr<DisplayTarget> CreateSurfaceDisplayTarget(
    fml::RefPtr<OutputSurface> output, std::shared_ptr<TextureBudget> budget,
    fml::RefPtr<fml::TaskRunner> runner, std::shared_ptr<OutputMailbox> mailbox,
    std::function<void()> available) {
  if (!output || !output->IsValid()) return nullptr;
  auto target = std::make_unique<SurfaceDisplayTarget>(
      std::move(output), std::move(budget), std::move(runner),
      std::move(mailbox), std::move(available));
  return target->IsValid() ? std::move(target) : nullptr;
}
}  // namespace clay::display
