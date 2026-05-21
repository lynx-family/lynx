// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"
#include "platform/embedder/resource/js_source_loader_desktop.h"

namespace lynx {
namespace runtime {
namespace js {

std::string JSSourceLoaderDesktop::LoadJSSource(const std::string& path) {
  // The host app should implement the resource fetch logic for kAssets type.
  UNIMPLEMENTED();
  return "";
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
