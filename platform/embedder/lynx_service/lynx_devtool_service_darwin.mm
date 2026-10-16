// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLazyRegister.h>

#include "platform/embedder/lynx_service/lynx_devtool_service.h"

namespace lynx {
namespace embedder {

service::EmbedderDevToolService* GetDevToolService() {
  // A pure C++ Headless client may query before LynxEnv is initialized. Run the
  // SDK's existing lazy registration tasks before looking up the service.
  [LynxLazyRegister loadLynxInitTask];
  return service::get_service<service::EmbedderDevToolService>();
}

}  // namespace embedder
}  // namespace lynx
