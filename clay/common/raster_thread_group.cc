// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/raster_thread_group.h"

#include <memory>
#include <utility>

#include "clay/fml/logging.h"
namespace clay::display {
namespace {
std::atomic<uint64_t> next_group{1};
}
RasterThreadGroup::RasterThreadGroup(fml::RefPtr<fml::TaskRunner> platform,
                                     fml::RefPtr<fml::TaskRunner> worker,
                                     uint32_t size)
    : platform_(std::move(platform)),
      worker_(std::move(worker)),
      size_(size),
      generation_(next_group++) {}
std::shared_ptr<RasterBinding> RasterThreadGroup::RegisterRasterizer(
    uint64_t tree_id, uint64_t tree_generation,
    std::function<bool()> initialize, std::function<void(bool)> ready,
    std::function<void(uint64_t)> announce_layout,
    std::function<void(RasterBindingKey)> prepare) {
  FML_DCHECK(platform_->RunsTasksOnCurrentThread());
  if (!IsValid() || !tree_id || !tree_generation) {
    ready(false);
    return nullptr;
  }
  auto binding = std::shared_ptr<RasterBinding>(new RasterBinding(
      {tree_id, tree_generation, generation_, ++next_binding_, 0}, worker_,
      platform_, std::move(announce_layout)));
  prepare(binding->key());
  binding->Initialize(std::move(initialize), std::move(ready));
  return binding;
}
void RasterThreadGroup::UnregisterRasterizer(
    const std::shared_ptr<RasterBinding>& binding,
    std::function<void(std::function<void()>)> release,
    std::function<void()> drained) {
  FML_DCHECK(platform_->RunsTasksOnCurrentThread());
  if (binding && binding->key().group_generation == generation_)
    binding->Close(std::move(release), std::move(drained));
  else if (drained)
    platform_->PostTask(std::move(drained));
}
}  // namespace clay::display
