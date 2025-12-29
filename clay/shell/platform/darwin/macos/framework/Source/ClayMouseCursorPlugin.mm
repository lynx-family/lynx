// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <objc/message.h>

#import "clay/shell/platform/darwin/macos/framework/Source/ClayMouseCursorPlugin.h"

#include "clay/ui/platform/cursor_types.h"

using clay::CursorTypes;

static NSString* const kTypeKey = @"type";
static NSString* const kPathKey = @"path";

static NSDictionary* systemCursors;

// code from chromium/src/ui/base/cocoa/cursor_utils.mm
// Private interface to CoreCursor. See
// https://github.com/WebKit/WebKit/blob/main/Source/WebCore/PAL/pal/spi/mac/HIServicesSPI.h
//
// Note that the column/row resize cursors have a bar in the middle (e.g. <-|->)
// whereas the frame resize cursors have no bar in the middle (e.g. <-->).
enum class CrCoreCursorType : int32_t {
  kArrow = 0,                           // NSCursor.arrowCursor
  kIBeam = 1,                           // NSCursor.IBeamCursor
  kMakeAlias = 2,                       // NSCursor.dragLinkCursor
  kOperationNotAllowed = 3,             // NSCursor.operationNotAllowedCursor
  kBusyButClickable = 4,                // NSCursor.busyButClickableCursor (private)
  kCopy = 5,                            // NSCursor.dragCopyCursor
  kScreenShotSelection = 7,             // -
  kScreenShotSelectionToClip = 8,       // -
  kScreenShotWindow = 9,                // -
  kScreenShotWindowToClip = 10,         // -
  kClosedHand = 11,                     // NSCursor.closedHandCursor
  kOpenHand = 12,                       // NSCursor.openHandCursor
  kPointingHand = 13,                   // NSCursor.pointingHandCursor
  kCountingUpHand = 14,                 // -
  kCountingDownHand = 15,               // -
  kCountingUpAndDownHand = 16,          // -
  kColumnResizeLeft = 17,               // [NSCursor columnResizeCursorInDirections:]
  kColumnResizeRight = 18,              // [NSCursor columnResizeCursorInDirections:]
  kColumnResizeLeftRight = 19,          // NSCursor.columnResizeCursor
  kCrosshair = 20,                      // NSCursor.crosshairCursor
  kRowResizeUp = 21,                    // [NSCursor rowResizeCursorInDirections:]
  kRowResizeDown = 22,                  // [NSCursor rowResizeCursorInDirections:]
  kRowResizeUpDown = 23,                // NSCursor.rowResizeCursor
  kContextualMenu = 24,                 // NSCursor.contextualMenuCursor
  kDisappearingItem = 25,               // NSCursor.disappearingItemCursor
  kVerticalIBeam = 26,                  // NSCursor.IBeamCursorForVerticalLayout
  kFrameResizeEast = 27,                // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeEastWest = 28,            // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeNortheast = 29,           // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeNortheastSouthwest = 30,  // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeNorth = 31,               // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeNorthSouth = 32,          // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeNorthwest = 33,           // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeNorthwestSoutheast = 34,  // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeSoutheast = 35,           // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeSouth = 36,               // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeSouthwest = 37,           // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kFrameResizeWest = 38,                // [NSCursor frameResizeCursorFromPosition:inDirections:]
  kMove = 39,                           // oddly, not NSCursor._moveCursor (private)
  kHelp = 40,                           // NSCursor._helpCursor (private)
  kCell = 41,                           // -
  kZoomIn = 42,                         // NSCursor.zoomInCursor
  kZoomOut = 43,                        // NSCursor.zoomOutCursor
};

@interface CrCoreCursor : NSCursor

+ (id)cursorWithType:(CrCoreCursorType)type;
@property(readonly, nonatomic) CrCoreCursorType _coreCursorType;

@end

@implementation CrCoreCursor

@synthesize _coreCursorType = _type;

+ (id)cursorWithType:(CrCoreCursorType)type {
  return [[CrCoreCursor alloc] initWithType:type];
}

- (id)initWithType:(CrCoreCursorType)type {
  if ((self = [super init])) {
    _type = type;
  }
  return self;
}

@end

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
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
      if (@available(macOS 15.0, *))
        result = [NSCursor columnResizeCursor];
      else
#endif
        result = [NSCursor resizeLeftRightCursor];
      break;
    case CursorTypes::kResizedown:
      result = [NSCursor resizeDownCursor];
      break;
    case CursorTypes::kResizeleft:
      result = [NSCursor resizeLeftCursor];
      break;
    case CursorTypes::kResizeleftright:
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
      if (@available(macOS 15.0, *)) {
        result = [NSCursor frameResizeCursorFromPosition:NSCursorFrameResizePositionLeft
                                            inDirections:NSCursorFrameResizeDirectionsAll];
      } else
#endif
        result = [CrCoreCursor cursorWithType:CrCoreCursorType::kFrameResizeEastWest];
      break;
    case CursorTypes::kResizeright:
      result = [NSCursor resizeRightCursor];
      break;
    case CursorTypes::kResizerow:
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
      if (@available(macOS 15.0, *))
        result = NSCursor.rowResizeCursor;
      else
#endif
        result = NSCursor.resizeUpDownCursor;
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
