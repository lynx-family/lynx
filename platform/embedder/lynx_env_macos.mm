// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_env_priv.h"

#import <Foundation/Foundation.h>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx::embedder {

void InitializeEmbedder() {
  base::UIThread::Init();

#if ENABLE_TESTBENCH_RECORDER
  // Set storage directory for testbench recorder.
  if (tasm::LynxEnv::GetInstance().GetStorageDirectory().empty()) {
    NSString* filePath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask,
                                                              YES) lastObject];
    if (filePath) {
      tasm::LynxEnv::GetInstance().SetStorageDirectory([filePath UTF8String]);
    }
  }
#endif
}

}  // namespace lynx::embedder
