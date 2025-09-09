// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIMethodProcessor.h>

@interface LynxUIScrollViewInternal : LynxUI <LynxBaseScrollView *>

- (void)scrollTo:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;
- (void)scrollBy:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;
- (void)autoScroll:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;

- (void)scrollInto:(LynxUI *)value isSmooth:(BOOL)isSmooth type:(NSString *)type;
@end
