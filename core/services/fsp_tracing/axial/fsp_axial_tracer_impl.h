// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FSP_TRACING_AXIAL_FSP_AXIAL_TRACER_IMPL_H_
#define CORE_SERVICES_FSP_TRACING_AXIAL_FSP_AXIAL_TRACER_IMPL_H_

#include <memory>

#include "core/services/fsp_tracing/base/fsp_tracer_impl.h"

namespace lynx {
namespace tasm {
namespace timing {

struct FSPAxialSnapshot;
class FSPAxialTracer : public FSPTracerImpl<FSPAxialTracer, FSPAxialConfig> {
 public:
  using FSPTracerImpl<FSPAxialTracer, FSPAxialConfig>::FSPTracerImpl;

 private:
  friend class FSPTracerImpl<FSPAxialTracer, FSPAxialConfig>;

  std::unique_ptr<FSPSnapshot> CreateSnapshot() override;
  FSPResult CaptureSnapshotImpl(
      FSPSnapshot& snapshot,
      lynx::base::geometry::IntSize container_size) override;
  void FillSnapshotImpl(FSPSnapshot& snapshot,
                        const FSPContentInfo& info) override;

  static bool DiffAxialSnapshots(FSPAxialSnapshot& current,
                                 FSPAxialSnapshot* previous,
                                 const FSPAxialConfig& config);
};

struct FSPAxialSnapshot : public FSPSnapshot {
 private:
  friend class FSPAxialTracer;
  static constexpr size_t X_PROJECTIONS_LEN = 256;
  static constexpr size_t Y_PROJECTIONS_LEN = 512;

  std::bitset<X_PROJECTIONS_LEN> x_projections;
  std::bitset<Y_PROJECTIONS_LEN> y_projections;

  lynx::base::geometry::IntSize projection_size;
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FSP_TRACING_AXIAL_FSP_AXIAL_TRACER_IMPL_H_
