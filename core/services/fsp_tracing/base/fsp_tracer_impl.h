// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FSP_TRACING_BASE_FSP_TRACER_IMPL_H_
#define CORE_SERVICES_FSP_TRACING_BASE_FSP_TRACER_IMPL_H_

#include <array>
#include <memory>
#include <string>
#include <utility>

#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/services/fsp_tracing/base/fsp_snapshot.h"
#include "core/services/fsp_tracing/fsp_config.h"
#include "core/services/fsp_tracing/fsp_trace_event_def.h"
#include "core/services/fsp_tracing/fsp_tracer.h"

namespace lynx {
namespace tasm {
namespace timing {

template <class Derived, class Config>
class FSPTracerImpl : public FSPTracer {
 public:
  FSPTracerImpl(const Config& config, FSPTracer::SnapshotCallback callback,
                uint64_t flow_id = 0)
      : FSPTracer(std::move(callback)), config_(config), flow_id_(flow_id) {}

  FSPTracerImpl(const FSPTracerImpl&) = delete;
  FSPTracerImpl& operator=(const FSPTracerImpl&) = delete;

  FSPTracerImpl(FSPTracerImpl&&) = delete;
  FSPTracerImpl& operator=(FSPTracerImpl&&) = delete;

  FSPResult CaptureSnapshot(lynx::base::geometry::IntSize container_size,
                            uint64_t snapshot_flow_id = 0) override {
    TRACE_EVENT(
        LYNX_TRACE_CATEGORY_FSP, FSP_TRACER_CAPTURE_SNAPSHOT,
        [&](lynx::perfetto::EventContext ctx) {
          auto instance_flow_id = this->flow_id_;
          if (instance_flow_id != 0) {
            ctx.event()->add_flow_ids(instance_flow_id);
          }

          if (snapshot_flow_id != 0) {
            ctx.event()->add_flow_ids(snapshot_flow_id);
          }

          auto* debug = ctx.event()->add_debug_annotations();
          debug->set_name(FSP_DEBUG_SIZE);
          debug->set_string_value(base::FormatString(
              "%dx%d", container_size.Width(), container_size.Height()));
        });

    auto result = static_cast<Derived*>(this)->CaptureSnapshotImpl(
        SwapCurrentSnapshot(), container_size);
    return result;
  }

  void FillSnapshot(FSPSnapshot& snapshot, const FSPContentInfo& content_info,
                    uint64_t snapshot_flow_id = 0) override {
    TRACE_EVENT(
        LYNX_TRACE_CATEGORY_FSP, FSP_TRACER_FILL_SNAPSHOT,
        [&](lynx::perfetto::EventContext ctx) {
          auto instance_flow_id = this->flow_id_;
          if (instance_flow_id != 0) {
            ctx.event()->add_flow_ids(instance_flow_id);
          }

          if (snapshot_flow_id != 0) {
            ctx.event()->add_flow_ids(snapshot_flow_id);
          }

          auto* debug = ctx.event()->add_debug_annotations();
          debug->set_name(FSP_DEBUG_RECT);
          debug->set_string_value(base::FormatString(
              "[%d,%d %dx%d]", content_info.rect.X(), content_info.rect.Y(),
              content_info.rect.Width(), content_info.rect.Height()));

          auto* debug2 = ctx.event()->add_debug_annotations();
          debug2->set_name(FSP_DEBUG_UI_SIGN);
          debug2->set_int_value(content_info.ui_sign);
        });

    assert(std::addressof(snapshot) == current_snapshot_.get());
    if (current_snapshot_) {
      static_cast<Derived*>(this)->FillSnapshotImpl(*current_snapshot_,
                                                    content_info);
    }
  }

  ~FSPTracerImpl() = default;

 protected:
  Config config_;
  uint64_t flow_id_;

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
