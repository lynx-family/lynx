// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_CONSOLE_MESSAGE_POSTMAN_H_
#define CORE_INSPECTOR_CONSOLE_MESSAGE_POSTMAN_H_

#include <memory>
#include <string>

namespace lynx {
namespace piper {
class InspectorRuntimeObserverNG;

struct ConsoleMessage {
  ConsoleMessage(const std::string& text, int32_t level, int64_t timestamp)
      : text_(text), level_(level), timestamp_(timestamp){};
  std::string text_;
  int32_t level_;
  int64_t timestamp_;
};

class ConsoleMessagePostMan {
 public:
  ConsoleMessagePostMan() = default;
  virtual ~ConsoleMessagePostMan() = default;

  virtual void OnMessagePosted(const ConsoleMessage& message) = 0;
  virtual void InsertRuntimeObserver(
      const std::shared_ptr<InspectorRuntimeObserverNG>& observer) = 0;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_INSPECTOR_CONSOLE_MESSAGE_POSTMAN_H_
