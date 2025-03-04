// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lazy_bundle/lazy_bundle_utils.h"

#include <utility>

#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {
namespace lazy_bundle {

namespace {

/**
 * |- code: int
 * |- data
 *   |- url: string
 *   |- sync: bool
 *   |- error_msg: string
 *   |- mode: string
 *   |- evalResult: object
 *   |- perf_info: object
 * |- detail (compatible with old formats)
 *   |- schema: string
 *   |- cache: bool (meaningless now)
 *   |- errMsg: string
 */
lepus::Value ConstructEventMessage(
    const std::string& url, bool need_compatibility,
    int code = error::E_SUCCESS, const std::string& error_msg = "",
    bool sync = false, lepus::Value eval_result = lepus::Value(),
    LazyBundleState state = LazyBundleState::STATE_SUCCESS,
    lepus::Value perf_info = lepus::Value()) {
  BASE_STATIC_STRING_DECL(kCode, "code");
  BASE_STATIC_STRING_DECL(kData, "data");
  BASE_STATIC_STRING_DECL(kUrl, "url");
  BASE_STATIC_STRING_DECL(kErrorMsg, "error_msg");
  BASE_STATIC_STRING_DECL(kMode, "mode");
  BASE_STATIC_STRING_DECL(kPerfInfo, "perf_info");
  // following keys are deprecated
  BASE_STATIC_STRING_DECL(kSchema, "schema");
  BASE_STATIC_STRING_DECL(kErrMsg, "errMsg");
  BASE_STATIC_STRING_DECL(kCache, "cache");

  auto event_message = lepus::Dictionary::Create();

  // attach code
  event_message->SetValue(kCode, code);

  // attach data
  auto lepus_url = lepus::Value(url);
  auto lepus_msg = lepus::Value(error_msg);
  auto data_dict = lepus::Dictionary::Create({
      {kUrl, lepus_url},
      {kSync, lepus::Value(sync)},
      {kErrorMsg, lepus_msg},
      {kMode, lepus::Value(GenerateModeInfo(state))},
  });
  if (!eval_result.IsNil()) {
    data_dict->SetValue(BASE_STATIC_STRING(kEvalResult),
                        std::move(eval_result));
  }
  if (!perf_info.IsNil()) {
    data_dict->SetValue(kPerfInfo, std::move(perf_info));
  }
  event_message->SetValue(kData, std::move(data_dict));

  // attach detail if need to be compatible with old formats
  if (need_compatibility) {
    event_message->SetValue(BASE_STATIC_STRING(kDetail),
                            lepus::Dictionary::Create({
                                {kSchema, lepus_url},
                                {kCache, lepus::Value(false)},
                                {kErrMsg, lepus_msg},
                            }));
  }

  return lepus::Value(std::move(event_message));
}
}  // namespace

lepus::Value ConstructSuccessMessageForMTS(const std::string& url, bool sync,
                                           lepus::Value eval_result,
                                           LazyBundleState state,
                                           lepus::Value perf_info) {
  return ConstructEventMessage(url, false, error::E_SUCCESS, "", sync,
                               std::move(eval_result), state,
                               std::move(perf_info));
}

lepus::Value ConstructErrorMessageForMTS(const std::string& url, const int code,
                                         const std::string& error_msg,
                                         bool sync) {
  return ConstructEventMessage(url, false, code, error_msg, sync);
}

lepus::Value ConstructSuccessMessageForBTS(const std::string& url) {
  return ConstructEventMessage(url, true);
}

lepus::Value ConstructErrorMessageForBTS(const std::string& url, int32_t code,
                                         const std::string& msg) {
  return ConstructEventMessage(url, true, code, msg);
}

std::string GenerateModeInfo(LazyBundleState state) {
  switch (state) {
    case LazyBundleState::STATE_PRELOAD:
      return "preload";
    case LazyBundleState::STATE_CACHE:
      return "cache";
    default:
      break;
  }
  return "normal";
}

}  // namespace lazy_bundle
}  // namespace tasm
}  // namespace lynx
