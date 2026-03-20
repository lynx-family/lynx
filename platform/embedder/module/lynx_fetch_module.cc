// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/module/lynx_fetch_module.h"

#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "platform/embedder/lynx_service/lynx_http_service_priv.h"
#include "platform/embedder/lynx_service/lynx_service_center_priv.h"
#include "third_party/weak-node-api/headers/napi.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

class FetchCallbackContext {
 public:
  FetchCallbackContext(Napi::Function resolve, Napi::Function reject)
      : resolve(Napi::Reference<Napi::Function>::New(resolve, 1)),
        reject(Napi::Reference<Napi::Function>::New(reject, 1)) {}
  Napi::Reference<Napi::Function> resolve;
  Napi::Reference<Napi::Function> reject;
};

void InvokeJSCallback(Napi::Env env, Napi::Function func,
                      FetchCallbackContext* context,
                      lynx_http_response_t* response) {
  LOGD("LynxFetchModule::InvokeJSCallback");
  if (response->status_code <= 0) {
    Napi::Object error = Napi::Object::New(env);
    error.Set("code", response->status_code);
    error.Set("message", response->status_text);
    context->reject.Value().Call({error});
    // Release response
    lynx_http_response_release(response);
  } else {
    Napi::Object result = Napi::Object::New(env);
    result.Set("url", response->url);
    result.Set("status", response->status_code);
    result.Set("statusText", response->status_text);
    Napi::Object headers = Napi::Object::New(env);
    for (auto& header : response->headers) {
      headers.Set(header.first.c_str(), header.second.c_str());
    }
    result.Set("headers", headers);
    if (response->body.content != nullptr && response->body.length > 0) {
      // To avoid copying, create an ArrayBuffer directly using the address of
      // the response body, and release the response in the finalize callback of
      // the ArrayBuffer.
      Napi::ArrayBuffer body = Napi::ArrayBuffer::New(
          env, response->body.content, response->body.length,
          [](napi_env env, void* finalize_data, void* finalize_hint) {
            lynx_http_response_t* response =
                reinterpret_cast<lynx_http_response_t*>(finalize_hint);
            if (response != nullptr) {
              // Release response
              lynx_http_response_release(response);
            }
          },
          response);
      result.Set("body", body);
    } else {
      // Release response
      lynx_http_response_release(response);
    }
    context->resolve.Value().Call({result});
  }
}
void ThreadSafeFunctionFinalizer(Napi::Env env, void* finalizer_data,
                                 FetchCallbackContext* context) {
  LOGD("LynxFetchModule::ThreadSafeFunctionFinalizer");
  if (context) {
    delete context;
  }
}

Napi::Value Fetch(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  const auto& fetch_args = info[0].As<Napi::Object>();
  const auto& resolve = info[1].As<Napi::Function>();
  const auto& reject = info[2].As<Napi::Function>();

  // Unwrap the url, method, headers, and body from the fetch args.
  std::string url = fetch_args["url"].As<Napi::String>();
  std::string method = fetch_args["method"].As<Napi::String>();
  Napi::Object headers = fetch_args["headers"].As<Napi::Object>();
  Napi::ArrayBuffer body = fetch_args["body"].As<Napi::ArrayBuffer>();

  lynx_service_center_t* service_center = lynx_service_get_center_instance();
  lynx_http_service_t* http_service = reinterpret_cast<lynx_http_service_t*>(
      lynx_service_get_service(service_center, kServiceTypeHttp));
  lynx_http_request_t* request = lynx_http_request_create(url);
  request->method = method.empty() ? "GET" : method.c_str();
  // Add headers to the request.
  Napi::Array header_names = headers.GetPropertyNames();
  for (uint32_t i = 0; i < header_names.Length(); i++) {
    std::string header_name = header_names.Get(i).As<Napi::String>();
    std::string header_value =
        headers.Get(header_name.c_str()).As<Napi::String>();
    request->headers[header_name] = header_value;
  }
  if (body.ByteLength() > 0) {
    // Assign the body data to the request.
    uint8_t* data = (uint8_t*)body.Data();
    request->body.assign(data, data + body.ByteLength());
  }
  auto tsf = Napi::TypedThreadSafeFunction<
      FetchCallbackContext, lynx_http_response_t,
      InvokeJSCallback>::New(env, "", 10, 1,
                             new FetchCallbackContext(resolve, reject),
                             ThreadSafeFunctionFinalizer);
  lynx_http_response_t* response = lynx_http_response_create(
      [tsf = std::move(tsf)](lynx_http_response_t* origin_response) mutable {
        // The origin response will be released after this callback, create a
        // wrapped response to keep the content, should be released after js
        // callback.
        lynx_http_response_t* response = lynx_http_response_create(nullptr);
        lynx_http_response_wrap(origin_response, response);
        if (tsf.NonBlockingCall(response) != napi_ok) {
          LOGW("LynxFetchModule::NonBlockingCall failed");
          // Release response if tsf failed.
          lynx_http_response_release(response);
        }
        tsf.Release();
      });
  lynx_http_service_request(http_service, request, response);
  return env.Undefined();
}

napi_value LynxFetchModuleCreator(napi_env c_env, napi_value c_exports,
                                  const char* module_name, void* opaque) {
  // Wrapper to C++.
  Napi::Env env = c_env;
  Napi::Object exports(c_env, c_exports);
  exports["fetch"] = Napi::Function::New(env, &Fetch, "fetch");
  return c_exports;
}

}  // namespace embedder
}  // namespace lynx

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif
