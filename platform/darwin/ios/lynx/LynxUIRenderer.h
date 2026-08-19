// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIRendererProtocol.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIRenderer : NSObject <LynxUIRendererProtocol>

- (BOOL)DispatchPlatformInputEvent:(NSArray*)iEventData withData:(NSArray*)fEventData;
- (void)DispatchPlatformLongPress;
- (void)DispatchPlatformTap;
- (void)SetPlatformEventRootActive:(NSInteger)rootSign active:(BOOL)active;
- (void)SetPlatformEventRootOffset:(NSInteger)rootSign
                           offsetX:(CGFloat)offsetX
                           offsetY:(CGFloat)offsetY;
- (BOOL)IsPlatformEventTargetEventThrough:(NSInteger)rootSign point:(CGPoint)point;
- (BOOL)IsPlatformEventTargetIgnoreFocus:(NSInteger)rootSign point:(CGPoint)point;

@end

NS_ASSUME_NONNULL_END
