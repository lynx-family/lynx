// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/lynx_actor.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/thread.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

fml::RefPtr<fml::TaskRunner> GetHookTaskRunner() {
  static base::NoDestructor<fml::Thread> thread("Test_Runner");
  return thread->GetTaskRunner();
}

class LynxActorTest : public ::testing::Test {
 protected:
  LynxActorTest() = default;
  ~LynxActorTest() override = default;

  void SetUp() override {
    actor_ = std::make_shared<LynxActor<std::string>>(
        std::make_unique<std::string>(), task_runner_);
  }

  void TearDown() override {
    actor_->ActSync([](auto& str) { str = nullptr; });
  }

  fml::RefPtr<fml::TaskRunner> task_runner_ = GetHookTaskRunner();
  std::shared_ptr<LynxActor<std::string>> actor_;
  fml::AutoResetWaitableEvent arwe_;
};

TEST_F(LynxActorTest, ActAsync) {
  actor_->Act([](auto& str) { *str = "MAGA"; });
  ASSERT_TRUE(actor_->ActSync([](auto& str) { return *str == "MAGA"; }));
}

TEST_F(LynxActorTest, ActAsyncAfterDestroy) {
  actor_->ActSync([](auto& str) { str.reset(); });

  bool result = true;
  actor_->Act([&result](auto& str) mutable { result = false; });
  task_runner_->PostTask([this]() mutable { arwe_.Signal(); });
  arwe_.Wait();
  ASSERT_TRUE(result);
}

TEST_F(LynxActorTest, ActSync) {
  actor_->ActSync([](auto& str) { *str = "MAGA"; });
  ASSERT_TRUE(actor_->ActSync([](auto& str) { return *str == "MAGA"; }));
}

TEST_F(LynxActorTest, ActSyncAfterDestroy) {
  actor_->ActSync([](auto& str) { str.reset(); });

  bool result = true;
  actor_->ActSync([&result](auto& str) mutable { result = false; });
  ASSERT_TRUE(result);
}

TEST_F(LynxActorTest, ActSyncWithRet) {
  actor_->ActSync([](auto& str) { *str = "MAGA"; });
  ASSERT_EQ(actor_->ActSync([](auto& str) { return *str; }), "MAGA");
}

TEST_F(LynxActorTest, ActSyncWithRetAfterDestroy) {
  actor_->ActSync([](auto& str) { str.reset(); });

  ASSERT_TRUE(actor_->ActSync([](auto& str) { return *str; }).empty());
}

}  // namespace shell
}  // namespace lynx
