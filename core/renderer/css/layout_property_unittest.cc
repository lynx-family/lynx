// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/layout_property.h"

#include <atomic>
#include <thread>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

TEST(LayoutPropertyTest, BasicFunction) {
#define EXPECT_LAYOUT_PROPERTY(name, type) \
  EXPECT_EQ(LayoutProperty::ConsumptionTest(kPropertyID##name), type);
  FOREACH_LAYOUT_PROPERTY(EXPECT_LAYOUT_PROPERTY)
#undef EXPECT_LAYOUT_PROPERTY

  EXPECT_TRUE(LayoutProperty::IsLayoutOnly(kPropertyIDPosition));
  EXPECT_FALSE(LayoutProperty::IsLayoutWanted(kPropertyIDPosition));
  EXPECT_TRUE(LayoutProperty::IsLayoutWanted(kPropertyIDBorderTopWidth));
  EXPECT_FALSE(LayoutProperty::IsLayoutOnly(kPropertyIDBorderTopWidth));
}

TEST(LayoutPropertyTest, UnconsumedPropertiesAreSkipped) {
  for (auto id :
       {kPropertyIDBackgroundColor, kPropertyIDOpacity, kPropertyIDTransform}) {
    EXPECT_EQ(LayoutProperty::ConsumptionTest(id), ConsumptionStatus::SKIP);
    EXPECT_FALSE(LayoutProperty::IsLayoutOnly(id));
    EXPECT_FALSE(LayoutProperty::IsLayoutWanted(id));
  }
}

TEST(LayoutPropertyTest, ConcurrentAccess) {
  constexpr int kThreadCount = 16;
  constexpr int kLoopTimes = 10000;
  std::atomic<bool> start{false};
  std::vector<std::thread> threads;
  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([&start] {
      while (!start.load()) {
        std::this_thread::yield();
      }
      for (int j = 0; j < kLoopTimes; ++j) {
        EXPECT_EQ(LayoutProperty::ConsumptionTest(kPropertyIDPosition),
                  ConsumptionStatus::LAYOUT_ONLY);
        EXPECT_EQ(LayoutProperty::ConsumptionTest(kPropertyIDBorderTopWidth),
                  ConsumptionStatus::LAYOUT_WANTED);
        EXPECT_EQ(LayoutProperty::ConsumptionTest(kPropertyIDBackgroundColor),
                  ConsumptionStatus::SKIP);
      }
    });
  }
  start.store(true);
  for (auto& thread : threads) {
    thread.join();
  }
}

}  // namespace tasm
}  // namespace lynx
