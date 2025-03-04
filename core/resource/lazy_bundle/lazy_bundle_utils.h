// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_UTILS_H_
#define CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_UTILS_H_

#include <string>

#include "base/include/debug/lynx_error.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
namespace lazy_bundle {

constexpr char kEventFail[] = "fail";
constexpr char kEventSuccess[] = "success";
constexpr char kDetail[] = "detail";
constexpr char kSync[] = "sync";
constexpr char kEvalResult[] = "evalResult";

/**
 * To indicate the loading state of a lazy bundle.
 * STATE_CACHE and STATE_PRELOAD also indicate that the loading is successful,
 * but the resource sources are different. STATE_SUCCESS specifically refers to
 * loaded by LazyBundleLoader.
 */
enum class LazyBundleState : uint8_t {
  STATE_UNKNOWN = 0,
  STATE_SUCCESS,
  STATE_FAIL,
  STATE_PRELOAD,
  STATE_CACHE,
};

/**
 * Generate 'mode' field for lazy bundle event message to indicate how the
 * component is loaded. if the component is preloaded -> preload if the
 * component has been loaded and exists in the memory -> cache if the component
 * is loaded by loader -> normal
 */
std::string GenerateModeInfo(LazyBundleState state);

/**
 * Generate lazy bundle success message for main thread, format:
 * |- code: int
 * |- data
 *   |- url: string
 *   |- sync: bool
 *   |- error_msg: string
 *   |- mode: string
 *   |- evalResult: object
 *   |- perf_info: object
 */
lepus::Value ConstructSuccessMessageForMTS(
    const std::string& url, bool sync,
    lepus::Value eval_result = lepus::Value(),
    LazyBundleState state = LazyBundleState::STATE_SUCCESS,
    lepus::Value perf_info = lepus::Value());

/**
 * Generate lazy bundle error message for main thread, format:
 * |- code: int
 * |- data
 *   |- url: string
 *   |- sync: bool
 *   |- error_msg: string
 *   |- mode: string
 */
lepus::Value ConstructErrorMessageForMTS(const std::string& url, const int code,
                                         const std::string& error_msg,
                                         bool sync);

/**
 * Generate lazy bundle success message for background thread, format:
 * |- code: int
 * |- data
 *   |- url: string
 *   |- sync: bool
 *   |- error_msg: string
 *   |- mode: string
 * |- detail
 *   |- schema: string
 *   |- cache: bool
 *   |- errMsg: string
 * The differences from BT message is that:
 * 1. the 'detail' field is included for compatibility
 * 2. the 'sync' field always 'false'
 * 3. the 'evalResult' and 'perf_info' fields are not included for compatibility
 */
lepus::Value ConstructSuccessMessageForBTS(const std::string& url);

/**
 * Generate lazy bundle error message for background thread,
 * format is the same as SuccessMessageForBT
 */
lepus::Value ConstructErrorMessageForBTS(const std::string& url, int32_t code,
                                         const std::string& msg);

}  // namespace lazy_bundle
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_UTILS_H_
