// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_HTTP_SERVICE_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_HTTP_SERVICE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "capi/lynx_http_service_capi.h"
#include "lynx_service_center.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief `LynxHttpRequest` is a class that represents an HTTP request,
 * allowing developers to set request headers, request body, and other
 * request parameters.
 */
class LynxHttpRequest {
 public:
  explicit LynxHttpRequest(lynx_http_request_t* request) : request_(request) {
    ExtractRequestInfo();
  }
  ~LynxHttpRequest() { lynx_http_request_release(request_); }

  /**
   * @apidoc
   * @brief Get the URL of the HTTP request.
   * @return The URL of the HTTP request.
   */
  const std::string& GetUrl() const { return url_; }

  /**
   * @apidoc
   * @brief Get the method of the HTTP request.
   * @return The method of the HTTP request.
   */
  const std::string& GetMethod() const { return method_; }

  /**
   * @apidoc
   * @brief Get the headers of the HTTP request.
   * @return The headers of the HTTP request.
   */
  const std::unordered_map<std::string, std::string>& GetHeaders() const {
    return headers_;
  }

  /**
   * @apidoc
   * @brief Get the body of the HTTP request.
   * @return The body of the HTTP request.
   */
  const std::vector<uint8_t>& GetBody() const { return body_; }

 private:
  void ExtractRequestInfo() {
    const char* url = lynx_http_request_get_url(request_);
    url_ = url ? url : "";
    const char* method = lynx_http_request_get_method(request_);
    method_ = method ? method : "";
    size_t header_count = lynx_http_request_get_header_count(request_);
    for (unsigned i = 0; i < header_count; ++i) {
      const char* key = nullptr;
      const char* value = nullptr;
      lynx_http_request_get_header(request_, i, &key, &value);
      if (key && value) {
        headers_[key] = value;
      }
    }
    const uint8_t* body = nullptr;
    size_t body_size = lynx_http_request_get_body(request_, &body);
    if (body_size > 0 && body) {
      body_.assign(body, body + body_size);
    }
  }

  lynx_http_request_t* request_;
  std::string url_;
  std::string method_;
  std::unordered_map<std::string, std::string> headers_;
  std::vector<uint8_t> body_;
};

/**
 * @apidoc
 * @brief `LynxHttpResponse` is a class that represents an HTTP response,
 * allowing developers to set response status code, response status text,
 * response headers, and response body.
 */
class LynxHttpResponse {
 public:
  explicit LynxHttpResponse(lynx_http_response_t* response)
      : response_(response) {}
  ~LynxHttpResponse() { lynx_http_response_release(response_); }

  /**
   * @apidoc
   * @brief Set the status code of the HTTP response.
   * @param code The status code of the HTTP response.
   */
  void SetStatusCode(int code) {
    lynx_http_response_set_status_code(response_, code);
  }

  /**
   * @apidoc
   * @brief Set the status text of the HTTP response.
   * @param msg The status text of the HTTP response.
   */
  void SetStatusText(const char* msg) {
    lynx_http_response_set_status_text(response_, msg);
  }

  /**
   * @apidoc
   * @brief Add a header to the HTTP response.
   * @param key The key of the header.
   * @param value The value of the header.
   */
  void AddHeader(const std::string& key, const std::string& value) {
    lynx_http_response_add_header(response_, key.c_str(), value.c_str());
  }

  /**
   * @apidoc
   * @brief Set the body of the HTTP response.
   * @param content The body content of the HTTP response.
   * @param length The length of the body content.
   * @param dtor The destructor of the body content.
   * @param opaque The opaque data of the body content.
   */
  void SetBody(uint8_t* content, size_t length,
               void (*dtor)(uint8_t*, size_t, void*) = nullptr,
               void* opaque = nullptr) {
    lynx_http_response_set_body(response_, content, length, dtor, opaque);
  }

  void Complete() { lynx_http_response_callback(response_); }

 private:
  lynx_http_response_t* response_;
};

/**
 * @apidoc
 * @brief `LynxHttpService` is a class that represents a HTTP service, allowing
 * developers to send HTTP requests and receive HTTP responses.
 */
class LynxHttpService : public LynxServiceBase,
                        public std::enable_shared_from_this<LynxHttpService> {
 public:
  LynxHttpService() = default;
  ~LynxHttpService() = default;

  void InitIfNeeded() override {
    if (http_service_) {
      return;
    }
    http_service_ = lynx_http_service_create_with_finalizer(
        new std::weak_ptr<LynxHttpService>(weak_from_this()),
        [](lynx_http_service_t* http_service, void* user_data) {
          std::weak_ptr<LynxHttpService>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxHttpService>*>(user_data);
          delete weak_ptr;
        });
    lynx_http_service_bind(http_service_, [](lynx_http_service_t* http_service,
                                             lynx_http_request_t* request,
                                             lynx_http_response_t* response) {
      auto req = std::make_shared<LynxHttpRequest>(request);
      auto resp = std::make_shared<LynxHttpResponse>(response);
      std::weak_ptr<LynxHttpService>* weak_ptr =
          reinterpret_cast<std::weak_ptr<LynxHttpService>*>(
              lynx_http_service_get_user_data(http_service));
      std::shared_ptr<LynxHttpService> shared_http_service =
          weak_ptr ? weak_ptr->lock() : nullptr;
      if (!shared_http_service) {
        resp->SetStatusCode(-1);
        resp->SetStatusText("http service is destroyed");
      } else {
        shared_http_service->Request(req, resp);
      }
    });
  }

  lynx_service_type_e GetServiceType() override { return kServiceTypeHttp; }

  void* Impl() override { return http_service_; }

  /**
   * @apidoc
   * @brief Send an HTTP request.
   * @param request The HTTP request to be sent.
   * @param response The HTTP response to receive the response.
   */
  virtual void Request(std::shared_ptr<LynxHttpRequest> request,
                       std::shared_ptr<LynxHttpResponse> response) = 0;

 private:
  lynx_http_service_t* http_service_ = nullptr;
};
}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_HTTP_SERVICE_H_
