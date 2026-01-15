// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus_context/bindings/resource/response_handler_in_lepus.h"

#include "core/runtime/common/bindings/resource/response_promise.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class TestDelegate : public runtime::ResponseHandlerProxy::Delegate {
 public:
  void InvokeResponsePromiseCallback(base::closure closure) override {
    called_ = true;
    if (closure) {
      closure();
    }
  }

  bool called() const { return called_; }

 private:
  bool called_ = false;
};

TEST(ResponseHandlerInLepusTest, ListenerFiresWithoutHoldingHandler) {
  TestDelegate delegate;
  auto promise =
      std::make_shared<runtime::ResponsePromise<tasm::BundleResourceInfo>>();
  bool resource_callback_called = false;
  {
    auto handler = fml::MakeRefCounted<ResponseHandlerInLepus>(
        delegate, /*url=*/"test-url", promise);

    handler->AddResourceListener(
        [&resource_callback_called](tasm::BundleResourceInfo info) {
          resource_callback_called = true;
        });

    // Release handler here.
  }

  tasm::BundleResourceInfo info;
  info.url = "test-url";
  info.code = 200;
  promise->SetValue(info);

  EXPECT_TRUE(delegate.called());
  EXPECT_TRUE(resource_callback_called);
}

}  // namespace tasm
}  // namespace lynx
