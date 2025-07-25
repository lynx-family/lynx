// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/fsp_tracing/fsp_tracer.h"

#include <memory>
#include <utility>

#include "base/include/timer/time_utils.h"
#include "core/services/fsp_tracing/area/fsp_area_tracer_impl.h"
#include "core/services/fsp_tracing/axial/fsp_axial_tracer_impl.h"
#include "core/services/fsp_tracing/base/fsp_snapshot.h"

namespace lynx {
namespace tasm {
namespace timing {

std::unique_ptr<FSPTracer> FSPTracer::Create(const FSPConfig& config,
                                             SnapshotCallback callback) {
  if (const FSPAxialConfig* cfg = std::get_if<FSPAxialConfig>(&config.v)) {
    return std::make_unique<FSPAxialTracer>(*cfg, std::move(callback));
  } else if (const FSPAreaConfig* cfg = std::get_if<FSPAreaConfig>(&config.v)) {
    return std::make_unique<FSPAreaTracer>(*cfg, std::move(callback));
  } else {
    return nullptr;
  }
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
