// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_service/lynx_http_response.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxHttpResponse, Create) {
  lynx_http_response_t* response = lynx_http_response_create(nullptr, nullptr);
  EXPECT_TRUE(response != nullptr);
  EXPECT_EQ(lynx_http_response_get_status_code(response), -1);
  lynx_http_response_set_status_code(response, 200);
  EXPECT_EQ(lynx_http_response_get_status_code(response), 200);
  EXPECT_TRUE(response->status_text.empty());
  lynx_http_response_set_status_text(response, "OK");
  EXPECT_STREQ(response->status_text.c_str(), "OK");

  uint8_t body[] = {'o', 'k'};
  lynx_http_response_set_body(response, body, sizeof(body), nullptr, nullptr);
  const uint8_t* response_body = nullptr;
  EXPECT_EQ(lynx_http_response_get_body(response, &response_body),
            sizeof(body));
  ASSERT_NE(response_body, nullptr);
  EXPECT_EQ(response_body[0], 'o');
  EXPECT_EQ(response_body[1], 'k');

  lynx_http_response_release(response);
}

TEST(LynxHttpResponse, Callback) {
  int callback_count = 0;
  lynx_http_response_t* response = lynx_http_response_create(
      [](lynx_http_response_t* response, void* user_data) {
        ++*static_cast<int*>(user_data);
        EXPECT_EQ(lynx_http_response_get_status_code(response), -1);
        EXPECT_STREQ(response->status_text.c_str(), "error");
      },
      &callback_count);
  EXPECT_TRUE(response != nullptr);
  lynx_http_response_set_status_code(response, -1);
  lynx_http_response_set_status_text(response, "error");
  lynx_http_response_callback(response);
  lynx_http_response_release(response);
  EXPECT_EQ(callback_count, 1);
}
