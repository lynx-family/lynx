// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/page_config.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

/**
 * 1. if target config has not been set,
 * GetValue() shoule be equal to expect_default_value;
 * 2. if target config has been set to true,
 * GetValue() shoule be equal to expect_true_value;
 * 3. if target config has been set to false,
 * GetValue() shoule be equal to expect_false_value.
 */
#define CHECK_CONFIG_VALUE(func_name, expect_default_value, expect_true_value, \
                           expect_false_value)                                 \
  PageConfig page_config;                                                      \
  EXPECT_EQ(expect_default_value, page_config.Get##func_name());               \
  page_config.Set##func_name(true);                                            \
  EXPECT_EQ(expect_true_value, page_config.Get##func_name());                  \
  page_config.Set##func_name(false);                                           \
  EXPECT_EQ(expect_false_value, page_config.Get##func_name());

TEST(PageConfigTest, EnableParallelParseElementTemplate) {
  PageConfig page_config;
  EXPECT_FALSE(page_config.GetEnableParallelParseElementTemplate());

  page_config.pipeline_scheduler_config_ = 1;
  EXPECT_TRUE(page_config.GetEnableParallelParseElementTemplate());
}

TEST(PageConfigTest, EnableUseContextPool) {
  CHECK_CONFIG_VALUE(EnableUseContextPool, true, true, false);
}

TEST(PageConfigTest, EnableComponentAsyncDecode) {
  CHECK_CONFIG_VALUE(EnableComponentAsyncDecode, false, true, false);
}

TEST(PageConfigTest, EnableSignalAPI0) {
  PageConfig page_config;
  EXPECT_EQ(page_config.GetEnableSignalAPIBoolValue(), false);
  EXPECT_EQ(page_config.GetEnableSignalAPI(), TernaryBool::UNDEFINE_VALUE);

  page_config.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableSignalAPI\" : true\n}");
  EXPECT_EQ(page_config.GetEnableSignalAPIBoolValue(), true);
  EXPECT_EQ(page_config.GetEnableSignalAPI(), TernaryBool::TRUE_VALUE);
}

TEST(PageConfigTest, EnableSignalAPI1) {
  PageConfig page_config;
  EXPECT_EQ(page_config.GetEnableSignalAPIBoolValue(), false);
  EXPECT_EQ(page_config.GetEnableSignalAPI(), TernaryBool::UNDEFINE_VALUE);

  page_config.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableSignalAPI\" : false\n}");
  EXPECT_EQ(page_config.GetEnableSignalAPIBoolValue(), false);
  EXPECT_EQ(page_config.GetEnableSignalAPI(), TernaryBool::FALSE_VALUE);
}

#undef CHECK_CONFIG_VALUE

}  // namespace test
}  // namespace tasm
}  // namespace lynx
