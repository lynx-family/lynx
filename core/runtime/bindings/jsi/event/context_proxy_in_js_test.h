// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_EVENT_CONTEXT_PROXY_IN_JS_TEST_H_
#define CORE_RUNTIME_BINDINGS_JSI_EVENT_CONTEXT_PROXY_IN_JS_TEST_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "core/runtime/bindings/jsi/event/context_proxy_in_js.h"
#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/bindings/jsi/lynx.h"
#include "core/runtime/jsi/jsi_unittest.h"
#include "core/runtime/piper/js/mock_template_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {

class JSRuntimeTestMockDelegate : public runtime::test::MockTemplateDelegate {
 public:
  event::DispatchEventResult DispatchMessageEvent(
      runtime::MessageEvent event) override;

  void ClearResult();
  std::string DumpResult();

  std::stringstream ss_;
};

class JSRuntimeTestMockJSApp : public HostObject {
 public:
  JSRuntimeTestMockJSApp(std::shared_ptr<Runtime> rt) : rt_(rt) {}
  ~JSRuntimeTestMockJSApp() = default;

  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual void set(Runtime*, const PropNameID& name,
                   const Value& value) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

  size_t call_count{0};
  std::vector<size_t> count_ary{0};
  std::vector<std::vector<piper::Value>> args_ary;
  std::weak_ptr<Runtime> rt_;
};

class ContextProxyInJSTest : public JSITestBase {
 public:
  ContextProxyInJSTest() = default;
  ~ContextProxyInJSTest() override = default;

  void SetUp() override;
  void TearDown() override {}

  std::shared_ptr<JSRuntimeTestMockJSApp> mock_js_app_;
  std::shared_ptr<App> app_;
  std::shared_ptr<LynxProxy> lynx_proxy_;

  JSRuntimeTestMockDelegate delegate_;
};

}  // namespace test
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_EVENT_CONTEXT_PROXY_IN_JS_TEST_H_
