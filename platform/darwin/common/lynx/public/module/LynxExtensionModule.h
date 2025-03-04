// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * This is a experimental API, it is unstable and may break at any time.
 */

#import "LynxContext.h"
#import "LynxGroup.h"

@protocol LynxExtensionModule <NSObject>

+ (NSString *)name;

- (instancetype)initWithLynxContext:(LynxContext *)context group:(LynxGroup *)group;

- (void *)getExtensionDelegate;
// TODO(chenyouhui): Remove this function later.
- (void)setUp;

@end
