// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_H_
#define CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_H_

#include <cstdint>

namespace lynx {
namespace tasm {

enum class LifecycleState : uint32_t {
  kUninitialized,
  kInactive,

  // When tasm is active, it traverses these states.
  // Note: kInRender and kAfterRender are not enable now.
  // kInRender,
  // kAfterRender,

  kInStyleResolve,
  kAfterStyleResolve,

  kInPerformLayout,
  kAfterPerformLayout,

  kUIOpFlush,

  // Once pipeline starts destorying, we cannot return
  // to the render/resolve/layout/flush states.
  kStopped,
};

class PipelineLifecycle {
 public:
  PipelineLifecycle() = default;
  ~PipelineLifecycle() = default;
  PipelineLifecycle(const PipelineLifecycle&) = delete;
  PipelineLifecycle& operator=(const PipelineLifecycle&) = delete;
  PipelineLifecycle(PipelineLifecycle&&) = default;
  PipelineLifecycle& operator=(PipelineLifecycle&&) = default;

  bool AdvanceTo(LifecycleState state);
  LifecycleState GetState() const { return state_; };
  bool IsActive() const;
  bool IsInStyleResolve() const;
  bool IsInPerformLayout() const;
  bool IsUIOpFlush() const;

 private:
  bool CanAdvanceTo(LifecycleState state) const;

  LifecycleState state_{LifecycleState::kUninitialized};
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_H_
