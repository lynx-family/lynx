// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/pipeline/pipeline_context_manager.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {

PipelineContextManager::PipelineContextManager() {
  enable_unified_pixel_pipeline_ =
      LynxEnv::GetInstance().EnableUnifiedPixelPipeline();
}

PipelineContext* PipelineContextManager::CreateAndUpdateCurrentPipelineContext(
    const std::shared_ptr<PipelineOptions>& pipeline_options,
    bool is_major_updated) {
  auto current_version = current_pipeline_context_
                             ? current_pipeline_context_->GetVersion()
                             : PipelineVersion::Create();
  auto pipeline_context =
      PipelineContext::Create(current_version, is_major_updated);
  if (!pipeline_context) {
    LOGE("create pipeline context get nullptr");
    return nullptr;
  }
  pipeline_options->enable_unified_pixel_pipeline =
      enable_unified_pixel_pipeline_;
  pipeline_context->SetOptions(pipeline_options);
  current_pipeline_context_ = pipeline_context.get();
  pipeline_contexts_.emplace(pipeline_context->GetVersion(),
                             std::move(pipeline_context));

  return current_pipeline_context_;
}

PipelineContext* PipelineContextManager::GetPipelineContextByVersion(
    const PipelineVersion& version) const {
  if (auto it = pipeline_contexts_.find(version);
      it != pipeline_contexts_.end()) {
    return it->second.get();
  }

  LOGE("pipeline context not found by version: " << version.ToString())
  return nullptr;
}
}  // namespace tasm
}  // namespace lynx
