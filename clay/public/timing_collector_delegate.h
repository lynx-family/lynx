// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_PUBLIC_TIMING_COLLECTOR_DELEGATE_H_
#define CLAY_PUBLIC_TIMING_COLLECTOR_DELEGATE_H_

#include <string>
#include <utility>
#include <vector>

namespace clay {

class TimingCollectorDelegate {
 public:
  virtual ~TimingCollectorDelegate() = default;
  virtual bool HasPipelineIds() const = 0;
  virtual std::vector<std::string> GetPipelineIds() const = 0;
  virtual void OnPipelineEnd(
      std::vector<std::pair<std::string, uint64_t>> timings,
      std::vector<std::string> pipeline_id_list) = 0;
};

}  // namespace clay

#endif  // CLAY_PUBLIC_TIMING_COLLECTOR_DELEGATE_H_
