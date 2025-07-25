// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FSP_TRACING_AREA_FSP_AREA_TRACER_IMPL_H_
#define CORE_SERVICES_FSP_TRACING_AREA_FSP_AREA_TRACER_IMPL_H_

#include <memory>
#include <string>

#include "core/services/fsp_tracing/base/fsp_tracer_impl.h"

namespace lynx {
namespace tasm {
namespace timing {

struct FSPAreaSnapshot;

// Class responsible for tracking and calculating First Stable Paint
class FSPAreaTracer : public FSPTracerImpl<FSPAreaTracer, FSPAreaConfig> {
 public:
  using FSPTracerImpl<FSPAreaTracer, FSPAreaConfig>::FSPTracerImpl;

 private:
  friend class FSPTracerImpl<FSPAreaTracer, FSPAreaConfig>;

  std::unique_ptr<FSPSnapshot> CreateSnapshot() override;
  FSPResult CaptureSnapshotImpl(
      FSPSnapshot& snapshot,
      lynx::base::geometry::IntSize container_size) override;
  void FillSnapshotImpl(FSPSnapshot& snapshot,
                        const FSPContentInfo& info) override;
  std::string InspectSnapshotImpl(FSPSnapshot& current,
                                  FSPSnapshot* previous) override;

  static bool DiffAreaSnapshots(FSPAreaSnapshot& current,
                                FSPAreaSnapshot* previous,
                                const FSPAreaConfig& config,
                                std::ostringstream* inspection_out);
};

struct FSPAreaSnapshot : public FSPSnapshot {
 private:
  friend class FSPAreaTracer;

  static constexpr size_t X_PROJECTIONS_LEN = 256;
  static constexpr size_t Y_PROJECTIONS_LEN = 512;
  static constexpr size_t PROJECTIONS_LEN =
      X_PROJECTIONS_LEN * Y_PROJECTIONS_LEN;

  std::bitset<PROJECTIONS_LEN> projections;
  size_t projection_area = 0;
};
}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FSP_TRACING_AREA_FSP_AREA_TRACER_IMPL_H_
