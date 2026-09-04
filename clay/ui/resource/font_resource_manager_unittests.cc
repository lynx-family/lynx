// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/thread.h"
#include "clay/common/service/service_manager.h"
#include "clay/fml/logging.h"
#include "clay/fml/paths.h"
#include "clay/net/loader/resource_loader_creator_service.h"
#include "clay/net/net_loader_manager.h"
#include "clay/testing/thread_test.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/resource/font_resource_manager.h"

namespace clay {
namespace {

class EphemeralBufferResourceLoader final : public ResourceLoader {
 public:
  EphemeralBufferResourceLoader(
      fml::RefPtr<fml::TaskRunner> callback_task_runner,
      std::shared_ptr<std::vector<uint8_t>> callback_buffer,
      std::atomic_bool* callback_on_runner)
      : callback_task_runner_(std::move(callback_task_runner)),
        callback_buffer_(std::move(callback_buffer)),
        callback_on_runner_(callback_on_runner) {}

  void Load(const std::string&,
            const std::function<void(const uint8_t*, size_t)>& callback,
            const ResourceType, bool) override {
    callback_task_runner_->PostTask(
        [callback, callback_task_runner = callback_task_runner_,
         callback_buffer = callback_buffer_,
         callback_on_runner = callback_on_runner_]() {
          callback_on_runner->store(
              callback_task_runner->RunsTasksOnCurrentThread());
          callback(callback_buffer->data(), callback_buffer->size());
          std::fill(callback_buffer->begin(), callback_buffer->end(), 0xFF);
        });
  }

  RawResource LoadSync(const std::string&, const ResourceType, bool) override {
    return {0, nullptr};
  }

 private:
  fml::RefPtr<fml::TaskRunner> callback_task_runner_;
  std::shared_ptr<std::vector<uint8_t>> callback_buffer_;
  std::atomic_bool* callback_on_runner_;
};

}  // namespace

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

TEST_F(FontResourceManagerTest, DataUriPreparedOnIOAndCompletedOnLoadRunner) {
  auto font_resource_manager = std::make_shared<FontResourceManager>();
  auto io_task_runner = CreateNewThread("font-io");
  auto load_task_runner = CreateNewThread("font-loader");
  fml::AutoResetWaitableEvent io_task_started;
  fml::AutoResetWaitableEvent allow_io_task;
  io_task_runner->PostTask([&]() {
    io_task_started.Signal();
    allow_io_task.Wait();
  });
  io_task_started.Wait();

  const std::string family_name = "async_data_uri_font";
  const std::string data_url = "data:font/ttf;base64,AA==";
  std::atomic_int callback_count = 0;
  std::atomic_bool callback_success = false;
  std::atomic_bool callback_on_load_runner = false;
  std::string callback_family;
  std::string callback_url;
  load_task_runner->PostSyncTask([&]() {
    font_resource_manager->LoadFontAsync(
        load_task_runner, io_task_runner, nullptr, nullptr, family_name,
        {data_url},
        [&](bool success, const std::string& loaded_family,
            const std::string& loaded_url) {
          callback_success.store(success);
          callback_on_load_runner.store(
              load_task_runner->RunsTasksOnCurrentThread());
          callback_family = loaded_family;
          callback_url = loaded_url;
          callback_count.fetch_add(1);
        });
  });

  EXPECT_EQ(callback_count.load(), 0);
  EXPECT_TRUE(font_resource_manager->HasFontResourceLoading(family_name));
  EXPECT_FALSE(font_resource_manager->HasFontResource(family_name));

  fml::AutoResetWaitableEvent load_task_started;
  fml::AutoResetWaitableEvent allow_load_task;
  load_task_runner->PostTask([&]() {
    load_task_started.Signal();
    allow_load_task.Wait();
  });
  load_task_started.Wait();

  allow_io_task.Signal();
  io_task_runner->PostSyncTask([]() {});

  EXPECT_EQ(callback_count.load(), 0);
  EXPECT_TRUE(font_resource_manager->HasFontResourceLoading(family_name));
  EXPECT_FALSE(font_resource_manager->HasFontResource(family_name));

  allow_load_task.Signal();
  load_task_runner->PostSyncTask([]() {});

  EXPECT_EQ(callback_count.load(), 1);
  EXPECT_TRUE(callback_success.load());
  EXPECT_TRUE(callback_on_load_runner.load());
  EXPECT_EQ(callback_family, family_name);
  EXPECT_EQ(callback_url, data_url);
  EXPECT_FALSE(font_resource_manager->HasFontResourceLoading(family_name));
  EXPECT_TRUE(font_resource_manager->HasFontResource(family_name));
  auto font_resource = font_resource_manager->GetResource(family_name);
  ASSERT_NE(font_resource.data, nullptr);
  ASSERT_EQ(font_resource.length, 1u);
  EXPECT_EQ(font_resource.data.get()[0], 0u);
}

TEST_F(FontResourceManagerTest,
       NonDataResourceCopiedOnIOAndCompletedOnLoadRunner) {
  auto font_resource_manager = std::make_shared<FontResourceManager>();
  auto io_task_runner = CreateNewThread("font-io");
  auto load_task_runner = CreateNewThread("font-loader");
  fml::AutoResetWaitableEvent io_task_started;
  fml::AutoResetWaitableEvent allow_io_task;
  io_task_runner->PostTask([&]() {
    io_task_started.Signal();
    allow_io_task.Wait();
  });
  io_task_started.Wait();

  const std::vector<uint8_t> expected_font_data = {0x10, 0x20, 0x30, 0x40};
  auto callback_buffer =
      std::make_shared<std::vector<uint8_t>>(expected_font_data);
  std::atomic_bool creator_received_io_runner = false;
  std::atomic_bool loader_callback_on_io_runner = false;
  auto current_task_runner = GetCurrentTaskRunner();
  auto service_manager = ServiceManager::Create(
      std::make_tuple(current_task_runner, current_task_runner, io_task_runner,
                      io_task_runner));
  service_manager->RegisterService<ResourceLoaderCreatorService>(
      std::make_shared<ResourceLoaderCreatorService>(
          [io_task_runner, callback_buffer, &creator_received_io_runner,
           &loader_callback_on_io_runner](
              fml::RefPtr<fml::TaskRunner> callback_task_runner,
              std::shared_ptr<ResourceLoaderIntercept>) {
            creator_received_io_runner.store(callback_task_runner.get() ==
                                             io_task_runner.get());
            return std::make_shared<EphemeralBufferResourceLoader>(
                std::move(callback_task_runner), callback_buffer,
                &loader_callback_on_io_runner);
          }));

  const std::string family_name = "async_non_data_font";
  const std::string font_url = "https://example.com/font.ttf";
  std::atomic_int callback_count = 0;
  std::atomic_bool callback_success = false;
  std::atomic_bool callback_on_load_runner = false;
  std::string callback_family;
  std::string callback_url;
  load_task_runner->PostSyncTask([&]() {
    font_resource_manager->LoadFontAsync(
        load_task_runner, io_task_runner, nullptr, service_manager, family_name,
        {font_url},
        [&](bool success, const std::string& loaded_family,
            const std::string& loaded_url) {
          callback_success.store(success);
          callback_on_load_runner.store(
              load_task_runner->RunsTasksOnCurrentThread());
          callback_family = loaded_family;
          callback_url = loaded_url;
          callback_count.fetch_add(1);
        });
  });

  EXPECT_TRUE(creator_received_io_runner.load());
  EXPECT_EQ(callback_count.load(), 0);
  EXPECT_TRUE(font_resource_manager->HasFontResourceLoading(family_name));
  EXPECT_FALSE(font_resource_manager->HasFontResource(family_name));

  fml::AutoResetWaitableEvent load_task_started;
  fml::AutoResetWaitableEvent allow_load_task;
  load_task_runner->PostTask([&]() {
    load_task_started.Signal();
    allow_load_task.Wait();
  });
  load_task_started.Wait();

  allow_io_task.Signal();
  io_task_runner->PostSyncTask([]() {});

  EXPECT_TRUE(loader_callback_on_io_runner.load());
  EXPECT_EQ(*callback_buffer,
            std::vector<uint8_t>(expected_font_data.size(), 0xFF));
  EXPECT_EQ(callback_count.load(), 0);
  EXPECT_TRUE(font_resource_manager->HasFontResourceLoading(family_name));
  EXPECT_FALSE(font_resource_manager->HasFontResource(family_name));

  allow_load_task.Signal();
  load_task_runner->PostSyncTask([]() {});

  EXPECT_EQ(callback_count.load(), 1);
  EXPECT_TRUE(callback_success.load());
  EXPECT_TRUE(callback_on_load_runner.load());
  EXPECT_EQ(callback_family, family_name);
  EXPECT_EQ(callback_url, font_url);
  EXPECT_FALSE(font_resource_manager->HasFontResourceLoading(family_name));
  EXPECT_TRUE(font_resource_manager->HasFontResource(family_name));
  auto font_resource = font_resource_manager->GetResource(family_name);
  ASSERT_NE(font_resource.data, nullptr);
  ASSERT_EQ(font_resource.length, expected_font_data.size());
  EXPECT_TRUE(std::equal(expected_font_data.begin(), expected_font_data.end(),
                         font_resource.data.get()));
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
  auto io_task_runner = CreateNewThread("failed-font-io");

  font_collection_->PreLoadFontOnMem(load_task_runner, io_task_runner, nullptr,
                                     nullptr, family_name,
                                     {"data:font/ttf,invalid"});
  io_task_runner->PostSyncTask([]() {});
  load_task_runner->PostSyncTask([]() {});

  EXPECT_EQ(font_collection_->font_download_callback_.count(family_name), 0u);
  EXPECT_FALSE(callback_called);
  EXPECT_FALSE(font_collection_->HasFontResourceLoading(family_name));
}

}  // namespace clay
