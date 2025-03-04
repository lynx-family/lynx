// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_OBSERVER_INSPECTOR_COMMON_OBSERVER_H_
#define CORE_INSPECTOR_OBSERVER_INSPECTOR_COMMON_OBSERVER_H_

#include <string>

namespace lynx {
namespace tasm {

class InspectorCommonObserver {
 public:
  virtual ~InspectorCommonObserver() noexcept = default;

  virtual void EndReplayTest(const std::string& file_path) = 0;
  virtual void SendLayoutTree() = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_INSPECTOR_OBSERVER_INSPECTOR_COMMON_OBSERVER_H_
