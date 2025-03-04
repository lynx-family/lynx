// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxSSRHelper : NSObject

- (void)onLoadSSRDataBegan:(NSString *)url;

- (void)onHydrateBegan:(NSString *)url;

- (void)onHydrateFinished:(NSString *)url;

- (void)onErrorOccurred:(NSInteger)code sourceError:(NSError *)source;

- (BOOL)isHydratePending;

#pragma mark - event

- (BOOL)shouldSendEventToSSR;

+ (NSArray *)processEventParams:(NSArray *)params;

@end

NS_ASSUME_NONNULL_END
