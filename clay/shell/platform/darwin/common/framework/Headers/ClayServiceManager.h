// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import "FlutterMacros.h"

FLUTTER_DARWIN_EXPORT
@interface ClayServiceManager : NSObject

+ (void)registerGlobalService:(Protocol *)protocol service:(id)service;
- (void)registerService:(Protocol *)protocol service:(id)service;
- (id)getService:(Protocol *)protocol;

@end
