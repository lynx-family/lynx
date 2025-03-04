// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_INSPECTOR_CLIENT_NG_MOCK_H_
#define DEVTOOL_TESTING_MOCK_INSPECTOR_CLIENT_NG_MOCK_H_

#include <memory>
#include <queue>
#include <string>

#include "devtool/fundamentals/js_inspect/inspector_client_ng.h"

namespace lynx {
namespace testing {

class InspectorClientNGMock : public devtool::InspectorClientNG {
 public:
  InspectorClientNGMock() = default;
  ~InspectorClientNGMock() override = default;

  void SetStopAtEntry(bool stop_at_entry, int instance_id) override {
    stop_at_entry_ = stop_at_entry;
  }
  void DispatchMessage(const std::string& message, int instance_id) override {
    message_queue_.push(message);
  }

 private:
  bool stop_at_entry_{false};
  std::queue<std::string> message_queue_;
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_INSPECTOR_CLIENT_NG_MOCK_H_
