// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_DEVTOOL_SERVICE_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_DEVTOOL_SERVICE_H_

#include <service_api/services/devtool/embedder_devtool_service.h>

namespace lynx {
namespace embedder {

// Returns nullptr when the optional debugging component is not linked.
service::EmbedderDevToolService* GetDevToolService();

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_DEVTOOL_SERVICE_H_
