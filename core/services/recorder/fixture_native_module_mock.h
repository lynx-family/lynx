// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_FIXTURE_NATIVE_MODULE_MOCK_H_
#define CORE_SERVICES_RECORDER_FIXTURE_NATIVE_MODULE_MOCK_H_

#include <string>

namespace lynx {
namespace tasm {
namespace recorder {

// Builds the JavaScript helper used by fixture.js to match and dispatch
// recorded NativeModule calls. ignored_keys_json must be a JSON array.
std::string BuildNativeModuleMockScript(const std::string& ignored_keys_json);

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_FIXTURE_NATIVE_MODULE_MOCK_H_
