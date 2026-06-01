// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_GENERIC_RESOURCE_FETCHER_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_GENERIC_RESOURCE_FETCHER_H_

#include <memory>
#include <string>

#include "capi/lynx_generic_resource_fetcher_capi.h"
#include "capi/lynx_memory_capi.h"
#include "lynx_resource_request.h"
#include "lynx_resource_response.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief `LynxGenericResourceFetcher` is defined inside `LynxEngine`
 * and injected from outside to implement a general resource loading interface.
 */
class LynxGenericResourceFetcher
    : public std::enable_shared_from_this<LynxGenericResourceFetcher> {
 public:
  LynxGenericResourceFetcher() : fetcher_(nullptr) {}

  void InitIfNeeded() {
    if (fetcher_) {
      return;
    }
    fetcher_ = lynx_generic_resource_fetcher_create_with_finalizer(
        new std::weak_ptr<LynxGenericResourceFetcher>(weak_from_this()),
        [](lynx_generic_resource_fetcher_t* fetcher, void* user_data) {
          std::weak_ptr<LynxGenericResourceFetcher>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxGenericResourceFetcher>*>(
                  user_data);
          delete weak_ptr;
        });
    lynx_generic_resource_fetcher_bind_fetch_resource(
        fetcher_, [](lynx_generic_resource_fetcher_t* fetcher,
                     lynx_resource_request_t* request,
                     lynx_resource_response_t* response) {
          auto req = std::make_shared<resource::LynxResourceRequest>(request);
          auto resp =
              std::make_shared<resource::LynxResourceResponse>(response);
          std::weak_ptr<LynxGenericResourceFetcher>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxGenericResourceFetcher>*>(
                  lynx_generic_resource_fetcher_get_user_data(fetcher));
          std::shared_ptr<LynxGenericResourceFetcher> shared_fetcher =
              weak_ptr ? weak_ptr->lock() : nullptr;
          if (!shared_fetcher) {
            resp->SetCode(-1);
            resp->SetErrorMessage("resource fetcher is destroyed");
          } else {
            shared_fetcher->FetchResource(req, resp);
          }
        });
    lynx_generic_resource_fetcher_bind_fetch_resource_path(
        fetcher_, [](lynx_generic_resource_fetcher_t* fetcher,
                     lynx_resource_request_t* request,
                     lynx_resource_response_t* response) {
          auto req = std::make_shared<resource::LynxResourceRequest>(request);
          auto resp =
              std::make_shared<resource::LynxResourceResponse>(response);
          std::weak_ptr<LynxGenericResourceFetcher>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxGenericResourceFetcher>*>(
                  lynx_generic_resource_fetcher_get_user_data(fetcher));
          std::shared_ptr<LynxGenericResourceFetcher> shared_fetcher =
              weak_ptr ? weak_ptr->lock() : nullptr;
          if (!shared_fetcher) {
            resp->SetCode(-1);
            resp->SetErrorMessage("resource fetcher is destroyed");
          } else {
            shared_fetcher->FetchResourcePath(req, resp);
          }
        });

    lynx_generic_resource_fetcher_bind_cancel_fetch(
        fetcher_, [](lynx_generic_resource_fetcher_t* fetcher,
                     lynx_resource_request_id request_id) {
          std::weak_ptr<LynxGenericResourceFetcher>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxGenericResourceFetcher>*>(
                  lynx_generic_resource_fetcher_get_user_data(fetcher));
          std::shared_ptr<LynxGenericResourceFetcher> shared_fetcher =
              weak_ptr ? weak_ptr->lock() : nullptr;
          if (shared_fetcher) {
            shared_fetcher->Cancel(request_id);
          }
        });
    lynx_generic_resource_fetcher_bind_intercept_func(
        fetcher_,
        [](const char* url, bool should_decode, void* user_data) -> char* {
          if (!url) {
            return nullptr;
          }
          std::weak_ptr<LynxGenericResourceFetcher>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxGenericResourceFetcher>*>(
                  user_data);
          std::shared_ptr<LynxGenericResourceFetcher> shared_fetcher =
              weak_ptr ? weak_ptr->lock() : nullptr;
          if (!shared_fetcher) {
            return nullptr;
          }
          std::string intercepted_url =
              shared_fetcher->InterceptUrl(url, should_decode);
          if (intercepted_url.empty()) {
            return nullptr;
          }
          return lynx_strdup(intercepted_url.c_str());
        });
  }

  virtual ~LynxGenericResourceFetcher() {
    if (fetcher_) {
      lynx_generic_resource_fetcher_release(fetcher_);
    }
  }
  /**
   * @apidoc
   * @brief This function is designed to fetch a resource. It's designed to be
   * called potentially from the UI thread. To avoid blocking the UI and ensure
   * smooth user experience, developers are expected to override this method and
   * post the actual resource fetching task to an IO thread.
   * @param request The resource request object.
   * @param response The resource response object.
   */
  virtual void FetchResource(
      std::shared_ptr<resource::LynxResourceRequest> request,
      std::shared_ptr<resource::LynxResourceResponse> response) {}

  /**
   * @apidoc
   * @brief This function is designed to fetch a resource path. It's designed to
   * be called potentially from the UI thread. To avoid blocking the UI and
   * ensure smooth user experience, developers are expected to override this
   * method and post the actual resource fetching task to an IO thread.
   * @param request The resource request object.
   * @param response The resource response object.
   */
  virtual void FetchResourcePath(
      std::shared_ptr<resource::LynxResourceRequest> request,
      std::shared_ptr<resource::LynxResourceResponse> response) {}

  /**
   * @apidoc
   * @brief Cancels a resource request with the specified ID. This virtual
   * method provides a way to cancel an ongoing resource request identified by
   * the given ID. The default implementation does nothing, and derived classes
   * should override this method to implement the actual cancellation logic.
   * @param request_id The ID of the resource request to be canceled.
   */
  virtual void Cancel(lynx_resource_request_id request_id) {}

  /**
   * @apidoc
   * @brief Intercepts the input resource URL before it is fetched.
   * Derived classes can override this method to rewrite the original URL.
   * @param origin_url The original resource URL.
   * @param should_decode Whether the returned URL should be decoded.
   * @return The intercepted URL. The default implementation returns the
   * original URL unchanged.
   */
  virtual std::string InterceptUrl(const std::string& origin_url,
                                   bool should_decode) {
    return origin_url;
  }

  lynx_generic_resource_fetcher_t* Impl() { return fetcher_; }

 private:
  lynx_generic_resource_fetcher_t* fetcher_ = nullptr;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_GENERIC_RESOURCE_FETCHER_H_
