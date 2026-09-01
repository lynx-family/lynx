// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_MACOS_COMMON_LYNX_OVERLAY_VIEW_H_
#define PLATFORM_DARWIN_MACOS_COMMON_LYNX_OVERLAY_VIEW_H_

#import <Cocoa/Cocoa.h>

@class ClayViewProvider;

// Transparent host view for Lynx overlay content above native plugin views.
// Hit testing only keeps events in painted regions and passes through the rest.
@interface ClayOverlayView : NSView

@property(nonatomic, nullable, weak) ClayViewProvider* eventDelegate;

// Updates hit-test regions keyed by the owning View ID.
- (void)updateOpaqueRects:(NSDictionary<NSNumber*, NSValue*>* _Nonnull)rects
                viewOrder:(NSArray<NSNumber*>* _Nonnull)viewOrder;

- (void)setEventsPassThrough:(BOOL)eventsPassThrough forViewId:(int64_t)viewId;
- (void)removeOpaqueRectForViewId:(int64_t)viewId;
- (void)removeHitTestStateForViewId:(int64_t)viewId;

// Converts a Lynx top-left device-pixel rect into this view's point space.
- (NSRect)viewRectFromDevicePixelRect:(NSRect)rect contentsScale:(CGFloat)scale;

@end

#endif  // PLATFORM_DARWIN_MACOS_COMMON_LYNX_OVERLAY_VIEW_H_
