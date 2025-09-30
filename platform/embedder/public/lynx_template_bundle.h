// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_TEMPLATE_BUNDLE_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_TEMPLATE_BUNDLE_H_

#include <string>

#include "capi/lynx_template_bundle_capi.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief `TemplateBundle` is the output product of the PreDecode capability
 * provided by the Lynx SDK. Client developers can parse the Lynx App Bundle
 * product in advance to obtain the `TemplateBundle` object and consume the App
 * Bundle product.
 */
class LynxTemplateBundle {
 public:
  LynxTemplateBundle(uint8_t* content, size_t size,
                     void (*dtor)(uint8_t*, size_t, void*) = nullptr,
                     void* opaque = nullptr)
      : template_bundle_(
            lynx_template_bundle_create(content, size, dtor, opaque)) {}

  ~LynxTemplateBundle() { lynx_template_bundle_release(template_bundle_); }

  /**
   * @apidoc
   * @brief Determines whether the current `TemplateBundle` object is valid.
   * @return `true` if the current `TemplateBundle` object is valid, else
   * `false`.
   */
  bool IsValid() const {
    return lynx_template_bundle_is_valid(template_bundle_);
  }

  /**
   * @apidoc
   * @brief When `TemplateBundle` is an invalid object, use this method to
   * obtain the exception information that occurred during template parsing
   * @return The exception information.
   */
  std::string GetErrorMessage() {
    const char* error_message =
        lynx_template_bundle_get_error_message(template_bundle_);
    return error_message ? std::string(error_message) : "";
  }

  lynx_template_bundle_t* Impl() { return template_bundle_; }

  LynxTemplateBundle(const LynxTemplateBundle&) = delete;
  LynxTemplateBundle& operator=(const LynxTemplateBundle&) = delete;

 private:
  lynx_template_bundle_t* template_bundle_;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_TEMPLATE_BUNDLE_H_
