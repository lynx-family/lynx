// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SERVICES_FSP_TRACING_BASE_FSP_SNAPSHOT_H_
#define CORE_SERVICES_FSP_TRACING_BASE_FSP_SNAPSHOT_H_

#include "core/services/fsp_tracing/fsp_tracer.h"

namespace lynx {
namespace tasm {
namespace timing {

// Base class for all snapshot variants.
struct FSPSnapshot {
  long last_ui_sign{FSPResult::kUnknownUISign};
  double last_change_timestamp{0.0};
  lynx::base::geometry::IntSize container_size;

  FSPResult GetResult(bool stable) {
    return ((FSPResult){
        .is_stable = stable,
        .last_ui_sign = last_ui_sign,
        .last_change_timestamp = last_change_timestamp,
    });
  }
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FSP_TRACING_BASE_FSP_SNAPSHOT_H_
