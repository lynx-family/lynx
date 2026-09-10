// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <optional>
#include <string>

#include "platform/embedder/lynx_service/lynx_trail_service_priv.h"

lynx_trail_service_t::~lynx_trail_service_t() {
  if (finalizer) {
    finalizer(this, user_data);
  }
}

LYNX_EXTERN_C lynx_trail_service_t* lynx_trail_service_create(void* user_data) {
  return lynx_trail_service_create_with_finalizer(user_data, nullptr);
}

LYNX_EXTERN_C lynx_trail_service_t* lynx_trail_service_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_trail_service_t*, void*)) {
  auto* trail_service = new lynx_trail_service_t;
  trail_service->user_data = user_data;
  trail_service->finalizer = finalizer;
  return trail_service;
}

LYNX_EXTERN_C void* lynx_trail_service_get_user_data(
    lynx_trail_service_t* trail_service) {
  return trail_service ? trail_service->user_data : nullptr;
}

LYNX_EXTERN_C void lynx_trail_service_bind(lynx_trail_service_t* trail_service,
                                           lynx_trail_string_value_func f) {
  if (trail_service) {
    trail_service->string_value_func.store(f, std::memory_order_release);
  }
}

LYNX_EXTERN_C void lynx_trail_service_get_string_value(
    lynx_trail_service_t* trail_service, const char* key,
    lynx_trail_string_value_callback_func callback, void* user_data) {
  if (!callback) {
    return;
  }

  std::optional<std::string> value;
  if (trail_service) {
    trail_service->AddRef();
    auto func =
        trail_service->string_value_func.load(std::memory_order_acquire);
    if (func && key) {
      const char* raw_value = func(trail_service, key);
      if (raw_value) {
        value = raw_value;
      }
    }
  }

  callback(value ? value->c_str() : nullptr, user_data);
  if (trail_service) {
    trail_service->Release();
  }
}

LYNX_EXTERN_C void lynx_trail_service_release(
    lynx_trail_service_t* trail_service) {
  if (!trail_service) {
    return;
  }
  trail_service->Release();
}
