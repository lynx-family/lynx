// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_DISPLAY_COMPOSITOR_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_DISPLAY_COMPOSITOR_H_
#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/flow/compositor/compositor_frame.h"
namespace clay::display {
struct DisplaySubmission {
  std::shared_ptr<GPUCompletion> completion;
  std::shared_ptr<void> resources;
  bool presented = false;
  bool retry = false;
  // Independent composed GPU output; never aliases a Raster input snapshot.
  std::shared_ptr<const OutputTexture> output;
};
class DisplayTarget {
 public:
  virtual ~DisplayTarget() = default;
  // A null completion is permitted only when no sampling commands were issued.
  virtual DisplaySubmission Present(const CompositorFrame& frame) = 0;
  virtual void Detach() = 0;
};
class DisplayCompositor
    : public std::enable_shared_from_this<DisplayCompositor> {
 public:
  using Presented =
      std::function<void(const CompositorFrame&, PresentationFeedback)>;
  DisplayCompositor(fml::RefPtr<fml::TaskRunner> runner,
                    std::unique_ptr<DisplayTarget> target, WindowKey window,
                    std::function<void()> request_display, Presented presented,
                    std::function<void(PipelineError)> error = nullptr);
  ~DisplayCompositor();
  void Accept(std::shared_ptr<const CompositorFrame> frame);
  void InvalidateCandidate();
  void SetEnabled(bool enabled);
  void Stop();
  // Called only at a display opportunity; never by a retirement task.
  bool Compose(fml::TimePoint deadline);
  // Metadata and one composed output only. Raster inputs live in in_flight_
  // until their sampling commands complete. Explicit screenshot requests may
  // retain this output independently of the next displayed frame.
  const std::shared_ptr<const CompositorFrame>& current() const {
    return current_;
  }

 private:
  struct Use {
    std::shared_ptr<ResourceRecord> record;
    uint64_t id;
  };
  struct InFlight {
    std::shared_ptr<const CompositorFrame> frame;
    std::vector<Use> uses;
    DisplaySubmission submission;
    fml::TimePoint submitted;
  };
  void PollRetirement();
  void Fail(PipelineError error);
  void SchedulePoll();
  void ScheduleReadyPoll(fml::TimePoint deadline);
  fml::RefPtr<fml::TaskRunner> runner_;
  std::unique_ptr<DisplayTarget> target_;
  const WindowKey window_;
  std::function<void()> request_display_;
  Presented presented_;
  std::function<void(PipelineError)> error_;
  fml::TimePoint candidate_since_;
  fml::TimePoint ready_poll_deadline_;
  bool faulted_ = false;
  std::shared_ptr<const CompositorFrame> candidate_, current_;
  std::list<InFlight> in_flight_;
  bool enabled_ = true, stopped_ = false, poll_pending_ = false;
  bool ready_poll_pending_ = false;
};
}  // namespace clay::display
#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_DISPLAY_COMPOSITOR_H_
