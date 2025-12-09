// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_client_priv.h"
#include "platform/embedder/public/lynx_view_client.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

class TestLynxViewClient : public lynx::pub::LynxViewClient {
 public:
  TestLynxViewClient() {}
  ~TestLynxViewClient() {}
  void OnPageStart(const char* url) override {
    page_start_count_++;
    url_ = url;
  }
  void OnLoadSuccess() override { load_success_count_++; }
  void OnFirstScreen() override { first_screen_count_++; }
  void OnPageUpdated() override { page_updated_count_++; }
  void OnDataUpdated() override { data_updated_count_++; }
  void OnDestroy() override { destroy_count_++; }
  void OnRuntimeReady() override { runtime_ready_count_++; }
  void OnReceivedError(int error_code, const char* message) override {
    received_error_count_++;
    error_code_ = error_code;
    error_message_ = message;
  }
  void OnTimingSetup(const char* timing_info) override {
    timing_setup_count_++;
    timing_info_ = timing_info;
  }
  void OnTimingUpdate(const char* timing_info, const char* update_timing,
                      const char* update_flag) override {
    timing_update_count_++;
    timing_info_ = timing_info;
    update_timing_ = update_timing;
    update_flag_ = update_flag;
  }
  void OnEnterForeground() override { enter_foreground_count_++; }
  void OnEnterBackground() override { enter_background_count_++; }

  int page_start_count_ = 0;
  const char* url_ = nullptr;
  int load_success_count_ = 0;
  int first_screen_count_ = 0;
  int page_updated_count_ = 0;
  int data_updated_count_ = 0;
  int destroy_count_ = 0;
  int runtime_ready_count_ = 0;
  int received_error_count_ = 0;
  int error_code_ = 0;
  const char* error_message_ = nullptr;
  int timing_setup_count_ = 0;
  const char* timing_info_ = nullptr;
  int timing_update_count_ = 0;
  const char* update_timing_ = nullptr;
  const char* update_flag_ = nullptr;
  int enter_foreground_count_ = 0;
  int enter_background_count_ = 0;
};

TEST(LynxViewClientTest, TestLynxViewClient) {
  auto client = std::make_shared<TestLynxViewClient>();
  lynx_view_client_t* c_client = client->Impl();

  EXPECT_EQ(c_client->user_data, client.get());

  EXPECT_EQ(client->page_start_count_, 0);
  c_client->on_page_start(c_client, "test");
  EXPECT_EQ(client->page_start_count_, 1);
  EXPECT_STREQ(client->url_, "test");

  EXPECT_EQ(client->load_success_count_, 0);
  c_client->on_load_success(c_client);
  EXPECT_EQ(client->load_success_count_, 1);

  EXPECT_EQ(client->first_screen_count_, 0);
  c_client->on_first_screen(c_client);
  EXPECT_EQ(client->first_screen_count_, 1);

  EXPECT_EQ(client->page_updated_count_, 0);
  c_client->on_page_updated(c_client);
  EXPECT_EQ(client->page_updated_count_, 1);

  EXPECT_EQ(client->data_updated_count_, 0);
  c_client->on_data_updated(c_client);
  EXPECT_EQ(client->data_updated_count_, 1);

  EXPECT_EQ(client->destroy_count_, 0);
  c_client->on_destroy(c_client);
  EXPECT_EQ(client->destroy_count_, 1);

  EXPECT_EQ(client->runtime_ready_count_, 0);
  c_client->on_runtime_ready(c_client);
  EXPECT_EQ(client->runtime_ready_count_, 1);

  EXPECT_EQ(client->received_error_count_, 0);
  c_client->on_received_error(c_client, 1, "test");
  EXPECT_EQ(client->received_error_count_, 1);
  EXPECT_EQ(client->error_code_, 1);
  EXPECT_STREQ(client->error_message_, "test");

  EXPECT_EQ(client->timing_setup_count_, 0);
  c_client->on_timing_setup(c_client, "test");
  EXPECT_EQ(client->timing_setup_count_, 1);
  EXPECT_STREQ(client->timing_info_, "test");

  EXPECT_EQ(client->timing_update_count_, 0);
  c_client->on_timing_update(c_client, "test", "test", "test");
  EXPECT_EQ(client->timing_update_count_, 1);
  EXPECT_STREQ(client->timing_info_, "test");
  EXPECT_STREQ(client->update_timing_, "test");
  EXPECT_STREQ(client->update_flag_, "test");

  EXPECT_EQ(client->enter_foreground_count_, 0);
  c_client->on_enter_foreground(c_client);
  EXPECT_EQ(client->enter_foreground_count_, 1);

  EXPECT_EQ(client->enter_background_count_, 0);
  c_client->on_enter_background(c_client);
  EXPECT_EQ(client->enter_background_count_, 1);
}
