// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DARWIN_COMMON_LYNX_MODULE_LYNXEMBEDDEDMODULE_H_
#define DARWIN_COMMON_LYNX_MODULE_LYNXEMBEDDEDMODULE_H_
#import <Foundation/Foundation.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxContextModule.h>
#import <Lynx/LynxModule.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxEmbeddedModule : NSObject <LynxContextModule>
- (instancetype)initWithLynxContext:(LynxContext *)context;

- (instancetype)initWithLynxContext:(LynxContext *)context WithParam:(id)param;
@end

NS_ASSUME_NONNULL_END
#endif  // DARWIN_COMMON_LYNX_MODULE_LYNXEMBEDDEDMODULE_H_
