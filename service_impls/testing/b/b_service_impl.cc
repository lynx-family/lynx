// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <service_api/service_api.h>

#include <iostream>
#include <string>

class BServiceImpl : public lynx::service::b_service::BService {
  const std::string desc;

 public:
  explicit BServiceImpl(const std::string& desc) : desc(desc) {}
  const std::string get_descriptor() override { return desc; }
};

LYNX_SERVICE_REGISTER_CREATOR({ return new BServiceImpl("with creator"); });
