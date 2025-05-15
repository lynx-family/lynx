// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_MANAGER_H_
#define CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_MANAGER_H_

#include <map>
#include <memory>

#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/pipeline/pipeline_version.h"

namespace lynx {
namespace tasm {
class PipelineContextManager {
 public:
  PipelineContextManager();
  ~PipelineContextManager() = default;

  PipelineContext* CreateAndUpdateCurrentPipelineContext(
      const std::shared_ptr<PipelineOptions>& pipeline_options,
      bool is_major_updated = false);
  PipelineContext* GetCurrentPipelineContext() const {
    return current_pipeline_context_;
  }
  PipelineContext* GetPipelineContextByVersion(
      const PipelineVersion& version) const;

 private:
  std::map<PipelineVersion, const std::unique_ptr<PipelineContext>>
      pipeline_contexts_{};
  PipelineContext* current_pipeline_context_{nullptr};

  bool enable_unified_pixel_pipeline_{false};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_MANAGER_H_
