// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FSP_TRACING_BASE_FSP_TRACER_IMPL_H_
#define CORE_SERVICES_FSP_TRACING_BASE_FSP_TRACER_IMPL_H_

#include <array>
#include <memory>
#include <utility>

#include "core/services/fsp_tracing/base/fsp_snapshot.h"
#include "core/services/fsp_tracing/fsp_config.h"
#include "core/services/fsp_tracing/fsp_tracer.h"

namespace lynx {
namespace tasm {
namespace timing {

template <class Derived, class Config>
class FSPTracerImpl : public FSPTracer {
 public:
  FSPTracerImpl(const Config& config, FSPTracer::SnapshotCallback callback)
      : FSPTracer(std::move(callback)), config_(config) {}

  FSPTracerImpl(const FSPTracerImpl&) = delete;
  FSPTracerImpl& operator=(const FSPTracerImpl&) = delete;

  FSPTracerImpl(FSPTracerImpl&&) = delete;
  FSPTracerImpl& operator=(FSPTracerImpl&&) = delete;

  std::optional<FSPResult> CaptureSnapshot(
      lynx::base::geometry::IntSize container_size) override {
    return static_cast<Derived*>(this)->CaptureSnapshotImpl(
        SwapCurrentSnapshot(), container_size);
  }

  void FillSnapshot(FSPSnapshot& snapshot,
                    const FSPContentInfo& content_info) override {
    assert(std::addressof(snapshot) == current_snapshot_.get());
    if (current_snapshot_) {
      static_cast<Derived*>(this)->FillSnapshotImpl(*current_snapshot_,
                                                    content_info);
    }
  }

  ~FSPTracerImpl() = default;

 protected:
  Config config_;

  std::unique_ptr<FSPSnapshot> current_snapshot_{nullptr};
  std::unique_ptr<FSPSnapshot> previous_snapshot_{nullptr};

  virtual std::unique_ptr<FSPSnapshot> CreateSnapshot() = 0;
  virtual FSPResult CaptureSnapshotImpl(
      FSPSnapshot& snapshot, lynx::base::geometry::IntSize container_size) = 0;
  virtual void FillSnapshotImpl(FSPSnapshot& snapshot,
                                const FSPContentInfo& content_info) = 0;

  void ResetSnapshots() {
    current_snapshot_ = nullptr;
    previous_snapshot_ = nullptr;
  }

  FSPSnapshot& SwapCurrentSnapshot() {
    if (!current_snapshot_) {
      current_snapshot_ = CreateSnapshot();
      previous_snapshot_ = nullptr;
    } else if (!previous_snapshot_) {
      previous_snapshot_.swap(current_snapshot_);
      current_snapshot_ = CreateSnapshot();
    } else {
      current_snapshot_.swap(previous_snapshot_);
    }

    return *current_snapshot_;
  }
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FSP_TRACING_BASE_FSP_TRACER_IMPL_H_
