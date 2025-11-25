// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/page_config.h"

#include <vector>

namespace lynx {
namespace tasm {
bool PageConfig::GetEnableParallelElement() const {
  bool enableParallelElementFromSchedulerConfig =
      (GetPipelineSchedulerConfig() & kEnableParallelElementMask) > 0;
  bool isParallelElementConfigUndefined =
      ((GetPipelineSchedulerConfig() & kDisableParallelElementMask) == 0) &&
      !enableParallelElementFromSchedulerConfig;
  // enableParallelElement from pipelineSchedulerConfig would override
  // enableParallelElement encode option
  return (enableParallelElementFromSchedulerConfig ||
          (isParallelElementConfigUndefined &&
           (enable_parallel_element_ ||
            enable_level_order_traversing_ == TernaryBool::TRUE_VALUE)));
}

bool PageConfig::GetEnableLevelOrderTraversing() {
  if (enable_level_order_traversing_ != TernaryBool::UNDEFINE_VALUE) {
    return enable_level_order_traversing_ == TernaryBool::TRUE_VALUE;
  }

  // level_order_traversing from pipelineSchedulerConfig would override
  // level_order_traversing env
  if ((pipeline_scheduler_config_ & kDisableParallelElementLevelOrderMask) >
      0) {
    enable_level_order_traversing_ = TernaryBool::FALSE_VALUE;
    return false;
  }

  if ((pipeline_scheduler_config_ & kEnableParallelElementLevelOrderMask) > 0) {
    enable_level_order_traversing_ = TernaryBool::TRUE_VALUE;
    return true;
  }

  auto value_from_config = LynxEnv::GetInstance().EnableLevelOrderTraversing();
  enable_level_order_traversing_ =
      value_from_config ? TernaryBool::TRUE_VALUE : TernaryBool::FALSE_VALUE;
  return value_from_config;
}
}  // namespace tasm
}  // namespace lynx
