// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import "FlutterMacros.h"

// TODO: Merge this file and FlutterPluginRegistrarMacOS.h with the iOS FlutterPlugin.h, sharing
// all but the platform-specific methods.

@protocol FlutterPluginRegistrar;

/**
 * Implemented by the platform side of a Flutter plugin.
 *
 * Defines a set of optional callback methods and a method to set up the plugin
 * and register it to be called by other application components.
 *
 * Currently the macOS version of FlutterPlugin has very limited functionality, but is expected to
 * expand over time to more closely match the functionality of the iOS FlutterPlugin.
 */
FLUTTER_DARWIN_EXPORT
@protocol FlutterPlugin <NSObject>

/**
 * Creates an instance of the plugin to register with |registrar| using the desired
 * FlutterPluginRegistrar methods.
 */
+ (void)registerWithRegistrar:(nonnull id<FlutterPluginRegistrar>)registrar;

@end
