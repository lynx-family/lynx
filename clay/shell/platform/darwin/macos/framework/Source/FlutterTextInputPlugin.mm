// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterTextInputPlugin.h"
#include <cstddef>
#include "clay/shell/platform/darwin/macos/framework/Headers/ClayViewProvider.h"
#include "clay/shell/platform/darwin/macos/framework/Headers/FlutterView.h"

#import <Foundation/Foundation.h>
#import <objc/message.h>

#include <algorithm>
#include <memory>

#include "clay/fml/logging.h"
#include "clay/fml/platform/darwin/string_range_sanitization.h"
#include "clay/shell/platform/common/text_editing_delta.h"
#include "clay/shell/platform/common/text_input_model.h"
#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterAppDelegate.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterViewController_Internal.h"

#include "clay/shell/platform/darwin/macos/framework/Headers/FlutterEngine.h"

static NSString* const kMultilineInputType = @"TextInputType.multiline";
static NSString* const kTextAffinityDownstream = @"TextAffinity.downstream";
static NSString* const kTextAffinityUpstream = @"TextAffinity.upstream";
static NSString* const kSelectionBaseKey = @"selectionBase";
static NSString* const kSelectionExtentKey = @"selectionExtent";
static NSString* const kSelectionAffinityKey = @"selectionAffinity";
static NSString* const kSelectionIsDirectionalKey = @"selectionIsDirectional";
static NSString* const kComposingBaseKey = @"composingBase";
static NSString* const kComposingExtentKey = @"composingExtent";
static NSString* const kTextKey = @"text";

/**
 * The affinity of the current cursor position. If the cursor is at a position representing
 * a line break, the cursor may be drawn either at the end of the current line (upstream)
 * or at the beginning of the next (downstream).
 */
typedef NS_ENUM(NSUInteger, FlutterTextAffinity) {
  kFlutterTextAffinityUpstream,
  kFlutterTextAffinityDownstream
};

/*
 * Updates a range given base and extent fields.
 */
static clay::TextRange RangeFromBaseExtent(NSNumber* base, NSNumber* extent,
                                           const clay::TextRange& range) {
  if (base == nil || extent == nil) {
    return range;
  }
  if (base.intValue == -1 && extent.intValue == -1) {
    return clay::TextRange(0, 0);
  }
  return clay::TextRange([base unsignedLongValue], [extent unsignedLongValue]);
}

@interface NSEvent (KeyEquivalentMarker)

// Internally marks that the event was received through performKeyEquivalent:.
// When text editing is active, keyboard events that have modifier keys pressed
// are received through performKeyEquivalent: instead of keyDown:. If such event
// is passed to TextInputContext but doesn't result in a text editing action it
// needs to be forwarded by FlutterKeyboardManager to the next responder.
- (void)markAsKeyEquivalent;

// Returns YES if the event is marked as a key equivalent.
- (BOOL)isKeyEquivalent;

@end

@implementation NSEvent (KeyEquivalentMarker)

// This field doesn't need a value because only its address is used as a unique identifier.
static char markerKey;

- (void)markAsKeyEquivalent {
  objc_setAssociatedObject(self, &markerKey, @true, OBJC_ASSOCIATION_RETAIN);
}

- (BOOL)isKeyEquivalent {
  return [objc_getAssociatedObject(self, &markerKey) boolValue] == YES;
}

@end

/**
 * Private properties of FlutterTextInputPlugin.
 */
@interface FlutterTextInputPlugin ()

/**
 * A text input context, representing a connection to the Cocoa text input system.
 */
@property(nonatomic) NSTextInputContext* textInputContext;

/**
 * The FlutterViewController to manage input for.
 */
@property(nonatomic, weak) FlutterViewController* flutterViewController;

/**
 * Whether the text input is shown in the view.
 *
 * Defaults to TRUE on startup.
 */
@property(nonatomic) BOOL shown;

/**
 * The current state of the keyboard and pressed keys.
 */
@property(nonatomic) uint64_t previouslyPressedFlags;

/**
 * The affinity for the current cursor position.
 */
@property FlutterTextAffinity textAffinity;

/**
 * ID of the text input client.
 */
@property(nonatomic, nonnull) NSNumber* clientID;

/**
 * Keyboard type of the client. See available options:
 * https://api.flutter.dev/flutter/services/TextInputType-class.html
 */
@property(nonatomic, nonnull) NSString* inputType;

/**
 * An action requested by the user on the input client. See available options:
 * https://api.flutter.dev/flutter/services/TextInputAction-class.html
 */
@property(nonatomic, nonnull) NSString* inputAction;

/**
 * Set to true if the last event fed to the input context produced a text editing command
 * or text output. It is reset to false at the beginning of every key event, and is only
 * used while processing this event.
 */
@property(nonatomic) BOOL eventProducedOutput;

/**
 * Whether to enable the sending of text input updates from the engine to the
 * framework as TextEditingDeltas rather than as one TextEditingValue.
 * For more information on the delta model, see:
 * https://master-api.flutter.dev/flutter/services/TextInputConfiguration/enableDeltaModel.html
 */
@property(nonatomic) BOOL enableDeltaModel;

/**
 * Used to gather multiple selectors performed in one run loop turn. These
 * will be all sent in one platform channel call so that the framework can process
 * them in single microtask.
 */
@property(nonatomic) NSMutableArray* pendingSelectors;

/**
 * Updates the text input model with state received from the framework via the
 * TextInput.setEditingState message.
 */
- (void)setEditingState:(NSDictionary*)state;

/**
 * Informs the Flutter framework of changes to the text input model's state by
 * sending the entire new state.
 */
- (void)updateEditState;

/**
 * Informs the Flutter framework of changes to the text input model's state by
 * sending only the difference.
 */
- (void)updateEditStateWithDelta:(const clay::TextEditingDelta)delta;

/**
 * Updates the stringValue and selectedRange that stored in the NSTextView interface
 * that this plugin inherits from.
 *
 * If there is a FlutterTextField uses this plugin as its field editor, this method
 * will update the stringValue and selectedRange through the API of the FlutterTextField.
 */
- (void)updateTextAndSelection;

/**
 * Return the string representation of the current textAffinity.
 */
- (NSString*)textAffinityString;

@property(nonatomic, weak) FlutterEngine* flutterEngine;
@property(nonatomic, weak) ClayViewProvider* clayProvider;

/**
 * Allow overriding run loop mode for test.
 */
@property(readwrite, nonatomic) NSString* customRunLoopMode;

@end

@implementation FlutterTextInputPlugin {
  /**
   * The currently active text input model.
   */
  std::unique_ptr<clay::TextInputModel> _activeModel;

  /**
   * Transform for current the editable. Used to determine position of accent selection menu.
   */
  CATransform3D _editableTransform;

  /**
   * Current position of caret in local (editable) coordinates.
   */
  CGRect _caretRect;
}

- (instancetype)initWithViewController:(FlutterViewController*)viewController {
  // The view needs an empty frame otherwise it is visible on dark background.
  // https://github.com/flutter/flutter/issues/118504
  self = [super initWithFrame:NSZeroRect];
  if (self != nil) {
    _flutterViewController = viewController;
    _flutterEngine = viewController.engine;
    _shown = FALSE;
    // NSTextView does not support _weak reference, so this class has to
    // use __unsafe_unretained and manage the reference by itself.
    //
    // Since the dealloc removes the handler, the pointer should
    // be valid if the handler is ever called.
    __unsafe_unretained FlutterTextInputPlugin* unsafeSelf = self;
    _textInputContext = [[NSTextInputContext alloc] initWithClient:unsafeSelf];
    _previouslyPressedFlags = 0;

    // Initialize with the zero matrix which is not
    // an affine transform.
    _editableTransform = CATransform3D();
    _caretRect = CGRectNull;
  }
  return self;
}

- (BOOL)isFirstResponder {
  if (!self.clayProvider.viewLoaded) {
    return false;
  }
  return [self.clayProvider.flutterView.window firstResponder] == self;
}

- (void)dealloc {
}

- (instancetype)initWithClayProvider:(ClayViewProvider*)clayProvider {
  // The view needs an empty frame otherwise it is visible on dark background.
  // https://github.com/flutter/flutter/issues/118504
  self = [super initWithFrame:NSZeroRect];
  if (self != nil) {
    _clayProvider = clayProvider;
    _flutterEngine = clayProvider.engine;
    _shown = FALSE;
    // NSTextView does not support _weak reference, so this class has to
    // use __unsafe_unretained and manage the reference by itself.
    //
    // Since the dealloc removes the handler, the pointer should
    // be valid if the handler is ever called.
    __unsafe_unretained FlutterTextInputPlugin* unsafeSelf = self;
    _textInputContext = [[NSTextInputContext alloc] initWithClient:unsafeSelf];
    _previouslyPressedFlags = 0;

    // Initialize with the zero matrix which is not
    // an affine transform.
    _editableTransform = CATransform3D();
    _caretRect = CGRectNull;
  }
  return self;
}

#pragma mark - Private

- (void)resignAndRemoveFromSuperview {
  if (self.superview != nil) {
    // With accessiblity enabled TextInputPlugin is inside _client, so take the
    // nextResponder from the _client.
    NSResponder* nextResponder = self.nextResponder;
    [self.window makeFirstResponder:nextResponder];
    [self removeFromSuperview];
  }
}
- (void)showTextInput {
  // Ensure the plugin is in hierarchy. Only do this with accessibility disabled.
  // When accessibility is enabled cocoa will reparent the plugin inside
  // FlutterTextField in [FlutterTextField startEditing].
  if (_flutterEngine && _flutterEngine.view != nil) {
    [_flutterEngine.view addSubview:self];
  }
  [self.window makeFirstResponder:self];
  _shown = TRUE;
}

- (void)hideTextInput {
  [self resignAndRemoveFromSuperview];
  _shown = FALSE;
}

- (NSString*)filterInput:(NSString*)inputString withPattern:(NSString*)pattern {
  if (!inputString || !pattern) {
    return inputString;
  }
  NSError* error = nil;
  NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:pattern
                                                                         options:0
                                                                           error:&error];
  if (error) {
    const char* errorDescription = [error.localizedDescription UTF8String];
    FML_LOG(ERROR) << "FlutterTextInputPlugin error: " << errorDescription;
    return inputString;
  }
  NSString* filteredString =
      [regex stringByReplacingMatchesInString:inputString
                                      options:0
                                        range:NSMakeRange(0, inputString.length)
                                 withTemplate:@""];
  return filteredString;
}

- (void)setTextInputClient:(int)clientId
               inputAction:(NSString*)inputAction
                 inputType:(NSString*)inputType {
  NSNumber* clientID = @(clientId);
  if (clientID != nil) {
    _clientID = clientID;
    _inputAction = inputAction;
    _enableDeltaModel = false;
    _inputType = inputType;
    self.textAffinity = kFlutterTextAffinityUpstream;

    _activeModel = std::make_unique<clay::TextInputModel>();
  }
}

- (void)clearTextInputClient {
  [self resignAndRemoveFromSuperview];
  // If there's an active mark region, commit it, end composing, and clear the IME's mark text.
  if (_activeModel && _activeModel->composing()) {
    _activeModel->CommitComposing();
    _activeModel->EndComposing();
  }
  [_textInputContext discardMarkedText];

  _clientID = nil;
  _inputAction = nil;
  _enableDeltaModel = NO;
  _inputType = nil;
  _activeModel = nullptr;
}

- (void)setEditableTransform:(NSArray*)matrix {
  CATransform3D* transform = &_editableTransform;

  transform->m11 = [matrix[0] doubleValue];
  transform->m12 = [matrix[1] doubleValue];
  transform->m13 = [matrix[2] doubleValue];
  transform->m14 = [matrix[3] doubleValue];

  transform->m21 = [matrix[4] doubleValue];
  transform->m22 = [matrix[5] doubleValue];
  transform->m23 = [matrix[6] doubleValue];
  transform->m24 = [matrix[7] doubleValue];

  transform->m31 = [matrix[8] doubleValue];
  transform->m32 = [matrix[9] doubleValue];
  transform->m33 = [matrix[10] doubleValue];
  transform->m34 = [matrix[11] doubleValue];

  transform->m41 = [matrix[12] doubleValue];
  transform->m42 = [matrix[13] doubleValue];
  transform->m43 = [matrix[14] doubleValue];
  transform->m44 = [matrix[15] doubleValue];
}

- (void)updateCaretRect:(NSDictionary*)dictionary {
  NSAssert(dictionary[@"x"] != nil && dictionary[@"y"] != nil && dictionary[@"width"] != nil &&
               dictionary[@"height"] != nil,
           @"Expected a dictionary representing a CGRect, got %@", dictionary);
  _caretRect = CGRectMake([dictionary[@"x"] doubleValue], [dictionary[@"y"] doubleValue],
                          [dictionary[@"width"] doubleValue], [dictionary[@"height"] doubleValue]);
}

- (void)setEditingState:(NSDictionary*)state {
  NSString* selectionAffinity = state[kSelectionAffinityKey];
  if (selectionAffinity != nil) {
    _textAffinity = [selectionAffinity isEqualToString:kTextAffinityUpstream]
                        ? kFlutterTextAffinityUpstream
                        : kFlutterTextAffinityDownstream;
  }

  NSString* text = state[kTextKey];

  clay::TextRange selected_range = RangeFromBaseExtent(
      state[kSelectionBaseKey], state[kSelectionExtentKey], _activeModel->selection());

  clay::TextRange composing_range = RangeFromBaseExtent(
      state[kComposingBaseKey], state[kComposingExtentKey], _activeModel->composing_range());
  if (composing_range.collapsed()) {
    _activeModel->EndComposing();
  }

  const bool wasComposing = _activeModel->composing();
  _activeModel->SetText([text UTF8String], selected_range, composing_range);
  if (composing_range.collapsed() && wasComposing) {
    [_textInputContext discardMarkedText];
  }

  [self updateTextAndSelection];
}

- (NSDictionary*)editingState {
  if (_activeModel == nullptr) {
    return nil;
  }

  NSString* const textAffinity = [self textAffinityString];

  int composingBase = _activeModel->composing() ? _activeModel->composing_range().base() : -1;
  int composingExtent = _activeModel->composing() ? _activeModel->composing_range().extent() : -1;

  return @{
    kSelectionBaseKey : @(_activeModel->selection().base()),
    kSelectionExtentKey : @(_activeModel->selection().extent()),
    kSelectionAffinityKey : textAffinity,
    kSelectionIsDirectionalKey : @NO,
    kComposingBaseKey : @(composingBase),
    kComposingExtentKey : @(composingExtent),
    kTextKey : [NSString stringWithUTF8String:_activeModel->GetText().c_str()]
  };
}

- (void)updateEditState {
  if (_activeModel == nullptr) {
    return;
  }

  NSDictionary* state = [self editingState];
  NSString* text = state[kTextKey];
  NSNumber* selectionBase = state[kSelectionBaseKey];
  NSNumber* selectionExtent = state[kSelectionExtentKey];
  NSString* selectionAffinity = state[kSelectionAffinityKey];
  NSNumber* composingBase = state[kComposingBaseKey];
  NSNumber* composingExtent = state[kComposingExtentKey];

  [_flutterEngine updateEditState:_clientID.intValue
                    selectionBase:selectionBase.unsignedLongLongValue
                  composingExtent:composingExtent.unsignedLongLongValue
                selectionAffinity:selectionAffinity ? selectionAffinity.UTF8String : ""
                             text:text ? text.UTF8String : ""
                  selectionExtent:selectionExtent.unsignedLongLongValue
                    composingBase:composingBase.unsignedLongLongValue];

  [self updateTextAndSelection];
}

- (void)updateEditStateWithDelta:(const clay::TextEditingDelta)delta {
  [self updateTextAndSelection];
}

- (void)updateTextAndSelection {
  NSAssert(_activeModel != nullptr, @"Flutter text model must not be null.");
  NSString* text = @(_activeModel->GetText().data());
  int start = _activeModel->selection().base();
  int extend = _activeModel->selection().extent();
  NSRange selection = NSMakeRange(MIN(start, extend), ABS(start - extend));
  // There may be a native text field client if VoiceOver is on.
  // In this case, this plugin has to update text and selection through
  // the client in order for VoiceOver to announce the text editing
  // properly.
  self.string = text;
  [self setSelectedRange:selection];
}

- (NSString*)textAffinityString {
  return (self.textAffinity == kFlutterTextAffinityUpstream) ? kTextAffinityUpstream
                                                             : kTextAffinityDownstream;
}

- (BOOL)handleKeyEvent:(NSEvent*)event {
  if (event.type == NSEventTypeKeyUp ||
      (event.type == NSEventTypeFlagsChanged && event.modifierFlags < _previouslyPressedFlags)) {
    return NO;
  }
  _previouslyPressedFlags = event.modifierFlags;
  if (!_shown) {
    return NO;
  }

  _eventProducedOutput = NO;
  BOOL res = [_textInputContext handleEvent:event];
  // NSTextInputContext#handleEvent returns YES if the context handles the event. One of the reasons
  // the event is handled is because it's a key equivalent. But a key equivalent might produce a
  // text command (indicated by calling doCommandBySelector) or might not (for example, Cmd+Q). In
  // the latter case, this command somehow has not been executed yet and Flutter must dispatch it to
  // the next responder. See https://github.com/flutter/flutter/issues/106354 .
  if (event.isKeyEquivalent && !_eventProducedOutput) {
    return NO;
  }
  return res;
}

- (BOOL)isComposing {
  return _activeModel && !_activeModel->composing_range().collapsed();
}

#pragma mark -
#pragma mark NSResponder

- (void)keyDown:(NSEvent*)event {
  [self.clayProvider keyDown:event];
}

- (void)keyUp:(NSEvent*)event {
  [self.clayProvider keyUp:event];
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
  if ([_clayProvider isDispatchingKeyEvent:event]) {
    // When NSWindow is nextResponder, keyboard manager will send to it
    // unhandled events (through [NSWindow keyDown:]). If event has has both
    // control and cmd modifiers set (i.e. cmd+control+space - emoji picker)
    // NSWindow will then send this event as performKeyEquivalent: to first
    // responder, which is FlutterTextInputPlugin. If that's the case, the
    // plugin must not handle the event, otherwise the emoji picker would not
    // work (due to first responder returning YES from performKeyEquivalent:)
    // and there would be endless loop, because FlutterViewController will
    // send the event back to [keyboardManager handleEvent:].
    return NO;
  }
  [event markAsKeyEquivalent];
  [self.clayProvider keyDown:event];
  return YES;
}

- (void)flagsChanged:(NSEvent*)event {
  [self.clayProvider flagsChanged:event];
}

- (void)mouseDown:(NSEvent*)event {
  [self.clayProvider mouseDown:event];
}

- (void)mouseUp:(NSEvent*)event {
  [self.clayProvider mouseUp:event];
}

- (void)mouseDragged:(NSEvent*)event {
  [self.clayProvider mouseDragged:event];
}

- (void)rightMouseDown:(NSEvent*)event {
  [self.clayProvider rightMouseDown:event];
}

- (void)rightMouseUp:(NSEvent*)event {
  [self.clayProvider rightMouseUp:event];
}

- (void)rightMouseDragged:(NSEvent*)event {
  [self.clayProvider rightMouseDragged:event];
}

- (void)otherMouseDown:(NSEvent*)event {
  [self.clayProvider otherMouseDown:event];
}

- (void)otherMouseUp:(NSEvent*)event {
  [self.clayProvider otherMouseUp:event];
}

- (void)otherMouseDragged:(NSEvent*)event {
  [self.clayProvider otherMouseDragged:event];
}

- (void)mouseMoved:(NSEvent*)event {
  [self.clayProvider mouseMoved:event];
}

- (void)scrollWheel:(NSEvent*)event {
  [self.clayProvider scrollWheel:event];
}

- (NSTextInputContext*)inputContext {
  return _textInputContext;
}

#pragma mark -
#pragma mark NSTextInputClient

- (void)insertTab:(id)sender {
  // Implementing insertTab: makes AppKit send tab as command, instead of
  // insertText with '\t'.
}

- (void)insertText:(id)string replacementRange:(NSRange)range {
  if (_activeModel == nullptr) {
    return;
  }

  _eventProducedOutput |= true;

  if (range.location != NSNotFound) {
    // The selected range can actually have negative numbers, since it can start
    // at the end of the range if the user selected the text going backwards.
    // Cast to a signed type to determine whether or not the selection is reversed.
    long signedLength = static_cast<long>(range.length);
    long location = range.location;
    long textLength = _activeModel->text_range().end();

    size_t base = std::clamp(location, 0L, textLength);
    size_t extent = std::clamp(location + signedLength, 0L, textLength);

    _activeModel->SetSelection(clay::TextRange(base, extent));
  }

  clay::TextRange oldSelection = _activeModel->selection();
  clay::TextRange composingBeforeChange = _activeModel->composing_range();
  clay::TextRange replacedRange(-1, -1);

  std::string textBeforeChange = _activeModel->GetText().c_str();
  std::string utf8String = [string UTF8String];
  _activeModel->AddText(utf8String);
  if (_activeModel->composing()) {
    replacedRange = composingBeforeChange;
    _activeModel->CommitComposing();
    _activeModel->EndComposing();
  } else {
    replacedRange = range.location == NSNotFound
                        ? clay::TextRange(oldSelection.base(), oldSelection.extent())
                        : clay::TextRange(range.location, range.location + range.length);
  }
  if (_enableDeltaModel) {
    [self updateEditStateWithDelta:clay::TextEditingDelta(textBeforeChange, replacedRange,
                                                          utf8String)];
  } else {
    [self updateEditState];
  }
}

- (void)doCommandBySelector:(SEL)selector {
  _eventProducedOutput |= selector != NSSelectorFromString(@"noop:");
  if ([self respondsToSelector:selector]) {
    // Note: The more obvious [self performSelector...] doesn't give ARC enough information to
    // handle retain semantics properly. See https://stackoverflow.com/questions/7017281/ for more
    // information.
    IMP imp = [self methodForSelector:selector];
    void (*func)(id, SEL, id) = reinterpret_cast<void (*)(id, SEL, id)>(imp);
    func(self, selector, nil);
  }

  if (selector == @selector(insertNewline:)) {
    // Already handled through text insertion (multiline) or action.
    return;
  }

  // Group multiple selectors received within a single run loop turn so that
  // the framework can process them in single microtask.
  NSString* name = NSStringFromSelector(selector);
  if (_pendingSelectors == nil) {
    _pendingSelectors = [NSMutableArray array];
  }
  [_pendingSelectors addObject:name];

  if (_pendingSelectors.count == 1) {
    __weak NSMutableArray* selectors = _pendingSelectors;

    CFStringRef runLoopMode = self.customRunLoopMode != nil
                                  ? (__bridge CFStringRef)self.customRunLoopMode
                                  : kCFRunLoopCommonModes;

    CFRunLoopPerformBlock(CFRunLoopGetMain(), runLoopMode, ^{
      if (selectors.count > 0) {
        [selectors removeAllObjects];
      }
    });
  }
}

- (void)insertNewline:(id)sender {
  if (_activeModel == nullptr) {
    return;
  }
  if (_activeModel->composing()) {
    _activeModel->CommitComposing();
    _activeModel->EndComposing();
  }
  if ([self.inputType isEqualToString:kMultilineInputType]) {
    [self insertText:@"\n" replacementRange:self.selectedRange];
  }
  [_flutterEngine performInputAction:(self.clientID.intValue)];
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
  if (_activeModel == nullptr) {
    return;
  }
  std::string textBeforeChange = _activeModel->GetText().c_str();
  if (!_activeModel->composing()) {
    _activeModel->BeginComposing();
  }

  if (replacementRange.location != NSNotFound) {
    // According to the NSTextInputClient documentation replacementRange is
    // computed from the beginning of the marked text. That doesn't seem to be
    // the case, because in situations where the replacementRange is actually
    // specified (i.e. when switching between characters equivalent after long
    // key press) the replacementRange is provided while there is no composition.
    _activeModel->SetComposingRange(
        clay::TextRange(replacementRange.location,
                        replacementRange.location + replacementRange.length),
        0);
  }

  clay::TextRange composingBeforeChange = _activeModel->composing_range();
  clay::TextRange selectionBeforeChange = _activeModel->selection();

  // Input string may be NSString or NSAttributedString.
  BOOL isAttributedString = [string isKindOfClass:[NSAttributedString class]];
  std::string marked_text = isAttributedString ? [[string string] UTF8String] : [string UTF8String];
  _activeModel->UpdateComposingText(marked_text);

  // Update the selection within the marked text.
  long signedLength = static_cast<long>(selectedRange.length);
  long location = selectedRange.location + _activeModel->composing_range().base();
  long textLength = _activeModel->text_range().end();

  size_t base = std::clamp(location, 0L, textLength);
  size_t extent = std::clamp(location + signedLength, 0L, textLength);
  _activeModel->SetSelection(clay::TextRange(base, extent));

  if (_enableDeltaModel) {
    [self updateEditStateWithDelta:clay::TextEditingDelta(textBeforeChange,
                                                          selectionBeforeChange.collapsed()
                                                              ? composingBeforeChange
                                                              : selectionBeforeChange,
                                                          marked_text)];
  } else {
    [self updateEditState];
  }
}

- (void)unmarkText {
  if (_activeModel == nullptr) {
    return;
  }
  _activeModel->CommitComposing();
  _activeModel->EndComposing();
  if (_enableDeltaModel) {
    [self updateEditStateWithDelta:clay::TextEditingDelta(_activeModel->GetText().c_str())];
  } else {
    [self updateEditState];
  }
}

- (NSRange)markedRange {
  if (_activeModel == nullptr) {
    return NSMakeRange(NSNotFound, 0);
  }
  return NSMakeRange(
      _activeModel->composing_range().base(),
      _activeModel->composing_range().extent() - _activeModel->composing_range().base());
}

- (BOOL)hasMarkedText {
  return _activeModel != nullptr && _activeModel->composing_range().length() > 0;
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
  if (_activeModel == nullptr) {
    return nil;
  }
  if (actualRange != nil) {
    *actualRange = range;
  }
  NSString* text = [NSString stringWithUTF8String:_activeModel->GetText().c_str()];
  NSString* substring = [text substringWithRange:range];
  return [[NSAttributedString alloc] initWithString:substring attributes:nil];
}

- (NSArray<NSString*>*)validAttributesForMarkedText {
  return @[];
}

// Returns the bounding CGRect of the transformed incomingRect, in screen
// coordinates.
- (CGRect)screenRectFromFrameworkTransform:(CGRect)incomingRect {
  CGPoint points[] = {
      incomingRect.origin,
      CGPointMake(incomingRect.origin.x, incomingRect.origin.y + incomingRect.size.height),
      CGPointMake(incomingRect.origin.x + incomingRect.size.width, incomingRect.origin.y),
      CGPointMake(incomingRect.origin.x + incomingRect.size.width,
                  incomingRect.origin.y + incomingRect.size.height)};

  CGPoint origin = CGPointMake(CGFLOAT_MAX, CGFLOAT_MAX);
  CGPoint farthest = CGPointMake(-CGFLOAT_MAX, -CGFLOAT_MAX);

  for (int i = 0; i < 4; i++) {
    const CGPoint point = points[i];

    CGFloat x = _editableTransform.m11 * point.x + _editableTransform.m21 * point.y +
                _editableTransform.m41;
    CGFloat y = _editableTransform.m12 * point.x + _editableTransform.m22 * point.y +
                _editableTransform.m42;

    const CGFloat w = _editableTransform.m14 * point.x + _editableTransform.m24 * point.y +
                      _editableTransform.m44;

    if (w == 0.0) {
      return CGRectZero;
    } else if (w != 1.0) {
      x /= w;
      y /= w;
    }

    origin.x = MIN(origin.x, x);
    origin.y = MIN(origin.y, y);
    farthest.x = MAX(farthest.x, x);
    farthest.y = MAX(farthest.y, y);
  }
  const NSView* fromView = _clayProvider.flutterView;
  const CGRect rectInWindow = [fromView
      convertRect:CGRectMake(origin.x, origin.y, farthest.x - origin.x, farthest.y - origin.y)
           toView:nil];
  NSWindow* window = fromView.window;
  return window ? [window convertRectToScreen:rectInWindow] : rectInWindow;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
  // This only determines position of caret instead of any arbitrary range, but it's enough
  // to properly position accent selection popup
  // if (!self.clayProvider.viewLoaded || CGRectEqualToRect(_caretRect, CGRectNull)) {
  //   return CGRectZero;
  // }
  // CGRect rect = [self screenRectFromFrameworkTransform:_caretRect];
  // double viewHeight = self.clayProvider.flutterView.frame.size.height;
  // rect.origin.y = viewHeight - rect.origin.y - rect.size.height;
  // return [self.clayProvider.flutterView.window convertRectToScreen:rect];
  return [self screenRectFromFrameworkTransform:_caretRect];
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
  // TODO(cbracken): Implement.
  // Note: This function can't easily be implemented under the system-message architecture.
  return 0;
}

// Override method to avoid drawing cursor
- (void)drawInsertionPointInRect:(NSRect)rect color:(NSColor*)color turnedOn:(BOOL)flag {
}

// This method needs to be overridden to ensure that the cursor is not redrawn when the text
// changes
- (void)setNeedsDisplayInRect:(NSRect)invalidRect {
}

// Set isEditable to NO to disable editing and cursor drawing.
- (BOOL)isEditable {
  return NO;
}

// Setting isSelectable to NO disables selection and cursor display.
- (BOOL)isSelectable {
  return NO;
}

@end
