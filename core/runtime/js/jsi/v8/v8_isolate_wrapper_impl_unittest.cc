// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "base/include/fml/message_loop.h"
#include "core/runtime/js/jsi/jsi_unittest.h"
#include "core/runtime/js/jsi/v8/v8_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace js {
namespace test {

TEST(V8IsolateWrapperImplTest, AsyncWebAssemblyInstantiationCompletes) {
  using namespace std::chrono_literals;

  // Async Wasm compilation completes on a worker thread, then posts its
  // resolution task to the isolate's foreground task runner. The test only
  // pumps fml, matching how the Lynx JS thread runs in production.
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto runtime = MakeRuntimeFactory<V8Runtime>(nullptr);
  Scope scope(*runtime);
  auto webassembly = runtime->global().getProperty(*runtime, "WebAssembly");
  ASSERT_TRUE(webassembly.has_value());
  if (webassembly->isUndefined()) {
    GTEST_SKIP() << "WebAssembly is disabled by this V8 configuration";
  }

  auto result = runtime->evaluateJavaScript(std::make_shared<StringBuffer>(R"(
        globalThis.wasmState = 'pending';
        const bytes = new Uint8Array([
          0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
          0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7f,
          0x03, 0x02, 0x01, 0x00,
          0x07, 0x0a, 0x01, 0x06, 0x61, 0x6e, 0x73, 0x77, 0x65, 0x72,
          0x00, 0x00,
          0x0a, 0x06, 0x01, 0x04, 0x00, 0x41, 0x2a, 0x0b
        ]);
        WebAssembly.instantiate(bytes).then(
          ({instance}) => {
            globalThis.wasmResult = instance.exports.answer();
            globalThis.wasmState = 'fulfilled';
          },
          (error) => {
            globalThis.wasmState = `rejected: ${error}`;
          });
      )"),
                                            "webassembly_async_test.js");
  ASSERT_TRUE(result.has_value());

  constexpr auto kTimeout = 5s;
  constexpr auto kPollInterval = 10ms;
  const auto deadline = std::chrono::steady_clock::now() + kTimeout;
  std::string state;
  do {
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
    auto state_value = runtime->global().getProperty(*runtime, "wasmState");
    ASSERT_TRUE(state_value.has_value());
    state = state_value->getString(*runtime).utf8(*runtime);
    if (state != "pending") {
      break;
    }
    std::this_thread::sleep_for(kPollInterval);
  } while (std::chrono::steady_clock::now() < deadline);

  EXPECT_EQ(state, "fulfilled");
  auto wasm_result = runtime->global().getProperty(*runtime, "wasmResult");
  ASSERT_TRUE(wasm_result.has_value());
  EXPECT_EQ(wasm_result->getNumber(), 42);
}

}  // namespace test
}  // namespace js
}  // namespace runtime
}  // namespace lynx
