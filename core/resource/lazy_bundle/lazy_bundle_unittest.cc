// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#define private public
#define protected public

#include "base/include/fml/task_runner.h"
#include "base/include/lynx_actor.h"
#include "core/renderer/tasm/testing/event_tracker_mock.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/resource/external_resource/external_resource_loader.h"
#include "core/resource/lazy_bundle/lazy_bundle_lifecycle_option.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/performance_controller.h"
#include "core/services/performance/performance_event_sender.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

namespace {

class ImmediateTaskRunner : public fml::TaskRunner {
 public:
  ImmediateTaskRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
};

struct RecordedPerformanceEntry {
  int call_count{0};
  std::unique_ptr<pub::Value> entry;
};

class RecordingPerformanceEventSender
    : public performance::PerformanceEventSender {
 public:
  explicit RecordingPerformanceEventSender(
      RecordedPerformanceEntry* recorded_entry)
      : PerformanceEventSender(std::make_shared<pub::PubValueFactoryDefault>()),
        recorded_entry_(recorded_entry) {}

  void OnPerformanceEvent(
      std::unique_ptr<pub::Value> entry,
      performance::EventType = performance::kEventTypeAll) override {
    ++recorded_entry_->call_count;
    recorded_entry_->entry = std::move(entry);
  }

 private:
  RecordedPerformanceEntry* recorded_entry_;
};

std::shared_ptr<shell::LynxActor<performance::PerformanceController>>
CreatePerformanceActor(RecordedPerformanceEntry* recorded_entry) {
  auto task_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  return std::make_shared<shell::LynxActor<performance::PerformanceController>>(
      std::make_unique<performance::PerformanceController>(
          std::make_unique<RecordingPerformanceEventSender>(recorded_entry),
          nullptr, 1),
      task_runner, 1);
}

void MarkMTSLazyBundleUrl(
    const std::shared_ptr<shell::LynxActor<performance::PerformanceController>>&
        perf_controller_actor,
    const std::string& url) {
  auto loader = std::make_shared<LazyBundleLoader>(nullptr);
  loader->SetPerfControllerActor(perf_controller_actor);
  loader->AppendUrlToLifecycleOptionMap(
      url, std::make_unique<LazyBundleLifecycleOption>(url, 1, true));
}

class ImmediateResourceLoader : public pub::LynxResourceLoader {
 public:
  explicit ImmediateResourceLoader(pub::LynxResourceResponse response)
      : response_(std::move(response)) {}

 protected:
  void LoadResourceInternal(
      const pub::LynxResourceRequest&,
      base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback)
      override {
    callback(response_);
  }

 private:
  pub::LynxResourceResponse response_;
};

}  // namespace

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

TEST(LazyBundleLoaderTest, CallBackInfoPreservesVerifyErrorCode) {
  auto status_info = LazyBundleLoader::CallBackInfo::CreateStatusInfo(
      error::E_APP_BUNDLE_VERIFY_INVALID_SIGNATURE, "invalid signature");

  LazyBundleLoader::CallBackInfo callback_info("lynx", {}, std::nullopt,
                                               std::move(status_info));

  EXPECT_EQ(error::E_APP_BUNDLE_VERIFY_INVALID_SIGNATURE,
            callback_info.error_code);
  EXPECT_NE(std::string::npos,
            callback_info.error_msg.find("invalid signature"));
}

TEST(LazyBundleLoaderTest, CallBackInfoPreservesBadResponseErrorCode) {
  auto status_info = LazyBundleLoader::CallBackInfo::CreateStatusInfo(
      error::E_APP_BUNDLE_LOAD_BAD_RESPONSE, "fetch binary is undefined");

  LazyBundleLoader::CallBackInfo callback_info("lynx", {}, std::nullopt,
                                               std::move(status_info));

  EXPECT_EQ(error::E_APP_BUNDLE_LOAD_BAD_RESPONSE, callback_info.error_code);
  EXPECT_NE(std::string::npos,
            callback_info.error_msg.find("fetch binary is undefined"));
}

TEST(LazyBundleLoaderTest, CallBackInfoMapsLocalErrorCode) {
  auto status_info =
      LazyBundleLoader::CallBackInfo::CreateStatusInfo(-1, "error response");

  LazyBundleLoader::CallBackInfo callback_info("lynx", {}, std::nullopt,
                                               std::move(status_info));

  EXPECT_EQ(error::E_LAZY_BUNDLE_LOAD_BAD_RESPONSE, callback_info.error_code);
  EXPECT_NE(std::string::npos, callback_info.error_msg.find("error response"));
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

TEST(LazyBundleTest, GetLazyBundleEntryForExternalSuccess) {
  constexpr int32_t kInstanceId = 1;
  auto option = LazyBundleLifecycleOption("lynx", kInstanceId, true);
  option.RecordRequireStart(100);
  option.RecordRequireEnd(150, true, 32);
  option.RecordDecodeStart(160);
  option.RecordDecodeEnd(190);
  option.SetLoadResult(true);

  auto performance_entry = option.GetLazyBundleEntry();
  ASSERT_NE(performance_entry, nullptr);
  EXPECT_EQ(performance_entry->GetValueForKey(timing::kEntryType)->str(),
            "resource");
  EXPECT_EQ(performance_entry->GetValueForKey(timing::kEntryName)->str(),
            "lazyBundle");
  EXPECT_EQ(performance_entry->GetValueForKey("componentUrl")->str(), "lynx");
  EXPECT_EQ(performance_entry->GetValueForKey("size")->Int64(), 32);
  EXPECT_TRUE(performance_entry->GetValueForKey("loadSuccess")->Bool());
  EXPECT_EQ(performance_entry->GetValueForKey("mode")->str(), "normal");
  EXPECT_EQ(performance_entry->GetValueForKey("requireStart")->UInt64(), 100);
  EXPECT_EQ(performance_entry->GetValueForKey("requireEnd")->UInt64(), 150);
  EXPECT_EQ(performance_entry->GetValueForKey("decodeStart")->UInt64(), 160);
  EXPECT_EQ(performance_entry->GetValueForKey("decodeEnd")->UInt64(), 190);
}

TEST(LazyBundleTest, GetLazyBundleEntryForExternalFailure) {
  constexpr int32_t kInstanceId = 1;
  auto option = LazyBundleLifecycleOption("lynx", kInstanceId, true);
  option.RecordRequireStart(100);
  option.RecordRequireEnd(150, true, 0);
  option.SetLoadResult(false);

  auto performance_entry = option.GetLazyBundleEntry();
  ASSERT_NE(performance_entry, nullptr);
  EXPECT_EQ(performance_entry->GetValueForKey("componentUrl")->str(), "lynx");
  EXPECT_EQ(performance_entry->GetValueForKey("size")->Int64(), 0);
  EXPECT_FALSE(performance_entry->GetValueForKey("loadSuccess")->Bool());
  EXPECT_EQ(performance_entry->GetValueForKey("mode")->str(), "normal");
  EXPECT_EQ(performance_entry->GetValueForKey("requireStart")->UInt64(), 100);
  EXPECT_EQ(performance_entry->GetValueForKey("requireEnd")->UInt64(), 150);
  EXPECT_EQ(performance_entry->GetValueForKey("decodeStart")->UInt64(), 0);
  EXPECT_EQ(performance_entry->GetValueForKey("decodeEnd")->UInt64(), 0);
}

TEST(LazyBundleTest, GetLazyBundleEntrySkipsCacheMode) {
  constexpr int32_t kInstanceId = 1;
  auto option = LazyBundleLifecycleOption("lynx", kInstanceId, true);
  option.mode = LazyBundleState::STATE_CACHE;

  EXPECT_EQ(option.GetLazyBundleEntry(), nullptr);
}

TEST(LazyBundleTest, ReportLazyBundleEntryOnlyOnce) {
  RecordedPerformanceEntry recorded_entry;
  auto perf_controller_actor = CreatePerformanceActor(&recorded_entry);
  auto option = LazyBundleLifecycleOption("lynx", 1, true);
  option.SetPerfControllerActor(perf_controller_actor);
  option.RecordRequireStart(100);
  option.RecordRequireEnd(150, true, 32);
  option.RecordDecodeStart(160);
  option.RecordDecodeEnd(190);
  option.SetLoadResult(true);

  option.ReportLazyBundleEntry();
  option.ReportLazyBundleEntry();

  ASSERT_EQ(recorded_entry.call_count, 1);
  ASSERT_NE(recorded_entry.entry, nullptr);
  EXPECT_EQ(recorded_entry.entry->GetValueForKey("componentUrl")->str(),
            "lynx");
  EXPECT_TRUE(recorded_entry.entry->GetValueForKey("loadSuccess")->Bool());
  EXPECT_EQ(recorded_entry.entry->GetValueForKey("size")->Int64(), 32);
}

TEST(LazyBundleTest, ExternalResourceFailureReportsLazyBundleEntry) {
  pub::LynxResourceResponse response;
  response.err_code = -1;
  response.err_msg = "resource request failed";
  auto resource_loader =
      std::make_shared<ImmediateResourceLoader>(std::move(response));
  auto external_resource_loader =
      std::make_shared<shell::ExternalResourceLoader>(resource_loader);
  RecordedPerformanceEntry recorded_entry;
  auto perf_controller_actor = CreatePerformanceActor(&recorded_entry);
  external_resource_loader->SetPerfControllerActor(perf_controller_actor);

  external_resource_loader->LoadLazyBundle("lynx", 1);

  ASSERT_EQ(recorded_entry.call_count, 1);
  ASSERT_NE(recorded_entry.entry, nullptr);
  EXPECT_FALSE(recorded_entry.entry->GetValueForKey("loadSuccess")->Bool());
  EXPECT_EQ(recorded_entry.entry->GetValueForKey("size")->Int64(), 0);
  EXPECT_EQ(recorded_entry.entry->GetValueForKey("decodeStart")->UInt64(), 0);
  EXPECT_EQ(recorded_entry.entry->GetValueForKey("decodeEnd")->UInt64(), 0);
}

TEST(LazyBundleTest, MTSLazyBundleUrlSuppressesExternalResourceEntry) {
  RecordedPerformanceEntry recorded_entry;
  auto perf_controller_actor = CreatePerformanceActor(&recorded_entry);
  MarkMTSLazyBundleUrl(perf_controller_actor, "lynx");

  pub::LynxResourceResponse response;
  response.err_code = -1;
  response.err_msg = "resource request failed";
  auto resource_loader =
      std::make_shared<ImmediateResourceLoader>(std::move(response));
  auto external_resource_loader =
      std::make_shared<shell::ExternalResourceLoader>(resource_loader);
  external_resource_loader->SetPerfControllerActor(perf_controller_actor);

  external_resource_loader->LoadLazyBundle("lynx", 1);

  EXPECT_EQ(recorded_entry.call_count, 0);
}

TEST(LazyBundleTest, MTSLazyBundleUrlDoesNotSuppressDifferentBTSUrl) {
  RecordedPerformanceEntry recorded_entry;
  auto perf_controller_actor = CreatePerformanceActor(&recorded_entry);
  MarkMTSLazyBundleUrl(perf_controller_actor, "mts-url");

  auto bts_option = LazyBundleLifecycleOption("bts-url", 1, true,
                                              LazyBundleEntryOrigin::kBTS);
  bts_option.SetPerfControllerActor(perf_controller_actor);
  bts_option.ReportLazyBundleEntry();

  ASSERT_EQ(recorded_entry.call_count, 1);
  ASSERT_NE(recorded_entry.entry, nullptr);
  EXPECT_EQ(recorded_entry.entry->GetValueForKey("componentUrl")->str(),
            "bts-url");
}

TEST(LazyBundleTest, BTSLazyBundleEntryBeforeMTSIsPreserved) {
  RecordedPerformanceEntry recorded_entry;
  auto perf_controller_actor = CreatePerformanceActor(&recorded_entry);

  auto first_bts_option =
      LazyBundleLifecycleOption("lynx", 1, true, LazyBundleEntryOrigin::kBTS);
  first_bts_option.SetPerfControllerActor(perf_controller_actor);
  first_bts_option.ReportLazyBundleEntry();
  ASSERT_EQ(recorded_entry.call_count, 1);

  MarkMTSLazyBundleUrl(perf_controller_actor, "lynx");

  auto second_bts_option =
      LazyBundleLifecycleOption("lynx", 1, true, LazyBundleEntryOrigin::kBTS);
  second_bts_option.SetPerfControllerActor(perf_controller_actor);
  second_bts_option.ReportLazyBundleEntry();
  EXPECT_EQ(recorded_entry.call_count, 1);

  auto mts_option = LazyBundleLifecycleOption("lynx", 1, true);
  mts_option.SetPerfControllerActor(perf_controller_actor);
  mts_option.ReportLazyBundleEntry();
  EXPECT_EQ(recorded_entry.call_count, 2);
}

TEST(LazyBundleTest, ReloadClearsMTSLazyBundleUrls) {
  RecordedPerformanceEntry recorded_entry;
  auto perf_controller_actor = CreatePerformanceActor(&recorded_entry);
  MarkMTSLazyBundleUrl(perf_controller_actor, "lynx");

  auto suppressed_bts_option =
      LazyBundleLifecycleOption("lynx", 1, true, LazyBundleEntryOrigin::kBTS);
  suppressed_bts_option.SetPerfControllerActor(perf_controller_actor);
  suppressed_bts_option.ReportLazyBundleEntry();
  EXPECT_EQ(recorded_entry.call_count, 0);

  perf_controller_actor->Act(
      [](auto& performance) { performance->ResetStateBeforeReload(); });

  auto bts_option_after_reload =
      LazyBundleLifecycleOption("lynx", 1, true, LazyBundleEntryOrigin::kBTS);
  bts_option_after_reload.SetPerfControllerActor(perf_controller_actor);
  bts_option_after_reload.ReportLazyBundleEntry();
  EXPECT_EQ(recorded_entry.call_count, 1);
}

TEST(LazyBundleTest, MissingExternalResourceLoaderReportsLazyBundleEntry) {
  auto external_resource_loader =
      std::make_shared<shell::ExternalResourceLoader>();
  RecordedPerformanceEntry recorded_entry;
  auto perf_controller_actor = CreatePerformanceActor(&recorded_entry);
  external_resource_loader->SetPerfControllerActor(perf_controller_actor);

  external_resource_loader->LoadLazyBundle("lynx", 1);

  ASSERT_EQ(recorded_entry.call_count, 1);
  ASSERT_NE(recorded_entry.entry, nullptr);
  EXPECT_FALSE(recorded_entry.entry->GetValueForKey("loadSuccess")->Bool());
  EXPECT_EQ(recorded_entry.entry->GetValueForKey("size")->Int64(), 0);
}

TEST(LazyBundleTest, ExternalResourceCancellationSkipsLazyBundleEntry) {
  pub::LynxResourceResponse response;
  response.data = {1};
  auto resource_loader =
      std::make_shared<ImmediateResourceLoader>(std::move(response));
  auto external_resource_loader =
      std::make_shared<shell::ExternalResourceLoader>(resource_loader);
  RecordedPerformanceEntry recorded_entry;
  auto perf_controller_actor = CreatePerformanceActor(&recorded_entry);
  external_resource_loader->SetPerfControllerActor(perf_controller_actor);

  // No engine actor means the page lifecycle has already been detached.
  external_resource_loader->LoadLazyBundle("lynx", 1);

  EXPECT_EQ(recorded_entry.call_count, 0);
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
