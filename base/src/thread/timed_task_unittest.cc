// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/thread/timed_task.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/thread.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#if OS_WIN
#include <Windows.h>
#include <synchapi.h>

static void usleep(long num) {
  SleepEx(num < 500 ? 1 : (num + 500) / 1000, true);
}
#endif

namespace lynx {
namespace base {

class TimedTaskTest : public ::testing::Test {
 protected:
  TimedTaskTest() = default;
  ~TimedTaskTest() override = default;

  void SetUp() override {
    thread_.GetTaskRunner()->PostTask(
        [this]() { manager_ = std::make_unique<TimedTaskManager>(); });
  }

  void TearDown() override {
    arwe_.Reset();
    thread_.GetTaskRunner()->PostTask([this]() {
      manager_ = nullptr;
      arwe_.Signal();
    });

    arwe_.Wait();
  }

  void WaitResult(int64_t delay) {
    thread_.GetTaskRunner()->PostTask([this, delay]() {
      manager_->SetTimeout([this]() { arwe_.Signal(); }, delay);
    });
    arwe_.Wait();
  }

  // delay 10ms
  static constexpr int64_t DELAY = 10;

  static constexpr int32_t LOOP = 10;

  fml::Thread thread_;
  std::unique_ptr<TimedTaskManager> manager_;
  fml::AutoResetWaitableEvent arwe_;
  int32_t result_ = 0;
};

TEST_F(TimedTaskTest, SetTimeout) {
  int32_t expect = result_;

  for (int32_t i = 1; i <= LOOP; ++i) {
    thread_.GetTaskRunner()->PostTask(
        [this]() { manager_->SetTimeout([this]() { ++result_; }, DELAY); });
    ++expect;
  }

  WaitResult(DELAY);
  ASSERT_EQ(result_, expect);
}

TEST_F(TimedTaskTest, SetInterval) {
  int32_t expect = 0;

  thread_.GetTaskRunner()->PostTask([this, &expect]() mutable {
    manager_->SetInterval(
        [this, &expect]() mutable {
          ++result_;
          if (++expect == LOOP) {
            arwe_.Signal();
          }
        },
        DELAY);
  });

  arwe_.Wait();
  ASSERT_EQ(result_, expect);
}

TEST_F(TimedTaskTest, StopSetTimeout) {
  int32_t expect = result_;

  for (int32_t i = 0; i < LOOP; ++i) {
    thread_.GetTaskRunner()->PostTask([this]() {
      uint32_t id = manager_->SetTimeout([this]() { ++result_; }, DELAY);
      manager_->StopTask(id);
    });
  }

  WaitResult(DELAY);
  ASSERT_EQ(result_, expect);
}

TEST_F(TimedTaskTest, StopSetInterval) {
  int32_t expect = 0;
  int32_t id = 0;

  thread_.GetTaskRunner()->PostTask([this, &id]() mutable {
    id = manager_->SetInterval(
        [this]() {
          ++result_;
          arwe_.Signal();
        },
        DELAY);
  });

  usleep(DELAY * LOOP * 100);

  thread_.GetTaskRunner()->PostTask([this, id, &expect]() {
    expect = result_;
    manager_->StopTask(id);
    arwe_.Signal();
  });
  arwe_.Wait();

  usleep(DELAY * LOOP * 1000);
  ASSERT_EQ(result_, expect);
}

TEST_F(TimedTaskTest, StopAllTasks) {
  int32_t expect = result_;

  for (int32_t i = 0; i < LOOP; ++i) {
    thread_.GetTaskRunner()->PostTask(
        [this]() { manager_->SetTimeout([this]() { ++result_; }, DELAY); });
  }

  thread_.GetTaskRunner()->PostTask([this]() { manager_->StopAllTasks(); });

  WaitResult(DELAY);
  ASSERT_EQ(result_, expect);
}

TEST_F(TimedTaskTest, StopOtherTaskInSetTimeout) {
  for (int32_t i = 0; i < LOOP; ++i) {
    thread_.GetTaskRunner()->PostTask([this]() {
      uint32_t id = manager_->SetTimeout([this]() { ++result_; }, DELAY);
      manager_->SetTimeout([this, id]() { manager_->StopTask(id); }, DELAY / 2);
    });
  }

  WaitResult(DELAY);
  ASSERT_EQ(result_, 0);
}

TEST_F(TimedTaskTest, StopSelfTaskInSetTimeout) {
  int32_t expect = result_;
  for (int32_t i = 0; i < LOOP; ++i) {
    thread_.GetTaskRunner()->PostTask([this, id = i + 1]() {
      manager_->SetTimeout(
          [this, id]() {
            manager_->StopTask(id);
            ++result_;
          },
          DELAY);
    });
    ++expect;
  }

  WaitResult(DELAY);
  ASSERT_EQ(result_, expect);
}

TEST_F(TimedTaskTest, StopOtherTaskInSetInterval) {
  int32_t expect = 0;

  for (int32_t i = 0; i < LOOP; ++i) {
    thread_.GetTaskRunner()->PostTask(
        [this]() { manager_->SetTimeout([this]() { ++result_; }, DELAY); });
  }

  thread_.GetTaskRunner()->PostTask([this]() mutable {
    manager_->SetInterval(
        [this]() {
          for (int32_t i = 1; i <= LOOP; ++i) {
            manager_->StopTask(i);
          }
        },
        DELAY / 100);
  });

  usleep(DELAY * LOOP * 1000);
  ASSERT_EQ(result_, expect);
}

TEST_F(TimedTaskTest, StopSelfTaskInSetInterval) {
  thread_.GetTaskRunner()->PostTask([this]() mutable {
    manager_->SetInterval(
        [this]() {
          manager_->StopTask(1);
          ++result_;
        },
        DELAY);
  });

  usleep(DELAY * LOOP * 1000);
  ASSERT_EQ(result_, 1);
}

}  // namespace base
}  // namespace lynx
