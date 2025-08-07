// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/page_config.h"

#include <vector>

#include "core/template_bundle/template_codec/binary_decoder/auto_gen_lynx_config_constants.h"

namespace lynx {
namespace tasm {

/**
 * @name: pipelineSchedulerConfig
 * @description: Scheduler config for pipeline, including
 * enableParallelElement/list-framework batch render and other scheduler config
 * @platform: Both
 * @supportVersion: 3.1
 */
static constexpr const char* const kPipelineSchedulerConfig =
    "pipelineSchedulerConfig";

const PageConfig::PageConfigMap<TernaryBool>& PageConfig::GetFuncBoolMap() {
  static const base::NoDestructor<const PageConfigMap<TernaryBool>>
      kPageConfigFuncBoolMap{{
          {config::kTrailNewImage,
           {&PageConfig::SetTrailNewImage, &PageConfig::GetTrailNewImage}},
          {config::kAsyncRedirect,
           {&PageConfig::SetAsyncRedirect, &PageConfig::GetAsyncRedirect}},
          {config::kEnableUseMapBuffer,
           {&PageConfig::SetEnableUseMapBuffer,
            &PageConfig::GetEnableUseMapBuffer}},
          {config::kEnableUIOperationOptimize,
           {&PageConfig::SetEnableUIOperationOptimize,
            &PageConfig::GetEnableUIOperationOptimize}},
          {config::kEnableNativeList,
           {&PageConfig::SetEnableNativeList,
            &PageConfig::GetEnableNativeList}},
          {config::kEnableFiberElementForRadonDiff,
           {&PageConfig::SetEnableFiberElementForRadonDiff,
            &PageConfig::GetEnableFiberElementForRadonDiff}},
          {config::kEnableMicrotaskPromisePolyfill,
           {&PageConfig::SetEnableMicrotaskPromisePolyfill,
            &PageConfig::GetEnableMicrotaskPromisePolyfill}},
          {config::kEnableSignalAPI,
           {&PageConfig::SetEnableSignalAPI, &PageConfig::GetEnableSignalAPI}},
          {config::kEnableOptPushStyleToBundle,
           {&PageConfig::SetEnableOptPushStyleToBundle,
            &PageConfig::GetEnableOptPushStyleToBundle}},
          {config::kEnableNativeScheduleCreateViewAsync,
           {&PageConfig::SetEnableNativeScheduleCreateViewAsync,
            &PageConfig::GetEnableNativeScheduleCreateViewAsync}},
          {config::kEnableUnifiedPipeline,
           {&PageConfig::SetEnableUnifiedPipeline,
            &PageConfig::GetEnableUnifiedPipeline}},
      }};
  return *kPageConfigFuncBoolMap;
}

const PageConfig::PageConfigMap<uint64_t>& PageConfig::GetFuncUint64Map() {
  static const base::NoDestructor<const PageConfigMap<uint64_t>>
      kPageConfigFuncUint64Map{{{kPipelineSchedulerConfig,
                                 {&PageConfig::SetPipelineSchedulerConfig,
                                  &PageConfig::GetPipelineSchedulerConfig}}}};
  return *kPageConfigFuncUint64Map;
}

bool PageConfig::GetEnableParallelElement() const {
  bool enableParallelElementFromSchedulerConfig =
      (GetPipelineSchedulerConfig() & kEnableParallelElementMask) > 0;
  bool isParallelElementConfigUndefined =
      ((GetPipelineSchedulerConfig() & kDisableParallelElementMask) == 0) &&
      !enableParallelElementFromSchedulerConfig;
  // enableParallelElement from pipelineSchedulerConfig would override
  // enableParallelElement encode option
  return (enableParallelElementFromSchedulerConfig ||
          (isParallelElementConfigUndefined && enable_parallel_element_));
}

}  // namespace tasm
}  // namespace lynx
