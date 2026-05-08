// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"
#include "platform/embedder/lynx_service/lynx_security_service_priv.h"
#include "platform/embedder/lynx_service/lynx_service_center_priv.h"
#include "platform/embedder/lynx_template_bundle_priv.h"

LYNX_EXTERN_C lynx_template_bundle_t* lynx_template_bundle_create(
    uint8_t* content, size_t length, void (*dtor)(uint8_t*, size_t, void*),
    void* opaque) {
  lynx_template_bundle_t* bundle = new lynx_template_bundle_t();
  lynx_security_service_t* security_service =
      reinterpret_cast<lynx_security_service_t*>(lynx_service_get_service(
          lynx_service_get_center_instance(), kServiceTypeSecurity));
  // verify content with security service.
  const char* error_msg = nullptr;
  if (lynx_security_service_verify_tasm(security_service, content, length, "",
                                        kTypeTemplate, &error_msg) != 0) {
    LOGE("lynx_template_bundle_create verification failed");
    bundle->error = error_msg ? error_msg : "template verification failed.";
    if (dtor) {
      dtor(content, length, opaque);
    }
  } else {
    // Content copy free is currently not supported, we hope it will be
    // supported later.
    std::vector<uint8_t> source(content, content + length);
    if (dtor) {
      dtor(content, length, opaque);
    }
    auto template_bundle = std::make_shared<lynx::tasm::LynxTemplateBundle>();
    std::string error =
        template_bundle->FromBinaryGreedy(std::move(source), "");
    if (!error.empty()) {
      // decode failed.
      bundle->error = error;
    } else {
      // decode success.
      bundle->template_bundle = std::move(template_bundle);
    }
  }

  return bundle;
}

LYNX_EXTERN_C int lynx_template_bundle_is_valid(
    lynx_template_bundle_t* bundle) {
  return bundle->template_bundle ? 1 : 0;
}

LYNX_EXTERN_C const char* lynx_template_bundle_get_error_message(
    lynx_template_bundle_t* bundle) {
  return bundle->error.c_str();
}

LYNX_EXTERN_C void lynx_template_bundle_release(
    lynx_template_bundle_t* bundle) {
  delete bundle;
}
