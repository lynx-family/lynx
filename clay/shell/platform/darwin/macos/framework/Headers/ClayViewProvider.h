// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef FLUTTER_CLAYVIEWPROVIDER_H_
#define FLUTTER_CLAYVIEWPROVIDER_H_

#import <Cocoa/Cocoa.h>
#include <CoreFoundation/CFCGTypes.h>
#include <objc/objc.h>

#import "ClayServiceManager.h"
#import "FlutterEngine.h"
#import "FlutterMacros.h"
#import "FlutterPluginRegistrarMacOS.h"
#import "FlutterView.h"
#import "FlutterViewController.h"

@class FlutterTextInputPlugin;
@class ClayMouseCursorPlugin;

typedef void (*HostLogCallback)(const char* _Nullable log_level, const char* _Nullable file,
                                const int line, const char* _Nullable log_info);

NS_ASSUME_NONNULL_BEGIN

FLUTTER_DARWIN_EXPORT
@interface ClayViewProvider : NSResponder <FlutterPluginRegistry>

/**
 * The Flutter engine associated with this view controller.
 */
@property(nonatomic, nonnull, readonly) FlutterEngine* engine;

// The FlutterView for this view controller.
@property(nonatomic, readonly, nullable) FlutterView* flutterView;

/**
 * The style of mouse tracking to use for the view. Defaults to
 * FlutterMouseTrackingModeInKeyWindow.
 */
@property(nonatomic) FlutterMouseTrackingMode mouseTrackingMode;

@property(nonatomic, readonly, nullable) FlutterTextInputPlugin* textInputPlugin;

@property(nonatomic, strong, nonnull) ClayMouseCursorPlugin* mouseCursorPlugin;

- (instancetype)init;

- (void)viewDidLoad;
- (void)viewWillAppear;
- (void)viewWillDisappear;
- (nullable void*)clayViewContext;
- (void)onEnterForeground;
- (void)onEnterBackground;
- (void)setVisible:(BOOL)visible;
- (void)setFrame:(CGRect)frame;
/**
 * Returns YES if provided event is being currently redispatched by keyboard manager.
 */
- (BOOL)isDispatchingKeyEvent:(nonnull NSEvent*)event;

- (BOOL)viewLoaded;
- (BOOL)isLaunchEngineSuccess;
- (void)setInterceptUrlCallback:(FlutterViewShouldInterceptUrlCallback _Nullable)callback;
- (void)requestIME:(nullable void*)callback arg:(nullable void*)arg;

- (void)performMouseDragLeave;
- (void)performMouseDragEnterAndOverAtPoint:(NSPoint)point;
- (void)performMouseDragDropAtPoint:(NSPoint)point
                               type:(NSString* _Nonnull)type
                        dropContent:(id _Nonnull)content;

- (ClayServiceManager*)GetClayServiceManager;

@end

NS_ASSUME_NONNULL_END

#endif  // FLUTTER_CLAYVIEWPROVIDER_H_
