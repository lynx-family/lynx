// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_VIEW_CLIENT_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_VIEW_CLIENT_H_

#include "capi/lynx_view_client_capi.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief A client that provides callbacks for `LynxView`'s lifecycle and other
 * events.
 */
class LynxViewClient {
 public:
  static LynxViewClient* Unwrap(lynx_view_client_t* client) {
    return static_cast<LynxViewClient*>(lynx_view_client_get_user_data(client));
  }

  LynxViewClient() : client_(lynx_view_client_create(this)) {
    // Bind callbacks to lynx_view_client_t.
    lynx_view_client_bind_on_page_start(
        client_, [](lynx_view_client_t* client, const char* url) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnPageStart(url);
          }
        });
    lynx_view_client_bind_on_load_success(
        client_, [](lynx_view_client_t* client) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnLoadSuccess();
          }
        });
    lynx_view_client_bind_on_first_screen(
        client_, [](lynx_view_client_t* client) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnFirstScreen();
          }
        });
    lynx_view_client_bind_on_page_updated(
        client_, [](lynx_view_client_t* client) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnPageUpdated();
          }
        });
    lynx_view_client_bind_on_data_updated(
        client_, [](lynx_view_client_t* client) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnDataUpdated();
          }
        });
    lynx_view_client_bind_on_destroy(client_, [](lynx_view_client_t* client) {
      auto* wrapper = LynxViewClient::Unwrap(client);
      if (wrapper) {
        wrapper->OnDestroy();
      }
    });
    lynx_view_client_bind_on_runtime_ready(
        client_, [](lynx_view_client_t* client) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnRuntimeReady();
          }
        });
    lynx_view_client_bind_on_received_error(
        client_,
        [](lynx_view_client_t* client, int error_code, const char* message) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnReceivedError(error_code, message);
          }
        });
    lynx_view_client_bind_on_timing_setup(
        client_, [](lynx_view_client_t* client, const char* timing_info) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnTimingSetup(timing_info);
          }
        });
    lynx_view_client_bind_on_timing_update(
        client_, [](lynx_view_client_t* client, const char* timing_info,
                    const char* update_timing, const char* update_flag) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnTimingUpdate(timing_info, update_timing, update_flag);
          }
        });
    lynx_view_client_bind_on_enter_foreground(
        client_, [](lynx_view_client_t* client) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnEnterForeground();
          }
        });
    lynx_view_client_bind_on_enter_background(
        client_, [](lynx_view_client_t* client) {
          auto* wrapper = LynxViewClient::Unwrap(client);
          if (wrapper) {
            wrapper->OnEnterBackground();
          }
        });
  }
  virtual ~LynxViewClient() { lynx_view_client_release(client_); }

  /**
   * @apidoc
   * @brief Called when page start loading.
   * @param url The page URL
   */
  virtual void OnPageStart(const char* url) {}

  /**
   * @apidoc
   * @brief Called when page load finish.
   */
  virtual void OnLoadSuccess() {}

  /**
   * @apidoc
   * @brief Called when first screen layout completed.
   */
  virtual void OnFirstScreen() {}

  /**
   * @apidoc
   * @brief Called when page update.
   */
  virtual void OnPageUpdated() {}

  /**
   * @apidoc
   * @brief Called when data update, but the view may not be updated.
   */
  virtual void OnDataUpdated() {}

  /**
   * @apidoc
   * @brief Called after the page is destroyed.
   * @note The callback executed in the main thread.
   */
  virtual void OnDestroy() {}

  /**
   * @apidoc
   * @brief Called when JS environment preparation completed.
   * @note The callback is in an asynchronous thread.
   */
  virtual void OnRuntimeReady() {}

  /**
   * @apidoc
   * @brief Called when error received.
   * @param error_code The type of the error.
   * @param message The error message.
   */
  virtual void OnReceivedError(int error_code, const char* message) {}

  /**
   * @apidoc
   * @brief Called when timing setup.
   * @param timing_info The timing info.
   */
  virtual void OnTimingSetup(const char* timing_info) {}

  /**
   * @apidoc
   * @brief Called when timing update.
   * @param timing_info The timing info.
   * @param update_timing The update timing.
   * @param update_flag The update flag.
   */
  virtual void OnTimingUpdate(const char* timing_info,
                              const char* update_timing,
                              const char* update_flag) {}

  /**
   * @apidoc
   * @brief Called when the view enters the foreground.
   */
  virtual void OnEnterForeground() {}

  /**
   * @apidoc
   * @brief Called when the view enters the background.
   */
  virtual void OnEnterBackground() {}

  lynx_view_client_t* Impl() { return client_; }

  LynxViewClient(const LynxViewClient&) = delete;
  LynxViewClient& operator=(const LynxViewClient&) = delete;

 private:
  lynx_view_client_t* client_;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_VIEW_CLIENT_H_
