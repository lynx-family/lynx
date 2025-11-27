// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/darwin/macos/framework/Headers/ClayViewProvider.h"
#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterEngine.h"

#import <Cocoa/Cocoa.h>

#include <memory>

#import "base/include/closure.h"
#import "clay/public/clay.h"
#import "clay/shell/platform/darwin/macos/framework/Source/ClayMouseCursorPlugin.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterPlatformViewController.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterRenderer.h"

@interface FlutterEngine ()

/**
 * True if the engine is currently running.
 */
@property(nonatomic, readonly) BOOL running;

/**
 * Provides the renderer config needed to initialize the engine.
 */
@property(nonatomic, readonly, nullable) FlutterRenderer* renderer;

/**
 * The executable name for the current process.
 */
@property(nonatomic, readonly, nonnull) NSString* executableName;

/**
 * This just returns the NSPasteboard so that it can be mocked in the tests.
 */
@property(nonatomic, readonly, nonnull) NSPasteboard* pasteboard;

@property(nonatomic, weak) ClayViewProvider* clayViewProvider;

/**
 * Informs the engine that the associated view controller's view size has changed.
 */
- (void)updateWindowMetrics;

/**
 * Dispatches the given pointer event data to engine.
 */
- (void)sendPointerEvent:(const ClayPointerEvent&)event;

/**
 * Dispatches the given pointer event data to engine.
 */
- (void)sendKeyEvent:(const ClayKeyEvent&)event
            callback:(nullable ClayKeyEventCallback)callback
            userData:(nullable void*)userData;

- (nonnull FlutterPlatformViewController*)platformViewController;

- (BOOL)postUIThreadTask:(const fml::closure&)callback;

- (void)setViewProvider:(nonnull ClayViewProvider*)viewProvider;

@end
