// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display/display_compositor.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/gfx/skity/skity_image.h"
#include "skity/render/canvas.hpp"

namespace clay::display {
void DrawDisplayInputs(skity::Canvas& canvas,
                       const std::vector<TextureEntry>& outputs,
                       const std::vector<std::shared_ptr<SkityImage>>& images,
                       uint32_t background) {
  FML_DCHECK(outputs.size() == images.size());
  canvas.Clear(background);
  for (size_t i = 0; i < outputs.size(); ++i) {
    const auto& entry = outputs[i];
    canvas.Save();
    canvas.Translate(entry.properties.x, entry.properties.y);
    if (entry.texture_ref->image().origin == ImageOrigin::kBottomLeft) {
      canvas.Translate(0, entry.texture_ref->image().height);
      canvas.Scale(1, -1);
    }
    const auto& rect = entry.texture_ref->image().valid_rect;
    canvas.ClipRect(
        skity::Rect::MakeXYWH(rect.x, rect.y, rect.width, rect.height));
    canvas.DrawImage(images[i]->gr_image(), 0, 0);
    canvas.Restore();
  }
}
bool CanForwardWholeOutput(const RasterResult& result) {
  if (!result.config.IsValid() || result.outputs.size() != 1) return false;
  const auto& entry = result.outputs.front();
  if (!entry.texture_ref) return false;
  const auto& image = entry.texture_ref->image();
  const auto& key = entry.texture_ref->resource()->key();
  const auto& config = result.config;
  return entry.properties.x == 0 && entry.properties.y == 0 &&
         image.width == config.width && image.height == config.height &&
         image.format == config.format && image.alpha == config.alpha &&
         image.color == config.color &&
         image.valid_rect == PixelRect{0, 0, config.width, config.height} &&
         key.device_id == config.device_id &&
         key.device_generation == config.device_generation;
}
DisplayCompositor::DisplayCompositor(
    fml::RefPtr<fml::TaskRunner> runner, std::unique_ptr<DisplayTarget> target,
    WindowKey window, std::function<void()> request, Presented presented,
    std::function<void(PipelineError)> error, Snapshot snapshot)
    : runner_(std::move(runner)),
      target_(std::move(target)),
      window_(window),
      request_display_(std::move(request)),
      presented_(std::move(presented)),
      error_(std::move(error)),
      snapshot_(std::move(snapshot)) {}
DisplayCompositor::~DisplayCompositor() {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  FML_DCHECK(in_flight_.empty());
}
void DisplayCompositor::Accept(std::shared_ptr<const CompositorFrame> frame) {
  TRACE_EVENT("clay", "DisplayCompositor::Accept");
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (stopped_ || !frame || frame->window != window_) return;
  if (!candidate_) candidate_since_ = fml::TimePoint::Now();
  candidate_ = std::move(frame);
  if (enabled_) request_display_();
}
void DisplayCompositor::InvalidateCandidate() { candidate_.reset(); }
void DisplayCompositor::SetEnabled(bool enabled) {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (enabled && !enabled_) {
    candidate_since_ = fml::TimePoint::Now();
    for (auto& flight : in_flight_) flight.submitted = candidate_since_;
  }
  enabled_ = enabled;
  if (enabled_ && candidate_ && !stopped_) request_display_();
}
void DisplayCompositor::Stop() {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (stopped_) return;
  stopped_ = true;
  candidate_.reset();
  SetCurrentFrame(nullptr);
  last_source_frame_.reset();
  presented_ = nullptr;
  request_display_ = nullptr;
  error_ = nullptr;
  target_->Detach();
  PollRetirement();
}
void DisplayCompositor::SetCurrentFrame(
    std::shared_ptr<const CompositorFrame> frame) {
  current_ = std::move(frame);
  if (snapshot_) std::atomic_store(snapshot_.get(), current_);
}
bool DisplayCompositor::Compose(fml::TimePoint deadline) {
  TRACE_EVENT("clay", "DisplayCompositor::Compose");
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (stopped_ || faulted_ || !enabled_ || !candidate_) return false;
  retry_deadline_ = deadline;
  if (fml::TimePoint::Now() > deadline ||
      in_flight_.size() >= kMaxDisplayFramesInFlight)
    return true;
  if ((fml::TimePoint::Now() - candidate_since_).ToMilliseconds() > 2000) {
    Fail(PipelineError::kGpuTimeout);
    return false;
  }
  std::vector<std::pair<std::shared_ptr<ResourceRecord>, uint64_t>> uses;
  for (const auto& entry : candidate_->result->outputs) {
    auto record = entry.texture_ref->resource();
    auto id = record->BeginRead();
    if (!id) {
      for (auto& [record, id] : uses) record->CancelRead(id);
      candidate_.reset();
      return false;
    }
    uses.push_back({record, id});
  }
  auto frame = std::move(candidate_);
  TRACE_EVENT("clay", "DisplayCompositor::Submit", "frame_id", frame->frame_id,
              "render_id", frame->result->render_version);
  auto submission = target_->Present(*frame);
  if (!submission.completion) {
    for (auto& [record, id] : uses) record->CancelRead(id);
    if (submission.retry) {
      candidate_ = std::move(frame);
      // The host may release an output just after this attempt. Request another
      // opportunity within this cycle instead of always losing a whole VSync.
      // The delayed retry only requests; the frame clock still enforces the
      // deadline and one successful submission per cycle.
      ScheduleRetry(deadline);
    } else {
      Fail(PipelineError::kDisplayFailure);
    }
    return candidate_ != nullptr;
  }
  for (auto& [record, id] : uses) record->EndRead(id, submission.completion);
  const auto& output = submission.output;
  bool valid_ownership = false;
  if (output) {
    switch (submission.output_mode) {
      case DisplaySubmission::OutputMode::kComposed:
        valid_ownership = std::none_of(
            frame->result->outputs.begin(), frame->result->outputs.end(),
            [&](const auto& entry) {
              return entry.texture_ref->resource() == output->resource();
            });
        break;
      case DisplaySubmission::OutputMode::kForwarded:
        valid_ownership = CanForwardWholeOutput(*frame->result) &&
                          output == frame->result->outputs.front().texture_ref;
        break;
      case DisplaySubmission::OutputMode::kNativeWindow:
        break;
    }
  }
  const bool window =
      submission.output_mode == DisplaySubmission::OutputMode::kNativeWindow;
  bool submitted = submission.presented &&
                   ((window && !output) ||
                    (output && output->ready() && valid_ownership &&
                     output->resource()->key().device_id ==
                         frame->result->config.device_id &&
                     output->resource()->key().device_generation ==
                         frame->result->config.device_generation &&
                     output->image().width == frame->result->config.width &&
                     output->image().height == frame->result->config.height &&
                     output->image().format == frame->result->config.format &&
                     output->image().alpha == frame->result->config.alpha &&
                     output->image().color == frame->result->config.color));
  if (submitted) last_source_frame_ = frame;
  if (submitted && window) {
    // The window owns presentation storage. Keep its source snapshot available
    // for explicit screenshots and reattachment without an extra composition.
    SetCurrentFrame(frame);
    current_output_mode_ = submission.output_mode;
  } else if (submitted) {
    // Forwarding keeps the original snapshot's CPU lease, so Raster cannot
    // overwrite the current frame even after native GPU reads have completed.
    auto result = std::make_shared<RasterResult>(*frame->result);
    result->outputs.clear();
    result->outputs.push_back({1, output->content_version(), output, {}});
    SetCurrentFrame(std::make_shared<const CompositorFrame>(
        CompositorFrame{frame->frame_id, frame->window, std::move(result)}));
    current_output_mode_ = submission.output_mode;
  }
  if (!submitted &&
      submission.output_mode == DisplaySubmission::OutputMode::kForwarded)
    RetireSubmission(*frame, submission);
  in_flight_.push_back({frame, std::move(submission), fml::TimePoint::Now()});
  if (submitted) {
    // Submission is not evidence of scanout. Native feedback may be
    // unavailable.
    if (presented_) presented_(*frame);
  }
  if (!submitted) Fail(PipelineError::kDisplayFailure);
  SchedulePoll();
  return false;
}
void DisplayCompositor::RetireSubmission(const CompositorFrame& frame,
                                         const DisplaySubmission& submission) {
  for (const auto& entry : frame.result->outputs) entry.texture_ref->Retire();
  if (submission.output) submission.output->Retire();
  if (last_source_frame_ &&
      std::any_of(last_source_frame_->result->outputs.begin(),
                  last_source_frame_->result->outputs.end(),
                  [](const auto& entry) {
                    return entry.texture_ref->resource()->IsRetired();
                  }))
    last_source_frame_.reset();
  if (!current_) return;
  const bool invalidate_current =
      current_output_mode_ == DisplaySubmission::OutputMode::kForwarded
          ? std::any_of(current_->result->outputs.begin(),
                        current_->result->outputs.end(),
                        [](const auto& entry) {
                          return entry.texture_ref->resource()->IsRetired();
                        })
          : current_->frame_id == frame.frame_id &&
                current_->window == frame.window;
  if (invalidate_current) SetCurrentFrame(nullptr);
}
void DisplayCompositor::Fail(PipelineError error) {
  if (faulted_ || stopped_) return;
  faulted_ = true;
  FML_LOG(ERROR) << "Display pipeline failed, code=" << static_cast<int>(error);
  if (error_) error_(error);
}
void DisplayCompositor::ScheduleRetry(fml::TimePoint deadline) {
  retry_deadline_ = deadline;
  if (retry_pending_ || stopped_ || faulted_ || !enabled_ || !candidate_ ||
      fml::TimePoint::Now() >= deadline)
    return;
  retry_pending_ = true;
  runner_->PostDelayedTask(
      [weak = weak_from_this()] {
        auto self = weak.lock();
        if (!self) return;
        self->retry_pending_ = false;
        if (self->stopped_ || self->faulted_ || !self->enabled_ ||
            !self->candidate_ || fml::TimePoint::Now() >= self->retry_deadline_)
          return;
        // Retry resource acquisition for the latest candidate. Producer GPU
        // dependencies are queued by Present and never polled to permit work.
        self->request_display_();
      },
      fml::TimeDelta::FromMilliseconds(1));
}
void DisplayCompositor::SchedulePoll() {
  if (poll_pending_ || in_flight_.empty()) return;
  poll_pending_ = true;
  runner_->PostDelayedTask(
      [self = shared_from_this()] {
        self->poll_pending_ = false;
        self->PollRetirement();
      },
      fml::TimeDelta::FromMilliseconds(faulted_ || stopped_ ? 250 : 1));
}
void DisplayCompositor::PollRetirement() {
  TRACE_EVENT("clay", "DisplayCompositor::PollRetirement");
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  for (auto it = in_flight_.begin(); it != in_flight_.end();) {
    auto state = it->submission.completion->Poll();
    if (state == CompletionStatus::kPending) {
      if (enabled_ &&
          (fml::TimePoint::Now() - it->submitted).ToMilliseconds() > 2000)
        Fail(PipelineError::kGpuTimeout);
      ++it;
      continue;
    }
    if (state == CompletionStatus::kFailed) {
      Fail(PipelineError::kDisplayFailure);
      RetireSubmission(*it->frame, it->submission);
    }
    if (!it->submission.completion->IsSafeToRelease()) {
      ++it;
      continue;
    }
    // Destroy backend imports before releasing the immutable frame leases.
    it = in_flight_.erase(it);
  }
  SchedulePoll();
  if (!stopped_ && !faulted_ && enabled_ && candidate_) request_display_();
}
}  // namespace clay::display
