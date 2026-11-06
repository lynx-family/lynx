// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/fixture_replay_harmony.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"
#include "third_party/zlib/contrib/minizip/zip.h"

namespace lynx {
namespace devtool {
namespace {

void RemoveDirectory(const std::string& path) {
  auto* directory = opendir(path.c_str());
  if (!directory) return;
  while (auto* entry = readdir(directory)) {
    std::string name = entry->d_name;
    if (name == "." || name == "..") continue;
    auto child = path + "/" + name;
    struct stat info {};
    if (lstat(child.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) {
      RemoveDirectory(child);
    } else {
      std::remove(child.c_str());
    }
  }
  closedir(directory);
  rmdir(path.c_str());
}

class HarmonyFixtureReplayTest : public ::testing::Test {
 protected:
  struct Entry {
    std::string path;
    std::string content;
    unsigned long attributes = 0;
  };

  void SetUp() override {
    root_ = ::testing::TempDir() + "lynx_harmony_fixture_XXXXXX";
    ASSERT_NE(mkdtemp(root_.data()), nullptr);
    archive_ = root_ + "/fixture.zip";
    directory_ = root_ + "/content";
    ASSERT_EQ(mkdir(directory_.c_str(), 0700), 0);
  }

  void TearDown() override { RemoveDirectory(root_); }

  void WriteArchive(const std::vector<Entry>& entries) {
    auto zip = zipOpen64(archive_.c_str(), APPEND_STATUS_CREATE);
    ASSERT_NE(zip, nullptr);
    for (const auto& entry : entries) {
      zip_fileinfo info{};
      info.external_fa = entry.attributes;
      ASSERT_EQ(zipOpenNewFileInZip(zip, entry.path.c_str(), &info, nullptr, 0,
                                    nullptr, 0, nullptr, Z_DEFLATED,
                                    Z_DEFAULT_COMPRESSION),
                ZIP_OK);
      ASSERT_EQ(
          zipWriteInFileInZip(zip, entry.content.data(), entry.content.size()),
          ZIP_OK);
      ASSERT_EQ(zipCloseFileInZip(zip), ZIP_OK);
    }
    ASSERT_EQ(zipClose(zip, nullptr), ZIP_OK);
  }

  void PatchCentralField(size_t offset, uint32_t value) {
    std::ifstream input(archive_, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(input)), {});
    const auto central = bytes.find("PK\1\2");
    ASSERT_NE(central, std::string::npos);
    for (size_t i = 0; i < 4; ++i)
      bytes[central + offset + i] = value >> (i * 8);
    std::ofstream output(archive_, std::ios::binary | std::ios::trunc);
    output.write(bytes.data(), bytes.size());
  }

  std::string root_;
  std::string archive_;
  std::string directory_;
};

TEST_F(HarmonyFixtureReplayTest, EvaluatesExtractedActionsAndSharedData) {
  WriteArchive({{"fixture.js", R"(export default function(ctx) {
    ctx.setGlobalProps(ctx.readAsset('props.json'));
    ctx.loadTemplate('https://example.com/page', 'template.bin', {count: 2});
    ctx.after(75, () => ctx.dispatch('sendGlobalEvent', {arguments: ['tick', []]}));
    ctx.sharedData('quoted"key', {value: 'escaped\\text'});
    ctx.register('Example', 'request', () => 1);
  })"},
                {"assets/", ""},
                {"assets/props.json", R"({"theme":"dark"})"},
                {"assets/template.bin", "template"},
                {"config.json", "unused invalid metadata"},
                {"component_list.json", "unused invalid metadata"}});
  auto result = LoadFixtureForReplay(archive_, directory_);
  ASSERT_TRUE(result.has_value()) << result.error();
  rapidjson::Document data;
  data.Parse(result->c_str());
  ASSERT_FALSE(data.HasParseError());
  const auto& actions = data["actions"];
  ASSERT_EQ(actions.Size(), 3u);
  EXPECT_STREQ(actions[0]["params"]["global_props"]["theme"].GetString(),
               "dark");
  EXPECT_EQ(actions[1]["params"]["templateData"]["count"].GetInt(), 2);
  // cspell:disable-next-line
  EXPECT_STREQ(actions[1]["params"]["source"].GetString(), "dGVtcGxhdGU=");
  EXPECT_EQ(actions[2]["delayMs"].GetInt(), 75);
  EXPECT_STREQ(data["sharedData"]["quoted\"key"]["value"].GetString(),
               "escaped\\text");
  EXPECT_EQ(access((directory_ + "/assets/template.bin").c_str(), R_OK), 0);
}

TEST_F(HarmonyFixtureReplayTest, SerializesEmptyEvaluation) {
  WriteArchive({{"fixture.js", "export default function() {}"}});
  auto result = LoadFixtureForReplay(archive_, directory_);
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(*result, R"({"actions":[],"sharedData":{}})");
}

TEST_F(HarmonyFixtureReplayTest, RejectsInvalidTemplateAssets) {
  for (const auto* path :
       {"../outside", "missing.bin", "assets/", "empty.bin"}) {
    SCOPED_TRACE(path);
    RemoveDirectory(directory_);
    ASSERT_EQ(mkdir(directory_.c_str(), 0700), 0);
    WriteArchive(
        {{"fixture.js", std::string("export default function(ctx) {") +
                            "ctx.loadTemplate('https://example.com', '" + path +
                            "', {});}"},
         {"assets/empty.bin", ""}});
    EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
  }
}

TEST_F(HarmonyFixtureReplayTest, BoundsRepeatedTemplateExpansion) {
  WriteArchive({{"fixture.js", R"(export default function(ctx) {
    for (let i = 0; i < 5; ++i) ctx.loadTemplate('url', 'page.bin', {});
  })"},
                {"assets/page.bin", std::string(3 * 1024 * 1024, 'x')}});
  auto result = LoadFixtureForReplay(archive_, directory_);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Fixture templates exceed size limit");
}

TEST_F(HarmonyFixtureReplayTest, RejectsUnsafePaths) {
  for (const auto* path :
       {"../escape", "/absolute", "assets/../escape", "assets//file",
        "./fixture.js", "C:/file", "assets\\file"}) {
    SCOPED_TRACE(path);
    WriteArchive({{path, "payload"}});
    EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
  }
  EXPECT_NE(access((root_ + "/escape").c_str(), F_OK), 0);
}

TEST_F(HarmonyFixtureReplayTest, RejectsDuplicateEntries) {
  WriteArchive({{"fixture.js", "export default function() {}"},
                {"fixture.js", "overwritten"}});
  EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
}

TEST_F(HarmonyFixtureReplayTest, RejectsSymlinksAndDirectoryPayloads) {
  WriteArchive({{"assets/link", "../../outside",
                 static_cast<unsigned long>(S_IFLNK) << 16}});
  EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
  WriteArchive({{"assets/", "payload"}});
  EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
}

TEST_F(HarmonyFixtureReplayTest, RejectsOversizedAndCorruptEntries) {
  WriteArchive({{"fixture.js", "export default function() {}"}});
  PatchCentralField(24, 128 * 1024 * 1024 + 1);
  EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
  WriteArchive({{"fixture.js", "export default function() {}"}});
  PatchCentralField(16, 0);
  EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
}

TEST_F(HarmonyFixtureReplayTest, RejectsMissingAndInvalidScripts) {
  WriteArchive({{"assets/data.json", "{}"}});
  EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
  WriteArchive({{"fixture.js", "export default function( {"}});
  EXPECT_FALSE(LoadFixtureForReplay(archive_, directory_).has_value());
}

}  // namespace
}  // namespace devtool
}  // namespace lynx
