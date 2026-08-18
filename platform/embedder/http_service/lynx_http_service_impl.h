// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_HTTP_SERVICE_LYNX_HTTP_SERVICE_IMPL_H_
#define PLATFORM_EMBEDDER_HTTP_SERVICE_LYNX_HTTP_SERVICE_IMPL_H_

#include <memory>

#include "platform/embedder/public/lynx_http_service.h"

namespace lynx {
namespace embedder {

// A reusable, open-source HTTP service backed by each platform's native system
// networking library (WinHTTP on Windows, NSURLSession on macOS/iOS). It is a
// shared embedder library so that any desktop embedder app can enable JS
// fetch() by registering it, rather than each app reimplementing the transport.
//
// Register it once at startup:
//   lynx::pub::LynxServiceCenter::GetInstance().RegisterService(
//       std::make_shared<lynx::embedder::LynxHttpServiceImpl>());
//
// The per-platform transport lives in the platform-specific translation units
// (lynx_http_service_impl_win.cc, lynx_http_service_impl_darwin.mm).
class LynxHttpServiceImpl : public pub::LynxHttpService {
 public:
  LynxHttpServiceImpl() = default;
  ~LynxHttpServiceImpl() = default;

  void Request(std::shared_ptr<pub::LynxHttpRequest> request,
               std::shared_ptr<pub::LynxHttpResponse> response) override;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_HTTP_SERVICE_LYNX_HTTP_SERVICE_IMPL_H_
