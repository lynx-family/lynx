// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_devtool/lynx_devtool_service_factory.h"

namespace lynx {
namespace embedder {

// The existing iOS Inspector owner retains this object file in static links.
// Registration uses SDK lazy-load tasks in both Debug and Release.
DEFINE_LINK_ANCHOR(embedder_devtool_service);
LYNX_SERVICE_REGISTER_CREATOR({ return CreateEmbedderDevToolService(); });

}  // namespace embedder
}  // namespace lynx
