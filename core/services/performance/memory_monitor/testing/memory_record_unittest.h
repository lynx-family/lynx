// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_TESTING_MEMORY_RECORD_UNITTEST_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_TESTING_MEMORY_RECORD_UNITTEST_H_

#include <string>
#include <thread>
#include <utility>

#include "core/services/performance/memory_monitor/memory_record.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

class MemoryRecordTest : public ::testing::Test {
 protected:
  void SetUp() override;
  performance::MemoryRecord empty_record_;
  performance::MemoryRecord basic_record_;
  performance::MemoryRecord detailed_record_;
};
}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_TESTING_MEMORY_RECORD_UNITTEST_H_
