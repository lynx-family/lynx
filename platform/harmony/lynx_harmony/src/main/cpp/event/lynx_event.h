// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_LYNX_EVENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_LYNX_EVENT_H_

#include <string>
#include <utility>

namespace lynx {
namespace tasm {
namespace harmony {

enum class LynxEventType { kNone, kTouch, kMouse, kWheel, kKeyboard, kCustom };

class LynxEvent {
 public:
  LynxEvent(int id, std::string name, LynxEventType type)
      : id_(id), name_(std::move(name)), type_(type) {
    time_stamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
  }

  int ID() const { return id_; }

  const std::string& Name() const { return name_; }

  LynxEventType EventType() const { return type_; }

  void SetTimeStamp(long long time_stamp) { time_stamp_ = time_stamp; }

 private:
  int id_{-1};
  std::string name_;
  LynxEventType type_{LynxEventType::kNone};
  long long time_stamp_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_LYNX_EVENT_H_
