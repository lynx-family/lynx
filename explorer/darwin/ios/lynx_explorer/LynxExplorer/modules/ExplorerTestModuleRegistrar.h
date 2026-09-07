// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@class LynxConfig;
@class LynxBackgroundRuntimeOptions;

NS_ASSUME_NONNULL_BEGIN

/// Keeps Explorer's platform-test module set identical across every iOS page
/// construction path.
@interface ExplorerTestModuleRegistrar : NSObject

/// The ordered module class list, exposed so host wiring can be asserted without
/// depending on LynxConfig's private registry representation.
@property(nonatomic, readonly, copy, class) NSArray<Class> *moduleClasses;

+ (void)registerModulesInConfig:(LynxConfig *)config NS_SWIFT_NAME(registerModules(in:));
+ (void)registerBackgroundModulesInOptions:(LynxBackgroundRuntimeOptions *)options
    NS_SWIFT_NAME(registerBackgroundModules(in:));

@end

NS_ASSUME_NONNULL_END
