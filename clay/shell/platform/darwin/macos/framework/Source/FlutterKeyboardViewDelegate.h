// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

#import "clay/public/clay.h"

namespace clay {

// Signature used to notify that a keyboard layout has changed.
typedef void (^KeyboardLayoutNotifier)();

// The printable result of a key under certain modifiers, used to derive key
// mapping.
typedef struct {
  // The printable character.
  //
  // If `isDeadKey` is true, then this is the character when pressing the same
  // dead key twice.
  uint32_t character;

  // Whether this character is a dead key.
  //
  // A dead key is a key that is not counted as text per se, but holds a
  // diacritics to be added to the next key.
  bool isDeadKey;
} LayoutClue;

}  // namespace clay

/**
 * An interface for a class that can provides |FlutterKeyboardManager| with
 * platform-related features.
 *
 * This protocol is typically implemented by |FlutterViewController|.
 */
@protocol FlutterKeyboardViewDelegate

@required
/**
 * Get the next responder to dispatch events that the keyboard system
 * (including text input) do not handle.
 *
 * If the |nextResponder| is null, then those events will be discarded.
 */
@property(nonatomic, readonly, nullable) NSResponder* nextResponder;

/**
 * Dispatch events to the framework to be processed by |HardwareKeyboard|.
 *
 * This method typically forwards events to
 * |FlutterEngine.sendKeyEvent:callback:userData:|.
 */
- (void)sendKeyEvent:(const ClayKeyEvent&)event
            callback:(nullable ClayKeyEventCallback)callback
            userData:(nullable void*)userData;

/**
 * Dispatch events that are not handled by the keyboard event handlers
 * to the text input handler.
 *
 * This method typically forwards events to |TextInputPlugin.handleKeyEvent|.
 */
- (BOOL)onTextInputKeyEvent:(nonnull NSEvent*)event;

/**
 * Add a listener that is called whenever the user changes keyboard layout.
 *
 * Only one listeners is supported. Adding new ones overwrites the current one.
 * Assigning nil unsubscribes.
 */
- (void)subscribeToKeyboardLayoutChange:(nullable clay::KeyboardLayoutNotifier)callback;

/**
 * Querying the printable result of a key under the given modifier state.
 */
- (clay::LayoutClue)lookUpLayoutForKeyCode:(uint16_t)keyCode shift:(BOOL)shift;

@end
