// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SERVICE_API_TESTING_A_A_SERVICE_H_
#define SERVICE_API_TESTING_A_A_SERVICE_H_

#include <service_api/service_api.h>

#include <string>

namespace lynx::service::a_service {
class LYNX_SERVICE_DECLARE(AService) : public BaseService<AService> {
 public:
  virtual const std::string get_descriptor() = 0;
  ~AService() override = default;
};
}  // namespace lynx::service::a_service

#endif  // SERVICE_API_TESTING_A_A_SERVICE_H_
