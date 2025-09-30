// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_SECURITY_SERVICE_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_SECURITY_SERVICE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "capi/lynx_security_service_capi.h"
#include "capi/lynx_service_center_capi.h"
#include "lynx_service_center.h"

namespace lynx {
namespace pub {

class LynxSecurityService
    : public LynxServiceBase,
      public std::enable_shared_from_this<LynxSecurityService> {
 public:
  LynxSecurityService() = default;
  ~LynxSecurityService() = default;

  void InitIfNeeded() override {
    if (security_service_) {
      return;
    }
    security_service_ = lynx_security_service_create_with_finalizer(
        new std::weak_ptr<LynxSecurityService>(weak_from_this()),
        [](lynx_security_service_t* security_service, void* user_data) {
          std::weak_ptr<LynxSecurityService>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxSecurityService>*>(user_data);
          delete weak_ptr;
        });
    lynx_security_service_bind(
        security_service_, [](lynx_security_service_t* security_service,
                              uint8_t* content, size_t length, const char* url,
                              lynx_tasm_type_e type, const char** error_msg) {
          std::weak_ptr<LynxSecurityService>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxSecurityService>*>(
                  lynx_security_service_get_user_data(security_service));
          std::shared_ptr<LynxSecurityService> shared_security_service =
              weak_ptr ? weak_ptr->lock() : nullptr;
          if (shared_security_service) {
            return shared_security_service->VerifyTASM(content, length, url,
                                                       type, error_msg);
          }
          return 0;
        });
  }

  lynx_service_type_e GetServiceType() override { return kServiceTypeSecurity; }

  void* Impl() override { return security_service_; }

  /**
   * @brief This function is designed to use specified verify logic to check the
   * template consistency.
   * @param content The TASM content to be checked.
   * @param length The TASM content length.
   * @param url The TASM content URL.
   * @param type The TASM type.
   * @param error_msg The error message.
   * @return int The verification result.
   */
  virtual int VerifyTASM(uint8_t* content, size_t length, const char* url,
                         lynx_tasm_type_e type, const char** error_msg) = 0;

 private:
  lynx_security_service_t* security_service_ = nullptr;
};
}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_SECURITY_SERVICE_H_
