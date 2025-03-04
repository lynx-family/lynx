// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/tasm_operation_queue_async.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

class TASMOperationQueueAsyncTest : public ::testing::Test {
 protected:
  TASMOperationQueueAsyncTest() = default;
  ~TASMOperationQueueAsyncTest() override = default;

  fml::RefPtr<fml::TaskRunner> tasm_runner_ =
      MockRunnerManufactor::GetHookTASMTaskRunner();

  fml::AutoResetWaitableEvent arwe_;
  std::shared_ptr<TASMOperationQueueAsync> operation_queue_ =
      std::make_shared<TASMOperationQueueAsync>();

  int32_t actual_ = 0;
  int32_t expect_ = 0;
  constexpr static int32_t kOpsCounts = 20;
};

TEST_F(TASMOperationQueueAsyncTest, FlushNonTrivialOperationsOnceOnTASM) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
  operation_queue_->AppendPendingTask();

  tasm_runner_->PostTask([this] { ASSERT_TRUE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);
}

TEST_F(
    TASMOperationQueueAsyncTest,
    FlushNonTrivialOperationsOnceOnTASMWhenAppendPendingTaskNeededDuringFlush) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

  tasm_runner_->PostTask([this] {
    ASSERT_FALSE(operation_queue_->Flush());
    arwe_.Signal();
  });

  arwe_.Wait();
  ASSERT_EQ(actual_, 0);

  arwe_.Reset();
  operation_queue_->SetAppendPendingTaskNeededDuringFlush(true);

  tasm_runner_->PostTask([this] { ASSERT_TRUE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);

  arwe_.Reset();
  actual_ = 0;

  operation_queue_->SetAppendPendingTaskNeededDuringFlush(false);

  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

  tasm_runner_->PostTask([this] {
    ASSERT_FALSE(operation_queue_->Flush());
    arwe_.Signal();
  });

  arwe_.Wait();
  ASSERT_EQ(actual_, 0);
}

TEST_F(TASMOperationQueueAsyncTest, FlushTrivialOperationsOnceOnTASM) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
  operation_queue_->AppendPendingTask();

  tasm_runner_->PostTask([this] { ASSERT_FALSE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);
}

TEST_F(TASMOperationQueueAsyncTest,
       FlushTrivialOperationsOnceOnTASMWhenAppendPendingTaskNeededDuringFlush) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

  tasm_runner_->PostTask([this] {
    ASSERT_FALSE(operation_queue_->Flush());
    arwe_.Signal();
  });

  arwe_.Wait();
  ASSERT_EQ(actual_, 0);

  arwe_.Reset();
  operation_queue_->SetAppendPendingTaskNeededDuringFlush(true);

  tasm_runner_->PostTask([this] { ASSERT_FALSE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);

  arwe_.Reset();
  actual_ = 0;

  operation_queue_->SetAppendPendingTaskNeededDuringFlush(false);

  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

  tasm_runner_->PostTask([this] {
    ASSERT_FALSE(operation_queue_->Flush());
    arwe_.Signal();
  });

  arwe_.Wait();
  ASSERT_EQ(actual_, 0);
}

TEST_F(TASMOperationQueueAsyncTest, FlushMixedOperationsOnceOnTASM) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    if (i % 2) {
      operation_queue_->EnqueueOperation([this] { ++actual_; });
    } else {
      operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    }

    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
  operation_queue_->AppendPendingTask();

  tasm_runner_->PostTask([this] { ASSERT_TRUE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);
}

TEST_F(TASMOperationQueueAsyncTest,
       FlushMixedOperationsOnceOnTASMWhenAppendPendingTaskNeededDuringFlush) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    if (i % 2) {
      operation_queue_->EnqueueOperation([this] { ++actual_; });
    } else {
      operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    }

    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

  tasm_runner_->PostTask([this] {
    ASSERT_FALSE(operation_queue_->Flush());
    arwe_.Signal();
  });

  arwe_.Wait();
  ASSERT_EQ(actual_, 0);

  arwe_.Reset();
  operation_queue_->SetAppendPendingTaskNeededDuringFlush(true);

  tasm_runner_->PostTask([this] { ASSERT_TRUE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);

  arwe_.Reset();
  actual_ = 0;

  operation_queue_->SetAppendPendingTaskNeededDuringFlush(false);

  for (int32_t i = 0; i < kOpsCounts; ++i) {
    if (i % 2) {
      operation_queue_->EnqueueOperation([this] { ++actual_; });
    } else {
      operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    }

    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

  tasm_runner_->PostTask([this] {
    ASSERT_FALSE(operation_queue_->Flush());
    arwe_.Signal();
  });

  arwe_.Wait();
  ASSERT_EQ(actual_, 0);
}

TEST_F(TASMOperationQueueAsyncTest,
       EnqueueNonTrivialOperationsRepeatedlyOnTASM) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueOperation([this] { ++actual_; });

    if (kOpsCounts / 2 == ++expect_) {
      operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
      operation_queue_->AppendPendingTask();

      tasm_runner_->PostTask(
          [this] { ASSERT_TRUE(operation_queue_->Flush()); });
      arwe_.Wait();

      ASSERT_EQ(expect_, actual_);
      ASSERT_TRUE(actual_ == kOpsCounts / 2);

      arwe_.Reset();
    }
  }
}

TEST_F(
    TASMOperationQueueAsyncTest,
    EnqueueNonTrivialOperationsRepeatedlyOnTASMWhenAppendPendingTaskNeededDuringFlush) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueOperation([this] { ++actual_; });

    if (kOpsCounts / 2 == ++expect_) {
      operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

      tasm_runner_->PostTask([this] {
        ASSERT_FALSE(operation_queue_->Flush());
        arwe_.Signal();
      });
      arwe_.Wait();

      ASSERT_EQ(actual_, 0);

      arwe_.Reset();
      operation_queue_->SetAppendPendingTaskNeededDuringFlush(true);

      tasm_runner_->PostTask(
          [this] { ASSERT_TRUE(operation_queue_->Flush()); });
      arwe_.Wait();

      ASSERT_EQ(expect_, actual_);
      ASSERT_TRUE(actual_ == kOpsCounts / 2);

      arwe_.Reset();
      operation_queue_->SetAppendPendingTaskNeededDuringFlush(false);
    }
  }
}

TEST_F(TASMOperationQueueAsyncTest, EnqueueTrivialOperationsRepeatedlyOnTASM) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });

    if (kOpsCounts / 2 == ++expect_) {
      operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
      operation_queue_->AppendPendingTask();

      tasm_runner_->PostTask(
          [this] { ASSERT_FALSE(operation_queue_->Flush()); });
      arwe_.Wait();

      ASSERT_EQ(expect_, actual_);
      ASSERT_TRUE(actual_ == kOpsCounts / 2);

      arwe_.Reset();
    }
  }
}

TEST_F(
    TASMOperationQueueAsyncTest,
    EnqueueTrivialOperationsRepeatedlyOnTASMWhenAppendPendingTaskNeededDuringFlush) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });

    if (kOpsCounts / 2 == ++expect_) {
      operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

      tasm_runner_->PostTask([this] {
        ASSERT_FALSE(operation_queue_->Flush());
        arwe_.Signal();
      });
      arwe_.Wait();

      ASSERT_EQ(actual_, 0);

      arwe_.Reset();
      operation_queue_->SetAppendPendingTaskNeededDuringFlush(true);

      tasm_runner_->PostTask(
          [this] { ASSERT_FALSE(operation_queue_->Flush()); });
      arwe_.Wait();

      ASSERT_EQ(expect_, actual_);
      ASSERT_TRUE(actual_ == kOpsCounts / 2);

      arwe_.Reset();
      operation_queue_->SetAppendPendingTaskNeededDuringFlush(false);
    }
  }
}

TEST_F(TASMOperationQueueAsyncTest, EnqueueMixedOperationsRepeatedlyOnTASM) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    if (i % 2) {
      operation_queue_->EnqueueOperation([this] { ++actual_; });
    } else {
      operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    }

    if (kOpsCounts / 2 == ++expect_) {
      operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
      operation_queue_->AppendPendingTask();

      tasm_runner_->PostTask(
          [this] { ASSERT_TRUE(operation_queue_->Flush()); });
      arwe_.Wait();

      ASSERT_EQ(expect_, actual_);
      ASSERT_TRUE(actual_ == kOpsCounts / 2);

      arwe_.Reset();
    }
  }
}

TEST_F(
    TASMOperationQueueAsyncTest,
    EnqueueMixedOperationsRepeatedlyOnTASMWhenAppendPendingTaskNeededDuringFlush) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    if (i % 2) {
      operation_queue_->EnqueueOperation([this] { ++actual_; });
    } else {
      operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    }

    if (kOpsCounts / 2 == ++expect_) {
      operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });

      tasm_runner_->PostTask([this] {
        ASSERT_FALSE(operation_queue_->Flush());
        arwe_.Signal();
      });
      arwe_.Wait();

      ASSERT_EQ(actual_, 0);

      arwe_.Reset();
      operation_queue_->SetAppendPendingTaskNeededDuringFlush(true);

      tasm_runner_->PostTask(
          [this] { ASSERT_TRUE(operation_queue_->Flush()); });
      arwe_.Wait();

      ASSERT_EQ(expect_, actual_);
      ASSERT_TRUE(actual_ == kOpsCounts / 2);

      arwe_.Reset();
      operation_queue_->SetAppendPendingTaskNeededDuringFlush(false);
    }
  }
}

TEST_F(TASMOperationQueueAsyncTest,
       AppendPendingTaskWithTrivialOperationsRepeatedly) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->AppendPendingTask();

  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
  operation_queue_->AppendPendingTask();

  tasm_runner_->PostTask([this] { ASSERT_TRUE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);
}

TEST_F(TASMOperationQueueAsyncTest,
       AppendPendingTaskWithNonTrivialOperationsRepeatedly) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->AppendPendingTask();

  for (int32_t i = 0; i < kOpsCounts; ++i) {
    operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
  operation_queue_->AppendPendingTask();

  tasm_runner_->PostTask([this] { ASSERT_FALSE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);
}

TEST_F(TASMOperationQueueAsyncTest,
       AppendPendingTaskWithMixedOperationsRepeatedly) {
  for (int32_t i = 0; i < kOpsCounts; ++i) {
    if (i % 2) {
      operation_queue_->EnqueueOperation([this] { ++actual_; });
    } else {
      operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    }

    ++expect_;
  }

  operation_queue_->AppendPendingTask();

  for (int32_t i = 0; i < kOpsCounts; ++i) {
    if (i % 2) {
      operation_queue_->EnqueueOperation([this] { ++actual_; });
    } else {
      operation_queue_->EnqueueTrivialOperation([this] { ++actual_; });
    }

    ++expect_;
  }

  operation_queue_->EnqueueTrivialOperation([this] { arwe_.Signal(); });
  operation_queue_->AppendPendingTask();

  tasm_runner_->PostTask([this] { ASSERT_TRUE(operation_queue_->Flush()); });

  arwe_.Wait();
  ASSERT_EQ(expect_, actual_);
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
