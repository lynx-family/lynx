// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_CURSOR_TYPES_H_
#define CLAY_UI_PLATFORM_CURSOR_TYPES_H_

#include <map>
#include <string>

namespace clay {
enum class CursorTypes {
  kUnknow = 1,

  // cursor is a image which is downloaded from the internet.
  kNet,

  // cursor is a image which is obtained from the local file.
  kFile,

  ///  Determines the pointer style based on the current content
  ///  e.g. use the text style when the context is text
  kAuto,

  /// Hide the cursor.
  ///
  /// Any cursor other than [none] or [MouseCursor.uncontrolled] unhides the
  /// cursor.
  kNone,

  // STATUS

  /// The platform-dependent basic cursor.
  ///
  /// Typically the shape of an arrow.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_DEFAULT, TYPE_ARROW
  ///  * Web: default
  ///  * Windows: IDC_ARROW
  ///  * Linux: default
  ///  * macOS: arrowCursor
  kBasic,

  /// A cursor that emphasizes an element being clickable, such as a
  /// hyperlink.
  ///
  /// Typically the shape of a pointing hand.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_HAND
  ///  * Web: pointer
  ///  * Windows: IDC_HAND
  ///  * Linux: pointer
  ///  * macOS: pointingHandCursor
  kClick,

  /// A cursor indicating an operation that will not be carried out.
  ///
  /// Typically the shape of a circle with a diagonal line. May fall back to
  /// [noDrop].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_NO_DROP
  ///  * Web: not-allowed
  ///  * Windows: IDC_NO
  ///  * Linux: not-allowed
  ///  * macOS: operationNotAllowedCursor
  ///
  /// See also:
  ///
  ///  * [noDrop], which indicates somewhere that the current item may not be
  ///    dropped.
  kForbidden,

  /// A cursor indicating the status that the program is busy and therefore
  /// can not be interacted with.
  ///
  /// Typically the shape of an hourglass or a watch.
  ///
  /// This cursor is not available as a system cursor on macOS. Although macOS
  /// displays a "spinning ball" cursor when busy, it's handled by the OS and
  /// not exposed for applications to choose.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_WAIT
  ///  * Windows: IDC_WAIT
  ///  * Web: wait
  ///  * Linux: wait
  ///
  /// See also:
  ///
  ///  * [progress], which is similar to [wait] but the program can still be
  ///    interacted with.
  kWait,

  /// A cursor indicating the status that the program is busy but can still be
  /// interacted with.
  ///
  /// Typically the shape of an arrow with an hourglass or a watch at the
  /// corner. Does *not* fall back to [wait] if unavailable.
  ///
  /// Corresponds to:
  ///
  ///  * Web: progress
  ///  * Windows: IDC_APPSTARTING
  ///  * Linux: progress
  ///
  /// See also:
  ///
  ///  * [wait], which is similar to [progress] but the program can not be
  ///    interacted with.
  kProgress,

  /// A cursor indicating somewhere the user can trigger a context menu.
  ///
  /// Typically the shape of an arrow with a small menu at the corner.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_CONTEXT_MENU
  ///  * Web: context-menu
  ///  * Linux: context-menu
  ///  * macOS: contextualMenuCursor
  kContextmenu,

  /// A cursor indicating help information.
  ///
  /// Typically the shape of a question mark, or an arrow therewith.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_HELP
  ///  * Windows: IDC_HELP
  ///  * Web: help
  ///  * Linux: help
  kHelp,

  // SELECTION

  /// A cursor indicating selectable text.
  ///
  /// Typically the shape of a capital I.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_TEXT
  ///  * Web: text
  ///  * Windows: IDC_IBEAM
  ///  * Linux: text
  ///  * macOS: IBeamCursor
  kText,

  /// A cursor indicating selectable vertical text.
  ///
  /// Typically the shape of a capital I rotated to be horizontal. May fall
  /// back to [text].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_VERTICAL_TEXT
  ///  * Web: vertical-text
  ///  * Linux: vertical-text
  ///  * macOS: IBeamCursorForVerticalLayout
  kVerticaltext,

  /// A cursor indicating selectable table cells.
  ///
  /// Typically the shape of a hollow plus sign.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_CELL
  ///  * Web: cell
  ///  * Linux: cell
  kCell,

  /// A cursor indicating precise selection, such as selecting a pixel in a
  /// bitmap.
  ///
  /// Typically the shape of a crosshair.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_CROSSHAIR
  ///  * Web: crosshair
  ///  * Windows: IDC_CROSS
  ///  * Linux: crosshair
  ///  * macOS: crosshairCursor
  kPrecise,

  // DRAG-AND-DROP

  /// A cursor indicating moving something.
  ///
  /// Typically the shape of four-way arrow. May fall back to [allScroll].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_ALL_SCROLL
  ///  * Windows: IDC_SIZEALL
  ///  * Web: move
  ///  * Linux: move
  kMove,

  /// A cursor indicating something that can be dragged.
  ///
  /// Typically the shape of an open hand.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_GRAB
  ///  * Web: grab
  ///  * Linux: grab
  ///  * macOS: openHandCursor
  kGrab,

  /// A cursor indicating something that is being dragged.
  ///
  /// Typically the shape of a closed hand.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_GRABBING
  ///  * Web: grabbing
  ///  * Linux: grabbing
  ///  * macOS: closedHandCursor
  kGrabbing,

  /// A cursor indicating somewhere that the current item may not be dropped.
  ///
  /// Typically the shape of a hand with a [forbidden] sign at the corner. May
  /// fall back to [forbidden].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_NO_DROP
  ///  * Web: no-drop
  ///  * Windows: IDC_NO
  ///  * Linux: no-drop
  ///  * macOS: operationNotAllowedCursor
  ///
  /// See also:
  ///
  ///  * [forbidden], which indicates an action that will not be carried out.
  kNodrop,

  /// A cursor indicating that the current operation will create an alias of,
  /// or a shortcut of the item.
  ///
  /// Typically the shape of an arrow with a shortcut icon at the corner.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_ALIAS
  ///  * Web: alias
  ///  * Linux: alias
  ///  * macOS: dragLinkCursor
  kAlias,

  /// A cursor indicating that the current operation will copy the item.
  ///
  /// Typically the shape of an arrow with a boxed plus sign at the corner.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_COPY
  ///  * Web: copy
  ///  * Linux: copy
  ///  * macOS: dragCopyCursor
  kSystemmousecursor,

  /// A cursor indicating that the current operation will result in the
  /// disappearance of the item.
  ///
  /// Typically the shape of an arrow with a cloud of smoke at the corner.
  ///
  /// Corresponds to:
  ///
  ///  * macOS: disappearingItemCursor
  kDisappearing,

  // RESIZING AND SCROLLING

  /// A cursor indicating scrolling in any direction.
  ///
  /// Typically the shape of a dot surrounded by 4 arrows.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_ALL_SCROLL
  ///  * Windows: IDC_SIZEALL
  ///  * Web: all-scroll
  ///  * Linux: all-scroll
  ///
  /// See also:
  ///
  ///  * [move], which indicates moving in any direction.
  kAllscroll,

  /// A cursor indicating resizing an object bidirectionally from its left or
  /// right edge.
  ///
  /// Typically the shape of a bidirectional arrow pointing left and right.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_HORIZONTAL_DOUBLE_ARROW
  ///  * Web: ew-resize
  ///  * Windows: IDC_SIZEWE
  ///  * Linux: ew-resize
  ///  * macOS: resizeLeftRightCursor
  kResizeleftright,

  /// A cursor indicating resizing an object bidirectionally from its top or
  /// bottom edge.
  ///
  /// Typically the shape of a bidirectional arrow pointing up and down.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_VERTICAL_DOUBLE_ARROW
  ///  * Web: ns-resize
  ///  * Windows: IDC_SIZENS
  ///  * Linux: ns-resize
  ///  * macOS: resizeUpDownCursor
  kResizeupdown,

  /// A cursor indicating resizing an object bidirectionally from its top left
  /// or bottom right corner.
  ///
  /// Typically the shape of a bidirectional arrow pointing upper left and
  /// lower right.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_TOP_LEFT_DIAGONAL_DOUBLE_ARROW
  ///  * Web: nwse-resize
  ///  * Windows: IDC_SIZENWSE
  ///  * Linux: nwse-resize
  kResizeupleftdownright,

  /// A cursor indicating resizing an object bidirectionally from its top
  /// right or bottom left corner.
  ///
  /// Typically the shape of a bidirectional arrow pointing upper right and
  /// lower left.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_TOP_RIGHT_DIAGONAL_DOUBLE_ARROW
  ///  * Windows: IDC_SIZENESW
  ///  * Web: nesw-resize
  ///  * Linux: nesw-resize
  kResizeuprightdownleft,

  /// A cursor indicating resizing an object from its top edge.
  ///
  /// Typically the shape of an arrow pointing up. May fallback to
  /// [resizeUpDown].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_VERTICAL_DOUBLE_ARROW
  ///  * Web: n-resize
  ///  * Windows: IDC_SIZENS
  ///  * Linux: n-resize
  ///  * macOS: resizeUpCursor
  kResizeup,

  /// A cursor indicating resizing an object from its bottom edge.
  ///
  /// Typically the shape of an arrow pointing down. May fallback to
  /// [resizeUpDown].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_VERTICAL_DOUBLE_ARROW
  ///  * Web: s-resize
  ///  * Windows: IDC_SIZENS
  ///  * Linux: s-resize
  ///  * macOS: resizeDownCursor
  kResizedown,

  /// A cursor indicating resizing an object from its left edge.
  ///
  /// Typically the shape of an arrow pointing left. May fallback to
  /// [resizeLeftRight].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_HORIZONTAL_DOUBLE_ARROW
  ///  * Web: w-resize
  ///  * Windows: IDC_SIZEWE
  ///  * Linux: w-resize
  ///  * macOS: resizeLeftCursor
  kResizeleft,

  /// A cursor indicating resizing an object from its right edge.
  ///
  /// Typically the shape of an arrow pointing right. May fallback to
  /// [resizeLeftRight].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_HORIZONTAL_DOUBLE_ARROW
  ///  * Web: e-resize
  ///  * Windows: IDC_SIZEWE
  ///  * Linux: e-resize
  ///  * macOS: resizeRightCursor
  kResizeright,

  /// A cursor indicating resizing an object from its top-left corner.
  ///
  /// Typically the shape of an arrow pointing upper left. May fallback to
  /// [resizeUpLeftDownRight].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_TOP_LEFT_DIAGONAL_DOUBLE_ARROW
  ///  * Web: nw-resize
  ///  * Windows: IDC_SIZENWSE
  ///  * Linux: nw-resize
  kResizeupleft,

  /// A cursor indicating resizing an object from its top-right corner.
  ///
  /// Typically the shape of an arrow pointing upper right. May fallback to
  /// [resizeUpRightDownLeft].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_TOP_RIGHT_DIAGONAL_DOUBLE_ARROW
  ///  * Web: ne-resize
  ///  * Windows: IDC_SIZENESW
  ///  * Linux: ne-resize
  kResizeupright,

  /// A cursor indicating resizing an object from its bottom-left corner.
  ///
  /// Typically the shape of an arrow pointing lower left. May fallback to
  /// [resizeUpRightDownLeft].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_TOP_RIGHT_DIAGONAL_DOUBLE_ARROW
  ///  * Web: sw-resize
  ///  * Windows: IDC_SIZENESW
  ///  * Linux: sw-resize
  kResizedownleft,

  /// A cursor indicating resizing an object from its bottom-right corner.
  ///
  /// Typically the shape of an arrow pointing lower right. May fallback to
  /// [resizeUpLeftDownRight].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_TOP_LEFT_DIAGONAL_DOUBLE_ARROW
  ///  * Web: se-resize
  ///  * Windows: IDC_SIZENWSE
  ///  * Linux: se-resize
  kResizedownright,

  /// A cursor indicating resizing a column, or an item horizontally.
  ///
  /// Typically the shape of arrows pointing left and right with a vertical
  /// bar separating them. May fallback to [resizeLeftRight].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_HORIZONTAL_DOUBLE_ARROW
  ///  * Web: col-resize
  ///  * Windows: IDC_SIZEWE
  ///  * Linux: col-resize
  ///  * macOS: resizeLeftRightCursor
  kResizecolumn,

  /// A cursor indicating resizing a row, or an item vertically.
  ///
  /// Typically the shape of arrows pointing up and down with a horizontal bar
  /// separating them. May fallback to [resizeUpDown].
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_VERTICAL_DOUBLE_ARROW
  ///  * Web: row-resize
  ///  * Windows: IDC_SIZENS
  ///  * Linux: row-resize
  ///  * macOS: resizeUpDownCursor
  kResizerow,

  // OTHER OPERATIONS

  /// A cursor indicating zooming in.
  ///
  /// Typically a magnifying glass with a plus sign.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_ZOOM_IN
  ///  * Web: zoom-in
  ///  * Linux: zoom-in
  kZoomin,

  /// A cursor indicating zooming out.
  ///
  /// Typically a magnifying glass with a minus sign.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_ZOOM_OUT
  ///  * Web: zoom-out
  ///  * Linux: zoom-out
  kZoomout,
};

class CursorTypeUtil {
 public:
  static const std::map<std::string, CursorTypes> cursor_type_map_;
  static CursorTypes ParseCursorType(const std::string& str);
};
}  // namespace clay

#endif  // CLAY_UI_PLATFORM_CURSOR_TYPES_H_
