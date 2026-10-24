// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_ENV_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_ENV_PRIV_H_

namespace lynx::embedder {

// Prepare the Engine environment on the main thread before constructing a
// renderer. An already installed host UI task runner is preserved.
// The build selects the default or macOS implementation.
void InitializeEmbedder();

}  // namespace lynx::embedder

#endif  // PLATFORM_EMBEDDER_LYNX_ENV_PRIV_H_
