// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * This is a experimental API, it is unstable and may break at any time.
 */

#import "LynxConfig.h"
#import "LynxContext.h"
#import "LynxGroup.h"
#import "LynxServiceProtocol.h"

@protocol LynxServiceExtensionProtocol <LynxServiceProtocol>

- (void)onLynxEnvSetup;

- (void)onLynxViewSetup:(LynxContext *)lynxContext
                  group:(LynxGroup *)group
                 config:(LynxConfig *)config;

- (void)onLynxViewDestroy;

@end
