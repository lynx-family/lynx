// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_service/lynx_http_request.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxHttpRequest, Create) {
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  EXPECT_TRUE(request != nullptr);
  EXPECT_STREQ(lynx_http_request_get_url(request), "test_url");

  request->headers["key1"] = "value1";
  request->headers["key2"] = "value2";
  EXPECT_EQ(lynx_http_request_get_header_count(request), 2);

  const char* header_key = nullptr;
  const char* header_value = nullptr;
  lynx_http_request_get_header(request, 0, &header_key, &header_value);
  EXPECT_STREQ(header_key, "key1");
  EXPECT_STREQ(header_value, "value1");

  lynx_http_request_get_header(request, 1, &header_key, &header_value);
  EXPECT_STREQ(header_key, "key2");
  EXPECT_STREQ(header_value, "value2");

  EXPECT_STREQ(lynx_http_request_get_method(request), "GET");

  lynx_http_request_release(request);
}
