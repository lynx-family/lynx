// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/tasm/testing/event_tracker_mock.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/resource/lazy_bundle/lazy_bundle_lifecycle_option.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

// Test that pre-registered bundle can be retrieved via GetTemplateBundle
TEST(LazyBundleLoaderTest, InsertAndGetTemplateBundle) {
  // Create LazyBundleLoader without resource_loader (for testing cache only)
  auto loader = std::make_shared<LazyBundleLoader>(nullptr);

  // Create a dummy LynxTemplateBundle
  LynxTemplateBundle bundle;

  const std::string kTestUrl = "test_component_url";

  // Insert bundle into loader
  loader->InsertTemplateBundle(kTestUrl, bundle);

  // Retrieve bundle via GetTemplateBundle
  auto retrieved_bundle = loader->GetTemplateBundle(kTestUrl);

  // Verify bundle was retrieved successfully
  EXPECT_TRUE(retrieved_bundle.has_value());
}

// Test that GetTemplateBundle returns nullopt for non-existent URL
TEST(LazyBundleLoaderTest, GetTemplateBundleNotFound) {
  auto loader = std::make_shared<LazyBundleLoader>(nullptr);

  const std::string kNonExistentUrl = "non_existent_url";

  auto result = loader->GetTemplateBundle(kNonExistentUrl);

  EXPECT_FALSE(result.has_value());
}

// Test that LoadFrameBundle uses pre-registered bundle without network request
// when bundle is pre-inserted
TEST(LazyBundleLoaderTest, LoadFrameBundleUsesCache) {
  // Create LazyBundleLoader without resource_loader to ensure no network
  // request
  auto loader = std::make_shared<LazyBundleLoader>(nullptr);

  // Create and insert a pre-registered bundle
  LynxTemplateBundle bundle;
  const std::string kTestUrl = "cached_component_url";
  loader->InsertTemplateBundle(kTestUrl, bundle);

  // Verify the bundle is in loaded_bundles_ (cache)
  auto cached_bundle = loader->GetTemplateBundle(kTestUrl);
  EXPECT_TRUE(cached_bundle.has_value());

  // Verify requiring_urls_ is empty (no pending requests yet)
  // This ensures LoadFrameBundle can proceed to check cache
  EXPECT_TRUE(loader->requiring_urls_.empty());
}

TEST(LazyBundleTest, GetLazyBundleEntry) {
  constexpr int32_t kInstanceId = 1;
  auto option = LazyBundleLifecycleOption("lynx", kInstanceId, true);

  // check event
  auto performance_entry = option.GetLazyBundleEntry();
  int len = performance_entry->Length();
  std::string type =
      performance_entry->GetValueForKey(timing::kEntryType)->str();
  std::string name =
      performance_entry->GetValueForKey(timing::kEntryName)->str();
  EXPECT_EQ(type, "resource");
  EXPECT_EQ(name, "lazyBundle");
  EXPECT_EQ(len, 10);
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
