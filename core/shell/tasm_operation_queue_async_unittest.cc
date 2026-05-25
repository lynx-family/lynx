// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/tasm_operation_queue_async.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/public/pipeline_option.h"
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

  static std::shared_ptr<tasm::PipelineOptions> MakeOptionsWithUpdatedLists(
      std::vector<int32_t> updated_list_elements) {
    auto options = std::make_shared<tasm::PipelineOptions>();
    options->updated_list_elements_ = std::move(updated_list_elements);
    return options;
  }
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

TEST_F(TASMOperationQueueAsyncTest,
       AppendPendingTaskCopiesUpdatedListElementsToReadyQueue) {
  auto options = MakeOptionsWithUpdatedLists({101, 202});

  operation_queue_->AppendPendingTask(options);

  EXPECT_EQ((std::vector<int32_t>{101, 202}), options->updated_list_elements_);
  EXPECT_EQ((std::vector<int32_t>{101, 202}),
            operation_queue_->GetReadyUpdatedListElements());
  EXPECT_TRUE(operation_queue_->GetReadyUpdatedListElements().empty());
}

TEST_F(TASMOperationQueueAsyncTest,
       AppendPendingTaskAppendsUpdatedListElementsInOrder) {
  auto first_options = MakeOptionsWithUpdatedLists({1, 2});
  auto second_options = MakeOptionsWithUpdatedLists({3, 4});

  operation_queue_->AppendPendingTask(first_options);
  operation_queue_->AppendPendingTask(second_options);

  EXPECT_EQ((std::vector<int32_t>{1, 2}),
            first_options->updated_list_elements_);
  EXPECT_EQ((std::vector<int32_t>{3, 4}),
            second_options->updated_list_elements_);
  EXPECT_EQ((std::vector<int32_t>{1, 2, 3, 4}),
            operation_queue_->GetReadyUpdatedListElements());
  EXPECT_TRUE(operation_queue_->GetReadyUpdatedListElements().empty());
}

TEST_F(TASMOperationQueueAsyncTest,
       AppendPendingTaskWithoutOptionsKeepsReadyUpdatedListElements) {
  auto options = MakeOptionsWithUpdatedLists({7, 8});

  operation_queue_->AppendPendingTask(options);
  operation_queue_->AppendPendingTask();

  EXPECT_EQ((std::vector<int32_t>{7, 8}), options->updated_list_elements_);
  EXPECT_EQ((std::vector<int32_t>{7, 8}),
            operation_queue_->GetReadyUpdatedListElements());
  EXPECT_TRUE(operation_queue_->GetReadyUpdatedListElements().empty());
}

TEST_F(TASMOperationQueueAsyncTest,
       AppendPendingTaskWithoutOptionsDoesNotCreateReadyUpdatedListElements) {
  operation_queue_->AppendPendingTask();

  EXPECT_TRUE(operation_queue_->GetReadyUpdatedListElements().empty());
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
