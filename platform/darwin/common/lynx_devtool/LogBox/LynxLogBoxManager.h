// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxLogBoxHelper.h"
#import "LynxLogBoxProxy.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxLogBoxManager : NSObject

- (instancetype)initWithViewController:(nullable UIViewController *)controller;

- (void)onNewLog:(nullable NSString *)message
       withLevel:(LynxLogBoxLevel)level
       withProxy:(LynxLogBoxProxy *)proxy;
- (void)onNewConsole:(nullable NSDictionary *)message withProxy:(LynxLogBoxProxy *)proxy;
- (void)updateTemplateUrl:(nullable NSString *)url withProxy:(LynxLogBoxProxy *)proxy;
- (void)showLogBoxWithLevel:(LynxLogBoxLevel)level;
- (void)removeCurrentLogsWithLevel:(LynxLogBoxLevel)level;
- (void)removeLogsWithLevel:(LynxLogBoxLevel)level;
- (void)changeView:(nullable NSNumber *)indexNum withLevel:(LynxLogBoxLevel)level;
- (void)reloadFromLogBox:(LynxLogBoxProxy *)proxy;
- (void)reloadWithProxy:(LynxLogBoxProxy *)proxy;           // long press or Page.reload
- (void)showConsoleMsgsWithProxy:(LynxLogBoxProxy *)proxy;  // long press menu
- (void)showNotification;
- (void)hideNotification;

NS_ASSUME_NONNULL_END

@end
