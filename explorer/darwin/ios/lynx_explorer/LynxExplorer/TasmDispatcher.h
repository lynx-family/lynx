// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface TasmDispatcher : NSObject

+ (instancetype)sharedInstance;
// open a new LynxView with ShellViewController
- (void)openTargetUrl:(NSString*)sourceUrl;
// for e2e testing
- (void)openTargetUrlSingleTop:(NSString*)sourceUrl;

@end

NS_ASSUME_NONNULL_END
