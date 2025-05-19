// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_H_
#define CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_H_

#include <memory>
#include <utility>

#include "core/renderer/pipeline/pipeline_option.h"
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
  }
  const std::shared_ptr<PipelineOptions> GetOptions() const { return options_; }
  const PipelineVersion& GetVersion() const { return version_; }
  std::size_t GetHash();

  // Set and get switches for pipeline stage and reload flags.
  bool IsResolveRequested() const;
  bool IsLayoutRequested() const;
  bool IsFlushUIOperationRequested() const;
  bool IsReload() const;
  void RequestResolve(bool resolve);
  void RequestLayout(bool layout);
  void RequestFlushUIOperation(bool flush);
  void MarkReload(bool reload);

 private:
  explicit PipelineContext(const PipelineVersion& version);

  std::shared_ptr<PipelineOptions> options_{nullptr};
  PipelineVersion version_;
  std::size_t hash_{0};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_PIPELINE_PIPELINE_CONTEXT_H_
