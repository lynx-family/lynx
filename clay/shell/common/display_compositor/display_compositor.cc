// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor/display_compositor.h"

#include <algorithm>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"

namespace clay::display {
DisplayCompositor::DisplayCompositor(fml::RefPtr<fml::TaskRunner> runner,
                                     std::unique_ptr<DisplayTarget> target,
                                     WindowKey window,
                                     std::function<void()> request,
                                     Presented presented,
                                     std::function<void(PipelineError)> error)
    : runner_(std::move(runner)),
      target_(std::move(target)),
      window_(window),
      request_display_(std::move(request)),
      presented_(std::move(presented)),
      error_(std::move(error)) {}
DisplayCompositor::~DisplayCompositor() {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  FML_DCHECK(in_flight_.empty());
}
void DisplayCompositor::Accept(std::shared_ptr<const CompositorFrame> frame) {
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
  current_.reset();
  presented_ = nullptr;
  request_display_ = nullptr;
  error_ = nullptr;
  target_->Detach();
  PollRetirement();
}
bool DisplayCompositor::Compose(fml::TimePoint deadline) {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (stopped_ || faulted_ || !enabled_ || !candidate_) return false;
  ready_poll_deadline_ = deadline;
  if (fml::TimePoint::Now() > deadline ||
      in_flight_.size() >= kMaxDisplayFramesInFlight)
    return true;
  if ((fml::TimePoint::Now() - candidate_since_).ToMilliseconds() > 2000) {
    Fail(PipelineError::kGpuTimeout);
    return false;
  }
  for (const auto& entry : candidate_->result->outputs) {
    auto state = entry.texture_ref->ready()->Poll();
    if (state == CompletionStatus::kPending) {
      // Keep current_ and the existing window contents. Readiness only
      // schedules another opportunity; it never blocks this Platform task on
      // production.
      ScheduleReadyPoll(deadline);
      return true;
    }
    if (state == CompletionStatus::kFailed) {
      entry.texture_ref->Retire();
      candidate_.reset();
      Fail(PipelineError::kProducerFailure);
      return false;
    }
  }
  std::vector<Use> uses;
  for (const auto& entry : candidate_->result->outputs) {
    auto record = entry.texture_ref->resource();
    auto id = record->BeginRead();
    if (!id) {
      for (auto& use : uses) use.record->CancelRead(use.id);
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
    for (auto& use : uses) use.record->CancelRead(use.id);
    if (submission.retry) {
      candidate_ = std::move(frame);
      // The host may release an output just after this attempt. Request another
      // opportunity within this cycle instead of always losing a whole VSync.
      // The delayed poll only requests; the frame clock still enforces the
      // deadline and one successful submission per cycle.
      ScheduleReadyPoll(deadline);
    } else {
      Fail(PipelineError::kDisplayFailure);
    }
    return candidate_ != nullptr;
  }
  for (auto& use : uses) use.record->EndRead(use.id, submission.completion);
  const auto& output = submission.output;
  bool submitted =
      submission.presented && output && output->ready() &&
      output->resource()->key().device_id == frame->result->config.device_id &&
      output->resource()->key().device_generation ==
          frame->result->config.device_generation &&
      output->image().width == frame->result->config.width &&
      output->image().height == frame->result->config.height &&
      output->image().format == frame->result->config.format &&
      output->image().alpha == frame->result->config.alpha &&
      output->image().color == frame->result->config.color &&
      std::none_of(frame->result->outputs.begin(), frame->result->outputs.end(),
                   [&](const auto& entry) {
                     return entry.texture_ref->resource() == output->resource();
                   });
  if (submitted) {
    // Keep the composed output for host reuse and explicit screenshots, rather
    // than retaining the Raster inputs after Display has sampled them.
    auto result = std::make_shared<RasterResult>(*frame->result);
    result->outputs.clear();
    result->outputs.push_back(
        {1, output->content_version(), output, {}, std::nullopt});
    current_ = std::make_shared<const CompositorFrame>(
        CompositorFrame{frame->frame_id, frame->window, std::move(result)});
  }
  in_flight_.push_back(
      {frame, std::move(uses), std::move(submission), fml::TimePoint::Now()});
  if (submitted) {
    // Submission is not evidence of scanout. Native feedback may be
    // unavailable.
    if (presented_) presented_(*frame, {});
  }
  if (!submitted) Fail(PipelineError::kDisplayFailure);
  SchedulePoll();
  return false;
}
void DisplayCompositor::Fail(PipelineError error) {
  if (faulted_ || stopped_) return;
  faulted_ = true;
  FML_LOG(ERROR) << "Display pipeline failed, code=" << static_cast<int>(error);
  if (error_) error_(error);
}
void DisplayCompositor::ScheduleReadyPoll(fml::TimePoint deadline) {
  ready_poll_deadline_ = deadline;
  if (ready_poll_pending_ || stopped_ || faulted_ || !enabled_ || !candidate_ ||
      fml::TimePoint::Now() >= deadline)
    return;
  ready_poll_pending_ = true;
  runner_->PostDelayedTask(
      [weak = weak_from_this()] {
        auto self = weak.lock();
        if (!self) return;
        self->ready_poll_pending_ = false;
        if (self->stopped_ || self->faulted_ || !self->enabled_ ||
            !self->candidate_ ||
            fml::TimePoint::Now() >= self->ready_poll_deadline_)
          return;
        // Observe the latest compatible candidate, not a superseded frame.
        bool pending = false;
        for (const auto& entry : self->candidate_->result->outputs) {
          auto state = entry.texture_ref->ready()->Poll();
          if (state == CompletionStatus::kFailed) {
            // Compose owns failure handling as well as actual submission.
            self->request_display_();
            return;
          }
          pending |= state == CompletionStatus::kPending;
        }
        if (pending) {
          self->ScheduleReadyPoll(self->ready_poll_deadline_);
          return;
        }
        // A ready notification only requests an opportunity. The frame clock
        // rechecks the deadline and whether this cycle already submitted.
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
      for (auto& use : it->uses) use.record->Retire();
      if (it->submission.output) it->submission.output->Retire();
      if (current_ && current_->frame_id == it->frame->frame_id &&
          current_->window == it->frame->window)
        current_.reset();
      if (presented_)
        presented_(*it->frame, {PresentationFeedback::Status::kFailed, 0});
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
