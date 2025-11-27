// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef LYNX_PLATFORM_DARWIN_MACOS_LYNX_SERVICE_HTTP_LYNX_HTTP_SERVICE_H_
#define LYNX_PLATFORM_DARWIN_MACOS_LYNX_SERVICE_HTTP_LYNX_HTTP_SERVICE_H_

#include <memory>

#include "lynx_http_service.h"

namespace lynx {
namespace service {
class LynxHttpServiceImpl : public pub::LynxHttpService {
 public:
  LynxHttpServiceImpl() = default;
  ~LynxHttpServiceImpl() = default;

  void Request(std::shared_ptr<pub::LynxHttpRequest> request,
               std::shared_ptr<pub::LynxHttpResponse> response) override;
};
}  // namespace service
}  // namespace lynx

#endif  // LYNX_PLATFORM_DARWIN_MACOS_LYNX_SERVICE_HTTP_LYNX_HTTP_SERVICE_H_
