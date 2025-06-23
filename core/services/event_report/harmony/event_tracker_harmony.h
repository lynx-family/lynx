// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_EVENT_REPORT_HARMONY_EVENT_TRACKER_HARMONY_H_
#define CORE_SERVICES_EVENT_REPORT_HARMONY_EVENT_TRACKER_HARMONY_H_

#include <node_api.h>

#include <string>
#include <unordered_map>

namespace lynx {
namespace tasm {
namespace report {
namespace harmony {
// The last thread strategy this lynxView is using, will be updated when the
// lynxView is init.
constexpr const static char* kPropThreadMode = "thread_mode";

/**
 * @brief Initializes the EventReporter class and exports it to JavaScript.
 *
 * This function defines the EventReporter class with its properties and
 * methods, and exports it to the JavaScript environment. It registers the class
 * and its methods using the Node-API (N-API).
 *
 * @param env The N-API environment.
 * @param exports The exports object to which the EventReporter class will be
 * added.
 * @return The modified exports object with the EventReporter class added.
 */
napi_value Init(napi_env env, napi_value exports);
}  // namespace harmony
}  // namespace report
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_EVENT_REPORT_HARMONY_EVENT_TRACKER_HARMONY_H_
