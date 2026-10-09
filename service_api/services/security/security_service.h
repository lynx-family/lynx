// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SERVICE_API_SERVICES_SECURITY_SECURITY_SERVICE_H_
#define SERVICE_API_SERVICES_SECURITY_SECURITY_SERVICE_H_

#include <service_api/service_api.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace lynx {
namespace service {
namespace security_service {

enum class LynxTasmType { kTemplate, kDynamicComponent };

struct TasmVerificationResult {
  bool verified{false};
  int error_code{0};
  uint64_t sign_id{0};
  std::string error_message;
  std::string extra_config;
};

class LYNX_SERVICE_DECLARE(LynxSecurityService)
    : public BaseService<LynxSecurityService> {
 public:
  ~LynxSecurityService() override = default;

  // The input is borrowed for this synchronous call and is never retained.
  virtual TasmVerificationResult VerifyTASM(const uint8_t* data, size_t size,
                                            const std::string& url,
                                            LynxTasmType type) = 0;
};

}  // namespace security_service
}  // namespace service
}  // namespace lynx

#endif  // SERVICE_API_SERVICES_SECURITY_SECURITY_SERVICE_H_
