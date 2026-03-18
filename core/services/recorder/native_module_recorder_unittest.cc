// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#include "core/services/recorder/native_module_recorder.h"
#undef private

#include "testing/utils/make_js_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace recorder {

TEST(NativeModuleRecorder, CircularReference_Simple) {
  auto rt = testing::utils::makeJSRuntime();
  runtime::js::Scope scope(*rt);

  auto buffer = std::make_unique<runtime::js::StringBuffer>(
      "let obj = {}; obj.self = obj; obj;");
  auto ret = rt->evaluateJavaScript(std::move(buffer), "");
  ASSERT_TRUE(ret.has_value());

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value result = NativeModuleRecorder::ParsePiperValueToJsonValue(
      ret.value(), rt.get(), &visited_objs);

  ASSERT_TRUE(result.IsObject());
  ASSERT_TRUE(result.HasMember("self"));
  ASSERT_TRUE(result["self"].IsString());
  EXPECT_STREQ(result["self"].GetString(), "[Circular Reference]");
}

TEST(NativeModuleRecorder, CircularReference_Mutual) {
  auto rt = testing::utils::makeJSRuntime();
  runtime::js::Scope scope(*rt);

  auto buffer = std::make_unique<runtime::js::StringBuffer>(
      "let a = {}; let b = {}; a.b = b; b.a = a; a;");
  auto ret = rt->evaluateJavaScript(std::move(buffer), "");
  ASSERT_TRUE(ret.has_value());

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value result = NativeModuleRecorder::ParsePiperValueToJsonValue(
      ret.value(), rt.get(), &visited_objs);

  ASSERT_TRUE(result.IsObject());
  ASSERT_TRUE(result.HasMember("b"));
  ASSERT_TRUE(result["b"].IsObject());
  ASSERT_TRUE(result["b"].HasMember("a"));
  ASSERT_TRUE(result["b"]["a"].IsString());
  EXPECT_STREQ(result["b"]["a"].GetString(), "[Circular Reference]");
}

TEST(NativeModuleRecorder, CircularReference_Array) {
  auto rt = testing::utils::makeJSRuntime();
  runtime::js::Scope scope(*rt);

  auto buffer = std::make_unique<runtime::js::StringBuffer>(
      "let arr = [1, 2]; arr.push(arr); arr;");
  auto ret = rt->evaluateJavaScript(std::move(buffer), "");
  ASSERT_TRUE(ret.has_value());

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value result = NativeModuleRecorder::ParsePiperValueToJsonValue(
      ret.value(), rt.get(), &visited_objs);

  ASSERT_TRUE(result.IsArray());
  ASSERT_EQ(result.Size(), 3u);
  ASSERT_TRUE(result[2].IsString());
  EXPECT_STREQ(result[2].GetString(), "[Circular Reference]");
}

TEST(NativeModuleRecorder, CircularReference_NestedObject) {
  auto rt = testing::utils::makeJSRuntime();
  runtime::js::Scope scope(*rt);

  auto buffer = std::make_unique<runtime::js::StringBuffer>(
      "let root = {}; root.level1 = {}; root.level1.level2 = {}; "
      "root.level1.level2.level3 = {}; root.level1.level2.level3.backToRoot = "
      "root; root;");
  auto ret = rt->evaluateJavaScript(std::move(buffer), "");
  ASSERT_TRUE(ret.has_value());

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value result = NativeModuleRecorder::ParsePiperValueToJsonValue(
      ret.value(), rt.get(), &visited_objs);

  ASSERT_TRUE(result.IsObject());
  ASSERT_TRUE(result.HasMember("level1"));
  ASSERT_TRUE(result["level1"].IsObject());
  ASSERT_TRUE(result["level1"].HasMember("level2"));
  ASSERT_TRUE(result["level1"]["level2"].IsObject());
  ASSERT_TRUE(result["level1"]["level2"].HasMember("level3"));
  ASSERT_TRUE(result["level1"]["level2"]["level3"].IsObject());
  ASSERT_TRUE(result["level1"]["level2"]["level3"].HasMember("backToRoot"));
  ASSERT_TRUE(result["level1"]["level2"]["level3"]["backToRoot"].IsString());
  EXPECT_STREQ(result["level1"]["level2"]["level3"]["backToRoot"].GetString(),
               "[Circular Reference]");
}

TEST(NativeModuleRecorder, CircularReference_MixedTypes) {
  auto rt = testing::utils::makeJSRuntime();
  runtime::js::Scope scope(*rt);

  auto buffer = std::make_unique<runtime::js::StringBuffer>(
      "let user = {name: 'Alice', age: 30, friends: []}; "
      "let friend1 = {name: 'Bob', friendOf: user}; "
      "let friend2 = {name: 'Charlie', friendOf: user}; "
      "user.friends.push(friend1); user.friends.push(friend2); user;");
  auto ret = rt->evaluateJavaScript(std::move(buffer), "");
  ASSERT_TRUE(ret.has_value());

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value result = NativeModuleRecorder::ParsePiperValueToJsonValue(
      ret.value(), rt.get(), &visited_objs);

  ASSERT_TRUE(result.IsObject());
  ASSERT_TRUE(result.HasMember("friends"));
  ASSERT_TRUE(result["friends"].IsArray());
  ASSERT_EQ(result["friends"].Size(), 2u);
  ASSERT_TRUE(result["friends"][0].IsObject());
  ASSERT_TRUE(result["friends"][0].HasMember("friendOf"));
  ASSERT_TRUE(result["friends"][0]["friendOf"].IsString());
  EXPECT_STREQ(result["friends"][0]["friendOf"].GetString(),
               "[Circular Reference]");
  ASSERT_TRUE(result["friends"][1].IsObject());
  ASSERT_TRUE(result["friends"][1].HasMember("friendOf"));
  ASSERT_TRUE(result["friends"][1]["friendOf"].IsString());
  EXPECT_STREQ(result["friends"][1]["friendOf"].GetString(),
               "[Circular Reference]");
}

TEST(NativeModuleRecorder, CircularReference_MultipleCycles) {
  auto rt = testing::utils::makeJSRuntime();
  runtime::js::Scope scope(*rt);

  auto buffer = std::make_unique<runtime::js::StringBuffer>(
      "let objA = {}; let objB = {}; let objC = {}; "
      "objA.toB = objB; objA.self = objA; objB.toC = objC; objC.toA = objA; "
      "objA;");
  auto ret = rt->evaluateJavaScript(std::move(buffer), "");
  ASSERT_TRUE(ret.has_value());

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value result = NativeModuleRecorder::ParsePiperValueToJsonValue(
      ret.value(), rt.get(), &visited_objs);

  ASSERT_TRUE(result.IsObject());
  ASSERT_TRUE(result.HasMember("self"));
  ASSERT_TRUE(result["self"].IsString());
  EXPECT_STREQ(result["self"].GetString(), "[Circular Reference]");
  ASSERT_TRUE(result.HasMember("toB"));
  ASSERT_TRUE(result["toB"].IsObject());
  ASSERT_TRUE(result["toB"].HasMember("toC"));
  ASSERT_TRUE(result["toB"]["toC"].IsObject());
  ASSERT_TRUE(result["toB"]["toC"].HasMember("toA"));
  ASSERT_TRUE(result["toB"]["toC"]["toA"].IsString());
  EXPECT_STREQ(result["toB"]["toC"]["toA"].GetString(), "[Circular Reference]");
}

TEST(NativeModuleRecorder, NoCircularReference_ComplexObject) {
  auto rt = testing::utils::makeJSRuntime();
  runtime::js::Scope scope(*rt);

  auto buffer = std::make_unique<runtime::js::StringBuffer>(
      "let root = {name: 'test', count: 42, items: ['item1', 'item2'], child: "
      "{id: 100}}; root;");
  auto ret = rt->evaluateJavaScript(std::move(buffer), "");
  ASSERT_TRUE(ret.has_value());

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value result = NativeModuleRecorder::ParsePiperValueToJsonValue(
      ret.value(), rt.get(), &visited_objs);

  ASSERT_TRUE(result.IsObject());
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
