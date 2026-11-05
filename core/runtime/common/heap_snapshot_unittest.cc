// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/heap_snapshot.h"

#include <chrono>
#include <cstdio>
#include <future>
#include <thread>
#include <utility>

#include "base/include/fml/thread.h"
#include "base/include/lynx_actor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace {

class HeapSnapshotRequestTest : public ::testing::Test {
 protected:
  struct Result {
    const char* error;
    std::thread::id thread;
  };

  void SetUp() override {
    path_ = ::testing::TempDir() + "lynx-heap-snapshot-" +
            std::to_string(reinterpret_cast<uintptr_t>(this));
  }
  void TearDown() override { std::remove(path_.c_str()); }

  std::unique_ptr<HeapSnapshotRequest> Request(std::string path) {
    auto promise = std::make_shared<std::promise<Result>>();
    result_ = promise->get_future();
    return std::make_unique<HeapSnapshotRequest>(
        std::move(path), [promise](const char* error) {
          promise->set_value({error, std::this_thread::get_id()});
        });
  }

  void ExpectResult(bool success, const char* expected_error = nullptr) {
    ASSERT_EQ(result_.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
    auto result = result_.get();
    EXPECT_EQ(result.error == nullptr, success) << result.error;
    if (expected_error) {
      EXPECT_STREQ(result.error, expected_error);
    }
    EXPECT_NE(result.thread, std::this_thread::get_id());
  }

  std::string path_;
  std::future<Result> result_;
};

TEST_F(HeapSnapshotRequestTest, WritesCompleteBufferBeforeCompletion) {
  auto request = Request(path_);
  request->Finish(std::make_unique<base::StringBuffer>("snapshot contents"),
                  nullptr);
  request.reset();
  ExpectResult(true);
  auto content = base::Buffer::ReadFromFile(path_);
  ASSERT_NE(content, nullptr);
  EXPECT_EQ(std::string(reinterpret_cast<const char*>(content->data()),
                        content->size()),
            "snapshot contents");
}

TEST_F(HeapSnapshotRequestTest, MissingParentReportsWriteError) {
  auto request = Request(path_ + "/snapshot");
  request->Finish(std::make_unique<base::StringBuffer>("snapshot"), nullptr);
  request.reset();
  ExpectResult(false, "Write failed");
}

TEST_F(HeapSnapshotRequestTest, CaptureFailurePreservesExistingFile) {
  ASSERT_TRUE(base::StringBuffer("previous").WriteToFile(path_));
  auto request = Request(path_);
  request->Fail("capture failed");
  request.reset();
  ExpectResult(false, "capture failed");
  auto content = base::Buffer::ReadFromFile(path_);
  ASSERT_NE(content, nullptr);
  EXPECT_EQ(content->size(), 8u);
}

TEST_F(HeapSnapshotRequestTest, EmptyPathReportsWriteError) {
  auto request = Request("");
  request->Finish(std::make_unique<base::StringBuffer>("snapshot"), nullptr);
  request.reset();
  ExpectResult(false, "Write failed");
}

TEST_F(HeapSnapshotRequestTest, DestroyedActorStillCompletesRequest) {
  fml::Thread owner("heap-snapshot-test");
  auto actor =
      std::make_shared<shell::LynxActor<int>>(nullptr, owner.GetTaskRunner());
  actor->ActAsync([request = Request(path_)](auto&) {
    ADD_FAILURE() << "Destroyed actor must not invoke the capture";
  });
  ExpectResult(false, "Cancelled");
}

TEST_F(HeapSnapshotRequestTest, DisabledActorStillCompletesRequest) {
  fml::Thread owner("heap-snapshot-test");
  auto actor = std::make_shared<shell::LynxActor<int>>(
      std::make_unique<int>(0), owner.GetTaskRunner(),
      shell::kUnknownInstanceId, false);
  actor->ActAsync([request = Request(path_)](auto&) {
    ADD_FAILURE() << "Disabled actor must not invoke the capture";
  });
  ExpectResult(false, "Cancelled");
}

}  // namespace
}  // namespace runtime
}  // namespace lynx
