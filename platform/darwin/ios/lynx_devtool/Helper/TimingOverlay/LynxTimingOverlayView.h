// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxFSPTracer+Internal.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxTimingOverlayHelper;

@protocol LynxTimingOverlayViewDataSource <NSObject>

- (BOOL)overlayViewIsHidden;
- (CGRect)overlayViewHighlightRect;
- (nullable UIColor*)overlayViewBackgroundColor;
- (NSArray<NSString*>*)overlayViewDescriptionLines;

@end

@interface LynxTimingOverlayView : UIView

- (instancetype)initWithFrame:(CGRect)frame
                   dataSource:(id<LynxTimingOverlayViewDataSource>)dataSource;

- (void)setNeedsUpdate;

- (void)installTapGestureRecognizer:(dispatch_block_t)tapOnce twice:(dispatch_block_t)tapTwice;

@end

NS_ASSUME_NONNULL_END
