// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterPlatformViews.h"

#include <map>
#include <unordered_set>

@interface FlutterPlatformViewController : NSViewController
@end

@interface FlutterPlatformViewController ()

/**
 * Returns the platform view associated with the viewId.
 */
- (nullable NSView*)platformViewWithID:(int64_t)viewId;

/**
 * Register a view factory by adding an entry into the platformViewFactories map with key factoryId
 * and value factory.
 */
- (void)registerViewFactory:(nonnull NSObject<FlutterPlatformViewFactory>*)factory
                     withId:(nonnull NSString*)factoryId;

/**
 * Removes platform views slated to be disposed via method handler calls.
 */
- (void)disposePlatformViews;

@end
