// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_WHITE_BOARD_INSPECTOR_DELEGATE_MOCK_H_
#define DEVTOOL_TESTING_MOCK_WHITE_BOARD_INSPECTOR_DELEGATE_MOCK_H_

#include <string>

#include "devtool/lynx_devtool/shared_data/white_board_inspector_delegate.h"

namespace lynx {
namespace devtool {
namespace testing {

class WhiteBoardInspectorDelegateMock : public WhiteBoardInspectorDelegate {
 public:
  explicit WhiteBoardInspectorDelegateMock(int view_id)
      : WhiteBoardInspectorDelegate(view_id) {}
  ~WhiteBoardInspectorDelegateMock() override = default;

  void SendEvent(const Json::Value& msg) override {
    event_message_ = msg.toStyledString();
  }

 private:
  std::string event_message_;
};

}  // namespace testing
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_WHITE_BOARD_INSPECTOR_DELEGATE_MOCK_H_
