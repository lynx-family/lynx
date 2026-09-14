// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/js_execution_control.h"

#include <string>
#include <utility>

#include "base/include/fml/synchronization/count_down_latch.h"
#include "core/base/threading/task_runner_manufactor.h"
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

void RegisterOnJSRunner(const std::string& group_name,
                        const std::shared_ptr<FakeVMInstance>& vm) {
  fml::CountDownLatch latch(1);
  base::TaskRunnerManufactor::GetJSRunner(group_name)->PostTask([vm, &latch] {
    RegisterVMInstance(vm);
    latch.CountDown();
  });
  latch.Wait();
}

void UnregisterOnJSRunner(const std::string& group_name, FakeVMInstance* vm) {
  fml::CountDownLatch latch(1);
  base::TaskRunnerManufactor::GetJSRunner(group_name)->PostTask([vm, &latch] {
    UnregisterVMInstance(vm);
    latch.CountDown();
  });
  latch.Wait();
}

}  // namespace

TEST(JSExecutionControlTest, CaptureSucceedsForRegisteredVM) {
  auto vm = std::make_shared<FakeVMInstance>();
  const std::string group_name = "execution-control-capture-success";
  RegisterOnJSRunner(group_name, vm);

  JSStackCaptureResult result = JSStackCaptureResult::kVMDestroyed;
  std::string stack;
  CaptureJavaScriptStack(group_name,
                         [&](JSStackCaptureResult r, std::string s) {
                           result = r;
                           stack = std::move(s);
                         });

  EXPECT_TRUE(vm->capture_called_);
  EXPECT_EQ(result, JSStackCaptureResult::kSuccess);
  EXPECT_EQ(stack, "at foo (a.js:1:1)\n");

  UnregisterOnJSRunner(group_name, vm.get());
}

TEST(JSExecutionControlTest, CaptureReportsVMDestroyedWhenUnregistered) {
  auto vm = std::make_shared<FakeVMInstance>();
  const std::string group_name = "execution-control-capture-unregistered";
  base::TaskRunnerManufactor::GetJSRunner(group_name);
  // Never registered.

  JSStackCaptureResult result = JSStackCaptureResult::kSuccess;
  CaptureJavaScriptStack(
      group_name, [&](JSStackCaptureResult r, std::string) { result = r; });

  EXPECT_FALSE(vm->capture_called_);
  EXPECT_EQ(result, JSStackCaptureResult::kVMDestroyed);
}

TEST(JSExecutionControlTest, CaptureReportsStackUnavailableWhenVMDeclines) {
  auto vm = std::make_shared<FakeVMInstance>();
  vm->capture_accepts_ = false;
  const std::string group_name = "execution-control-capture-declines";
  RegisterOnJSRunner(group_name, vm);

  JSStackCaptureResult result = JSStackCaptureResult::kSuccess;
  CaptureJavaScriptStack(
      group_name, [&](JSStackCaptureResult r, std::string) { result = r; });

  EXPECT_TRUE(vm->capture_called_);
  EXPECT_EQ(result, JSStackCaptureResult::kStackUnavailable);

  UnregisterOnJSRunner(group_name, vm.get());
}

TEST(JSExecutionControlTest, CaptureAfterUnregisterReportsVMDestroyed) {
  auto vm = std::make_shared<FakeVMInstance>();
  const std::string group_name = "execution-control-capture-destroyed";
  RegisterOnJSRunner(group_name, vm);
  UnregisterOnJSRunner(group_name, vm.get());

  JSStackCaptureResult result = JSStackCaptureResult::kSuccess;
  CaptureJavaScriptStack(
      group_name, [&](JSStackCaptureResult r, std::string) { result = r; });

  EXPECT_FALSE(vm->capture_called_);
  EXPECT_EQ(result, JSStackCaptureResult::kVMDestroyed);
}

TEST(JSExecutionControlTest, UnregisterDoesNotEraseReplacementVM) {
  auto old_vm = std::make_shared<FakeVMInstance>();
  auto replacement_vm = std::make_shared<FakeVMInstance>();
  const std::string group_name = "execution-control-replacement";
  RegisterOnJSRunner(group_name, old_vm);
  RegisterOnJSRunner(group_name, replacement_vm);

  UnregisterOnJSRunner(group_name, old_vm.get());

  EXPECT_TRUE(TerminateJavaScriptExecution(group_name));
  EXPECT_FALSE(old_vm->terminate_called_);
  EXPECT_TRUE(replacement_vm->terminate_called_);

  UnregisterOnJSRunner(group_name, replacement_vm.get());
}

TEST(JSExecutionControlTest, TerminateRoutesToRegisteredVM) {
  auto vm = std::make_shared<FakeVMInstance>();
  vm->terminate_result_ = true;
  const std::string group_name = "execution-control-terminate-success";
  RegisterOnJSRunner(group_name, vm);

  EXPECT_TRUE(TerminateJavaScriptExecution(group_name));
  EXPECT_TRUE(vm->terminate_called_);

  UnregisterOnJSRunner(group_name, vm.get());
}

TEST(JSExecutionControlTest, TerminateReturnsVMResult) {
  auto vm = std::make_shared<FakeVMInstance>();
  vm->terminate_result_ = false;
  const std::string group_name = "execution-control-terminate-false";
  RegisterOnJSRunner(group_name, vm);

  EXPECT_FALSE(TerminateJavaScriptExecution(group_name));
  EXPECT_TRUE(vm->terminate_called_);

  UnregisterOnJSRunner(group_name, vm.get());
}

TEST(JSExecutionControlTest, TerminateIgnoresUnregisteredVM) {
  auto vm = std::make_shared<FakeVMInstance>();
  const std::string group_name = "execution-control-terminate-unregistered";
  base::TaskRunnerManufactor::GetJSRunner(group_name);
  // Never registered: must not dereference the possibly-dangling pointer.

  EXPECT_FALSE(TerminateJavaScriptExecution(group_name));
  EXPECT_FALSE(vm->terminate_called_);
}

TEST(JSExecutionControlTest, CaptureCallbackCanTerminateExecution) {
  auto vm = std::make_shared<FakeVMInstance>();
  const std::string group_name = "execution-control-reentrant-terminate";
  RegisterOnJSRunner(group_name, vm);

  bool terminated = false;
  CaptureJavaScriptStack(
      group_name, [&](JSStackCaptureResult result, std::string) {
        EXPECT_EQ(result, JSStackCaptureResult::kSuccess);
        terminated = TerminateJavaScriptExecution(group_name);
      });

  EXPECT_TRUE(terminated);
  EXPECT_TRUE(vm->terminate_called_);
  UnregisterOnJSRunner(group_name, vm.get());
}

TEST(JSExecutionControlTest, ExpiredVMReportsDestroyed) {
  const std::string group_name = "execution-control-expired";
  {
    auto vm = std::make_shared<FakeVMInstance>();
    RegisterOnJSRunner(group_name, vm);
  }

  JSStackCaptureResult result = JSStackCaptureResult::kSuccess;
  CaptureJavaScriptStack(
      group_name, [&](JSStackCaptureResult r, std::string) { result = r; });

  EXPECT_EQ(result, JSStackCaptureResult::kVMDestroyed);
  EXPECT_FALSE(TerminateJavaScriptExecution(group_name));
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
