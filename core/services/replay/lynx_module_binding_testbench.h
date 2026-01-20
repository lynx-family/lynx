// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_LYNX_MODULE_BINDING_TESTBENCH_H_
#define CORE_SERVICES_REPLAY_LYNX_MODULE_BINDING_TESTBENCH_H_

#include <set>
#include <string>

#include "core/runtime/js/bindings/modules/lynx_module.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace runtime {
namespace js {
class LynxJSIModuleBindingTestBench : public HostObject {
 public:
  explicit LynxJSIModuleBindingTestBench(
      const LynxModuleProviderFunction &moduleProvider);
  ~LynxJSIModuleBindingTestBench() override = default;

  Value get(Runtime *rt, const PropNameID &prop) override;

  void setLynxModuleManagerPtr(const LynxJSIModuleBindingPtr moduleProvider);
  LynxJSIModuleBindingPtr getLynxModuleManagerPtr();

 private:
  // replay module Manager
  LynxModuleProviderFunction moduleProvider_;
  // lynx module Manager's ptr
  LynxJSIModuleBindingPtr moduleBindingPtrLynx_;

  // these modules will be called by moduleBindingPtrLynx_
  std::set<std::string> lynxModuleSet{
      // clang-format off
      "LynxRecorderReplayDataModule",
      "LynxUIMethodModule",
      "NavigationModule",
      "IntersectionObserverModule",
      "LynxSetModule",
      "NetworkingModule",
      "BDLynxModule",
      "JSBTestModule",
      "LynxResourceModule",
      "LynxAccessibilityModule",
      "LynxExposureModule",
      "LynxTestModule",
      "LynxConfigModule",
      "LynxFocusModule",
      // clang-format on
  };
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_LYNX_MODULE_BINDING_TESTBENCH_H_
