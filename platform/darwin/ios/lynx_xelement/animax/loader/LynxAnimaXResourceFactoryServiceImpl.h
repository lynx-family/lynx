// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AnimaX/AnimaXResourceFactoryService.h>
#import <Foundation/Foundation.h>
#import <Lynx/LynxUIContext.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * A service class that creates and manages AnimaX resource loaders.
 * This class implements the AnimaXResourceFactoryService protocol and provides
 * functionality to create different types of animation resource loaders.
 */
@interface LynxAnimaXResourceFactoryServiceImpl : NSObject <AnimaXResourceFactoryService>

/**
 * Initializes a new instance of LynxAnimaXResourceFactoryService with the provided context.
 *
 * @param context The LynxUIContext instance used to initialize the service.
 * @return An initialized instance of LynxAnimaXResourceFactoryService.
 */
- (instancetype)initWithLynxUIContext:(LynxUIContext *)context;

@end

NS_ASSUME_NONNULL_END
