// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_service/lynx_http_service_priv.h"
#include "platform/embedder/lynx_service/lynx_security_service_priv.h"
#include "platform/embedder/lynx_service/lynx_service_center_priv.h"
#include "platform/embedder/public/lynx_http_service.h"
#include "platform/embedder/public/lynx_service_center.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxService, Register) {
  lynx_service_center_t* service_center = lynx_service_get_center_instance();
  lynx_http_service_t* http_service = lynx_http_service_create(nullptr);
  EXPECT_TRUE(http_service != nullptr);
  lynx_service_register_service(service_center, kServiceTypeHttp, http_service);
  EXPECT_EQ(lynx_service_get_service(service_center, kServiceTypeHttp),
            http_service);
  lynx_http_service_release(http_service);

  lynx_security_service_t* security_service =
      lynx_security_service_create(nullptr);
  EXPECT_TRUE(security_service != nullptr);
  lynx_service_register_service(service_center, kServiceTypeSecurity,
                                security_service);
  EXPECT_EQ(lynx_service_get_service(service_center, kServiceTypeSecurity),
            security_service);
  lynx_security_service_release(security_service);
}

TEST(LynxService, HttpService) {
  lynx_service_center_t* service_center = lynx_service_get_center_instance();
  // Initialize http service
  lynx_http_service_t* service = lynx_http_service_create(nullptr);
  lynx_http_service_bind(
      service, [](lynx_http_service_t*, lynx_http_request_t* request,
                  lynx_http_response_t* response) {
        EXPECT_TRUE(request != nullptr);
        EXPECT_STREQ(lynx_http_request_get_url(request), "test_url");
        EXPECT_STREQ(lynx_http_request_get_method(request), "GET");
        EXPECT_TRUE(response != nullptr);
        lynx_http_response_set_status_code(response, 200);
        lynx_http_response_release(response);
        lynx_http_request_release(request);
      });
  lynx_service_register_service(service_center, kServiceTypeHttp, service);
  lynx_http_service_release(service);

  // Send http request
  lynx_http_service_t* http_service = reinterpret_cast<lynx_http_service_t*>(
      lynx_service_get_service(service_center, kServiceTypeHttp));
  lynx_http_request_t* request = lynx_http_request_create("test_url");
  lynx_http_response_t* response =
      lynx_http_response_create([](lynx_http_response_t* response) {
        EXPECT_TRUE(response != nullptr);
        EXPECT_EQ(response->status_code, 200);
      });
  lynx_http_service_request(http_service, request, response);
}

class TestLynxHttpService : public lynx::pub::LynxHttpService {
 public:
  void Request(std::shared_ptr<lynx::pub::LynxHttpRequest> request,
               std::shared_ptr<lynx::pub::LynxHttpResponse> response) override {
    EXPECT_STREQ(request->GetUrl().c_str(), "test_url");
    response->SetStatusCode(200);
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
  lynx_http_response_t* response =
      lynx_http_response_create([](lynx_http_response_t* response) {
        EXPECT_TRUE(response != nullptr);
        EXPECT_EQ(response->status_code, 200);
      });
  lynx_http_service_request(http_service, request, response);
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
}
