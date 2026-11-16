// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_RESOURCE_RETIREMENT_QUEUE_H_
#define CLAY_GFX_SHARED_IMAGE_RESOURCE_RETIREMENT_QUEUE_H_

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "clay/gfx/shared_image/output_texture.h"

namespace clay::display {
// No page, binding or mutable scene is retained while CPU/GPU users drain.
class ResourceRetirementQueue
    : public std::enable_shared_from_this<ResourceRetirementQueue> {
 public:
  static void Retire(fml::RefPtr<fml::TaskRunner> runner,
                     std::vector<std::shared_ptr<ResourceRecord>> records,
                     std::function<void()> drained = nullptr) {
    auto queue =
        std::shared_ptr<ResourceRetirementQueue>(new ResourceRetirementQueue(
            std::move(runner), std::move(records), std::move(drained)));
    for (auto& record : queue->records_) record->Retire();
    queue->Sweep();
  }

 private:
  ResourceRetirementQueue(fml::RefPtr<fml::TaskRunner> runner,
                          std::vector<std::shared_ptr<ResourceRecord>> records,
                          std::function<void()> drained)
      : runner_(std::move(runner)),
        records_(std::move(records)),
        drained_(std::move(drained)) {}
  void Sweep() {
    DCHECK(runner_->RunsTasksOnCurrentThread());
    records_.erase(std::remove_if(records_.begin(), records_.end(),
                                  [](auto& r) { return r->CanDestroy(); }),
                   records_.end());
    if (records_.empty()) {
      if (drained_) drained_();
      return;
    }
    runner_->PostDelayedTask([self = shared_from_this()] { self->Sweep(); },
                             fml::TimeDelta::FromMilliseconds(250));
  }
  fml::RefPtr<fml::TaskRunner> runner_;
  std::vector<std::shared_ptr<ResourceRecord>> records_;
  std::function<void()> drained_;
};
}  // namespace clay::display
#endif  // CLAY_GFX_SHARED_IMAGE_RESOURCE_RETIREMENT_QUEUE_H_
