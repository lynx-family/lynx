// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterViewController.h"

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterKeyboardViewDelegate.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterTextInputPlugin.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterView.h"

@interface FlutterViewController () <FlutterKeyboardViewDelegate>

// The FlutterView for this view controller.
@property(nonatomic, readonly, nullable) FlutterView* flutterView;

/**
 * The text input plugin that handles text editing state for text fields.
 */
@property(nonatomic, readonly, nonnull) FlutterTextInputPlugin* textInputPlugin;

/**
 * Returns YES if provided event is being currently redispatched by keyboard manager.
 */
- (BOOL)isDispatchingKeyEvent:(nonnull NSEvent*)event;

@end
