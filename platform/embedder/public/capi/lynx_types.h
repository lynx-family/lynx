// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TYPES_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  kLynxCursorTypeUnknown = 1,

  // cursor is a image which is downloaded from the internet.
  kLynxCursorTypeNet,

  // cursor is a image which is obtained from the local file.
  kLynxCursorTypeFile,

  ///  Determines the pointer style based on the current content
  ///  e.g. use the text style when the context is text
  kLynxCursorTypeAuto,

  /// Hide the cursor.
  ///
  /// Any cursor other than [none] or [MouseCursor.uncontrolled] unhides the
  /// cursor.
  kLynxCursorTypeNone,

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
  kLynxCursorTypeBasic,

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
  kLynxCursorTypeClick,

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
  kLynxCursorTypeForbidden,

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
  kLynxCursorTypeWait,

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
  kLynxCursorTypeProgress,

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
  kLynxCursorTypeContextmenu,

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
  kLynxCursorTypeHelp,

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
  kLynxCursorTypeText,

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
  kLynxCursorTypeVerticalText,

  /// A cursor indicating selectable table cells.
  ///
  /// Typically the shape of a hollow plus sign.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_CELL
  ///  * Web: cell
  ///  * Linux: cell
  kLynxCursorTypeCell,

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
  kLynxCursorTypePrecise,

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
  kLynxCursorTypeMove,

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
  kLynxCursorTypeGrab,

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
  kLynxCursorTypeGrabbing,

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
  kLynxCursorTypeNoDrop,

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
  kLynxCursorTypeAlias,

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
  kLynxCursorTypeSystemMouseCursor,

  /// A cursor indicating that the current operation will result in the
  /// disappearance of the item.
  ///
  /// Typically the shape of an arrow with a cloud of smoke at the corner.
  ///
  /// Corresponds to:
  ///
  ///  * macOS: disappearingItemCursor
  kLynxCursorTypeDisappearing,

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
  kLynxCursorTypeAllScroll,

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
  kLynxCursorTypeResizeLeftRight,

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
  kLynxCursorTypeResizeUpDown,

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
  kLynxCursorTypeResizeUpLeftDownRight,

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
  kLynxCursorTypeResizeUpRightDownLeft,

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
  kLynxCursorTypeResizeUp,

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
  kLynxCursorTypeResizeDown,

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
  kLynxCursorTypeResizeLeft,

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
  kLynxCursorTypeResizeRight,

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
  kLynxCursorTypeResizeUpLeft,

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
  kLynxCursorTypeResizeUpRight,

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
  kLynxCursorTypeResizeDownLeft,

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
  kLynxCursorTypeResizeDownRight,

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
  kLynxCursorTypeResizeColumn,

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
  kLynxCursorTypeResizeRow,

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
  kLynxCursorTypeZoomIn,

  /// A cursor indicating zooming out.
  ///
  /// Typically a magnifying glass with a minus sign.
  ///
  /// Corresponds to:
  ///
  ///  * Android: TYPE_ZOOM_OUT
  ///  * Web: zoom-out
  ///  * Linux: zoom-out
  kLynxCursorTypeZoomOut,
} lynx_cursor_type_e;

typedef enum {
  kLynxPointerPhaseCancel,
  /// The pointer, which must have been down (see kDown), is now up.
  ///
  /// For touch, this means that the pointer is no longer in contact with the
  /// screen. For a mouse, it means the last button was released. Note that if
  /// any other buttons are still pressed when one button is released, that
  /// should be sent as a kMove rather than a kUp.
  kLynxPointerPhaseUp,
  /// The pointer, which must have been been up, is now down.
  ///
  /// For touch, this means that the pointer has come into contact with the
  /// screen. For a mouse, it means a button is now pressed. Note that if any
  /// other buttons are already pressed when a new button is pressed, that
  /// should be sent as a kMove rather than a kDown.
  kLynxPointerPhaseDown,
  /// The pointer moved while down.
  ///
  /// This is also used for changes in button state that don't cause a kDown or
  /// kUp, such as releasing one of two pressed buttons.
  kLynxPointerPhaseMove,
  /// The pointer is now sending input to Lynx. For instance, a mouse has
  /// entered the area where the Lynx content is displayed.
  ///
  /// A pointer should always be added before sending any other events.
  kLynxPointerPhaseAdd,
  /// The pointer is no longer sending input to Lynx. For instance, a mouse
  /// has left the area where the Lynx content is displayed.
  ///
  /// A removed pointer should no longer send events until sending a new kAdd.
  kLynxPointerPhaseRemove,
  /// The pointer moved while up.
  kLynxPointerPhaseHover,
  /// A pan/zoom started on this pointer.
  kLynxPointerPhasePanZoomStart,
  /// The pan/zoom updated.
  kLynxPointerPhasePanZoomUpdate,
  /// The pan/zoom ended.
  kLynxPointerPhasePanZoomEnd,
} lynx_pointer_phase_e;

typedef enum {
  kLynxPointerSignalKindNone,
  kLynxPointerSignalKindScroll,
  kLynxPointerSignalKindScrollInertiaCancel,
  kLynxPointerSignalKindScale,
} lynx_pointer_signal_kind_e;

typedef enum {
  kLynxPointerDeviceKindMouse = 1,
  kLynxPointerDeviceKindTouch,
  kLynxPointerDeviceKindStylus,
  kLynxPointerDeviceKindTrackpad,
} lynx_pointer_device_kind_e;

/// Flags for the `buttons` field of `lynx_pointer_event_t` when `device_kind`
/// is `kLynxPointerDeviceKindMouse`.
typedef enum {
  kLynxPointerMouseButtonsMousePrimary = 1 << 0,
  kLynxPointerMouseButtonsMouseSecondary = 1 << 1,
  kLynxPointerMouseButtonsMouseMiddle = 1 << 2,
  kLynxPointerMouseButtonsMouseBack = 1 << 3,
  kLynxPointerMouseButtonsMouseForward = 1 << 4,
  /// If a mouse has more than five buttons, send higher bit shifted values
  /// corresponding to the button number: 1 << 5 for the 6th, etc.
} lynx_pointer_mouse_buttons_e;

typedef enum {
  kLynxKeyEventTypeUp = 1,
  kLynxKeyEventTypeDown,
  kLynxKeyEventTypeRepeat,
} lynx_key_event_type_e;

typedef enum {
  kLynxEventTypeUnknown = 0,

  // touch events
  kLynxEventTypeTouchStart,
  kLynxEventTypeTouchMove,
  kLynxEventTypeTouchCancel,
  kLynxEventTypeTouchEnd,
  kLynxEventTypeTap,
  kLynxEventTypeLongPress,

  // mouse events
  kLynxEventTypeMouseDown,
  kLynxEventTypeMouseUp,
  kLynxEventTypeMouseMove,
  kLynxEventTypeMouseClick,
  kLynxEventTypeMouseDoubleClick,
  kLynxEventTypeMouseLongPress,
  kLynxEventTypeMouseEnter,
  kLynxEventTypeMouseOver,
  kLynxEventTypeMouseLeave,
  // mouse drag and drop events
  kLynxEventTypeDragEnter,
  kLynxEventTypeDragOver,
  kLynxEventTypeDragLeave,
  kLynxEventTypeDrop,

  // wheel event
  kLynxEventTypeWheel,

  // key events
  kLynxEventTypeKeyDown,
  kLynxEventTypeKeyUp,
  // animation events
  kLynxEventTypeAnimationStart,
  kLynxEventTypeAnimationRepeat,
  kLynxEventTypeAnimationEnd,
  kLynxEventTypeAnimationCancel,
  kLynxEventTypeTransitionStart,
  kLynxEventTypeTransitionEnd,
} lynx_event_type_e;

typedef struct lynx_pointer_event_t {
  /// The size of this struct. Must be sizeof(lynx_pointer_event_t).
  size_t struct_size;
  lynx_pointer_phase_e phase;
  /// The timestamp at which the pointer event was generated.
  size_t timestamp;
  /// The x coordinate of the pointer event in physical pixels.
  double x;
  /// The y coordinate of the pointer event in physical pixels.
  double y;
  /// An optional device identifier. If this is not specified, it is assumed
  /// that the embedder has no multi-touch capability.
  int32_t device;
  lynx_pointer_signal_kind_e signal_kind;
  /// The x offset of the scroll in physical pixels.
  double scroll_delta_x;
  /// The y offset of the scroll in physical pixels.
  double scroll_delta_y;
  /// The type of the device generating this event.
  /// Backwards compatibility note: If this is not set, the device will be
  /// treated as a mouse, with the primary button set for `kDown` and `kMove`.
  /// If set explicitly to `kLynxPointerDeviceKindMouse`, you must set the
  /// correct buttons.
  lynx_pointer_device_kind_e device_kind;
  /// The buttons currently pressed, if any.
  int64_t buttons;
  /// The x offset of the pan/zoom in physical pixels.
  double pan_x;
  /// The y offset of the pan/zoom in physical pixels.
  double pan_y;
  /// The scale of the pan/zoom, where 1.0 is the initial scale.
  double scale;
  /// The rotation of the pan/zoom in radians, where 0.0 is the initial angle.
  double rotation;
  /// Whether the event is triggered by the touchpad
  size_t is_precise_scroll;
} lynx_pointer_event_t;

/// A structure to represent a key event.
typedef struct lynx_key_event_t {
  /// The size of this struct. Must be sizeof(lynx_key_event_t).
  size_t struct_size;
  /// The timestamp at which the key event was generated.
  double timestamp;
  /// The event kind.
  lynx_key_event_type_e type;
  /// The USB HID code for the physical key of the event.
  ///
  /// For the full definition and list of pre-defined physical keys.
  ///
  /// The only case that `physical` might be 0 is when this is an empty event.
  uint64_t physical;
  /// The key ID for the logical key of this event.
  ///
  /// For the full definition and a list of pre-defined logical keys.
  ///
  /// The only case that `logical` might be 0 is when this is an empty event.
  uint64_t logical;
  /// Null-terminated character input from the event. Can be null. Ignored for
  /// up events.
  const char* character;
  /// True if this event does not correspond to a native event.
  ///
  /// The embedder is likely to skip events and/or construct new events that do
  /// not correspond to any native events in order to conform the regularity
  /// of events (as documented in `lynx_key_event_t`). An example is when a key
  /// up is missed due to loss of window focus, on a platform that provides
  /// query to key pressing status, the embedder might realize that the key has
  /// been released at the next key event, and should construct a synthesized up
  /// event immediately before the actual event.
  ///
  /// An event being synthesized means that the `timestamp` might greatly
  /// deviate from the actual time when the event occurs physically.
  bool synthesized;
} lynx_key_event_t;

typedef struct lynx_task_runner_* lynx_task_runner;

typedef struct lynx_task_t {
  lynx_task_runner runner;
  uint64_t task;
} lynx_task_t;

///
/// Describes how to interpret the components of a pixel.
///
typedef enum lynx_color_type_e {
  ///
  /// RGBA with 8 bits per pixel (32bits total).
  ///
  kLynxColorTypeRGBA_8888,

  ///
  /// BGRA with 8 bits per pixel (32bits total).
  ///
  kLynxColorTypeBGRA_8888,
} lynx_color_type_e;

typedef struct lynx_accelerated_paint_info_t {
  /// The size of this struct. Must be sizeof(lynx_accelerated_paint_info_t).
  size_t struct_size;
  /// The handle to the shared texture.
  void* shared_texture_handle;
  /// The color type of the shared texture.
  lynx_color_type_e color_type;
  /// The width of the shared texture in pixels.
  uint32_t width;
  /// The height of the shared texture in pixels.
  uint32_t height;
} lynx_accelerated_paint_info_t;

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TYPES_H_
