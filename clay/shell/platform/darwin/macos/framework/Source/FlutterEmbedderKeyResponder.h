// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

#include "clay/public/clay.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterKeyPrimaryResponder.h"

typedef void* _VoidPtr;

typedef void (^FlutterSendEmbedderKeyEvent)(const ClayKeyEvent& /* event */,
                                            _Nullable ClayKeyEventCallback /* callback */,
                                            _Nullable _VoidPtr /* user_data */);

/**
 * A primary responder of |FlutterKeyboardManager| that handles events by
 * sending the converted events through the embedder API.
 *
 * This class communicates with the HardwareKeyboard API in the framework.
 */
@interface FlutterEmbedderKeyResponder : NSObject <FlutterKeyPrimaryResponder>

/**
 * Create an instance by specifying the function to send converted events to.
 *
 * The |sendEvent| is typically |FlutterEngine|'s |sendKeyEvent|.
 */
- (nonnull instancetype)initWithSendEvent:(_Nonnull FlutterSendEmbedderKeyEvent)sendEvent;

@end
