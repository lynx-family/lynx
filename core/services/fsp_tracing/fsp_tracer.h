// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FSP_TRACING_FSP_TRACER_H_
#define CORE_SERVICES_FSP_TRACING_FSP_TRACER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "lynx/base/include/base_export.h"
#include "lynx/base/include/closure.h"
#include "lynx/base/include/geometry/rect.h"
#include "core/services/fsp_tracing/fsp_config.h"

namespace lynx {
namespace tasm {
namespace timing {

struct FSPResult {
  static constexpr long kUnknownUISign = -1;

  bool is_stable{false};
  long last_ui_sign{-1};
  double last_change_timestamp{0.0};
};

struct FSPSnapshot;

struct FSPContentInfo {
  long ui_sign;
  double change_timestamp;
  lynx::base::geometry::IntRect rect;
};

// Class responsible for tracking and calculating First Stable Paint
class BASE_EXPORT_FOR_DEVTOOL FSPTracer {
 public:
  using SnapshotCallback = lynx::base::MoveOnlyClosure<void, FSPSnapshot&>;

  // Factory method to create a tracer instance.
  static std::unique_ptr<FSPTracer> Create(const FSPConfig& config,
                                           SnapshotCallback callback);

  // Capture the content information of the UI tree, return the tracing result
  // if the stability point has been reached.
  virtual FSPResult CaptureSnapshot(
      lynx::base::geometry::IntSize container_size,
      uint64_t snapshot_flow_id = 0) = 0;

  // Fill the snapshot with content information of an element in UI tree.
  virtual void FillSnapshot(FSPSnapshot& snapshot,
                            const FSPContentInfo& content_info,
                            uint64_t snapshot_flow_id = 0) = 0;

  // For DevTool;
  virtual std::string InspectCurrentSnapshot() = 0;

  virtual ~FSPTracer() = default;

 protected:
  SnapshotCallback callback_;

  FSPTracer(SnapshotCallback callback) : callback_(std::move(callback)){};

  // Prevent copying
  FSPTracer(const FSPTracer&) = delete;
  FSPTracer& operator=(const FSPTracer&) = delete;
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FSP_TRACING_FSP_TRACER_H_
