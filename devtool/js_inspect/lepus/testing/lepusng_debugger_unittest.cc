// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_debugger.h"

#include <gtest/gtest.h>

namespace lynx {
namespace debug {

TEST(MTSDebugInfoErrorTest, GetFormattedErrorMessageAllFields) {
  MTSDebugInfoError error;
  error.reason_ = "Failed to parse";
  error.detail_ = "JSON syntax error at line 42";
  error.file_name_ = "test.js";
  error.debug_info_url_ = "https://example.com/debug.json";

  auto msg = error.GetFormattedErrorMessage();
  EXPECT_NE(msg.find("Failed to parse"), std::string::npos);
  EXPECT_NE(msg.find("JSON syntax error at line 42"), std::string::npos);
  EXPECT_NE(msg.find("test.js"), std::string::npos);
  EXPECT_NE(msg.find("https://example.com/debug.json"), std::string::npos);
}

TEST(MTSDebugInfoErrorTest, GetFormattedErrorMessageEmptyFields) {
  MTSDebugInfoError error;

  auto msg = error.GetFormattedErrorMessage();
  EXPECT_NE(msg.find("MTS debug-info error!"), std::string::npos);
  EXPECT_NE(msg.find("Reason:"), std::string::npos);
  EXPECT_NE(msg.find("Detail:"), std::string::npos);
  EXPECT_NE(msg.find("File name:"), std::string::npos);
  EXPECT_NE(msg.find("Debug-info URL:"), std::string::npos);
}

TEST(MTSDebugInfoErrorTest, GetFormattedErrorMessagePartialFields) {
  MTSDebugInfoError error;
  error.reason_ = "Missing function_source";
  error.file_name_ = "debug.json";

  auto msg = error.GetFormattedErrorMessage();
  EXPECT_NE(msg.find("Missing function_source"), std::string::npos);
  EXPECT_NE(msg.find("debug.json"), std::string::npos);
}

TEST(DebugInfoDetailTest, DefaultConstruction) {
  DebugInfoDetail detail;
  EXPECT_EQ(detail.id_, -1);
  EXPECT_TRUE(detail.debug_info_url_.empty());
  EXPECT_TRUE(detail.debug_info_str_.empty());
  EXPECT_TRUE(detail.filename_parsed_pairs_.empty());
}

TEST(DebugInfoDetailTest, FieldAssignment) {
  DebugInfoDetail detail;
  detail.id_ = 42;
  detail.debug_info_url_ = "https://example.com/debug.json";
  detail.debug_info_str_ = R"({"function_number": 1})";
  detail.filename_parsed_pairs_.push_back({"main.js", true});

  EXPECT_EQ(detail.id_, 42);
  EXPECT_EQ(detail.debug_info_url_, "https://example.com/debug.json");
  EXPECT_EQ(detail.debug_info_str_, R"({"function_number": 1})");
  EXPECT_EQ(detail.filename_parsed_pairs_.size(), 1u);
  EXPECT_EQ(detail.filename_parsed_pairs_[0].first, "main.js");
  EXPECT_TRUE(detail.filename_parsed_pairs_[0].second);
}

TEST(DebugInfoDetailTest, MultipleFilenamePairs) {
  DebugInfoDetail detail;
  detail.filename_parsed_pairs_.push_back({"a.js", true});
  detail.filename_parsed_pairs_.push_back({"b.js", false});
  detail.filename_parsed_pairs_.push_back({"c.js", true});

  EXPECT_EQ(detail.filename_parsed_pairs_.size(), 3u);
  EXPECT_FALSE(detail.filename_parsed_pairs_[1].second);
}

}  // namespace debug
}  // namespace lynx
