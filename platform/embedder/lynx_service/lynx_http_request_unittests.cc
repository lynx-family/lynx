// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <map>
#include <string>
#include <vector>

#include "platform/embedder/public/capi/lynx_http_service_capi.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace {

std::map<std::string, std::string> RequestHeaders(
    lynx_http_request_t* request) {
  std::map<std::string, std::string> headers;
  size_t header_count = lynx_http_request_get_header_count(request);
  for (size_t i = 0; i < header_count; ++i) {
    const char* key = nullptr;
    const char* value = nullptr;
    lynx_http_request_get_header(request, i, &key, &value);
    EXPECT_NE(key, nullptr);
    EXPECT_NE(value, nullptr);
    if (key && value) {
      headers[key] = value;
    }
  }
  return headers;
}

std::vector<uint8_t> RequestBody(lynx_http_request_t* request) {
  const uint8_t* body = nullptr;
  size_t body_size = lynx_http_request_get_body(request, &body);
  if (body_size == 0) {
    return {};
  }
  EXPECT_NE(body, nullptr);
  return body ? std::vector<uint8_t>(body, body + body_size)
              : std::vector<uint8_t>();
}

}  // namespace

TEST(LynxHttpRequest, Create) {
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  ASSERT_NE(request, nullptr);
  EXPECT_STREQ(lynx_http_request_get_url(request), "test_url");
  EXPECT_STREQ(lynx_http_request_get_method(request), "GET");
  EXPECT_EQ(lynx_http_request_get_timeout_ms(request), 0);

  lynx_http_request_add_header(request, "key1", "value1");
  lynx_http_request_add_header(request, "key2", "value2");
  EXPECT_EQ(lynx_http_request_get_header_count(request), 2u);

  const char* header_key = nullptr;
  const char* header_value = nullptr;
  lynx_http_request_get_header(request, 0, &header_key, &header_value);
  EXPECT_STREQ(header_key, "key1");
  EXPECT_STREQ(header_value, "value1");

  lynx_http_request_get_header(request, 1, &header_key, &header_value);
  EXPECT_STREQ(header_key, "key2");
  EXPECT_STREQ(header_value, "value2");

  const uint8_t* body = nullptr;
  EXPECT_EQ(lynx_http_request_get_body(request, &body), 0u);

  lynx_http_request_release(request);
}

TEST(LynxHttpRequest, NullUrlCreatesEmptyUrl) {
  lynx_http_request_t* request = lynx_http_request_create(nullptr);
  ASSERT_NE(request, nullptr);
  EXPECT_STREQ(lynx_http_request_get_url(request), "");

  lynx_http_request_release(request);
}

TEST(LynxHttpRequest, MethodCopiesHostStringAndRestoresGet) {
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  ASSERT_NE(request, nullptr);
  {
    std::string method = "POST";
    lynx_http_request_set_method(request, method.c_str());
    method.assign("PATCH");
  }
  EXPECT_STREQ(lynx_http_request_get_method(request), "POST");

  lynx_http_request_set_method(request, "PUT");
  EXPECT_STREQ(lynx_http_request_get_method(request), "PUT");
  lynx_http_request_set_method(request, nullptr);
  EXPECT_STREQ(lynx_http_request_get_method(request), "GET");
  lynx_http_request_set_method(request, "POST");
  lynx_http_request_set_method(request, "");
  EXPECT_STREQ(lynx_http_request_get_method(request), "GET");

  lynx_http_request_release(request);
}

TEST(LynxHttpRequest, HeadersCopyHostStringsAndReplaceMatchingKeys) {
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  ASSERT_NE(request, nullptr);
  {
    std::string key = "Content-Type";
    std::string value = "application/json";
    lynx_http_request_add_header(request, key.c_str(), value.c_str());
    key.assign("Changed-Key");
    value.assign("changed");
  }
  EXPECT_EQ(RequestHeaders(request),
            (std::map<std::string, std::string>{
                {"Content-Type", "application/json"}}));

  lynx_http_request_add_header(request, "Content-Type", "text/plain");
  lynx_http_request_add_header(request, "content-type", "");
  lynx_http_request_add_header(request, nullptr, "ignored");
  lynx_http_request_add_header(request, "", "ignored");
  lynx_http_request_add_header(request, "Content-Type", nullptr);
  lynx_http_request_add_header(request, "Ignored-Key", nullptr);

  EXPECT_EQ(lynx_http_request_get_header_count(request), 2u);
  EXPECT_EQ(RequestHeaders(request),
            (std::map<std::string, std::string>{{"Content-Type", "text/plain"},
                                                {"content-type", ""}}));
  lynx_http_request_release(request);
}

TEST(LynxHttpRequest, TimeoutSupportsUpdateResetAndIgnoresNegativeValues) {
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  ASSERT_NE(request, nullptr);
  EXPECT_EQ(lynx_http_request_get_timeout_ms(request), 0);

  lynx_http_request_set_timeout_ms(request, -1);
  EXPECT_EQ(lynx_http_request_get_timeout_ms(request), 0);
  lynx_http_request_set_timeout_ms(request, 10000);
  EXPECT_EQ(lynx_http_request_get_timeout_ms(request), 10000);
  lynx_http_request_set_timeout_ms(request, -1);
  EXPECT_EQ(lynx_http_request_get_timeout_ms(request), 10000);
  lynx_http_request_set_timeout_ms(request, 20000);
  EXPECT_EQ(lynx_http_request_get_timeout_ms(request), 20000);
  lynx_http_request_set_timeout_ms(request, 0);
  EXPECT_EQ(lynx_http_request_get_timeout_ms(request), 0);

  lynx_http_request_release(request);
}

TEST(LynxHttpRequest, BodyCopiesBinaryDataAndSupportsReplacementAndClearing) {
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  ASSERT_NE(request, nullptr);
  {
    std::vector<uint8_t> body = {'a', 0, 'b', 255};
    lynx_http_request_set_body(request, body.data(), body.size());
    body.assign(4, 'x');
  }
  EXPECT_EQ(RequestBody(request), (std::vector<uint8_t>{'a', 0, 'b', 255}));

  const uint8_t* stored_body = nullptr;
  ASSERT_EQ(lynx_http_request_get_body(request, &stored_body), 4u);
  ASSERT_NE(stored_body, nullptr);
  lynx_http_request_set_body(request, stored_body + 1, 2);
  EXPECT_EQ(RequestBody(request), (std::vector<uint8_t>{0, 'b'}));

  const uint8_t replacement[] = {'z', 0};
  lynx_http_request_set_body(request, replacement, sizeof(replacement));
  EXPECT_EQ(RequestBody(request), (std::vector<uint8_t>{'z', 0}));
  lynx_http_request_set_body(request, nullptr, 4);
  EXPECT_EQ(RequestBody(request), (std::vector<uint8_t>{'z', 0}));

  lynx_http_request_set_body(request, nullptr, 0);
  EXPECT_EQ(lynx_http_request_get_body(request, nullptr), 0u);
  lynx_http_request_set_body(request, replacement, sizeof(replacement));
  lynx_http_request_set_body(request, replacement, 0);
  EXPECT_EQ(lynx_http_request_get_body(request, nullptr), 0u);

  lynx_http_request_release(request);
}

TEST(LynxHttpRequest, NullRequestSettersAreNoOps) {
  const uint8_t body[] = {'a', 0, 'b'};
  lynx_http_request_set_method(nullptr, "POST");
  lynx_http_request_add_header(nullptr, "Content-Type", "application/json");
  lynx_http_request_set_body(nullptr, body, sizeof(body));
  lynx_http_request_set_timeout_ms(nullptr, 10000);
  lynx_http_request_set_timeout_ms(nullptr, -1);
  EXPECT_EQ(lynx_http_request_get_timeout_ms(nullptr), 0);
}
