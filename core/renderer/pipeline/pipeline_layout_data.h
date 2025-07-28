// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PIPELINE_PIPELINE_LAYOUT_DATA_H_
#define CORE_RENDERER_PIPELINE_PIPELINE_LAYOUT_DATA_H_

#include "core/renderer/pipeline/pipeline_version.h"

namespace lynx {
namespace tasm {

struct PipelineLayoutData {
  bool layout_triggered{false};
  const PipelineVersion* pipeline_version{nullptr};
  bool is_first_layout{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_PIPELINE_PIPELINE_LAYOUT_DATA_H_
