// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseLogBoxProxy.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxLogBoxProxy : NSObject <LynxBaseLogBoxProxy>

@property(nonatomic, readwrite, nullable)
    NSMutableDictionary<NSNumber *, NSMutableArray *> *logMessages;         // level -> msg
@property(nonatomic, readwrite, nullable) NSMutableArray *consoleMessages;  // js console log
@property(nullable, copy, nonatomic, readonly) NSDictionary *allJsSource;
@property(nullable, copy, nonatomic, readonly) NSString *templateUrl;

- (instancetype)initWithLynxView:(nullable LynxView *)view;
- (void)setReloadHelper:(nullable LynxPageReloadHelper *)reloadHelper;
- (void)reloadLynxViewFromLogBox;  // click reload btn on logbox
- (void)setRuntimeId:(NSInteger)runtimeId;
- (nullable NSMutableArray *)logMessagesWithLevel:(LynxLogBoxLevel)level;
- (void)removeLogMessagesWithLevel:(LynxLogBoxLevel)level;
// Get instance id of LynxContext
- (int32_t)getInstanceId;

@end

NS_ASSUME_NONNULL_END
