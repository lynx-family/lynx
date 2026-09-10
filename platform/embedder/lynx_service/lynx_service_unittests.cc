// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include "platform/embedder/lynx_service/lynx_security_service_priv.h"
#include "platform/embedder/public/capi/lynx_http_service_capi.h"
#include "platform/embedder/public/capi/lynx_service_center_capi.h"
#include "platform/embedder/public/lynx_http_service.h"
#include "platform/embedder/public/lynx_service_center.h"
#include "platform/embedder/public/lynx_trail_service.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace {

struct HttpRequestResult {
  int handler_count = 0;
  int callback_count = 0;
  int body_destructor_count = 0;
  int status_code = 0;
  void* service_user_data = nullptr;
  void* response_user_data = nullptr;
  std::string request_url;
  std::string request_method;
  std::vector<uint8_t> response_body;
  lynx_http_request_t* pending_request = nullptr;
  lynx_http_response_t* pending_response = nullptr;
};

void RecordHttpResponse(lynx_http_response_t* response, void* user_data) {
  auto* result = static_cast<HttpRequestResult*>(user_data);
  ++result->callback_count;
  result->response_user_data = user_data;
  result->status_code = lynx_http_response_get_status_code(response);

  const uint8_t* body = nullptr;
  size_t body_size = lynx_http_response_get_body(response, &body);
  if (body && body_size > 0) {
    result->response_body.assign(body, body + body_size);
  }
}

void HandleHttpRequest(lynx_http_service_t* service,
                       lynx_http_request_t* request,
                       lynx_http_response_t* response) {
  void* user_data = lynx_http_service_get_user_data(service);
  auto* result = static_cast<HttpRequestResult*>(user_data);
  ++result->handler_count;
  result->service_user_data = user_data;
  result->request_url = lynx_http_request_get_url(request);
  result->request_method = lynx_http_request_get_method(request);

  lynx_http_response_set_status_code(response, 200);
  uint8_t body[] = {'o', 'k'};
  lynx_http_response_set_body(response, body, sizeof(body), nullptr, nullptr);
  lynx_http_response_callback(response);
  lynx_http_response_release(response);
  lynx_http_request_release(request);
}

void DeferHttpRequest(lynx_http_service_t* service,
                      lynx_http_request_t* request,
                      lynx_http_response_t* response) {
  auto* result =
      static_cast<HttpRequestResult*>(lynx_http_service_get_user_data(service));
  ++result->handler_count;
  result->pending_request = request;
  result->pending_response = response;
}

void DeleteHttpResponseBody(uint8_t* content, size_t, void* opaque) {
  auto* result = static_cast<HttpRequestResult*>(opaque);
  ++result->body_destructor_count;
  delete[] content;
}

}  // namespace

TEST(LynxService, Register) {
  lynx_service_center_t* service_center = lynx_service_get_center_instance();
  lynx_http_service_t* http_service = lynx_http_service_create(nullptr);
  EXPECT_TRUE(http_service != nullptr);
  lynx_service_register_service(service_center, kServiceTypeHttp, http_service);
  EXPECT_EQ(lynx_service_get_service(service_center, kServiceTypeHttp),
            http_service);
  lynx_http_service_release(http_service);
  lynx_service_unregister_service(service_center, kServiceTypeHttp,
                                  http_service);

  lynx_security_service_t* security_service =
      lynx_security_service_create(nullptr);
  EXPECT_TRUE(security_service != nullptr);
  lynx_service_register_service(service_center, kServiceTypeSecurity,
                                security_service);
  EXPECT_EQ(lynx_service_get_service(service_center, kServiceTypeSecurity),
            security_service);
  lynx_security_service_release(security_service);
  lynx_service_unregister_service(service_center, kServiceTypeSecurity,
                                  security_service);
}

TEST(LynxService, HttpService) {
  lynx_service_center_t* service_center = lynx_service_get_center_instance();
  HttpRequestResult result;
  lynx_http_service_t* service = lynx_http_service_create(&result);
  ASSERT_NE(service, nullptr);
  lynx_http_service_bind(service, HandleHttpRequest);
  lynx_service_register_service(service_center, kServiceTypeHttp, service);
  lynx_http_service_release(service);

  lynx_http_service_t* http_service = reinterpret_cast<lynx_http_service_t*>(
      lynx_service_get_service(service_center, kServiceTypeHttp));
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  lynx_http_response_t* response =
      lynx_http_response_create(RecordHttpResponse, &result);
  lynx_http_service_request(http_service, request, response);

  EXPECT_EQ(result.handler_count, 1);
  EXPECT_EQ(result.callback_count, 1);
  EXPECT_EQ(result.status_code, 200);
  EXPECT_EQ(result.service_user_data, &result);
  EXPECT_EQ(result.response_user_data, &result);
  EXPECT_EQ(result.request_url, "test_url");
  EXPECT_EQ(result.request_method, "GET");
  EXPECT_EQ(result.response_body, (std::vector<uint8_t>{'o', 'k'}));

  lynx_service_unregister_service(service_center, kServiceTypeHttp,
                                  http_service);
}

class TestLynxHttpService : public lynx::pub::LynxHttpService {
 public:
  void Request(std::shared_ptr<lynx::pub::LynxHttpRequest> request,
               std::shared_ptr<lynx::pub::LynxHttpResponse> response) override {
    EXPECT_STREQ(request->GetUrl().c_str(), "test_url");
    response->SetStatusCode(200);
    uint8_t body[] = {'o', 'k'};
    response->SetBody(body, sizeof(body));
    response->Complete();
  }
};

TEST(LynxService, HttpServiceCpp) {
  auto& service_center = lynx::pub::LynxServiceCenter::GetInstance();
  service_center.RegisterService(std::make_shared<TestLynxHttpService>());

  // Send http request
  lynx_service_center_t* c_service_center = lynx_service_get_center_instance();
  lynx_http_service_t* http_service = reinterpret_cast<lynx_http_service_t*>(
      lynx_service_get_service(c_service_center, kServiceTypeHttp));
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  HttpRequestResult result;
  lynx_http_response_t* response =
      lynx_http_response_create(RecordHttpResponse, &result);
  lynx_http_service_request(http_service, request, response);

  EXPECT_EQ(result.callback_count, 1);
  EXPECT_EQ(result.status_code, 200);
  EXPECT_EQ(result.response_user_data, &result);
  EXPECT_EQ(result.response_body, (std::vector<uint8_t>{'o', 'k'}));

  lynx_service_unregister_service(c_service_center, kServiceTypeHttp,
                                  http_service);
}

TEST(LynxService, NullHttpServiceCompletesWithErrorExactlyOnce) {
  HttpRequestResult result;
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  lynx_http_response_t* response =
      lynx_http_response_create(RecordHttpResponse, &result);
  auto* body = new uint8_t[1]{'x'};
  lynx_http_response_set_body(response, body, 1, DeleteHttpResponseBody,
                              &result);

  lynx_http_service_request(nullptr, request, response);

  EXPECT_EQ(result.handler_count, 0);
  EXPECT_EQ(result.callback_count, 1);
  EXPECT_EQ(result.status_code, -1);
  EXPECT_EQ(result.response_user_data, &result);
  EXPECT_EQ(result.body_destructor_count, 1);
}

TEST(LynxService, UnboundHttpServiceCompletesWithErrorExactlyOnce) {
  HttpRequestResult result;
  lynx_http_service_t* service = lynx_http_service_create(&result);
  ASSERT_NE(service, nullptr);
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  lynx_http_response_t* response =
      lynx_http_response_create(RecordHttpResponse, &result);

  lynx_http_service_request(service, request, response);

  EXPECT_EQ(result.handler_count, 0);
  EXPECT_EQ(result.callback_count, 1);
  EXPECT_EQ(result.status_code, -1);
  EXPECT_EQ(result.response_user_data, &result);
  lynx_http_service_release(service);
}

TEST(LynxService, NullHttpRequestCompletesWithErrorExactlyOnce) {
  HttpRequestResult result;
  lynx_http_service_t* service = lynx_http_service_create(&result);
  ASSERT_NE(service, nullptr);
  lynx_http_service_bind(service, HandleHttpRequest);
  lynx_http_response_t* response =
      lynx_http_response_create(RecordHttpResponse, &result);

  lynx_http_service_request(service, nullptr, response);

  EXPECT_EQ(result.handler_count, 0);
  EXPECT_EQ(result.callback_count, 1);
  EXPECT_EQ(result.status_code, -1);
  lynx_http_service_release(service);
}

TEST(LynxService, NullHttpResponseDoesNotInvokeHandler) {
  HttpRequestResult result;
  lynx_http_service_t* service = lynx_http_service_create(&result);
  ASSERT_NE(service, nullptr);
  lynx_http_service_bind(service, HandleHttpRequest);
  lynx_http_request_t* request = lynx_http_request_create("test_url");

  lynx_http_service_request(service, request, nullptr);

  EXPECT_EQ(result.handler_count, 0);
  EXPECT_EQ(result.callback_count, 0);
  lynx_http_service_release(service);
}

TEST(LynxService, HttpServiceCanCompleteAfterDispatchReturns) {
  HttpRequestResult result;
  lynx_http_service_t* service = lynx_http_service_create(&result);
  ASSERT_NE(service, nullptr);
  lynx_http_service_bind(service, DeferHttpRequest);
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  lynx_http_response_t* response =
      lynx_http_response_create(RecordHttpResponse, &result);

  lynx_http_service_request(service, request, response);

  EXPECT_EQ(result.handler_count, 1);
  EXPECT_EQ(result.callback_count, 0);
  ASSERT_NE(result.pending_request, nullptr);
  ASSERT_NE(result.pending_response, nullptr);

  lynx_http_response_set_status_code(result.pending_response, 204);
  lynx_http_response_callback(result.pending_response);
  lynx_http_response_release(result.pending_response);
  lynx_http_request_release(result.pending_request);
  result.pending_response = nullptr;
  result.pending_request = nullptr;

  EXPECT_EQ(result.callback_count, 1);
  EXPECT_EQ(result.status_code, 204);
  lynx_http_service_release(service);
}

TEST(LynxService, TrailService) {
  lynx_trail_service_t* service = lynx_trail_service_create(nullptr);
  ASSERT_NE(service, nullptr);
  lynx_trail_service_bind(service, [](lynx_trail_service_t*, const char* key) {
    return key && std::string(key) == "known" ? "value" : nullptr;
  });

  struct QueryResult {
    int callback_count = 0;
    std::optional<std::string> value;
  };
  auto callback = [](const char* value, void* user_data) {
    auto* result = static_cast<QueryResult*>(user_data);
    ++result->callback_count;
    if (value) {
      result->value = value;
    }
  };

  QueryResult known_result;
  lynx_trail_service_get_string_value(service, "known", callback,
                                      &known_result);
  EXPECT_EQ(known_result.callback_count, 1);
  ASSERT_TRUE(known_result.value.has_value());
  EXPECT_EQ(*known_result.value, "value");

  QueryResult missing_result;
  lynx_trail_service_get_string_value(service, "missing", callback,
                                      &missing_result);
  EXPECT_EQ(missing_result.callback_count, 1);
  EXPECT_FALSE(missing_result.value.has_value());

  QueryResult null_key_result;
  lynx_trail_service_get_string_value(service, nullptr, callback,
                                      &null_key_result);
  EXPECT_EQ(null_key_result.callback_count, 1);
  EXPECT_FALSE(null_key_result.value.has_value());

  QueryResult null_service_result;
  lynx_trail_service_get_string_value(nullptr, "known", callback,
                                      &null_service_result);
  EXPECT_EQ(null_service_result.callback_count, 1);
  EXPECT_FALSE(null_service_result.value.has_value());

  lynx_trail_service_get_string_value(service, "known", nullptr, nullptr);
  lynx_trail_service_release(service);
}

TEST(LynxService, TrailServiceCppHelper) {
  lynx_trail_service_t* service = lynx_trail_service_create(nullptr);
  ASSERT_NE(service, nullptr);
  lynx_trail_service_bind(service, [](lynx_trail_service_t*, const char* key) {
    if (!key) {
      return static_cast<const char*>(nullptr);
    }
    if (std::string(key) == "known") {
      return "value";
    }
    if (std::string(key) == "empty") {
      return "";
    }
    return static_cast<const char*>(nullptr);
  });

  auto known_value = lynx::pub::GetTrailStringValue(service, "known");
  ASSERT_TRUE(known_value.has_value());
  EXPECT_EQ(*known_value, "value");

  auto empty_value = lynx::pub::GetTrailStringValue(service, "empty");
  ASSERT_TRUE(empty_value.has_value());
  EXPECT_TRUE(empty_value->empty());

  EXPECT_FALSE(lynx::pub::GetTrailStringValue(service, "missing").has_value());
  EXPECT_FALSE(lynx::pub::GetTrailStringValue(nullptr, "known").has_value());

  lynx_trail_service_release(service);
}

TEST(LynxService, SecurityService) {
  lynx_service_center_t* service_center = lynx_service_get_center_instance();
  // Initialize security service
  lynx_security_service_t* service = lynx_security_service_create(nullptr);
  lynx_security_service_bind(
      service, [](lynx_security_service_t* security_service, uint8_t* content,
                  size_t length, const char* url, lynx_tasm_type_e type,
                  const char** error_msg) {
        if (strcmp(url, "test_url") == 0) {
          return 0;
        }
        *error_msg = "test_error";
        return -1;
      });
  lynx_service_register_service(service_center, kServiceTypeSecurity, service);
  lynx_security_service_release(service);

  // Verify TASM
  lynx_security_service_t* security_service =
      reinterpret_cast<lynx_security_service_t*>(
          lynx_service_get_service(service_center, kServiceTypeSecurity));
  const char* error_msg = nullptr;
  int ret = lynx_security_service_verify_tasm(
      security_service, (uint8_t*)"test_tasm", 10, "test_url", kTypeTemplate,
      &error_msg);
  EXPECT_EQ(ret, 0);

  ret = lynx_security_service_verify_tasm(
      security_service, (uint8_t*)"test_tasm", 10, "test_url2", kTypeTemplate,
      &error_msg);
  EXPECT_EQ(ret, -1);
  EXPECT_STREQ(error_msg, "test_error");

  lynx_service_unregister_service(service_center, kServiceTypeSecurity,
                                  security_service);
}
