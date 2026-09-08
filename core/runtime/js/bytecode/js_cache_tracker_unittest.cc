// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BYTECODE_JS_CACHE_TRACKER_UNITTEST_H_
#define CORE_RUNTIME_BYTECODE_JS_CACHE_TRACKER_UNITTEST_H_

#include "core/runtime/js/bytecode/js_cache_tracker_unittest.h"

#include "core/runtime/js/bytecode/js_cache_tracker.h"
#include "core/runtime/js/jsi/jsi.h"
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

namespace runtime {

namespace js {
namespace cache {
namespace testing {
using namespace lynx::tasm::report;
using namespace lynx::tasm::report::test;

void CheckCommonEventTrackerParams(MoveOnlyEvent &event, JSRuntimeType type,
                                   const std::string &stage) {
  EXPECT_EQ(event.GetName(), "lynxsdk_code_cache");
  const auto props = event.GetPropsAsMap();
  EXPECT_EQ(props.at("stage").GetStringValue(), stage);
  EXPECT_EQ(props.at("runtime_type").GetIntValue(), static_cast<int>(type));
}

void CheckOnGetBytecodeEvent(JSRuntimeType type, const std::string &source_url,
                             JsCacheType cache_type, bool cache_hit,
                             bool enable_user_bytecode, bool enable_bytecode,
                             double cost, double code_size) {
  MoveOnlyEvent event;
  GetEventParams(event, 1);
  CheckCommonEventTrackerParams(event, type, "get_code_cache");
  const auto props = event.GetPropsAsMap();
  EXPECT_EQ(props.at("source_url").GetStringValue(), source_url);
  EXPECT_EQ(props.at("cache_type").GetIntValue(), static_cast<int>(cache_type));
  EXPECT_EQ(props.at("cache_hit").GetIntValue(), cache_hit);
  EXPECT_EQ(props.at("enable_user_bytecode").GetIntValue(),
            enable_user_bytecode);
  EXPECT_EQ(props.at("enable_bytecode").GetIntValue(), enable_bytecode);
  EXPECT_GE(props.at("cost").GetDoubleValue(), cost);
  // kb so it is 0.
  EXPECT_GE(props.at("code_size").GetDoubleValue(), code_size);
}

void CheckBytecodeGenerateEvent(JSRuntimeType runtime_type, std::string url,
                                std::string template_url, bool generate_success,
                                double raw_size, double bytecode_size,
                                bool persist_success,
                                JsCacheErrorCode error_code,
                                MoveOnlyEvent event) {
  CheckCommonEventTrackerParams(event, runtime_type, "generate_code_cache");
  const auto props = event.GetPropsAsMap();
  EXPECT_EQ(props.at("source_url").GetStringValue(), url);
  EXPECT_EQ(props.at("template_url").GetStringValue(), template_url);
  EXPECT_EQ(props.at("generate_success").GetIntValue(), generate_success);
  EXPECT_GE(props.at("raw_size").GetDoubleValue(), raw_size / 1024.0);
  EXPECT_GE(props.at("code_cache_size").GetDoubleValue(),
            bytecode_size / 1024.0);
  EXPECT_EQ(props.at("persist_success").GetIntValue(), persist_success);
  EXPECT_NE(props.find("engine_version"), props.end());
  EXPECT_GE(props.at("generate_cost").GetDoubleValue(), 0);
  EXPECT_EQ(props.at("error_code").GetIntValue(), static_cast<int>(error_code));
}

void CheckCleanUpEvent(JSRuntimeType runtime_type, JsCacheErrorCode error_code,
                       MoveOnlyEvent event) {
  CheckCommonEventTrackerParams(event, runtime_type, "cleanup");
  const auto props = event.GetPropsAsMap();
  EXPECT_GT(props.at("disk_file_count").GetIntValue(), 0);
  EXPECT_GT(props.at("disk_file_size").GetDoubleValue(), 0);
  EXPECT_GT(props.at("clean_size").GetDoubleValue(), 0);
  EXPECT_GE(props.at("cost").GetDoubleValue(), 0);
  EXPECT_EQ(props.at("error_code").GetIntValue(), static_cast<int>(error_code));
}

void CheckCommonEventTrackerParams(MoveOnlyEvent &event) {
  EXPECT_EQ(event.GetName(), "lynxsdk_code_cache");
  const auto props = event.GetPropsAsMap();
  EXPECT_EQ(props.at("stage").GetStringValue(), "prepare_js");
  EXPECT_EQ(props.at("runtime_type").GetIntValue(),
            static_cast<int>(JSRuntimeType::quickjs));
}

void CheckPrepareJSEvent(const std::string &source_url, bool load_success,
                         cache::JsScriptType script_type, double cost,
                         cache::JsCacheErrorCode error_code, int event_depth) {
  MoveOnlyEvent event;
  GetEventParams(event, event_depth);
  CheckCommonEventTrackerParams(event);

  const auto props = event.GetPropsAsMap();
  EXPECT_EQ(props.at("source_url").GetStringValue(), source_url);
  EXPECT_EQ(props.at("script_type").GetIntValue(),
            static_cast<int>(script_type));
  EXPECT_EQ(props.at("load_success").GetIntValue(), load_success);
  EXPECT_EQ(props.at("error_code").GetIntValue(), static_cast<int>(error_code));
  EXPECT_GE(props.at("cost").GetDoubleValue(), cost);
}

}  // namespace testing
}  // namespace cache
}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_BYTECODE_JS_CACHE_TRACKER_UNITTEST_H_
