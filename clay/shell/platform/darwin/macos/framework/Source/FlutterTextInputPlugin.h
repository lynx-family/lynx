// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

#import "clay/shell/platform/darwin/macos/framework/Headers/ClayViewProvider.h"
#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterEngine.h"
#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterViewController.h"

/**
 * A plugin to handle text input.
 *
 * Responsible for bridging the native macOS text input system with the Flutter framework text
 * editing classes.
 *
 * This is not an FlutterPlugin since it needs access to FlutterViewController internals, so needs
 * to be managed differently.
 *
 * When accessibility is on, accessibility bridge creates a NSTextField, i.e. FlutterTextField,
 * for every text field in the Flutter. This plugin acts as a field editor for those NSTextField[s].
 */
@interface FlutterTextInputPlugin : NSTextView

/**
 * Initializes a text input plugin that coordinates key event handling with |viewController|.
 */
- (instancetype)initWithViewController:(FlutterViewController *)viewController;

/**
 * Whether this plugin is the first responder of this NSWindow.
 *
 * When accessibility is on, this plugin is set as the first responder to act as the field
 * editor for FlutterTextFields.
 *
 * Returns false if accessibility is off.
 */
- (BOOL)isFirstResponder;

/**
 * Handles key down events received from the view controller, responding YES if
 * the event was handled.
 *
 * Note, the Apple docs suggest that clients should override essentially all the
 * mouse and keyboard event-handling methods of NSResponder. However, experimentation
 * indicates that only key events are processed by the native layer; Flutter processes
 * mouse events. Additionally, processing both keyUp and keyDown results in duplicate
 * processing of the same keys.
 */
- (BOOL)handleKeyEvent:(NSEvent *)event;

/**
 * Whether this plugin has composing text.
 *
 * This is only true when the text input plugin is actively taking user input with composing text.
 */
// TODO (LongCatIsLooong): remove this method and implement a long-term fix for
// https://github.com/flutter/flutter/issues/85328.
- (BOOL)isComposing;

- (instancetype)initWithClayProvider:(ClayViewProvider *)clayProvider;

/**
 * Sets the text input client for the text input plugin.
 */
- (void)setTextInputClient:(int)clientId
               inputAction:(NSString *)inputAction
                 inputType:(NSString *)inputType;

/**
 * Clears the text input client for the text input plugin.
 */
- (void)clearTextInputClient;

/**
 * Sets the transform for the editable text area.
 */
- (void)setEditableTransform:(NSArray *)matrix;
/**
 * Sets the editing state for the text input plugin.
 */
- (void)setEditingState:(NSDictionary *)state;
/**
 * Updates the caret rect for the text input plugin.
 */
- (void)updateCaretRect:(NSDictionary *)dictionary;

- (void)showTextInput;
- (void)hideTextInput;
- (NSString *)filterInput:(NSString *)inputString withPattern:(NSString *)pattern;

@end

// Private methods made visible for testing
@interface FlutterTextInputPlugin (TestMethods)
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange;
- (NSDictionary *)editingState;
@property(readwrite, nonatomic) NSString *customRunLoopMode;
@end
