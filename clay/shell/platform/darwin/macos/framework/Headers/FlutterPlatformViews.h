// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef FLUTTER_FLUTTERPLATFORMVIEWS_H_
#define FLUTTER_FLUTTERPLATFORMVIEWS_H_

#import <AppKit/AppKit.h>

#import "FlutterMacros.h"

@protocol FlutterPlatformViewFactory <NSObject>

/**
 * Create a Platform View which is an `NSView`.
 *
 * A MacOS plugin should implement this method and return an `NSView`, which can be embedded in a
 * Flutter App.
 *
 * The implementation of this method should create a new `NSView`.
 *
 * @param viewId A unique identifier for this view.
 * @param args Parameters for creating the view sent from the Dart side of the
 * Flutter app. If `createArgsCodec` is not implemented, or if no creation arguments were sent from
 * the Dart code, this will be null. Otherwise this will be the value sent from the Dart code as
 * decoded by `createArgsCodec`.
 */
- (nonnull NSView*)createWithViewIdentifier:(int64_t)viewId arguments:(nullable id)args;

@end

#endif  // FLUTTER_FLUTTERPLATFORMVIEWS_H_
