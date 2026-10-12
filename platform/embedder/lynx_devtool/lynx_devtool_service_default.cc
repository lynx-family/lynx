// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_devtool/lynx_devtool_service_factory.h"
#include "platform/embedder/lynx_service/lynx_devtool_service.h"

namespace lynx {
namespace embedder {

service::EmbedderDevToolService* GetDevToolService() {
  static auto* instance = CreateEmbedderDevToolService();
  return instance;
}

}  // namespace embedder
}  // namespace lynx
