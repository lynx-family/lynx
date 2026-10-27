// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_UPLOAD_DEPENDENCY_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_UPLOAD_DEPENDENCY_H_

#include <memory>

#include "clay/gfx/shared_image/output_texture.h"

namespace clay::display {
// Raster-only upload-to-production dependency. A queued GPU wait permits CPU
// recording to continue, but does not mark either upload or output GPU-ready.
class UploadDependency {
 public:
  template <typename Create, typename EnqueueWait>
  bool Prepare(uint64_t submission, Create create, EnqueueWait enqueue_wait) {
    if (submission != submission_ || !completion_) {
      completion_ = create();
      submission_ = submission;
      enqueued_ = false;
    }
    if (!completion_) return false;
    auto status = completion_->Poll();
    if (status == CompletionStatus::kFailed) return false;
    if (status == CompletionStatus::kComplete || enqueued_) return true;
    // Polling a newly created fence here can starve every new layout when
    // retries are VSync-driven. Order production behind it on the GPU instead.
    enqueued_ = enqueue_wait(completion_);
    return enqueued_;
  }
  void Invalidate() {
    completion_.reset();
    enqueued_ = false;
  }

 private:
  uint64_t submission_ = 0;
  std::shared_ptr<GPUCompletion> completion_;
  bool enqueued_ = false;
};
}  // namespace clay::display

#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_UPLOAD_DEPENDENCY_H_
