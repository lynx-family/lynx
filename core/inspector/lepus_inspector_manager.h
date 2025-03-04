// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_LEPUS_INSPECTOR_MANAGER_H_
#define CORE_INSPECTOR_LEPUS_INSPECTOR_MANAGER_H_

#include <memory>

namespace lynx {
namespace lepus {
class InspectorLepusObserver;
class Context;

class LepusInspectorManager {
 public:
  virtual ~LepusInspectorManager() = default;

  virtual void InitInspector(
      Context* context,
      const std::shared_ptr<InspectorLepusObserver>& observer) = 0;
  virtual void DestroyInspector() = 0;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_INSPECTOR_LEPUS_INSPECTOR_MANAGER_H_
