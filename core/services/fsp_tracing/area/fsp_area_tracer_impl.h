// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FSP_TRACING_AREA_FSP_AREA_TRACER_IMPL_H_
#define CORE_SERVICES_FSP_TRACING_AREA_FSP_AREA_TRACER_IMPL_H_

#include <memory>

#include "core/services/fsp_tracing/base/fsp_tracer_impl.h"

namespace lynx {
namespace tasm {
namespace timing {

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
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FSP_TRACING_AREA_FSP_AREA_TRACER_IMPL_H_
