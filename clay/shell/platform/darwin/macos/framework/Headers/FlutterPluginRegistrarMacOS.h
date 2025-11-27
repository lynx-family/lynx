// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

#import "FlutterMacros.h"
#import "FlutterPlatformViews.h"
#import "FlutterPluginMacOS.h"

// TODO: Merge this file and FlutterPluginMacOS.h with the iOS FlutterPlugin.h, sharing all but
// the platform-specific methods.

/**
 * The protocol for an object managing registration for a plugin. It provides access to application
 * context, as allowing registering for callbacks for handling various conditions.
 *
 * Currently, the macOS PluginRegistrar has very limited functionality, but is expected to expand
 * over time to more closely match the functionality of FlutterPluginRegistrar.
 */
FLUTTER_DARWIN_EXPORT
@protocol FlutterPluginRegistrar <NSObject>

/**
 * The view displaying Flutter content. May return |nil|, for instance in a headless environment.
 *
 * WARNING: If/when multiple Flutter views within the same application are supported (#30701), this
 * API will change.
 */
@property(nullable, readonly) NSView* view;

/**
 * Registers a `FlutterPlatformViewFactory` for creation of platform views.
 *
 * Plugins expose `NSView` for embedding in Flutter apps by registering a view factory.
 *
 * @param factory The view factory that will be registered.
 * @param factoryId A unique identifier for the factory, the Dart code of the Flutter app can use
 *   this identifier to request creation of a `NSView` by the registered factory.
 */
- (void)registerViewFactory:(nonnull NSObject<FlutterPlatformViewFactory>*)factory
                     withId:(nonnull NSString*)factoryId;

@end

/**
 * A registry of Flutter macOS plugins.
 *
 * Plugins are identified by unique string keys, typically the name of the
 * plugin's main class.
 *
 * Plugins typically need contextual information and the ability to register
 * callbacks for various application events. To keep the API of the registry
 * focused, these facilities are not provided directly by the registry, but by
 * a `FlutterPluginRegistrar`, created by the registry in exchange for the unique
 * key of the plugin.
 *
 * There is no implied connection between the registry and the registrar.
 * Specifically, callbacks registered by the plugin via the registrar may be
 * relayed directly to the underlying iOS application objects.
 */
@protocol FlutterPluginRegistry <NSObject>

/**
 * Returns a registrar for registering a plugin.
 *
 * @param pluginKey The unique key identifying the plugin.
 */
- (nonnull id<FlutterPluginRegistrar>)registrarForPlugin:(nonnull NSString*)pluginKey;

@end
