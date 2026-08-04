// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxNodeAPILifecycleListener.h"
#import "LynxNodeAPIModule.h"

@implementation LynxNodeAPILifecycleListener

- (instancetype)initWithToken:(id)token {
  if (self = [super init]) {
    _token = token;
  }
  return self;
}

- (void)onRuntimeAttach:(void *)napiEnv {
  [LynxNodeAPIModule putEnv:napiEnv forToken:self.token];
}

- (void)onRuntimeDetach {
  [LynxNodeAPIModule removeEnvForToken:self.token];
}

@end
