// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_DISPLAY_COMPOSITOR_H_
#define CLAY_SHELL_COMMON_DISPLAY_DISPLAY_COMPOSITOR_H_
#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/flow/compositor/compositor_frame.h"
namespace skity {
class Canvas;
}
namespace clay {
class SkityImage;
}
namespace clay::display {
// Inputs are imported before recording; submission and synchronization remain
// with the caller.
void DrawDisplayInputs(skity::Canvas& canvas,
                       const std::vector<TextureEntry>& outputs,
                       const std::vector<std::shared_ptr<SkityImage>>& images,
                       uint32_t background = 0);
// A complete input snapshot may be forwarded without drawing a new output.
// The target checks native compatibility, including the snapshot's origin.
bool CanForwardWholeOutput(const RasterResult& result);
class DisplayTarget {
 public:
  virtual ~DisplayTarget() = default;
  // Queue each input's producer dependency in the consumer GPU context before
  // sampling it. Inputs may still be pending when Present is called.
  // A null completion is permitted only when no sampling commands were issued.
  // Completion covers submitted GPU reads. A native host that keeps sampling
  // afterward must retain its own immutable output lease until it releases the
  // image. Forwarding must not replace the input's producer completion.
  virtual DisplaySubmission Present(const CompositorFrame& frame) = 0;
  virtual void Detach() = 0;
};
class DisplayCompositor
    : public std::enable_shared_from_this<DisplayCompositor> {
 public:
  using Presented = std::function<void(const CompositorFrame&)>;
  // The consumer can read this immutable snapshot with std::atomic_load
  // without blocking on the rendering thread.
  using Snapshot = std::shared_ptr<std::shared_ptr<const CompositorFrame>>;
  DisplayCompositor(fml::RefPtr<fml::TaskRunner> runner,
                    std::unique_ptr<DisplayTarget> target, WindowKey window,
                    std::function<void()> request_display, Presented presented,
                    std::function<void(PipelineError)> error = nullptr,
                    Snapshot snapshot = nullptr);
  ~DisplayCompositor();
  void Accept(std::shared_ptr<const CompositorFrame> frame);
  void InvalidateCandidate();
  void SetEnabled(bool enabled);
  void Stop();
  // Called only at a display opportunity; never by a retirement task.
  bool Compose(fml::TimePoint deadline);
  // Exported outputs or the inputs last drawn into a native window. The current
  // snapshot and screenshots keep CPU leases; in_flight_ keeps GPU uses alive
  // independently until presentation commands complete.
  const std::shared_ptr<const CompositorFrame>& current() const {
    return current_;
  }
  std::shared_ptr<const CompositorFrame> latest() const {
    return candidate_ ? candidate_ : last_source_frame_;
  }

 private:
  struct InFlight {
    std::shared_ptr<const CompositorFrame> frame;
    DisplaySubmission submission;
    fml::TimePoint submitted;
  };
  void PollRetirement();
  void RetireSubmission(const CompositorFrame& frame,
                        const DisplaySubmission& submission);
  void Fail(PipelineError error);
  void SchedulePoll();
  void ScheduleRetry(fml::TimePoint deadline);
  void SetCurrentFrame(std::shared_ptr<const CompositorFrame> frame);
  fml::RefPtr<fml::TaskRunner> runner_;
  std::unique_ptr<DisplayTarget> target_;
  const WindowKey window_;
  std::function<void()> request_display_;
  Presented presented_;
  std::function<void(PipelineError)> error_;
  Snapshot snapshot_;
  fml::TimePoint candidate_since_;
  fml::TimePoint retry_deadline_;
  bool faulted_ = false;
  std::shared_ptr<const CompositorFrame> candidate_, current_,
      last_source_frame_;
  DisplaySubmission::OutputMode current_output_mode_ =
      DisplaySubmission::OutputMode::kComposed;
  std::list<InFlight> in_flight_;
  bool enabled_ = true, stopped_ = false, poll_pending_ = false;
  bool retry_pending_ = false;
};
}  // namespace clay::display
#endif  // CLAY_SHELL_COMMON_DISPLAY_DISPLAY_COMPOSITOR_H_
