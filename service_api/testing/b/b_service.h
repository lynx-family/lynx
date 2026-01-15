// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SERVICE_API_TESTING_B_B_SERVICE_H_
#define SERVICE_API_TESTING_B_B_SERVICE_H_

#include <service_api/service_api.h>

#include <string>

namespace lynx {
namespace service {
namespace b_service {
class LYNX_SERVICE_DECLARE(BService) : public BaseService<BService> {
 public:
  virtual const std::string get_descriptor() = 0;
  ~BService() override = default;
};
}  // namespace b_service
}  // namespace service
}  // namespace lynx

#endif  // SERVICE_API_TESTING_B_B_SERVICE_H_
