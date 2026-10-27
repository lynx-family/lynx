// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/staged_image_read_scope.h"

#include <utility>

#include "clay/fml/logging.h"
namespace clay::display {
namespace {
thread_local StagedImageReadScope* current_scope = nullptr;
struct Drain : std::enable_shared_from_this<Drain> {
  struct Write {
    std::shared_ptr<ResourceRecord> record;
    std::shared_ptr<const GPUCompletion> completion;
  };
  fml::RefPtr<fml::TaskRunner> runner;
  std::function<void(std::function<void()>)> context;
  std::vector<Write> writes;
  std::vector<std::function<void()>> releases;
  void Poll() {
    for (const auto& write : writes) {
      if (!write.completion || !write.completion->IsSafeToRelease()) {
        // Quarantined writes still retain their source/storage, but a missing
        // fence or failed query must not keep waking Raster at frame cadence.
        const bool quarantined =
            !write.completion ||
            write.completion->Poll() == CompletionStatus::kFailed;
        runner->PostDelayedTask(
            [self = shared_from_this()] { self->Poll(); },
            fml::TimeDelta::FromMilliseconds(quarantined ? 250 : 1));
        return;
      }
    }
    context([&] {
      for (auto& release : releases) release();
      releases.clear();
      writes.clear();
    });
  }
};
}  // namespace
StagedImageReadScope::StagedImageReadScope(
    fml::RefPtr<fml::TaskRunner> runner,
    std::function<void(std::function<void()>)> context)
    : runner_(std::move(runner)), context_(std::move(context)) {
  FML_DCHECK(!current_scope);
  current_scope = this;
}
StagedImageReadScope::~StagedImageReadScope() {
  current_scope = nullptr;
  if (releases_.empty()) return;
  auto drain = std::make_shared<Drain>();
  drain->runner = runner_;
  drain->context = context_;
  drain->writes.reserve(records_.size());
  for (auto& record : records_) {
    // Frame guards finish EndWrite before this scope ends. Keep that batch's
    // completion: the pool may reuse the record before a delayed poll runs.
    // A missing completion cannot prove safety, even if a later write has one.
    auto completion = record->ready();
    drain->writes.push_back({std::move(record), std::move(completion)});
  }
  drain->releases = std::move(releases_);
  drain->Poll();
}
StagedImageReadScope* StagedImageReadScope::Current() { return current_scope; }
void StagedImageReadScope::Hold(const void* source,
                                std::shared_ptr<SourceReadState> state,
                                std::function<void()> release) {
  if (!sources_.insert(source).second) return;
  state->pending = true;
  releases_.push_back([state, release = std::move(release)] {
    release();
    state->pending = false;
  });
}
}  // namespace clay::display
