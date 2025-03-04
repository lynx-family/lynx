// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_RECEIVER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_RECEIVER_H_

#include <memory>
#include <string>
#include <utility>

namespace lynx {
namespace devtool {

class MockReceiver {
 public:
  static MockReceiver& GetInstance() {
    static MockReceiver inst;
    return inst;
  }

  MockReceiver(const MockReceiver&) = delete;
  MockReceiver& operator=(const MockReceiver&) = delete;

  void Pull() {
    url_ = "";
    has_plug_ = false;
  }

  int32_t Plug(const std::string& url) {
    session_id_++;
    url_ = url;
    has_plug_ = true;
    return session_id_;
  }

  void OnMessage(const std::string& type, const std::string& msg) {
    received_message_ = std::make_pair(type, msg);
  }

  void OnMethodCalled(const std::string& json) { received_json_ = json; }

  void ResetAll() {
    url_ = "";
    received_message_ = {"", ""};
    received_json_ = "";
    has_plug_ = false;
  }

  std::string url_;
  bool has_plug_ = false;
  std::pair<std::string, std::string> received_message_;
  std::string received_json_;

 private:
  MockReceiver() = default;
  int32_t session_id_ = 0;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_RECEIVER_H_
