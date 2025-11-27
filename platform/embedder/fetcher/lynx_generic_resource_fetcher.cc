// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"
#include "platform/embedder/fetcher/lynx_generic_resource_fetcher_priv.h"
#include "platform/embedder/public/capi/lynx_memory_capi.h"

LYNX_EXTERN_C lynx_generic_resource_fetcher_t*
lynx_generic_resource_fetcher_create(void* user_data) {
  return lynx_generic_resource_fetcher_create_with_finalizer(user_data,
                                                             nullptr);
}

LYNX_EXTERN_C lynx_generic_resource_fetcher_t*
lynx_generic_resource_fetcher_create_with_finalizer(
    void* user_data,
    void (*finalizer)(lynx_generic_resource_fetcher_t*, void*)) {
  lynx_generic_resource_fetcher_t* fetcher =
      new lynx_generic_resource_fetcher_t;
  fetcher->user_data = user_data;
  fetcher->finalizer = finalizer;
  // The resource fetcher may be used in multiple threads, so we need to
  // manage it with ref count.
  lynx_generic_resource_fetcher_ref(fetcher);
  return fetcher;
}

LYNX_EXTERN_C void* lynx_generic_resource_fetcher_get_user_data(
    lynx_generic_resource_fetcher_t* fetcher) {
  return fetcher->user_data;
}

LYNX_EXTERN_C void lynx_generic_resource_fetcher_bind_fetch_resource(
    lynx_generic_resource_fetcher_t* fetcher, fetch_resource_func f) {
  fetcher->fetch_resource = f;
}

LYNX_EXTERN_C void lynx_generic_resource_fetcher_bind_fetch_resource_path(
    lynx_generic_resource_fetcher_t* fetcher, fetch_resource_func f) {
  fetcher->fetch_resource_path = f;
}

LYNX_EXTERN_C void lynx_generic_resource_fetcher_bind_cancel_fetch(
    lynx_generic_resource_fetcher_t* fetcher, cancel_fetch_func f) {
  fetcher->cancel_fetch = f;
}

LYNX_EXTERN_C void lynx_generic_resource_fetcher_release(
    lynx_generic_resource_fetcher_t* fetcher) {
  lynx_generic_resource_fetcher_unref(fetcher);
}

LYNX_EXTERN_C void lynx_generic_resource_fetcher_fetch_resource(
    lynx_generic_resource_fetcher_t* fetcher, lynx_resource_request_t* request,
    lynx_resource_response_t* response) {
  if (fetcher->fetch_resource) {
    fetcher->fetch_resource(fetcher, request, response);
  } else {
    lynx_resource_request_release(request);
    lynx_resource_response_set_code(response, -1);
    lynx_resource_response_set_error_message(response,
                                             "fetch_resource is unimplemented");
    lynx_resource_response_callback(response);
    lynx_resource_response_release(response);
  }
}

LYNX_EXTERN_C void lynx_generic_resource_fetcher_fetch_resource_path(
    lynx_generic_resource_fetcher_t* fetcher, lynx_resource_request_t* request,
    lynx_resource_response_t* response) {
  if (fetcher->fetch_resource_path) {
    fetcher->fetch_resource_path(fetcher, request, response);
  } else {
    lynx_resource_request_release(request);
    lynx_resource_response_set_code(response, -1);
    lynx_resource_response_set_error_message(
        response, "fetch_resource_path is unimplemented");
    lynx_resource_response_callback(response);
    lynx_resource_response_release(response);
  }
}

LYNX_EXTERN_C void lynx_generic_resource_fetcher_cancel_fetch(
    lynx_generic_resource_fetcher_t* fetcher,
    lynx_resource_request_id request_id) {
  if (fetcher->cancel_fetch) {
    fetcher->cancel_fetch(fetcher, request_id);
  }
}

void lynx_generic_resource_fetcher_ref(
    lynx_generic_resource_fetcher_t* fetcher) {
  fetcher->ref_count.fetch_add(1);
  DCHECK(fetcher->ref_count.load() > 0);
}

void lynx_generic_resource_fetcher_unref(
    lynx_generic_resource_fetcher_t* fetcher) {
  DCHECK(fetcher->ref_count.load() > 0);
  if (fetcher->ref_count.fetch_sub(1) == 1) {
    if (fetcher->finalizer) {
      fetcher->finalizer(fetcher, fetcher->user_data);
    }
    delete fetcher;
  }
}

LYNX_EXTERN_C void lynx_generic_resource_fetcher_bind_intercept_func(
    lynx_generic_resource_fetcher_t* fetcher,
    lynx_resource_intercept_func func) {
  if (fetcher) {
    fetcher->intercept_func = func;
  }
}

LYNX_EXTERN_C char* lynx_generic_resource_fetcher_intercept(
    lynx_generic_resource_fetcher_t* fetcher, const char* url,
    bool should_decode) {
  if (!url) {
    return nullptr;
  }
  if (fetcher && fetcher->intercept_func) {
    // The implementation of intercept_func is now also responsible
    // for using lynx_strdup() or lynx_malloc() to allocate the string.
    return fetcher->intercept_func(url, should_decode, fetcher->user_data);
  }
  return lynx_strdup(url);
}
