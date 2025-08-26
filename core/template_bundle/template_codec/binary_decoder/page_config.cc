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
          (isParallelElementConfigUndefined && enable_parallel_element_));
}

}  // namespace tasm
}  // namespace lynx
