// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScrollListener.h"
#import "LynxUI+Internal.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUI (Fluency)

// Used for scrollable LynxUI. Default value is false.
// When setting enableScrollMonitor as true, we will monitor the fluency metrics for this LynxUI and
// send the metrics to the LynxViewClient. Can be enabled by developer or enabled by default with
// some small probability。
@property(nonatomic, assign, readonly) BOOL enableScrollMonitor;

// Used for scrollable LynxUI. Only valid when `enableScrollMonitor` is true.
// It can distinguish which LynxUI is scrolling when we report fluency metrics.
@property(nonatomic, copy, readonly) NSString* scrollMonitorTagName;

- (void)postFluencyEventWithInfo:(LynxScrollInfo*)info;

- (LynxScrollInfo*)infoWithScrollView:(UIScrollView*)view selector:(SEL)selector;

@end

NS_ASSUME_NONNULL_END
