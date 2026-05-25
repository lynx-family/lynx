// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/pipeline/pipeline_context.h"

#include <memory>

#include "base/include/fml/hash_combine.h"
#include "base/include/log/logging.h"
#include "core/renderer/pipeline/pipeline_lifecycle.h"
#include "core/renderer/pipeline/pipeline_lifecycle_observer.h"
#include "core/renderer/pipeline/pipeline_version.h"

namespace lynx {
namespace tasm {
PipelineContext::PipelineContext(const PipelineVersion& version)
    : version_(version) {
  lifecycle_.AdvanceTo(LifecycleState::kInactive);
  observer_data_.pipeline_version = version_;
};

const std::unique_ptr<PipelineContext> PipelineContext::Create(
    const PipelineVersion& version, bool is_major_updated) {
  // We cannot use std::make_unique here, because the constructor is private.
  std::unique_ptr<PipelineContext> pipeline_context(new PipelineContext(
      is_major_updated ? version.GenerateNextMajorVersion()
                       : version.GenerateNextMinorVersion()));
  return pipeline_context;
}

bool PipelineContext::EnableUnifiedPipelineContext() const {
  if (!options_) {
    LOGE("options is nullptr");
    return false;
  }
  return options_->enable_unified_pixel_pipeline;
}

bool PipelineContext::IsResolveRequested() const {
  if (!options_) {
    LOGE("options is nullptr");
    return false;
  }
  return options_->resolve_requested;
}

bool PipelineContext::IsLayoutRequested() const {
  if (!options_) {
    LOGE("options is nullptr");
    return false;
  }
  return options_->layout_requested && !options_->render_for_recreate_engine;
}

bool PipelineContext::IsFlushUIOperationRequested() const {
  if (!options_) {
    LOGE("options is nullptr");
    return false;
  }
  return options_->flush_ui_requested;
}

bool PipelineContext::IsReload() const {
  if (!options_) {
    LOGE("options is nullptr");
    return false;
  }
  return options_->reload;
}

void PipelineContext::RequestResolve() {
  if (!options_) {
    LOGE("options is nullptr");
    return;
  }
  options_->resolve_requested = true;
}

void PipelineContext::RequestLayout() {
  if (!options_) {
    LOGE("options is nullptr");
    return;
  }
  options_->layout_requested = true;
}

void PipelineContext::RequestFlushUIOperation() {
  if (!options_) {
    LOGE("options is nullptr");
    return;
  }
  options_->flush_ui_requested = true;
}

void PipelineContext::MarkReload(bool reload) {
  if (!options_) {
    LOGE("options is nullptr");
    return;
  }
  options_->reload = reload;
}

std::size_t PipelineContext::GetHash() {
  if (hash_ == 0) {
    hash_ = fml::HashCombine();
    fml::HashCombineSeed(hash_, this, version_.GetMajor(), version_.GetMinor());
  }

  return hash_;
}

void PipelineContext::ResetResolveRequested() {
  if (!options_) {
    LOGE("options is nullptr");
    return;
  }
  options_->resolve_requested = false;
  options_->target_node = PipelineOptions::kInvalidTargetNodeId;
}

void PipelineContext::ResetLayoutRequested() {
  if (!options_) {
    LOGE("options is nullptr");
    return;
  }
  options_->layout_requested = false;
}

void PipelineContext::ResetFlushUIOperationRequested() {
  if (!options_) {
    LOGE("options is nullptr");
    return;
  }
  options_->flush_ui_requested = false;
}

bool PipelineContext::AdvanceLifecycleTo(LifecycleState state) {
  bool result = lifecycle_.AdvanceTo(state);
  if (!result) {
    LOGE("Failed to advance lifecycle to " << static_cast<uint32_t>(state));
    return result;
  }

  return result;
}

PipelineLifecycleObserver::Data PipelineContext::BuildLifecycleObserverData(
    LifecycleState prev_state, LifecycleState cur_state) const {
  auto observer_data = observer_data_;
  observer_data.prev_state = prev_state;
  observer_data.cur_state = cur_state;

  switch (cur_state) {
    case LifecycleState::kInStyleResolve:
    case LifecycleState::kAfterStyleResolve:
      observer_data.is_state_executed = IsResolveRequested();
      break;
    case LifecycleState::kInPerformLayout:
    case LifecycleState::kAfterPerformLayout:
      observer_data.is_state_executed = IsLayoutRequested();
      break;
    case LifecycleState::kUIOpFlush:
      observer_data.is_state_executed = IsFlushUIOperationRequested();
      break;
    case LifecycleState::kStopped:
      observer_data.is_state_executed = true;
      break;
    default:
      break;
  }

  return observer_data;
}

}  // namespace tasm
}  // namespace lynx
