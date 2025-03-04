// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_PIPELINE_OPTION_H_
#define CORE_PUBLIC_PIPELINE_OPTION_H_

#include <sys/types.h>

#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if ENABLE_TRACE_PERFETTO
#include "base/trace/native/trace_event.h"
#endif

#include "base/include/timer/time_utils.h"

namespace lynx {
namespace tasm {

using PipelineID = std::string;
using PipelineOrigin = std::string;

struct ListItemLifeOption {
  double update_duration() const {
    if (end_update_time_ != 0 && start_update_time_ != 0) {
      return (end_update_time_ - start_update_time_) / 1000.f;
    }
    return 0;
  }

  double render_duration() const {
    if (end_render_time_ != 0 && start_render_time_ != 0) {
      return (end_render_time_ - start_render_time_) / 1000.f;
    }
    return 0;
  }

  double dispatch_duration() const {
    if (end_dispatch_time_ != 0 && start_dispatch_time_ != 0) {
      return (end_dispatch_time_ - start_dispatch_time_) / 1000.f;
    }
    return 0;
  }

  double layout_duration() const {
    if (end_layout_time_ != 0 && start_layout_time_ != 0) {
      return (end_layout_time_ - start_layout_time_) / 1000.f;
    }
    return 0;
  }

  uint64_t start_render_time_{0};
  uint64_t end_render_time_{0};
  uint64_t start_dispatch_time_{0};
  uint64_t end_dispatch_time_{0};
  uint64_t start_update_time_{0};
  uint64_t end_update_time_{0};
  uint64_t start_layout_time_{0};
  uint64_t end_layout_time_{0};
};

struct PipelineOptions {
  // TODO(kechenglong): impl ToLepusValue here.
  // Default constructor that generates a unique PipelineID
  explicit PipelineOptions() {
    pipeline_id =
        PipelineIDGenerator::Instance()->GenerateThreadTimestampPipelineID();
    pipeline_start_timestamp = base::CurrentSystemTimeMicroseconds();
  }
  PipelineID pipeline_id;
  PipelineOrigin pipeline_origin;
  uint64_t pipeline_start_timestamp;
  bool need_timestamps{false};
  int64_t operation_id = 0;
  bool is_first_screen = false;
  // true if triggered by reloadTemplate, used to mark setup timing
  bool is_reload_template = false;
  // true if has layout
  // TODO(heshan):put to a new struct like LayoutResultBundle
  // which may just consumed by FinishLayoutOperation
  bool has_layout = false;
  // true if need call DispatchLayoutUpdates
  bool trigger_layout_ = true;
  // Whether mark entire tree dirty or not.
  bool force_resolve_style_ = false;
  // Whether mark entire tree dirty and reset style sheet or not.
  bool force_update_style_sheet_ = false;
  // This variable records the order of native update data. Used for syncFlush
  // only.
  uint32_t native_update_data_order_ = 0;
  // the component id of list
  int32_t list_comp_id_ = 0;
  // The id of list.
  int32_t list_id_ = 0;
  // The array of operation id in list batch render
  std::vector<int64_t> operation_ids_;
  // The array of list item id in list batch render
  std::vector<int32_t> list_item_ids_;
  // The ids of layout updated list elements.
  mutable std::vector<int32_t> updated_list_elements_;
  mutable ListItemLifeOption list_item_life_option_;
  bool enable_report_list_item_life_statistic_{false};
  // Return true if this pipeline is triggered by render list item.
  bool IsRenderListItem() const {
    return operation_id != 0 && list_id_ != 0 && list_comp_id_ != 0;
  }
#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event) const {
    auto* debug_pipeline_id = event->add_debug_annotations();
    debug_pipeline_id->set_name("pipeline_id");
    debug_pipeline_id->set_string_value(pipeline_id);
    auto* debug_pipeline_origin = event->add_debug_annotations();
    debug_pipeline_origin->set_name("pipeline_origin");
    debug_pipeline_origin->set_string_value(pipeline_origin);
    auto* debug_need_timestamps = event->add_debug_annotations();
    debug_need_timestamps->set_name("need_timestamps");
    debug_need_timestamps->set_string_value(need_timestamps ? "true" : "false");
    auto* debug_operation_id = event->add_debug_annotations();
    debug_operation_id->set_name("operation_id");
    debug_operation_id->set_string_value(std::to_string(operation_id));
    auto* debug_is_first_screen = event->add_debug_annotations();
    debug_is_first_screen->set_name("is_first_screen");
    debug_is_first_screen->set_string_value(is_first_screen ? "true" : "false");
    auto* debug_has_layout = event->add_debug_annotations();
    debug_has_layout->set_name("has_layout");
    debug_has_layout->set_string_value(has_layout ? "true" : "false");
  }
#endif

 private:
  // Helper class to generate pipelineID
  class PipelineIDGenerator {
   public:
    static PipelineIDGenerator* Instance() {
      static thread_local PipelineIDGenerator instance_;
      return &instance_;
    }
    PipelineID GenerateThreadTimestampPipelineID() {
      if (thread_id_prefix_.empty()) {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        thread_id_prefix_ = ss.str() + "_";
      }
      auto pipeline_id =
          thread_id_prefix_ + std::to_string(GetNextPipelineID());
      return pipeline_id;
    };

   private:
    std::string thread_id_prefix_;
    uint64_t pipeline_id_generator_{0};
    uint64_t GetNextPipelineID() { return ++pipeline_id_generator_; }
  };
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_PIPELINE_OPTION_H_
