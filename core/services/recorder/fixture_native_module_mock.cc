// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/fixture_native_module_mock.h"

namespace lynx {
namespace tasm {
namespace recorder {

namespace {

constexpr const char* kNativeModuleMockPrefix =
    R"JS(  // --- NativeModule mock matching (mirrors V1 strict matcher) ---
  var __ignoredKeys = )JS";

// This script defines the replay matching contract. Keep it readable so changes
// can be reviewed against the JSON recorder behavior.
constexpr const char* kNativeModuleMockSuffix = R"JS(;
  function __isIgnored(key) {
    return __ignoredKeys.indexOf(key) !== -1;
  }
  function __hasOwn(object, key) {
    return Object.prototype.hasOwnProperty.call(object, key);
  }
  function __sameUrl(actual, recorded) {
    if (actual.indexOf("http") !== 0 || recorded.indexOf("http") !== 0) {
      return false;
    }
    var actualParts = actual.split("?");
    var recordedParts = recorded.split("?");
    if (actualParts.length !== recordedParts.length ||
        actualParts[0] !== recordedParts[0]) {
      return false;
    }
    if (actualParts.length < 2) return true;
    var actualParams = actualParts[1].split("&");
    var recordedParams = recordedParts[1].split("&");
    if (actualParams.length !== recordedParams.length) return false;
    for (var i = 0; i < actualParams.length; i++) {
      if (actualParams[i] === recordedParams[i]) continue;
      var actualParam = actualParams[i].split("=");
      var recordedParam = recordedParams[i].split("=");
      if (actualParam[0] === recordedParam[0] &&
          __isIgnored(actualParam[0])) {
        continue;
      }
      return false;
    }
    return true;
  }
  function __match(actual, recorded) {
    if (recorded === "function" && typeof actual === "function") return true;
    if (recorded === "undefined" && actual === undefined) return true;
    if (recorded === "NaN" && typeof actual === "number" && actual !== actual) {
      return true;
    }
    if (typeof recorded === "string") {
      return typeof actual === "string" &&
             (actual === recorded || __sameUrl(actual, recorded));
    }
    if (typeof recorded === "number") {
      return typeof actual === "number" && Math.abs(actual - recorded) < 1e-7;
    }
    if (typeof recorded === "boolean") return actual === recorded;
    if (recorded === null) return actual === null;
    if (Array.isArray(recorded)) {
      if (!Array.isArray(actual) || actual.length !== recorded.length) {
        return false;
      }
      for (var i = 0; i < recorded.length; i++) {
        if (!__match(actual[i], recorded[i])) return false;
      }
      return true;
    }
    if (typeof recorded === "object") {
      if (actual === null || typeof actual !== "object" ||
          Array.isArray(actual)) {
        return false;
      }
      for (var key in actual) {
        if (!__hasOwn(actual, key)) continue;
        if (__isIgnored(key) && __hasOwn(recorded, key)) continue;
        if (!__hasOwn(recorded, key) ||
            !__match(actual[key], recorded[key])) {
          return false;
        }
      }
      return true;
    }
    return false;
  }
  function __matchArgs(actualArgs, recordedArgs) {
    if (!recordedArgs || actualArgs.length !== recordedArgs.length) {
      return false;
    }
    for (var i = 0; i < recordedArgs.length; i++) {
      if (!__match(actualArgs[i], recordedArgs[i])) return false;
    }
    return true;
  }
  function __dispatch(recordedCalls, actualArgs, callbacks) {
    for (var i = 0; i < recordedCalls.length; i++) {
      if (!__matchArgs(actualArgs, recordedCalls[i].args)) continue;
      var entry = recordedCalls[i];
      var recordedCallbacks = entry.callbacks || [];
      for (var j = 0; j < recordedCallbacks.length; j++) {
        var callback = callbacks[recordedCallbacks[j].index];
        if (callback) {
          callback(recordedCallbacks[j].value,
                   recordedCallbacks[j].delay || 0);
        }
      }
      return entry.returnValue;
    }
    return undefined;
  }
)JS";

}  // namespace

std::string BuildNativeModuleMockScript(const std::string& ignored_keys_json) {
  std::string script(kNativeModuleMockPrefix);
  script.append(ignored_keys_json);
  script.append(kNativeModuleMockSuffix);
  return script;
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
