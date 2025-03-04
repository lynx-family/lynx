// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_TRACING_FRAME_TRACE_SERVICE_H_
#define DEVTOOL_LYNX_DEVTOOL_TRACING_FRAME_TRACE_SERVICE_H_

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include <string>

#include "base/include/fml/thread.h"

namespace lynx {
namespace trace {

class FrameTraceService
    : public std::enable_shared_from_this<FrameTraceService> {
 public:
  FrameTraceService();
  ~FrameTraceService() = default;
  void SendScreenshots(const std::string& snapshot);
  void SendFPSData(const uint64_t& startTime, const uint64_t& endTime);
  void Initialize();

 private:
  void FPSTrace(const uint64_t& startTime, const uint64_t& endTime);
  void Screenshots(const std::string& snapshot);

 private:
  fml::Thread thread_;
};

}  // namespace trace
}  // namespace lynx
#endif
#endif  // DEVTOOL_LYNX_DEVTOOL_TRACING_FRAME_TRACE_SERVICE_H_
