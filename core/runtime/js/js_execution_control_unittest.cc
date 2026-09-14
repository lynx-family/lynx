// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/js_execution_control.h"

#include <string>
#include <utility>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace js {
namespace test {

namespace {

// A minimal VMInstance whose capture/terminate behaviors can be scripted so we
// can verify the engine-agnostic routing in js_execution_control without
// standing up a real JS engine.
class FakeVMInstance : public VMInstance {
 public:
  JSRuntimeType GetRuntimeType() const override { return JSRuntimeType::v8; }
  std::string GetDebugDescription() const override { return "fake"; }

  bool CaptureJavaScriptStack(
      base::MoveOnlyClosure<void, std::string> callback) override {
    capture_called_ = true;
    if (capture_accepts_) {
      callback(captured_stack_);
    }
    return capture_accepts_;
  }

  bool TerminateJavaScriptExecution() override {
    terminate_called_ = true;
    return terminate_result_;
  }

  bool capture_called_ = false;
  bool capture_accepts_ = true;
  std::string captured_stack_ = "at foo (a.js:1:1)\n";

  bool terminate_called_ = false;
  bool terminate_result_ = true;
};

}  // namespace

TEST(JSExecutionControlTest, CaptureSucceedsForRegisteredVM) {
  FakeVMInstance vm;
  RegisterVMInstance(&vm);

  JSStackCaptureResult result = JSStackCaptureResult::kVMDestroyed;
  std::string stack;
  CaptureJavaScriptStack(&vm, [&](JSStackCaptureResult r, std::string s) {
    result = r;
    stack = std::move(s);
  });

  EXPECT_TRUE(vm.capture_called_);
  EXPECT_EQ(result, JSStackCaptureResult::kSuccess);
  EXPECT_EQ(stack, "at foo (a.js:1:1)\n");

  UnregisterVMInstance(&vm);
}

TEST(JSExecutionControlTest, CaptureReportsVMDestroyedWhenUnregistered) {
  FakeVMInstance vm;
  // Never registered.

  JSStackCaptureResult result = JSStackCaptureResult::kSuccess;
  CaptureJavaScriptStack(
      &vm, [&](JSStackCaptureResult r, std::string) { result = r; });

  EXPECT_FALSE(vm.capture_called_);
  EXPECT_EQ(result, JSStackCaptureResult::kVMDestroyed);
}

TEST(JSExecutionControlTest, CaptureReportsStackUnavailableWhenVMDeclines) {
  FakeVMInstance vm;
  vm.capture_accepts_ = false;
  RegisterVMInstance(&vm);

  JSStackCaptureResult result = JSStackCaptureResult::kSuccess;
  CaptureJavaScriptStack(
      &vm, [&](JSStackCaptureResult r, std::string) { result = r; });

  EXPECT_TRUE(vm.capture_called_);
  EXPECT_EQ(result, JSStackCaptureResult::kStackUnavailable);

  UnregisterVMInstance(&vm);
}

TEST(JSExecutionControlTest, CaptureAfterUnregisterReportsVMDestroyed) {
  FakeVMInstance vm;
  RegisterVMInstance(&vm);
  UnregisterVMInstance(&vm);

  JSStackCaptureResult result = JSStackCaptureResult::kSuccess;
  CaptureJavaScriptStack(
      &vm, [&](JSStackCaptureResult r, std::string) { result = r; });

  EXPECT_FALSE(vm.capture_called_);
  EXPECT_EQ(result, JSStackCaptureResult::kVMDestroyed);
}

TEST(JSExecutionControlTest, TerminateRoutesToRegisteredVM) {
  FakeVMInstance vm;
  vm.terminate_result_ = true;
  RegisterVMInstance(&vm);

  EXPECT_TRUE(TerminateJavaScriptExecution(&vm));
  EXPECT_TRUE(vm.terminate_called_);

  UnregisterVMInstance(&vm);
}

TEST(JSExecutionControlTest, TerminateReturnsVMResult) {
  FakeVMInstance vm;
  vm.terminate_result_ = false;
  RegisterVMInstance(&vm);

  EXPECT_FALSE(TerminateJavaScriptExecution(&vm));
  EXPECT_TRUE(vm.terminate_called_);

  UnregisterVMInstance(&vm);
}

TEST(JSExecutionControlTest, TerminateIgnoresUnregisteredVM) {
  FakeVMInstance vm;
  // Never registered: must not dereference the possibly-dangling pointer.

  EXPECT_FALSE(TerminateJavaScriptExecution(&vm));
  EXPECT_FALSE(vm.terminate_called_);
}

TEST(JSExecutionControlTest, StackCaptureResultValuesMatchPlatformContract) {
  // These values are forwarded directly as the Java callback status, so they
  // must stay stable.
  EXPECT_EQ(static_cast<int32_t>(JSStackCaptureResult::kSuccess), 0);
  EXPECT_EQ(static_cast<int32_t>(JSStackCaptureResult::kStackUnavailable), 2);
  EXPECT_EQ(static_cast<int32_t>(JSStackCaptureResult::kVMDestroyed), 3);
}

}  // namespace test
}  // namespace js
}  // namespace runtime
}  // namespace lynx
