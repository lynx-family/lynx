// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/fluency/harmony/fluency_trace_helper_harmony.h"

#include <random>

#include "core/renderer/utils/lynx_env.h"
#include "core/services/fluency/fluency_tracer.h"

namespace lynx {
namespace tasm {
namespace fluency {
namespace harmony {

void FluencyTraceHelperHarmony::SetPageConfigProbability(double probability) {
  pageconfig_probability_ = probability;
  if (probability >= 0) {
    std::default_random_engine random_engine(static_cast<unsigned int>(
        std::chrono::system_clock::now().time_since_epoch().count()));
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    if (distribution(random_engine) <= probability) {
      force_status_ = FORCED_ON;
    } else {
      force_status_ = FORCED_OFF;
    }
  }
}

bool FluencyTraceHelperHarmony::ShouldSendAllScrollEvent() const {
  if (force_status_ == NON_FORCED) {
    // use settings
    return tasm::FluencyTracer::IsEnable();
  }
  return force_status_ == FORCED_ON;
}

}  // namespace harmony
}  // namespace fluency
}  // namespace tasm
}  // namespace lynx
