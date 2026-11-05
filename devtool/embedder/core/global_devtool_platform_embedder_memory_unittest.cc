// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/global_devtool_platform_embedder_memory.h"

#include <atomic>
#include <thread>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

TEST(EmbedderMemoryUsageTest, PreservesSharedVmTotalsAndPageAttribution) {
  tasm::performance::GlobalMemoryUsage usage;
  usage.total.total_bytes = 3000;
  usage.total.element_bytes = 200;
  usage.total.element_count = 12;
  usage.total.mts_bytes = 300;
  usage.total.bts_bytes = 2500;
  for (int32_t id : {2, 1}) {
    tasm::performance::InstanceMemoryUsage instance;
    instance.instance_id = id;
    instance.url = "file:///page?name=\"test\"&emoji=😀";
    instance.page.total_bytes = id == 1 ? 1100 : 1000;
    instance.page.bts_bytes = 700;
    instance.page.bts_shared = true;
    instance.bts_heap_bytes = 2500;
    instance.bts_runtime_type = "quickjs";
    usage.instances.push_back(std::move(instance));
  }
  auto result = BuildEmbedderMemoryUsageResult(usage, 100, 10, 2000, 6000);
  // The monitor already counted the shared VM once. Do not sum page totals
  // or add each page's whole VM heap again at the protocol boundary.
  EXPECT_EQ(result["totalBytes"].asInt64(), 3000);
  EXPECT_EQ(result["backgroundThreadRuntimeBytes"].asInt64(), 2500);
  EXPECT_DOUBLE_EQ(result["ratioToApp"].asDouble(), 0.5);
  EXPECT_EQ(result["elementNodeCount"].asInt(), 12);
  const auto& instance = result["instances"][0];
  EXPECT_EQ(instance["instanceId"].asInt(), 1);
  EXPECT_EQ(instance["backgroundThreadRuntimeBytes"].asInt64(), 700);
  EXPECT_EQ(instance["btsHeapBytes"].asInt64(), 2500);
  EXPECT_TRUE(instance["btsShared"].asBool());
  EXPECT_TRUE(instance["viewDetail"].isObject());
  EXPECT_TRUE(instance["viewDetail"].empty());
  EXPECT_EQ(instance["btsRuntimeType"].asString(), "quickjs");
  EXPECT_EQ(instance["btsRuntimeTotalBytes"].asInt64(), 2500);
  EXPECT_EQ(instance["viewBytes"].asInt64(), 0);
  Json::FastWriter writer;
  Json::Value decoded;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(writer.write(result), decoded));
  EXPECT_EQ(decoded["instances"][0]["url"].asString(), usage.instances[1].url);
}

TEST(EmbedderMemoryUsageTest, EmptySnapshotKeepsArrayAndUnavailableAppMemory) {
  auto result = BuildEmbedderMemoryUsageResult({}, 100, 10, 2000, -1);
  EXPECT_TRUE(result["instances"].isArray());
  EXPECT_TRUE(result["instances"].empty());
  EXPECT_TRUE(result["destroyedInstances"].isArray());
  EXPECT_TRUE(result["destroyedInstances"].empty());
  EXPECT_FALSE(result.isMember("vms"));
  EXPECT_EQ(result["expectedInstanceCount"].asUInt64(), 0u);
  EXPECT_EQ(result["completedInstanceCount"].asUInt64(), 0u);
  EXPECT_EQ(result["appBytes"].asInt64(), 0);
  EXPECT_DOUBLE_EQ(result["ratioToApp"].asDouble(), 0.0);
}

TEST(EmbedderMemoryUsageTest,
     PreservesDestroyedInstanceIdentityAndOmitsPageMetrics) {
  tasm::performance::GlobalMemoryUsage usage;
  tasm::performance::InstanceMemoryUsage destroyed;
  destroyed.instance_id = 9;
  destroyed.page_id = "native-page-key";
  destroyed.url = "https://host/page?route=detail#original😀";
  destroyed.bts_runtime_group_id = "shared";
  destroyed.bts_runtime_type = "quickjs";
  destroyed.bts_heap_bytes = 1000;
  destroyed.bts_residual_bytes = 400;
  usage.destroyed_instances.push_back(destroyed);
  const auto result =
      BuildEmbedderMemoryUsageResult(usage, 100, 10, 2000, 6000);
  ASSERT_EQ(result["destroyedInstances"].size(), 1u);
  const auto& page = result["destroyedInstances"][0];
  EXPECT_EQ(page["instanceId"].asInt(), 9);
  EXPECT_EQ(page["pageId"].asString(), "native-page-key");
  EXPECT_EQ(page["url"].asString(), destroyed.url);
  EXPECT_EQ(page["btsRuntimeGroupId"].asString(), "shared");
  EXPECT_EQ(page["btsRuntimeType"].asString(), "quickjs");
  EXPECT_EQ(page["btsRuntimeTotalBytes"].asInt64(), 1000);
  EXPECT_EQ(page["backgroundThreadRuntimeResidualBytes"].asInt64(), 400);
  for (const char* key : {"totalBytes", "elementBytes", "elementNodeCount",
                          "viewBytes", "viewDetail", "mainThreadRuntimeBytes",
                          "backgroundThreadRuntimeBytes"}) {
    EXPECT_FALSE(page.isMember(key)) << key;
  }
}

TEST(EmbedderMemoryUsageTest, MonitoringDisabledIsAnError) {
  tasm::performance::GlobalMemoryUsage usage;
  usage.status = tasm::performance::MemoryUsageQueryStatus::kMonitoringDisabled;
  const auto result = BuildEmbedderMemoryUsageResult(usage, 100, 10, 2000, 0);
  EXPECT_EQ(result["collectionStatus"].asString(), "error");
  EXPECT_EQ(result["errorMessage"].asString(),
            tasm::performance::kMemoryMonitoringDisabledError);
  EXPECT_DOUBLE_EQ(result["ratioToApp"].asDouble(), 0.0);
  EXPECT_TRUE(result["instances"].empty());
  EXPECT_TRUE(result["destroyedInstances"].empty());
}

TEST(EmbedderMemoryUsageTest, TimeoutSuppressesLateMonitorResult) {
  int completions = 0;
  std::string error;
  EmbedderMemoryUsageRequest request(
      [&completions, &error](const auto&, const auto& message) {
        ++completions;
        error = message;
      });
  request.Complete("{}", "timeout");
  request.Complete("{\"totalBytes\":100}", "");
  EXPECT_EQ(completions, 1);
  EXPECT_EQ(error, "timeout");
}

TEST(EmbedderMemoryUsageTest, ConcurrentCompletionRespondsExactlyOnce) {
  std::atomic<int> completions{0};
  EmbedderMemoryUsageRequest request(
      [&completions](const auto&, const auto&) { ++completions; });
  std::thread timeout([&request] { request.Complete("{}", "timeout"); });
  std::thread monitor([&request] { request.Complete("{}", ""); });
  timeout.join();
  monitor.join();
  EXPECT_EQ(completions.load(), 1);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
