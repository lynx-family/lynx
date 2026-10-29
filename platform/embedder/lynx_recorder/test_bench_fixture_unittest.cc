// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_recorder/test_bench_fixture.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "core/services/replay/fixture_context.h"
#include "platform/embedder/lynx_recorder/test_bench_utils.h"
#include "platform/embedder/public/capi/lynx_replay_capi.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/zlib/zlib.h"

namespace lynx {
namespace embedder {
namespace {
using Entry = std::pair<std::string, std::string>;
void Number(std::string& out, uint32_t value, size_t count) {
  for (size_t i = 0; i < count; ++i) out.push_back((value >> (8 * i)) & 255);
}
// A small stored ZIP producer, independent of the minizip extraction code.
std::string Archive(const std::vector<Entry>& entries) {
  std::string data, central;
  for (const auto& entry : entries) {
    uint32_t offset = data.size();
    uint32_t crc = crc32(0, reinterpret_cast<const Bytef*>(entry.second.data()),
                         entry.second.size());
    Number(data, 0x04034b50, 4);
    Number(data, 20, 2);
    data.append(8, '\0');
    Number(data, crc, 4);
    Number(data, entry.second.size(), 4);
    Number(data, entry.second.size(), 4);
    Number(data, entry.first.size(), 2);
    Number(data, 0, 2);
    data += entry.first;
    data += entry.second;
    Number(central, 0x02014b50, 4);
    Number(central, 20, 2);
    Number(central, 20, 2);
    central.append(8, '\0');
    Number(central, crc, 4);
    Number(central, entry.second.size(), 4);
    Number(central, entry.second.size(), 4);
    Number(central, entry.first.size(), 2);
    central.append(12, '\0');
    Number(central, offset, 4);
    central += entry.first;
  }
  uint32_t offset = data.size();
  data += central;
  Number(data, 0x06054b50, 4);
  data.append(4, '\0');
  Number(data, entries.size(), 2);
  Number(data, entries.size(), 2);
  Number(data, central.size(), 4);
  Number(data, offset, 4);
  Number(data, 0, 2);
  return data;
}
std::vector<Entry> Entries(const std::string& script = R"(
export default function(ctx) {
  ctx.after(25, () => ctx.loadTemplate('fixture', 'template/page.bin', {value: 7}));
  ctx.after(70, () => ctx.dispatch('updateDataByPreParsedData', {value: {count: 2}}));
  ctx.after(90, () => ctx.sendGlobalEvent(['fixture-event', {value: 3}]));
  ctx.register('FixtureModule', 'fetch', (args, callback) => {
    callback[0](ctx.readAsset('result.json'), 15);
    return {ok: true};
  });
})") {
  return {{"fixture.js", script},
          {"assets/template/page.bin", std::string("binary\0data", 11)},
          {"assets/result.json", R"({"answer":42})"},
          {"assets/empty.txt", ""},
          {"config.json", R"({"jsbSettings":{}})"},
          {"component_list.json", "[]"}};
}

TEST(DesktopFixture, EvaluatesActionsAndUsesTheSameDirectoryForModules) {
  std::string error;
  auto fixture = TestBenchFixture::Open(Archive(Entries()), &error);
  ASSERT_TRUE(fixture) << error;
  rapidjson::Document record;
  record.Parse(fixture->record());
  ASSERT_FALSE(record.HasParseError());
  EXPECT_EQ(record["Action List"][0]["RecordMillisecond"].GetInt64(), 25);
  EXPECT_EQ(record["Action List"][1]["RecordMillisecond"].GetInt64(), 70);
  EXPECT_EQ(record["Action List"][2]["Function Name"], "sendGlobalEvent");
  ASSERT_TRUE(record["Action List"][2]["Params"].IsArray());
  EXPECT_EQ(record["Action List"][2]["Params"][0], "fixture-event");
  EXPECT_TRUE(record["Config"].IsObject());
  EXPECT_TRUE(record["Component List"].IsArray());
  ASSERT_NE(fixture->Template("template/page.bin"), nullptr);
  EXPECT_EQ(*fixture->Template("template/page.bin"),
            std::string("binary\0data", 11));
  tasm::replay::FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture->directory()));
  auto result = context.Dispatch("FixtureModule", "fetch", "[{},null]", {1});
  ASSERT_TRUE(result.handled);
  EXPECT_EQ(result.return_value_json, R"({"ok":true})");
  ASSERT_EQ(result.callbacks.size(), 1u);
  EXPECT_EQ(result.callbacks[0].value_json, R"({"answer":42})");
  EXPECT_EQ(result.callbacks[0].delay_ms, 15);
  context.Destroy();
}

TEST(DesktopFixture, ReadsUtf8AssetPaths) {
  auto entries = Entries(R"(
export default function(ctx) {
  ctx.loadTemplate('fixture', 'template/€.bin', {});
  ctx.register('FixtureModule', 'read', () => ctx.readAsset('😀.json'));
})");
  entries[1].first = "assets/template/€.bin";
  entries[2].first = "assets/😀.json";
  std::string error;
  auto fixture = TestBenchFixture::Open(Archive(entries), &error);
  ASSERT_TRUE(fixture) << error;
  tasm::replay::FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture->directory()));
  auto result = context.Dispatch("FixtureModule", "read", "[]", {});
  EXPECT_TRUE(result.handled);
  EXPECT_EQ(result.return_value_json, R"({"answer":42})");
}

TEST(DesktopFixture, AssetsSurviveTheReplayOwnerUntilTheContextLeaseReleases) {
  std::string error, path;
  auto owner = TestBenchFixture::Open(Archive(Entries()), &error);
  ASSERT_TRUE(owner) << error;
  path = owner->directory();
  auto lease = owner;
  owner.reset();
  EXPECT_TRUE(
      ::fml::OpenDirectory(path.c_str(), false, ::fml::FilePermission::kRead)
          .is_valid());
  lease.reset();
  EXPECT_FALSE(
      ::fml::OpenDirectory(path.c_str(), false, ::fml::FilePermission::kRead)
          .is_valid());
}

TEST(DesktopFixture, RejectsUnsafeAndAmbiguousArchivePaths) {
  const std::vector<std::string> paths = {"../escape",
                                          "/absolute",
                                          "a/../escape",
                                          "a//b",
                                          "a\\b",
                                          "C:asset",
                                          "assets/NUL.txt",
                                          "assets/file.",
                                          "assets/file ",
                                          "assets/a:b",
                                          std::string("assets/a\0b", 10),
                                          "FIXTURE.JS",
                                          "fixture.js"};
  for (const auto& path : paths) {
    auto entries = Entries();
    entries.emplace_back(path, "bad");
    std::string error;
    EXPECT_FALSE(TestBenchFixture::Open(Archive(entries), &error)) << path;
    EXPECT_FALSE(error.empty());
  }
}

TEST(DesktopFixture, RejectsCorruptionMissingAssetsAndScriptFailures) {
  auto cases = std::vector<std::vector<Entry>>{
      Entries("throw new Error('broken');"),
      Entries("export default function(ctx) {}"),
      Entries("export default function(ctx) { "
              "ctx.loadTemplate('x', 'template/page.bin', {}); "
              "ctx.dispatch('initialLynxView', null); }"),
      Entries("export default function(ctx) { ctx.after(1e20, () => "
              "ctx.loadTemplate('x', 'template/page.bin', {})); }"),
      Entries("export default function(ctx) { ctx.loadTemplate('x', "
              "'../escape', {}); }")};
  auto missing = Entries();
  missing.erase(missing.begin() + 1);
  cases.push_back(missing);
  auto metadata = Entries();
  metadata.back().second = "{";
  cases.push_back(metadata);
  auto directory = Entries();
  directory.emplace_back("extra/", "payload");
  cases.push_back(directory);
  for (const auto& entries : cases) {
    std::string error;
    EXPECT_FALSE(TestBenchFixture::Open(Archive(entries), &error));
    EXPECT_FALSE(error.empty());
  }
  auto zip = Archive(Entries());
  zip[14] ^= 1;  // Corrupt local CRC without changing the central directory.
  std::string error;
  EXPECT_FALSE(TestBenchFixture::Open(zip, &error));
  EXPECT_FALSE(TestBenchFixture::Open(zip.substr(0, 40), &error));
  EXPECT_FALSE(TestBenchFixture::IsArchive("{\"Action List\":[]}"));
}

TEST(DesktopFixture, CApiReportsFailuresSynchronously) {
  int calls = 0;
  EXPECT_FALSE(lynx_replay_evaluate_fixture(
      nullptr, &calls, [](void* data, const char* message, size_t length) {
        ++*static_cast<int*>(data);
        EXPECT_GT(length, 0u);
        EXPECT_NE(message, nullptr);
      }));
  EXPECT_EQ(calls, 1);
}
}  // namespace
}  // namespace embedder
}  // namespace lynx
