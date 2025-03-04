// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxPageReloadHelper.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import "LynxLogBoxManager.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxLogBox : NSObject

- (instancetype)initWithLogBoxManager:(LynxLogBoxManager *)manager;
- (void)updateViewInfo:(nullable NSString *)url
          currentIndex:(NSInteger)index
            totalCount:(NSInteger)count;
- (void)updateTemplateUrl:(nullable NSString *)url;
- (BOOL)onNewLog:(nullable NSString *)message
       withLevel:(LynxLogBoxLevel)level
       withProxy:(LynxLogBoxProxy *)proxy;
- (BOOL)onNewConsole:(nullable NSDictionary *)message
           withProxy:(LynxLogBoxProxy *)proxy
              isOnly:(BOOL)only;
- (BOOL)isShowing;
- (BOOL)isConsoleOnly;
- (LynxLogBoxLevel)getCurrentLevel;
- (nullable LynxLogBoxProxy *)getCurrentProxy;
- (void)dismissIfNeeded;

@end

NS_ASSUME_NONNULL_END
