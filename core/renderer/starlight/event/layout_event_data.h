// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_DATA_H_
#define CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_DATA_H_

#include <string>

namespace lynx {
namespace starlight {

class LayoutEventData {};

class LayoutErrorData : public LayoutEventData {
 public:
  LayoutErrorData(const std::string& error_msg,
                  const std::string& fix_suggestion)
      : error_msg_(error_msg), fix_suggestion_(fix_suggestion) {}
  ~LayoutErrorData() = default;

  const std::string& getErrorMsg() const { return error_msg_; }
  const std::string& getFixSuggestion() const { return fix_suggestion_; }

 private:
  std::string error_msg_;
  std::string fix_suggestion_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_DATA_H_
