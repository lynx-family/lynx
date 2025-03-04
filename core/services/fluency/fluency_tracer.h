// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FLUENCY_FLUENCY_TRACER_H_
#define CORE_SERVICES_FLUENCY_FLUENCY_TRACER_H_

#include <stdlib.h>

#include <atomic>
#include <vector>

namespace lynx {
namespace tasm {
class FluencyTracer {
 public:
  static void SetForceEnable(bool b);
  static void SetNeedCheck();
  static bool IsEnable();
  void Trigger(int64_t time_stamp);

 private:
  void ReportFluency(double total_dur);
  static std::atomic<bool> need_check_;
  static std::atomic<bool> force_enable_;
  static std::atomic<bool> enable_;
  std::vector<double> frames_dur_;
  int64_t last_timestamp_ = 0;
  int64_t start_timestamp_ = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FLUENCY_FLUENCY_TRACER_H_
