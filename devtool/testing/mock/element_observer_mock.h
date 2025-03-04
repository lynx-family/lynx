// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_ELEMENT_OBSERVER_MOCK_H_
#define DEVTOOL_TESTING_MOCK_ELEMENT_OBSERVER_MOCK_H_

#include <memory>

#include "devtool/lynx_devtool/agent/inspector_element_observer_impl.h"
#include "devtool/testing/utils/method_tracker.h"

namespace lynx {
namespace testing {

class PostObserver : public lynx::devtool::InspectorElementObserverImpl {
 public:
  explicit PostObserver(
      const std::shared_ptr<lynx::devtool::InspectorTasmExecutor>
          &element_executor)
      : lynx::devtool::InspectorElementObserverImpl(element_executor) {}

  void OnElementNodeAdded(lynx::tasm::Element *ptr) override {
    auto &tracker = MethodTracker::GetInstance();
    tracker.CallMethod(MethodTracker::MethodName::ON_EVENT_NODE_ADDED, true);
    InspectorElementObserverImpl::OnElementNodeAdded(ptr);
  }
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_ELEMENT_OBSERVER_MOCK_H_
