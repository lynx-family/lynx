// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_service/lynx_http_response.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxHttpResponse, Create) {
  lynx_http_response_t* response = lynx_http_response_create(nullptr);
  EXPECT_TRUE(response != nullptr);
  EXPECT_EQ(response->status_code, -1);
  lynx_http_response_set_status_code(response, 200);
  EXPECT_EQ(response->status_code, 200);
  EXPECT_TRUE(response->status_text.empty());
  lynx_http_response_set_status_text(response, "OK");
  EXPECT_STREQ(response->status_text.c_str(), "OK");

  lynx_http_response_release(response);
}

TEST(LynxHttpResponse, Callback) {
  lynx_http_response_t* response =
      lynx_http_response_create([](lynx_http_response_t* response) {
        EXPECT_EQ(response->status_code, -1);
        EXPECT_STREQ(response->status_text.c_str(), "error");
      });
  EXPECT_TRUE(response != nullptr);
  lynx_http_response_set_status_code(response, -1);
  lynx_http_response_set_status_text(response, "error");
  lynx_http_response_callback(response);
  lynx_http_response_release(response);
}
