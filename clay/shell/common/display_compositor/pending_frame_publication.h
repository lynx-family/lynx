// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PENDING_FRAME_PUBLICATION_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PENDING_FRAME_PUBLICATION_H_
#include <functional>
#include <memory>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_point.h"
#include "clay/shell/common/display_compositor/frame_mailbox.h"

namespace clay::display {
// One submitted Raster write. The mailbox retains its previous ready frame
// until every output in this publication can be consumed without a CPU wait.
// All methods and the completion callback run on the supplied Raster runner.
class PendingFramePublication
    : public std::enable_shared_from_this<PendingFramePublication> {
 public:
  PendingFramePublication(RasterPublication publication,
                          std::shared_ptr<FrameMailbox> mailbox,
                          fml::RefPtr<fml::TaskRunner> runner,
                          std::function<void()> wake,
                          std::function<void(bool)> completed)
      : publication_(std::move(publication)),
        mailbox_(std::move(mailbox)),
        runner_(std::move(runner)),
        wake_(std::move(wake)),
        completed_(std::move(completed)) {}

  // Store shared ownership before Start: an already ready write can finish
  // synchronously and invoke completed from this call.
  void Start() {
    if (!pending_ || started_) return;
    started_ = true;
    submitted_ = fml::TimePoint::Now();
    Poll();
  }
  void Cancel() {
    pending_ = false;
    publication_ = {};
    completed_ = nullptr;
    wake_ = nullptr;
  }
  bool is_pending() const { return pending_; }

 private:
  void Finish(bool success) {
    pending_ = false;
    publication_ = {};
    wake_ = nullptr;
    auto completed = std::move(completed_);
    if (completed) completed(success);
  }
  void Fail(PipelineError error) {
    if (mailbox_->ReportFailure(error) && wake_) wake_();
    Finish(false);
  }
  void Poll() {
    if (!pending_) return;
    if (mailbox_->IsClosed()) {
      Finish(false);
      return;
    }
    if (!publication_ || publication_.result->outputs.empty()) {
      Fail(PipelineError::kProducerFailure);
      return;
    }
    bool pending = false;
    for (const auto& output : publication_.result->outputs) {
      if (!output.texture_ref || !output.texture_ref->ready()) {
        Fail(PipelineError::kProducerFailure);
        return;
      }
      auto status = output.texture_ref->ready()->Poll();
      if (status == CompletionStatus::kFailed) {
        Fail(PipelineError::kProducerFailure);
        return;
      }
      pending |= status == CompletionStatus::kPending;
    }
    if (pending) {
      if ((fml::TimePoint::Now() - submitted_).ToMilliseconds() > 2000) {
        Fail(PipelineError::kGpuTimeout);
        return;
      }
      runner_->PostDelayedTask([self = shared_from_this()] { self->Poll(); },
                               fml::TimeDelta::FromMilliseconds(1));
      return;
    }
    auto status = mailbox_->Publish(std::move(publication_));
    if (status == FrameMailbox::PublishResult::kWake && wake_) wake_();
    Finish(status != FrameMailbox::PublishResult::kRejected);
  }

  RasterPublication publication_;
  std::shared_ptr<FrameMailbox> mailbox_;
  fml::RefPtr<fml::TaskRunner> runner_;
  std::function<void()> wake_;
  std::function<void(bool)> completed_;
  fml::TimePoint submitted_;
  bool pending_ = true, started_ = false;
};
}  // namespace clay::display
#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PENDING_FRAME_PUBLICATION_H_
