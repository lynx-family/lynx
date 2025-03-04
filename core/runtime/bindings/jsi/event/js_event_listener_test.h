// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_EVENT_JS_EVENT_LISTENER_TEST_H_
#define CORE_RUNTIME_BINDINGS_JSI_EVENT_JS_EVENT_LISTENER_TEST_H_

#include <memory>
#include <utility>

#include "core/runtime/bindings/jsi/event/js_event_listener.h"
#include "core/runtime/jsi/jsi_unittest.h"
#include "core/runtime/piper/js/mock_template_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {

class JSClosureEventListenerTest : public JSITestBase {
 public:
  JSClosureEventListenerTest() = default;
  ~JSClosureEventListenerTest() override = default;

  void SetUp() override;

  void TearDown() override {}

 protected:
  std::shared_ptr<App> app_;
  runtime::test::MockTemplateDelegate delegate_;
};

}  // namespace test
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_EVENT_JS_EVENT_LISTENER_TEST_H_
