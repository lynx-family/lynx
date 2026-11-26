// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_COMMON_SERVICES_ERROR_REPORTING_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_ERROR_REPORTING_SERVICE_H_

#include <deque>
#include <functional>

#include "base/include/debug/lynx_error.h"
#include "clay/common/service/service.h"

namespace clay {

// Manually registered, per-engine error delivery on the UI thread.
// Owner::kUI declares thread affinity; direct calls do not switch threads.
// Use the static ReportError overload to report from any thread.
class ErrorReportingService : public Service<ErrorReportingService, Owner::kUI,
                                             ServiceFlags::kManualRegister> {
 public:
  using ErrorCallback = std::function<void(const lynx::base::LynxError&)>;

  // May be called from any thread. Delivers errors on the UI thread.
  // Does nothing if this service is not registered. The manager must remain
  // valid for the duration of this call; posted work does not capture it.
  static void ReportError(ServiceManager& manager, lynx::base::LynxError error);

  // UI thread only. Queues the error until a callback is available.
  void ReportError(lynx::base::LynxError error);
  // UI thread only. Binds the callback and drains pending errors when possible.
  void SetErrorCallback(ErrorCallback callback);
  // UI thread only. Clears the callback and pending errors, and rejects future
  // errors and callback changes. Repeated calls are safe.
  void Close();

 private:
  void DrainErrors();
  void OnDestroy() override;

  ErrorCallback callback_;
  std::deque<lynx::base::LynxError> pending_errors_;
  bool dispatching_ = false;
  bool closed_ = false;
};

}  // namespace clay
#endif  // CLAY_SHELL_COMMON_SERVICES_ERROR_REPORTING_SERVICE_H_
