// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/fixture_native_module_mock.h"

#include <memory>
#include <string>

#include "testing/utils/make_js_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace recorder {
namespace {

bool EvaluateNativeModuleMock(const std::string& ignored_keys_json,
                              const std::string& test_body) {
  std::string script = "(function() {\n";
  script.append(BuildNativeModuleMockScript(ignored_keys_json));
  script.append(test_body);
  script.append("\n})();");

  auto runtime = testing::utils::makeJSRuntime();
  runtime::js::Scope scope(*runtime);
  auto result = runtime->evaluateJavaScript(
      std::make_unique<runtime::js::StringBuffer>(script),
      "fixture-native-module-mock.js");
  return result.has_value() && result.value().isBool() &&
         result.value().getBool();
}

TEST(FixtureNativeModuleMock, MatchesObjectsAndOwnProperties) {
  EXPECT_TRUE(EvaluateNativeModuleMock(R"(["timestamp"])", R"JS(
    var recorded = [{
      args: [{
        hasOwnProperty: 7,
        nested: {hasOwnProperty: 8},
        timestamp: 1
      }],
      returnValue: "matched"
    }];
    var payload = {
      hasOwnProperty: 7,
      nested: {hasOwnProperty: 8},
      timestamp: 2
    };
    if (__dispatch(recorded, [payload], []) !== "matched") return false;
    var withoutPrototype = Object.assign(Object.create(null), payload);
    if (__dispatch(recorded, [withoutPrototype], []) !== "matched") {
      return false;
    }
    var inherited = Object.assign(Object.create({extra: true}), payload);
    if (__dispatch(recorded, [inherited], []) !== "matched") return false;
    payload.nested.hasOwnProperty = 9;
    return __dispatch(recorded, [payload], []) === undefined;
  )JS"));
}

TEST(FixtureNativeModuleMock, MatchesSpecialValuesUrlsAndCallbacks) {
  EXPECT_TRUE(EvaluateNativeModuleMock(R"(["timestamp"])", R"JS(
    var recorded = [
      {args: ["function"], returnValue: "function-string"},
      {args: ["undefined"], returnValue: "undefined-string"},
      {
        args: ["NaN"],
        returnValue: "nan-string",
        callbacks: [{index: 0, value: "callback", delay: 5}]
      },
      {
        args: ["https://example.com/p?timestamp=1&fixed=x"],
        returnValue: "url"
      }
    ];
    var callbackResults = [];
    var callbacks = [function(value, delay) {
      callbackResults.push([value, delay]);
    }];
    if (__dispatch(recorded, [function() {}], callbacks) !==
        "function-string") {
      return false;
    }
    if (__dispatch(recorded, ["function"], callbacks) !==
        "function-string") {
      return false;
    }
    if (__dispatch(recorded, [{}], callbacks) !== undefined) return false;
    if (__dispatch(recorded, [undefined], callbacks) !==
        "undefined-string") {
      return false;
    }
    if (__dispatch(recorded, ["undefined"], callbacks) !==
        "undefined-string") {
      return false;
    }
    if (__dispatch(recorded, [null], callbacks) !== undefined) return false;
    if (__dispatch(recorded, [NaN], callbacks) !== "nan-string") return false;
    if (__dispatch(recorded, ["NaN"], callbacks) !== "nan-string") return false;
    if (__dispatch(recorded, [123], callbacks) !== undefined) return false;
    if (__dispatch(
            recorded,
            ["https://example.com/p?timestamp=2&fixed=x"],
            callbacks) !== "url") {
      return false;
    }
    if (__dispatch(
            recorded,
            ["https://example.com/p?timestamp=2&fixed=y"],
            callbacks) !== undefined) {
      return false;
    }
    return JSON.stringify(callbackResults) ===
        '[["callback",5],["callback",5]]';
  )JS"));
}

}  // namespace
}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
