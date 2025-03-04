// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_UNITTEST_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_UNITTEST_H_

#include <memory>
#include <string>

#include "devtool/base_devtool/native/js_inspect/inspector_client_delegate_base_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class MockInspectorClientDelegateBaseImpl
    : public InspectorClientDelegateBaseImpl {
 public:
  MockInspectorClientDelegateBaseImpl(const std::string& vm_type)
      : InspectorClientDelegateBaseImpl(vm_type) {}

  void SendResponse(const std::string& message, int instance_id) override {}

  void PostTask(int instance_id, std::function<void()>&& closure) override {}
};

}  // namespace testing
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_UNITTEST_H_
