// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <service_api/service_api.h>

#include <iostream>
#include <string>

class AServiceImpl : public lynx::service::a_service::AService {
 public:
  const std::string get_descriptor() override { return "func_a"; }
};

LYNX_SERVICE_REGISTER(AServiceImpl);
DEFINE_LINK_ANCHOR(AServiceImpl);
