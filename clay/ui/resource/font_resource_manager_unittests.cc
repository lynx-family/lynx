// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <atomic>
#include <memory>
#include <string>

#include "base/include/fml/synchronization/waitable_event.h"
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

TEST_F(FontResourceManagerTest, DataUriReportsLoadingUntilDecodeCompletes) {
  auto font_resource_manager = std::make_shared<FontResourceManager>();
  auto load_task_runner = CreateNewThread("font-loader");
  fml::AutoResetWaitableEvent task_started;
  fml::AutoResetWaitableEvent allow_task_to_finish;
  load_task_runner->PostTask([&]() {
    task_started.Signal();
    allow_task_to_finish.Wait();
  });
  task_started.Wait();

  const std::string family_name = "async_data_uri_font";
  std::atomic_bool callback_called = false;
  font_resource_manager->LoadFontAsync(
      load_task_runner, nullptr, nullptr, family_name,
      {"data:font/ttf;base64,AA=="},
      [&](bool success, const std::string&, const std::string&) {
        callback_called = success;
      });

  EXPECT_TRUE(font_resource_manager->HasFontResourceLoading(family_name));
  EXPECT_FALSE(font_resource_manager->HasFontResource(family_name));

  allow_task_to_finish.Signal();
  load_task_runner->PostSyncTask([]() {});

  EXPECT_TRUE(callback_called.load());
  EXPECT_FALSE(font_resource_manager->HasFontResourceLoading(family_name));
  EXPECT_TRUE(font_resource_manager->HasFontResource(family_name));
}

TEST_F(FontResourceManagerTest, FontCallbackCanBeCancelled) {
  const std::string family_name = "cancelled_font_callback";
  bool callback_called = false;
  const auto callback_id = font_collection_->RegisterCallback(
      family_name, [&]() { callback_called = true; });

  EXPECT_EQ(font_collection_->font_download_callback_.count(family_name), 1u);
  font_collection_->UnregisterCallback(callback_id);
  EXPECT_EQ(font_collection_->font_download_callback_.count(family_name), 0u);

  font_collection_->OnLoadFontEnd(family_name);
  EXPECT_FALSE(callback_called);
}

TEST_F(FontResourceManagerTest, FailedFontLoadClearsCallbacks) {
  const std::string family_name = "failed_font_callback";
  bool callback_called = false;
  font_collection_->RegisterCallback(family_name,
                                     [&]() { callback_called = true; });
  auto load_task_runner = CreateNewThread("failed-font-loader");

  font_collection_->PreLoadFontOnMem(load_task_runner, nullptr, nullptr,
                                     family_name, {"data:font/ttf,invalid"});
  load_task_runner->PostSyncTask([]() {});

  EXPECT_EQ(font_collection_->font_download_callback_.count(family_name), 0u);
  EXPECT_FALSE(callback_called);
  EXPECT_FALSE(font_collection_->HasFontResourceLoading(family_name));
}

TEST_F(FontResourceManagerTest, FailedFontLoadCanBeRetried) {
  auto font_resource_manager = std::make_shared<FontResourceManager>();
  auto load_task_runner = CreateNewThread("retry-font-loader");
  const std::string family_name = "retry_font_callback";
  std::atomic_int callback_count = 0;
  std::atomic_bool first_load_succeeded = true;
  std::atomic_bool retry_succeeded = false;
  fml::AutoResetWaitableEvent first_load_finished;
  fml::AutoResetWaitableEvent retry_finished;

  font_resource_manager->LoadFontAsync(
      load_task_runner, nullptr, nullptr, family_name,
      {"data:font/ttf,invalid", "data:application/font,invalid"},
      [&](bool success, const std::string&, const std::string&) {
        ++callback_count;
        first_load_succeeded = success;
        first_load_finished.Signal();
      });
  first_load_finished.Wait();

  EXPECT_EQ(callback_count, 1);
  EXPECT_FALSE(first_load_succeeded);
  EXPECT_FALSE(font_resource_manager->HasFontResourceLoading(family_name));

  font_resource_manager->LoadFontAsync(
      load_task_runner, nullptr, nullptr, family_name,
      {"data:font/ttf;base64,AA=="},
      [&](bool success, const std::string&, const std::string&) {
        ++callback_count;
        retry_succeeded = success;
        retry_finished.Signal();
      });
  retry_finished.Wait();

  EXPECT_EQ(callback_count, 2);
  EXPECT_TRUE(retry_succeeded);
  EXPECT_TRUE(font_resource_manager->HasFontResource(family_name));
}

TEST_F(FontResourceManagerTest, ConcurrentFontLoadsShareOneResult) {
  auto font_resource_manager = std::make_shared<FontResourceManager>();
  auto load_task_runner = CreateNewThread("deduplicated-font-loader");
  fml::AutoResetWaitableEvent task_started;
  fml::AutoResetWaitableEvent allow_task_to_finish;
  load_task_runner->PostTask([&]() {
    task_started.Signal();
    allow_task_to_finish.Wait();
  });
  task_started.Wait();

  const std::string family_name = "deduplicated_font_load";
  std::atomic_int callback_count = 0;
  auto callback = [&](bool success, const std::string&, const std::string&) {
    EXPECT_TRUE(success);
    ++callback_count;
  };
  font_resource_manager->LoadFontAsync(load_task_runner, nullptr, nullptr,
                                       family_name,
                                       {"data:font/ttf;base64,AA=="}, callback);
  font_resource_manager->LoadFontAsync(load_task_runner, nullptr, nullptr,
                                       family_name,
                                       {"data:font/ttf;base64,AQ=="}, callback);

  EXPECT_TRUE(font_resource_manager->HasFontResourceLoading(family_name));
  allow_task_to_finish.Signal();
  load_task_runner->PostSyncTask([]() {});

  EXPECT_EQ(callback_count, 2);
  EXPECT_TRUE(font_resource_manager->HasFontResource(family_name));
  EXPECT_FALSE(font_resource_manager->HasFontResourceLoading(family_name));
}

TEST_F(FontResourceManagerTest, CachedFontLoadCompletesImmediately) {
  auto font_resource_manager = std::make_shared<FontResourceManager>();
  auto load_task_runner = CreateNewThread("cached-font-loader");
  const std::string family_name = "cached_font_load";
  font_resource_manager->LoadFontAsync(
      load_task_runner, nullptr, nullptr, family_name,
      {"data:font/ttf;base64,AA=="},
      [](bool, const std::string&, const std::string&) {});
  load_task_runner->PostSyncTask([]() {});
  ASSERT_TRUE(font_resource_manager->HasFontResource(family_name));

  bool callback_called = false;
  font_resource_manager->LoadFontAsync(
      load_task_runner, nullptr, nullptr, family_name,
      {"data:font/ttf;base64,AQ=="},
      [&](bool success, const std::string&, const std::string&) {
        callback_called = true;
        EXPECT_TRUE(success);
      });

  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(font_resource_manager->HasFontResourceLoading(family_name));
}

}  // namespace clay
