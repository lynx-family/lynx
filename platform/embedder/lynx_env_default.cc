// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/task_runner_manufactor.h"
#include "platform/embedder/lynx_env_priv.h"

namespace lynx::embedder {

void InitializeEmbedder() {
#if !defined(OS_WIN)
  base::UIThread::Init();
#endif
}

}  // namespace lynx::embedder
