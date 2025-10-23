// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_TEST_TIMEOUT_LISTENER_H_
#define TESTING_TEST_TIMEOUT_LISTENER_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/thread.h"
#include "clay/testing/testing.h"

namespace clay {
namespace testing {

class PendingTests;

class TestTimeoutListener : public ::testing::EmptyTestEventListener {
 public:
  explicit TestTimeoutListener(fml::TimeDelta timeout);

  ~TestTimeoutListener();

 private:
  const fml::TimeDelta timeout_;
  fml::Thread listener_thread_;
  fml::RefPtr<fml::TaskRunner> listener_thread_runner_;
  std::shared_ptr<PendingTests> pending_tests_;

  // |testing::EmptyTestEventListener|
  void OnTestStart(const ::testing::TestInfo& test_info) override;

  // |testing::EmptyTestEventListener|
  void OnTestEnd(const ::testing::TestInfo& test_info) override;

  BASE_DISALLOW_COPY_AND_ASSIGN(TestTimeoutListener);
};

}  // namespace testing
}  // namespace clay

#endif  // TESTING_TEST_TIMEOUT_LISTENER_H_
