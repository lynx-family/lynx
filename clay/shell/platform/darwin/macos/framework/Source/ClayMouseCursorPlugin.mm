// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <objc/message.h>

#import "clay/shell/platform/darwin/macos/framework/Source/ClayMouseCursorPlugin.h"

#include "clay/ui/platform/cursor_types.h"

using clay::CursorTypes;

static NSString* const kTypeKey = @"type";
static NSString* const kPathKey = @"path";

static NSDictionary* systemCursors;

/**
 * Maps a Flutter's constant to a platform's cursor object.
 *
 * Returns the arrow cursor for unknown constants, including kSystemShapeNone.
 */
static NSCursor* GetCursorByType(CursorTypes type) {
  NSCursor* result = nil;

  switch (type) {
    case CursorTypes::kAlias:
      result = [NSCursor dragLinkCursor];
      break;
    case CursorTypes::kBasic:
      result = [NSCursor arrowCursor];
      break;
    case CursorTypes::kClick:
      result = [NSCursor pointingHandCursor];
      break;
    case CursorTypes::kContextmenu:
      result = [NSCursor contextualMenuCursor];
      break;
    case CursorTypes::kSystemmousecursor:
      result = [NSCursor dragCopyCursor];
      break;
    case CursorTypes::kDisappearing:
      result = [NSCursor disappearingItemCursor];
      break;
    case CursorTypes::kForbidden:
      result = [NSCursor operationNotAllowedCursor];
      break;
    case CursorTypes::kGrab:
      result = [NSCursor openHandCursor];
      break;
    case CursorTypes::kGrabbing:
      result = [NSCursor closedHandCursor];
      break;
    case CursorTypes::kNodrop:
      result = [NSCursor operationNotAllowedCursor];
      break;
    case CursorTypes::kPrecise:
      result = [NSCursor crosshairCursor];
      break;
    case CursorTypes::kText:
      result = [NSCursor IBeamCursor];
      break;
    case CursorTypes::kResizecolumn:
      result = [NSCursor resizeLeftRightCursor];
      break;
    case CursorTypes::kResizedown:
      result = [NSCursor resizeDownCursor];
      break;
    case CursorTypes::kResizeleft:
      result = [NSCursor resizeLeftCursor];
      break;
    case CursorTypes::kResizeleftright:
      result = [NSCursor resizeLeftRightCursor];
      break;
    case CursorTypes::kResizeright:
      result = [NSCursor resizeRightCursor];
      break;
    case CursorTypes::kResizerow:
      result = [NSCursor resizeUpDownCursor];
      break;
    case CursorTypes::kResizeup:
      result = [NSCursor resizeUpCursor];
      break;
    case CursorTypes::kResizeupdown:
      result = [NSCursor resizeUpDownCursor];
      break;
    case CursorTypes::kVerticaltext:
      result = [NSCursor IBeamCursorForVerticalLayout];
      break;
    default:
      break;
  }

  if (result == nil) {
    // TODO(jiangwenlong): support all css cursor if need
    return [NSCursor arrowCursor];
  }

  return result;
}

@interface ClayMouseCursorPlugin ()
/**
 * Whether the cursor is currently hidden.
 */
@property(nonatomic) BOOL hidden;

/**
 * Handles the method call that activates a system cursor.
 *
 */
- (void)activateSystemCursor:(nonnull NSDictionary*)arguments;

/**
 * Displays the specified cursor.
 *
 * Unhides the cursor before displaying the cursor, and updates
 * internal states.
 */
- (void)displayCursorObject:(nonnull NSCursor*)cursorObject;

/**
 * Hides the cursor.
 */
- (void)hide;

@end

@implementation ClayMouseCursorPlugin

#pragma mark - Private

NSMutableDictionary* cachedSystemCursors;

- (instancetype)init {
  self = [super init];
  if (self) {
    cachedSystemCursors = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)dealloc {
  if (_hidden) {
    [NSCursor unhide];
  }
}

- (void)activateSystemCursor:(nonnull NSDictionary*)arguments {
  NSNumber* num = arguments[kTypeKey];
  NSString* path = arguments[kPathKey];
  if (num == NULL) {
    return;
  }
  CursorTypes type = (CursorTypes)[num intValue];

  if (type == clay::CursorTypes::kNone) {
    [self hide];
    return;
  }

  NSCursor* cursorObject = [ClayMouseCursorPlugin cursorFromType:type path:path];
  [self displayCursorObject:cursorObject];
}

- (void)activateSystemCursor:(int)type path:(const char*)path {
  NSDictionary* args =
      @{@"type" : @(type), @"path" : path ? [NSString stringWithUTF8String:path] : @""};
  [self activateSystemCursor:args];
}

- (void)displayCursorObject:(nonnull NSCursor*)cursorObject {
  [cursorObject set];
  if (_hidden) {
    [NSCursor unhide];
  }
  _hidden = NO;
}

- (void)hide {
  if (!_hidden) {
    [NSCursor hide];
  }
  _hidden = YES;
}

+ (NSCursor*)cursorFromType:(CursorTypes)type path:(NSString*)path {
  NSCursor* result = nil;

  switch (type) {
    case CursorTypes::kNet:
    case CursorTypes::kFile:
      // TODO(jiangwenlong) : support network and local file
      result = [NSCursor arrowCursor];
      break;
    default:
      result = GetCursorByType(type);
      break;
  }
  return result;
}

@end
