// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_PERF_CONTROLLER_CLAY_H_
#define CLAY_LYNX_ADAPTOR_PERF_CONTROLLER_CLAY_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/concurrent_queue.h"
#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "clay/shell/common/pipeline_timing_delegate.h"
#include "core/public/perf_controller_proxy.h"

namespace lynx {
namespace tasm {

class PerfControllerClay
    : public std::enable_shared_from_this<PerfControllerClay>,
      public clay::PipelineTimingDelegate {
 public:
  PerfControllerClay(
      const std::shared_ptr<shell::PerfControllerProxy>& controller,
      int32_t instance_id);
  ~PerfControllerClay() override = default;

  void SetUITaskRunner(fml::RefPtr<fml::TaskRunner> ui_task_runner) {
    ui_task_runner_ = ui_task_runner;
  }

  /**
   * @brief Mark paint-end timing event
   * this interface can only be called in the UI thread.
   */
  void OnPaintEnd();

  /**
   * @brief Set the pipeline id to be processed, which will be cached inside
   * this interface can only be called in the UI thread.
   */
  void SetNeedMarkPaintEndTiming(const tasm::PipelineID& pipeline_id);

  /**
   * @brief Check if there are any pipeline ids to be processed.
   * Overrides PipelineTimingDelegate::HasPipelineIds.
   * this interface can only be called in the UI thread.
   */
  bool HasPipelineIds() const override {
    DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
    return !pipeline_id_list_.empty();
  }

  /**
   * @brief Get cached pipeline ids to be process.
   * Overrides PipelineTimingDelegate::GetPipelineIds.
   * this interface can only be called in the UI thread.
   */
  std::vector<tasm::PipelineID> GetPipelineIds() const override {
    DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
    return pipeline_id_list_;
  }

  /**
   * @brief Mark pipeline-end event with additional clay timing events.
   * Overrides PipelineTimingDelegate::OnPipelineEnd.
   * this interface can be called in the UI or raster thread.
   * @param timings The timing data collected by clay
   * @param pipeline_id_list pipelineIDs associated with the timing event.
   */
  void OnPipelineEnd(std::vector<std::pair<std::string, uint64_t>> timings,
                     std::vector<std::string> pipeline_id_list) override;

 private:
  const std::shared_ptr<shell::PerfControllerProxy> perf_controller_proxy_;

  fml::RefPtr<fml::TaskRunner> ui_task_runner_;

  std::vector<tasm::PipelineID> pipeline_id_list_;
  std::atomic<int32_t> instance_id_ = {-1};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_PERF_CONTROLLER_CLAY_H_
