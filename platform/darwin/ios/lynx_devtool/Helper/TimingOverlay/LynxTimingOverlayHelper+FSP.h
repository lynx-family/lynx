// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxTimingOverlayHelper.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxFSPTracerObserver;

@interface LynxTimingFSPInfo : NSObject

- (BOOL)isEnded;

- (BOOL)isHidden;

- (UIColor *)backgroundColor;

- (NSInteger)highlightElementSign;

- (NSArray<NSString *> *)informationLines;

- (void)updateStartTimestamp:(uint64_t)timestamp;

@end

@interface LynxTimingOverlayHelper (FSP) <LynxFSPTracerObserver>

- (void)setupFSPWithUIOwner:(LynxUIOwner *)uiOwner view:(LynxView *)view;

- (LynxTimingFSPInfo *)fspInfo;

@end

NS_ASSUME_NONNULL_END
