// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_QUICKJS_DELEGATE_H_
#define DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_QUICKJS_DELEGATE_H_

#include <string>

namespace lynx {
namespace devtool {

// Declare the interfaces only called by Quickjs.
class InspectorClientQJSDelegate {
 public:
  virtual void OnConsoleMessage(const std::string& message, int instance_id,
                                int runtime_id) {}
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_QUICKJS_DELEGATE_H_
