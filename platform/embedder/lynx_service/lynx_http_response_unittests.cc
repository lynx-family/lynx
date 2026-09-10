// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <vector>

#include "platform/embedder/public/capi/lynx_http_service_capi.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace {

struct ResponseCallbackState {
  int callback_count = 0;
  int status_code = 0;
  int body_destructor_count = 0;
  void* received_user_data = nullptr;
  std::vector<uint8_t> body;
};

void RecordResponse(lynx_http_response_t* response, void* user_data) {
  auto* state = static_cast<ResponseCallbackState*>(user_data);
  ++state->callback_count;
  state->received_user_data = user_data;
  state->status_code = lynx_http_response_get_status_code(response);

  const uint8_t* body = nullptr;
  size_t body_size = lynx_http_response_get_body(response, &body);
  if (body && body_size > 0) {
    state->body.assign(body, body + body_size);
  }
}

void DeleteResponseBody(uint8_t* content, size_t, void* opaque) {
  auto* state = static_cast<ResponseCallbackState*>(opaque);
  ++state->body_destructor_count;
  delete[] content;
}

}  // namespace

TEST(LynxHttpResponse, CreateAndReadData) {
  lynx_http_response_t* response = lynx_http_response_create(nullptr, nullptr);
  ASSERT_NE(response, nullptr);
  EXPECT_EQ(lynx_http_response_get_status_code(response), -1);

  lynx_http_response_set_status_code(response, 200);
  EXPECT_EQ(lynx_http_response_get_status_code(response), 200);

  uint8_t body[] = {'o', 0, 'k'};
  lynx_http_response_set_body(response, body, sizeof(body), nullptr, nullptr);
  const uint8_t* response_body = nullptr;
  EXPECT_EQ(lynx_http_response_get_body(response, &response_body),
            sizeof(body));
  ASSERT_NE(response_body, nullptr);
  EXPECT_EQ(response_body[0], 'o');
  EXPECT_EQ(response_body[1], 0);
  EXPECT_EQ(response_body[2], 'k');
  EXPECT_EQ(lynx_http_response_get_body(response, nullptr), sizeof(body));

  lynx_http_response_release(response);
}

TEST(LynxHttpResponse, CallbackReceivesUserDataAndRunsExactlyOnce) {
  ResponseCallbackState state;
  lynx_http_response_t* response =
      lynx_http_response_create(RecordResponse, &state);
  ASSERT_NE(response, nullptr);

  lynx_http_response_set_status_code(response, 201);
  auto* body = new uint8_t[2]{'o', 'k'};
  lynx_http_response_set_body(response, body, 2, DeleteResponseBody, &state);

  lynx_http_response_callback(response);
  lynx_http_response_callback(response);
  lynx_http_response_release(response);

  EXPECT_EQ(state.callback_count, 1);
  EXPECT_EQ(state.received_user_data, &state);
  EXPECT_EQ(state.status_code, 201);
  EXPECT_EQ(state.body, (std::vector<uint8_t>{'o', 'k'}));
  EXPECT_EQ(state.body_destructor_count, 1);
}

TEST(LynxHttpResponse, NullGettersReturnDefaults) {
  EXPECT_EQ(lynx_http_response_get_status_code(nullptr), -1);

  const uint8_t sentinel = 0;
  const uint8_t* body = &sentinel;
  EXPECT_EQ(lynx_http_response_get_body(nullptr, &body), 0u);
  EXPECT_EQ(body, nullptr);
  EXPECT_EQ(lynx_http_response_get_body(nullptr, nullptr), 0u);
}
