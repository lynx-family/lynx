// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxDevtool.h"
#import "LynxPageReloadHelper.h"

typedef NS_ENUM(NSInteger, LynxLogBoxLevel) {
  LynxLogBoxLevelInfo,
  LynxLogBoxLevelWarning,
  LynxLogBoxLevelError,
};

NS_ASSUME_NONNULL_BEGIN

@protocol LynxBaseLogBoxProxy <NSObject>

@required

- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view;

- (void)onMovedToWindow;

- (void)setReloadHelper:(nullable LynxPageReloadHelper *)reload_helper;

- (void)showLogMessage:(LynxError *)error;

- (void)attachLynxView:(nonnull LynxView *)lynxView;

- (void)reloadLynxView;  // long press, Page.reload, etc

- (void)setRuntimeId:(NSInteger)runtimeId;

- (void)showConsole;

- (void)destroy;

- (void)setLynxDevtool:(LynxDevtool *)devtool;

@end

NS_ASSUME_NONNULL_END
