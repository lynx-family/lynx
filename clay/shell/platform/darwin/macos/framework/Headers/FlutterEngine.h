// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef FLUTTER_FLUTTERENGINE_H_
#define FLUTTER_FLUTTERENGINE_H_

#import <Foundation/Foundation.h>

#import "FlutterMacros.h"
#import "FlutterPluginRegistrarMacOS.h"

typedef NSString* _Nullable (^FlutterViewShouldInterceptUrlCallback)(NSString* _Nonnull originUrl,
                                                                     BOOL shouldDecode,
                                                                     NSInteger maxPathLength);

// TODO: Merge this file with the iOS FlutterEngine.h.

@class FlutterView;
@class FlutterViewController;
@class ClayViewProvider;

extern NSString* _Nonnull const kDragTextType;
extern NSString* _Nonnull const kDragFileType;
extern NSString* _Nonnull const kDragDropPathKey;
extern NSString* _Nonnull const kDragDropNameKey;
extern NSString* _Nonnull const kDragDropTypeKey;
extern NSString* _Nonnull const kDragDropSizeKey;
extern NSString* _Nonnull const kDragDropLastModifiedKey;

/**
 * Coordinates a single instance of execution of a Flutter engine.
 */
FLUTTER_DARWIN_EXPORT
@interface FlutterEngine : NSObject <FlutterPluginRegistry>

/**
 * Initializes an engine with the given project.
 *
 * @param labelPrefix Currently unused; in the future, may be used for labelling threads
 *                    as with the iOS FlutterEngine.
 */
- (nonnull instancetype)initWithName:(nonnull NSString*)labelPrefix;

/**
 * Initializes an engine that can run headlessly with the given project.
 *
 * @param labelPrefix Currently unused; in the future, may be used for labelling threads
 *                    as with the iOS FlutterEngine.
 */
- (nonnull instancetype)initWithName:(nonnull NSString*)labelPrefix
              allowHeadlessExecution:(BOOL)allowHeadlessExecution NS_DESIGNATED_INITIALIZER;

- (nonnull instancetype)init NS_UNAVAILABLE;

/**
 * Runs a Dart program on an Isolate from the main Dart library (i.e. the library that
 * contains `main()`).
 *
 * The first call to this method will create a new Isolate. Subsequent calls will return
 * immediately.
 *
 * @param entrypoint The name of a top-level function from the same Dart
 *   library that contains the app's main() function.  If this is nil, it will
 *   default to `main()`.  If it is not the app's main() function, that function
 *   must be decorated with `@pragma(vm:entry-point)` to ensure the method is not
 *   tree-shaken by the Dart compiler.
 * @return YES if the call succeeds in creating and running a Flutter Engine instance; NO otherwise.
 */
- (BOOL)runWithEntrypoint:(nullable NSString*)entrypoint;

/**
 * The default `FlutterViewController` associated with this engine, if any.
 *
 * The default view always has ID kFlutterDefaultViewId, and is the view
 * operated by the APIs that do not have a view ID specified.
 */
@property(nonatomic, nullable, weak) FlutterViewController* viewController;

/**
 * The `FlutterView` associated with this engine, if any.
 */
@property(nonatomic, nullable, weak) FlutterView* view;

/**
 * Shuts the Flutter engine if it is running. The FlutterEngine instance must always be shutdown
 * before it may be collected. Not shutting down the FlutterEngine instance before releasing it will
 * result in the leak of that engine instance.
 */
- (void)shutDownEngine;

- (nullable void*)clayViewContext;

- (void)onEnterForeground;
- (void)onEnterBackground;
- (void)setVisible:(BOOL)visible;

/**
 * set intercept image url callback
 */
- (void)setInterceptUrlCallback:(FlutterViewShouldInterceptUrlCallback _Nullable)callback;

- (void)updateEditState:(int)client_id
          selectionBase:(uint64_t)selection_base
        composingExtent:(uint64_t)composing_extent
      selectionAffinity:(const char* _Nullable)selection_affinity
                   text:(const char* _Nullable)text
        selectionExtent:(uint64_t)selection_extent
          composingBase:(uint64_t)composing_base;

- (void)performInputAction:(int)client_id;

- (void)performMouseDragLeave;
- (void)performMouseDragEnterAndOverAtPoint:(NSPoint)point;
- (void)performMouseDragDropAtPoint:(NSPoint)point
                               type:(NSString* _Nonnull)type
                        dropContent:(id _Nonnull)content;

@end

#endif  // FLUTTER_FLUTTERENGINE_H_
