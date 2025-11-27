// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterEngine.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterView.h"
#import "clay/shell/platform/embedder/embedder_surface_metal.h"

/**
 * Rendering backend agnostic FlutterRendererConfig provider to be used by the embedder API.
 */
@interface FlutterRenderer : NSObject

/**
 * Interface to the system GPU. Used to issue all the rendering commands.
 */
@property(nonatomic, readonly, nonnull) id<MTLDevice> device;

/**
 * Used to get the command buffers for the MTLDevice to render to.
 */
@property(nonatomic, readonly, nonnull) id<MTLCommandQueue> commandQueue;

/**
 * Intializes the renderer with the given FlutterEngine.
 */
- (nullable instancetype)initWithFlutterEngine:(nonnull FlutterEngine*)flutterEngine;

/**
 * Creates a Metal embedder surface.
 */
- (fml::RefPtr<clay::EmbedderSurfaceMetal>)createEmbedderSurfaceMetal;

@end
