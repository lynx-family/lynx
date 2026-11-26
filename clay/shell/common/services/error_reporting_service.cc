// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/shell/common/services/error_reporting_service.h"

#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/include/fml/task_runner.h"
#include "clay/common/service/service_manager.h"

namespace clay {

void ErrorReportingService::ReportError(ServiceManager& manager,
                                        lynx::base::LynxError error) {
  auto actor = manager.GetService<ErrorReportingService>();
  if (!actor) {
    return;
  }
  fml::TaskRunner::RunNowOrPostTask(
      manager.GetTaskRunners()->SelectTaskRunner<Owner::kUI>(),
      fml::MakeCopyable(
          [actor = std::move(actor), error = std::move(error)]() mutable {
            Puppet<Owner::kUI, ErrorReportingService> errors = std::move(actor);
            errors.Act([error = std::move(error)](auto& service) mutable {
              service.ReportError(std::move(error));
            });
          }));
}

void ErrorReportingService::ReportError(lynx::base::LynxError error) {
  if (closed_) {
    return;
  }
  pending_errors_.emplace_back(std::move(error));
  DrainErrors();
}

void ErrorReportingService::SetErrorCallback(ErrorCallback callback) {
  if (closed_) {
    return;
  }
  callback_ = std::move(callback);
  DrainErrors();
}

void ErrorReportingService::DrainErrors() {
  if (dispatching_) {
    return;
  }
  dispatching_ = true;
  while (!closed_ && callback_ && !pending_errors_.empty()) {
    auto error = std::move(pending_errors_.front());
    pending_errors_.pop_front();
    // Keep the active callback alive if it replaces itself or closes the
    // service.
    auto callback = callback_;
    callback(error);
  }
  dispatching_ = false;
}

void ErrorReportingService::Close() {
  closed_ = true;
  callback_ = nullptr;
  pending_errors_.clear();
}

void ErrorReportingService::OnDestroy() { Close(); }

}  // namespace clay
