// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FLUENCY_HARMONY_FLUENCY_TRACE_HELPER_HARMONY_H_
#define CORE_SERVICES_FLUENCY_HARMONY_FLUENCY_TRACE_HELPER_HARMONY_H_

namespace lynx {
namespace tasm {
namespace fluency {
namespace harmony {
class FluencyTraceHelperHarmony {
 public:
  enum ForceStatus { FORCED_ON, FORCED_OFF, NON_FORCED };
  void SetPageConfigProbability(double probability);
  double GetPageConfigProbability() const { return pageconfig_probability_; }
  bool ShouldSendAllScrollEvent() const;

 private:
  static constexpr double UNKNOWN_FLUENCY_PAGECONFIG_PROBABILITY = -1.0;
  double pageconfig_probability_ = UNKNOWN_FLUENCY_PAGECONFIG_PROBABILITY;
  ForceStatus force_status_ = NON_FORCED;
};
}  // namespace harmony
}  // namespace fluency
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FLUENCY_HARMONY_FLUENCY_TRACE_HELPER_HARMONY_H_
