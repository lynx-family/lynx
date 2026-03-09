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

// Updates painted regions used by hitTest:.
- (void)updateOpaqueRects:(NSArray<NSValue*>* _Nonnull)rects;

// Converts a Lynx top-left device-pixel rect into this view's point space.
- (NSRect)viewRectFromDevicePixelRect:(NSRect)rect contentsScale:(CGFloat)scale;

@end

#endif  // PLATFORM_DARWIN_MACOS_COMMON_LYNX_OVERLAY_VIEW_H_
