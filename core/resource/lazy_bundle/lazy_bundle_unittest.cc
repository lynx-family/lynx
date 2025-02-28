// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/tasm/testing/event_tracker_mock.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/resource/lazy_bundle/lazy_bundle_lifecycle_option.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(LazyBundleTest, SendTrackEvent) {
  // settings is true, send a timing event
  tasm::LynxEnv::GetInstance().external_env_map_
      [tasm::LynxEnv::Key::ENABLE_REPORT_DYNAMIC_COMPONENT_EVENT] = "true";
  constexpr int32_t kInstanceId = 1;
  LazyBundleLifecycleOption("lynx", kInstanceId);

  // flush tasks
  tasm::report::EventTracker::Flush(kInstanceId);
  tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostSyncTask(
      []() {});

  // check event
  auto const& all_events = tasm::report::EventTrackerWaitableEvent::stack_;
  ASSERT_EQ(all_events.size(), static_cast<size_t>(1));
  ASSERT_EQ(all_events.front().GetName(), "lynxsdk_lazy_bundle_timing");
}

TEST(LazyBundleTest, ConstructSuccessMessageForMTS) {
  /**
   * |- code: int
   * |- data
   *   |- url: string
   *   |- sync: bool
   *   |- error_msg: string
   *   |- mode: string
   *   |- evalResult: object
   *   |- perf_info: object
   */
  lepus::Value expect_msg = lepus::Value(lepus::Dictionary::Create({
      {base::String("code"), lepus::Value(0)},
      {base::String("data"),
       lepus::Value(lepus::Dictionary::Create({
           {base::String("url"), lepus::Value("lynx")},
           {base::String("sync"), lepus::Value(true)},
           {base::String("error_msg"), lepus::Value("")},
           {base::String("mode"), lepus::Value("normal")},
           {base::String("evalResult"), lepus::Value("eval")},
           {base::String("perf_info"), lepus::Value("perf")},
       }))},
  }));
  lepus::Value value = lazy_bundle::ConstructSuccessMessageForMTS(
      "lynx", true, lepus::Value("eval"), LazyBundleState::STATE_SUCCESS,
      lepus::Value("perf"));
  ASSERT_EQ(expect_msg, value);
}

TEST(LazyBundleTest, ConstructErrorMessageForMTS) {
  /**
   * |- code: int
   * |- data
   *   |- url: string
   *   |- sync: bool
   *   |- error_msg: string
   *   |- mode: string
   */
  lepus::Value expect_msg = lepus::Value(lepus::Dictionary::Create({
      {base::String("code"), lepus::Value(1601)},
      {base::String("data"),
       lepus::Value(lepus::Dictionary::Create({
           {base::String("url"), lepus::Value("lynx")},
           {base::String("sync"), lepus::Value(false)},
           {base::String("error_msg"), lepus::Value("network error")},
           {base::String("mode"), lepus::Value("normal")},
       }))},
  }));
  lepus::Value value = lazy_bundle::ConstructErrorMessageForMTS(
      "lynx", 1601, "network error", false);
  ASSERT_EQ(expect_msg, value);
}

TEST(LazyBundleTest, ConstructSuccessMessageForBTS) {
  /**
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
   */
  lepus::Value expect_msg = lepus::Value(lepus::Dictionary::Create({
      {base::String("code"), lepus::Value(0)},
      {base::String("data"), lepus::Value(lepus::Dictionary::Create({
                                 {base::String("url"), lepus::Value("lynx")},
                                 {base::String("sync"), lepus::Value(false)},
                                 {base::String("error_msg"), lepus::Value("")},
                                 {base::String("mode"), lepus::Value("normal")},
                             }))},
      {base::String("detail"),
       lepus::Value(lepus::Dictionary::Create({
           {base::String("schema"), lepus::Value("lynx")},
           {base::String("errMsg"), lepus::Value("")},
           {base::String("cache"), lepus::Value(false)},
       }))},
  }));
  lepus::Value value = lazy_bundle::ConstructSuccessMessageForBTS("lynx");
  ASSERT_EQ(expect_msg, value);
}

TEST(LazyBundleTest, ConstructErrorMessageForBTS) {
  /**
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
   */
  lepus::Value expect_msg = lepus::Value(lepus::Dictionary::Create({
      {base::String("code"), lepus::Value(1602)},
      {base::String("data"),
       lepus::Value(lepus::Dictionary::Create({
           {base::String("url"), lepus::Value("lynx")},
           {base::String("sync"), lepus::Value(false)},
           {base::String("error_msg"), lepus::Value("decode error")},
           {base::String("mode"), lepus::Value("normal")},
       }))},
      {base::String("detail"),
       lepus::Value(lepus::Dictionary::Create({
           {base::String("schema"), lepus::Value("lynx")},
           {base::String("errMsg"), lepus::Value("decode error")},
           {base::String("cache"), lepus::Value(false)},
       }))},
  }));
  lepus::Value value =
      lazy_bundle::ConstructErrorMessageForBTS("lynx", 1602, "decode error");
  ASSERT_EQ(expect_msg, value);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
