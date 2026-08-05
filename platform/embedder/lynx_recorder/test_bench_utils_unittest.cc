// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_recorder/test_bench_utils.h"

#include <gtest/gtest.h>

#include <string>

namespace lynx {
namespace embedder {
namespace test {

TEST(TestBenchUtilsTest, FindsRawJsonAfterLeadingWhitespace) {
  const std::string record_file = "\n\t\r {\"Action List\":[]}";

  size_t raw_json_start = FindRawJsonStart(record_file);

  ASSERT_NE(raw_json_start, std::string::npos);
  EXPECT_EQ(record_file.substr(raw_json_start), "{\"Action List\":[]}");
}

TEST(TestBenchUtilsTest, FindsRawJsonAfterUtf8BomAndWhitespace) {
  const std::string record_file = "\xEF\xBB\xBF\n {\"Action List\":[]}";

  size_t raw_json_start = FindRawJsonStart(record_file);

  ASSERT_NE(raw_json_start, std::string::npos);
  EXPECT_EQ(record_file.substr(raw_json_start), "{\"Action List\":[]}");
}

TEST(TestBenchUtilsTest, RejectsEncodedRecordFile) {
  EXPECT_EQ(FindRawJsonStart("eJyrVnIsLknMy0xRslKKjtVRKkotLkksKS1Wqo0FAA=="),
            std::string::npos);
}

}  // namespace test
}  // namespace embedder
}  // namespace lynx
