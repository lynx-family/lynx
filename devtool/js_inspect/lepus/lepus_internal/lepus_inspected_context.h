// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_H_

#include <memory>
#include <string>

namespace lepus_inspector {

class LepusInspectedContext
    : public std::enable_shared_from_this<LepusInspectedContext> {
 public:
  LepusInspectedContext() = default;
  virtual ~LepusInspectedContext() = default;

  virtual void SetDebugInfo(const std::string& url,
                            const std::string& debug_info) = 0;
  virtual void ProcessMessage(const std::string& message) = 0;

 private:
};

}  // namespace lepus_inspector

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTED_CONTEXT_H_
