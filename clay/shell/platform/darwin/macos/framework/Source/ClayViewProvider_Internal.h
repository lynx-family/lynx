// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/fml/memory/weak_ptr.h"
#import "clay/shell/platform/darwin/macos/framework/Headers/ClayViewProvider.h"
#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterView.h"
#import "clay/shell/platform/darwin/macos/framework/Source/ClayOverlayView.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterKeyboardViewDelegate.h"

@interface ClayViewProvider () <FlutterKeyboardViewDelegate>

/**
 * This just returns the NSPasteboard so that it can be mocked in the tests.
 */
@property(nonatomic, readonly, nonnull) NSPasteboard* pasteboard;

@property(nonatomic, strong, nullable) ClayOverlayView* overlayView;

- (void)setParent:(nullable NSView*)parent;
- (void)ensureOverlayOnTopmost:(nullable NSView*)parent;
- (void)detachOverlay;

#pragma mark - Private interface declaration.

@end
