// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseLogBoxProxy.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxLogBoxProxy : NSObject <LynxBaseLogBoxProxy>

@property(nonatomic, readwrite, nullable)
    NSMutableDictionary<NSNumber *, NSMutableArray *> *logMessages;  // level -> msg
@property(nullable, copy, nonatomic, readonly) NSDictionary *allJsSource;
@property(nullable, copy, nonatomic, readonly) NSString *templateUrl;

- (instancetype)initWithLynxView:(nullable LynxView *)view;
- (nullable NSMutableArray *)logMessagesWithLevel:(LynxLogBoxLevel)level;
- (void)removeLogMessagesWithLevel:(LynxLogBoxLevel)level;
- (NSString *)getErrorNamespace;

@end

NS_ASSUME_NONNULL_END
