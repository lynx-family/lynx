// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/fetcher/lynx_resource_response_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxResourceResponse, Create) {
  lynx_resource_response_t* response = lynx_resource_response_create(nullptr);
  EXPECT_TRUE(response != nullptr);
  EXPECT_EQ(response->code, -1);
  lynx_resource_response_set_code(response, 0);
  EXPECT_EQ(response->code, 0);
  EXPECT_TRUE(response->error_message.empty());
  lynx_resource_response_set_error_message(response, "error");
  EXPECT_STREQ(response->error_message.c_str(), "error");

  lynx_resource_response_release(response);
}

TEST(LynxResourceResponse, Callback) {
  lynx_resource_response_t* response =
      lynx_resource_response_create([](lynx_resource_response_t* response) {
        EXPECT_EQ(response->code, -1);
        EXPECT_STREQ(response->error_message.c_str(), "error");
      });
  EXPECT_TRUE(response != nullptr);
  lynx_resource_response_set_code(response, -1);
  lynx_resource_response_set_error_message(response, "error");
  lynx_resource_response_callback(response);
  lynx_resource_response_release(response);
}
