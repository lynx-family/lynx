// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>

#include "base/include/fml/thread.h"
#include "clay/fml/logging.h"
#include "clay/fml/paths.h"
#include "clay/net/net_loader_manager.h"
#include "clay/testing/thread_test.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/resource/font_resource_manager.h"

namespace clay {

class FontResourceManagerTest : public clay::testing::ThreadTest {
 protected:
  void SetUp() override { font_collection_ = FontCollection::Instance(); }

  void TearDown() override {}

  static std::vector<std::string> GetLocalFilePath();
  static std::vector<std::string> GetNetFileURL();

 protected:
  std::shared_ptr<FontCollection> font_collection_;
};

std::vector<std::string> FontResourceManagerTest::GetLocalFilePath() {
  // first : init
  const std::string test_file =
      "gen/lynx/clay/third_party/txt/assets/Roboto-Bold.ttf";
  auto directory = fml::paths::GetExecutableDirectoryPath();
  std::string ttf_path;
  if (directory.first) {
    auto dir = directory.second;
    auto pos = dir.find_last_of('/');
    if (pos != std::string::npos && dir.substr(pos + 1) == "exe.unstripped") {
      dir = dir.substr(0, pos);
    }
    ttf_path = fml::paths::JoinPaths({dir, test_file});
  } else {
    FML_LOG(ERROR) << "Failed get test font ttf files.";
    EXPECT_TRUE(false);
  }
#if OS_WIN
  return {"file:///" + ttf_path};
#else
  return {"file://" + ttf_path};
#endif
}

std::vector<std::string> FontResourceManagerTest::GetNetFileURL() {
  const std::string url = "https://www.fontsaddict.com/fontface/raw.ttf";
  const std::string fail_url = "https://1";
  return {fail_url, url};
}

TEST_F(FontResourceManagerTest, GetLocalResourceTest) {
  const std::string family_name = "local_font_file";
  auto local_files = GetLocalFilePath();
  font_collection_->font_resource_manager_->LoadFontSync(family_name,
                                                         local_files);
  auto font_resource =
      font_collection_->font_resource_manager_->GetResource(family_name);

  EXPECT_NE(font_resource.data, nullptr);
  EXPECT_EQ((int)font_resource.length, 170760);
}

TEST_F(FontResourceManagerTest, DISABLED_GetNetWorkResourceTest) {
  const std::string family_name = "net_font_file";
  auto net_files = GetNetFileURL();
  font_collection_->font_resource_manager_->LoadFontSync(family_name,
                                                         net_files);
  auto font_resource =
      font_collection_->font_resource_manager_->GetResource(family_name);

  EXPECT_NE(font_resource.data, nullptr);
  EXPECT_EQ((int)font_resource.length, 61260);
}

TEST_F(FontResourceManagerTest, FontCollectionTest) {
  const std::string family_name = "local_font_file";
  auto local_files = GetLocalFilePath();
  font_collection_->font_resource_manager_->LoadFontSync(family_name,
                                                         local_files);
  auto font_resource =
      font_collection_->font_resource_manager_->GetResource(family_name);

  EXPECT_NE(font_resource.data, nullptr);

  EXPECT_EQ((int)font_resource.length, 170760);
}

}  // namespace clay
