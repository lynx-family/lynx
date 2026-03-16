// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_H_
#define CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_H_

#include <memory>
#include <utility>

#include "core/public/pipeline_option.h"
#include "core/renderer/pipeline/pipeline_lifecycle.h"
#include "core/renderer/pipeline/pipeline_lifecycle_observer.h"
#include "core/renderer/pipeline/pipeline_version.h"

namespace lynx {
namespace tasm {
class PipelineContext {
 public:
  ~PipelineContext() = default;
  PipelineContext& operator=(const PipelineContext&) = delete;
  PipelineContext(const PipelineContext&) = delete;
  PipelineContext& operator=(PipelineContext&&) = default;
  PipelineContext(PipelineContext&&) = default;

  static const std::unique_ptr<PipelineContext> Create(
      const PipelineVersion& version, bool is_major_updated);

  void SetOptions(const std::shared_ptr<PipelineOptions>& options) {
    options_ = options;
    observer_data_.pipeline_id = options_->pipeline_id;
    observer_data_.pipeline_origin = options_->pipeline_origin;
  }
  const std::shared_ptr<PipelineOptions> GetOptions() const { return options_; }
  const PipelineVersion& GetVersion() const { return version_; }
  std::size_t GetHash();

  bool EnableUnifiedPipelineContext() const;

  // Set and get switches for pipeline stage and reload flags.
  bool IsResolveRequested() const;
  bool IsLayoutRequested() const;
  bool IsFlushUIOperationRequested() const;
  bool IsReload() const;
  void RequestResolve();
  void RequestLayout();
  void RequestFlushUIOperation();
  void MarkReload(bool reload);

  void ResetResolveRequested();
  void ResetLayoutRequested();
  void ResetFlushUIOperationRequested();

  // Pipeline lifecycle management.
  LifecycleState GetLifecycleState() const { return lifecycle_.GetState(); };
  PipelineLifecycleObserver::Data BuildLifecycleObserverData(
      LifecycleState prev_state, LifecycleState cur_state) const;

 private:
  friend class PipelineContextManager;
  explicit PipelineContext(const PipelineVersion& version);
  bool AdvanceLifecycleTo(LifecycleState state);

  std::shared_ptr<PipelineOptions> options_{nullptr};
  PipelineVersion version_;
  std::size_t hash_{0};
  PipelineLifecycle lifecycle_{};
  PipelineLifecycleObserver::Data observer_data_{};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_H_
