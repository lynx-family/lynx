// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterViewProvider.h"

@class FlutterEngine;

/**
 * A facade over FlutterEngine that allows FlutterEngine's children components
 * to query FlutterView.
 *
 * FlutterViewProvider only holds a weak reference to FlutterEngine.
 */
@interface FlutterViewEngineProvider : NSObject <FlutterViewProvider>

/**
 * Create a FlutterViewProvider with the underlying engine.
 */
- (nonnull instancetype)initWithEngine:(nonnull __weak FlutterEngine*)engine;

@end
