// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_UNITTEST_H_
#define CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_UNITTEST_H_

#include "core/runtime/jscache/js_cache_tracker_unittest.h"

#include "core/runtime/jscache/js_cache_tracker.h"
#include "core/runtime/jsi/jsi.h"
#include "core/services/event_report/event_tracker.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {

namespace tasm {
namespace report {
namespace test {
void GetEventParams(MoveOnlyEvent &event, int event_depth) {
  auto &event_builders = EventTracker::Instance()->tracker_event_builder_stack_;
  EXPECT_GE(event_builders.size() - event_depth, 0u);
  event_builders[event_builders.size() - event_depth](event);
  for (int i = 0; i < event_depth; i++) {
    event_builders.pop_back();
  }
}
}  // namespace test
}  // namespace report
}  // namespace tasm

namespace piper {
namespace cache {
namespace testing {
using namespace lynx::tasm::report;
using namespace lynx::tasm::report::test;

void CheckCommonEventTrackerParams(MoveOnlyEvent &event, JSRuntimeType type,
                                   const std::string &stage) {
  EXPECT_EQ(event.GetName(), "lynxsdk_code_cache");
  auto &string_props = event.GetStringProps();
  EXPECT_EQ(string_props.at("stage"), stage);
  auto &int_props = event.GetIntProps();
  EXPECT_EQ(int_props.at("runtime_type"), static_cast<int>(type));
}

void CheckOnGetBytecodeEvent(JSRuntimeType type, const std::string &source_url,
                             JsCacheType cache_type, bool cache_hit,
                             bool enable_user_bytecode, bool enable_bytecode,
                             double cost, double code_size) {
  MoveOnlyEvent event;
  GetEventParams(event, 1);
  CheckCommonEventTrackerParams(event, type, "get_code_cache");
  auto &str_props = event.GetStringProps();
  auto &int_props = event.GetIntProps();
  auto &double_props = event.GetDoubleProps();
  EXPECT_EQ(str_props.at("source_url"), source_url);
  EXPECT_EQ(int_props.at("cache_type"), static_cast<int>(cache_type));
  EXPECT_EQ(int_props.at("cache_hit"), cache_hit);
  EXPECT_EQ(int_props.at("enable_user_bytecode"), enable_user_bytecode);
  EXPECT_EQ(int_props.at("enable_bytecode"), enable_bytecode);
  EXPECT_GE(double_props.at("cost"), cost);
  // kb so it is 0.
  EXPECT_GE(double_props.at("code_size"), code_size);
}

void CheckBytecodeGenerateEvent(JSRuntimeType runtime_type, std::string url,
                                std::string template_url, bool generate_success,
                                double raw_size, double bytecode_size,
                                bool persist_success,
                                JsCacheErrorCode error_code,
                                MoveOnlyEvent event) {
  CheckCommonEventTrackerParams(event, runtime_type, "generate_code_cache");
  auto &str_props = event.GetStringProps();
  auto &int_props = event.GetIntProps();
  auto &double_props = event.GetDoubleProps();
  EXPECT_EQ(str_props.at("source_url"), url);
  EXPECT_EQ(str_props.at("template_url"), template_url);
  EXPECT_EQ(int_props.at("generate_success"), generate_success);
  EXPECT_GE(double_props.at("raw_size"), raw_size / 1024.0);
  EXPECT_GE(double_props.at("code_cache_size"), bytecode_size / 1024.0);
  EXPECT_EQ(int_props.at("persist_success"), persist_success);
  EXPECT_NE(str_props.find("engine_version"), str_props.end());
  EXPECT_GE(double_props.at("generate_cost"), 0);
  EXPECT_EQ(int_props.at("error_code"), static_cast<int>(error_code));
}

void CheckCleanUpEvent(JSRuntimeType runtime_type, JsCacheErrorCode error_code,
                       MoveOnlyEvent event) {
  CheckCommonEventTrackerParams(event, runtime_type, "cleanup");
  auto &int_props = event.GetIntProps();
  auto &double_props = event.GetDoubleProps();
  EXPECT_GT(int_props.at("disk_file_count"), 0);
  EXPECT_GT(double_props.at("disk_file_size"), 0);
  EXPECT_GT(double_props.at("clean_size"), 0);
  EXPECT_GE(double_props.at("cost"), 0);
  EXPECT_EQ(int_props.at("error_code"), static_cast<int>(error_code));
}

void CheckCommonEventTrackerParams(MoveOnlyEvent &event) {
  EXPECT_EQ(event.GetName(), "lynxsdk_code_cache");
  auto &string_props = event.GetStringProps();
  EXPECT_EQ(string_props.at("stage"), "prepare_js");
  auto &int_props = event.GetIntProps();
  EXPECT_EQ(int_props.at("runtime_type"),
            static_cast<int>(JSRuntimeType::quickjs));
}

void CheckPrepareJSEvent(const std::string &source_url, bool load_success,
                         cache::JsScriptType script_type, double cost,
                         cache::JsCacheErrorCode error_code, int event_depth) {
  MoveOnlyEvent event;
  GetEventParams(event, event_depth);
  CheckCommonEventTrackerParams(event);

  auto &str_props = event.GetStringProps();
  auto &int_props = event.GetIntProps();
  auto &double_props = event.GetDoubleProps();

  EXPECT_EQ(str_props.at("source_url"), source_url);
  EXPECT_EQ(int_props.at("script_type"), static_cast<int>(script_type));
  EXPECT_EQ(int_props.at("load_success"), load_success);
  EXPECT_EQ(int_props.at("error_code"), static_cast<int>(error_code));
  EXPECT_GE(double_props.at("cost"), cost);
}

}  // namespace testing
}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_UNITTEST_H_
