// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_VIEW_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_VIEW_CAPI_H_

#include "base/include/value/lynx_value_api.h"
#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_native_view_ lynx_native_view_t;
// Opaque platform native graphics handle. On Darwin this is IOSurfaceRef.
// On Windows this is a D3D shared HANDLE for the active graphics backend,
// not a texture/device pointer.
typedef struct lynx_surface_ lynx_surface_handle_t;

typedef enum lynx_native_event_type {
  kNativeEventUnkown = 0,
  kNativeEventDown,
  kNativeEventUp,
  kNativeEventMove,
  kNativeEventHover,
  kNativeEventSignal,
  kNativeEventCancel,
  kNativeEventPanZoomStart,
  kNativeEventPanZoomUpdate,
  kNativeEventPanZoomEnd,
} lynx_native_event_type_t;

typedef enum lynx_native_device_type {
  kNativeDeviceTouch,
  kNativeDeviceMouse,
  kNativeDeviceStylus,
  kNativeDeviceInvertedStylus,
  kNativeDeviceTrackpad,
} lynx_native_device_type_t;

typedef enum lynx_native_signal_kind {
  kNativeSignalNone,
  kNativeSignalStartScroll,
  kNativeSignalScroll,
  kNativeSignalEndScroll,
} lynx_native_signal_kind_t;

enum lynx_native_mouse_button {
  kNativeMouseButtonPrimary = 1 << 0,
  kNativeMouseButtonSecondary = 1 << 1,
  kNativeMouseButtonMiddle = 1 << 2,
  kNativeMouseButtonBack = 1 << 3,
  kNativeMouseButtonForward = 1 << 4,
};

typedef struct native_view_motion_event {
  // Time of event dispatch, relative to an arbitrary timeline.
  uint64_t timestamp;
  // The type of event
  lynx_native_event_type_t type;
  lynx_native_device_type_t device;
  lynx_native_signal_kind_t signal_kind;
  // Unique identifier for the pointer, not reused. Changes for each new pointer
  // down event.
  int pointer_id;
  // Unique identifier for the pointing device, reused across interactions.
  int device_id;
  // Bit field to indicate which buttons are pressed
  uint32_t buttons;
  // The coordinate of the event relative to the origin of the native view.
  float x;
  float y;
  // The coordinate of the event in viewport coordinates.
  float pageX;
  float pageY;
  // Distance in logical pixels that the pointer moved since the last events.
  float deltaX;
  float deltaY;
  // The scale (zoom factor) of the pan/zoom.
  double scale;
  // True if the alt key was down when the mouse event was fired.
  uint8_t altKey;
  // True if the control key was down when the mouse event was fired.
  uint8_t ctrlKey;
  // True if the shift key was down when the mouse event was fired.
  uint8_t shiftKey;
  // True if the meta key was down when the mouse event was fired.
  uint8_t metaKey;
} native_view_motion_event_t;

typedef enum lynx_surface_buffer_mode {
  kSingleBuffer,
  kDoubleBuffer,
  kTripleBuffer,
} lynx_surface_buffer_mode_t;

typedef struct lynx_native_view_callback_info {
  void* impl;
  int64_t callback_id;
} lynx_native_view_callback_info_t;

typedef lynx_native_view_t* (*lynx_native_view_creator)(void* opaque);
typedef void (*lynx_native_view_callback)(lynx_native_view_callback_info_t, int,
                                          lynx_value);

LYNX_CAPI_EXPORT lynx_native_view_t* lynx_native_view_create(void* user_data);
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_create(
    lynx_native_view_t*, bool (*)(lynx_native_view_t*, void* user_data));
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_attach(
    lynx_native_view_t*, void (*)(lynx_native_view_t*, void* user_data));
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_detach(
    lynx_native_view_t*, void (*)(lynx_native_view_t*, void* user_data));
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_destroy(
    lynx_native_view_t*, void (*)(lynx_native_view_t*, void* user_data));
LYNX_CAPI_EXPORT void lynx_native_view_bind_release(
    lynx_native_view_t*, void (*)(lynx_native_view_t*, void* user_data));
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_layout_changed(
    lynx_native_view_t*,
    void (*)(lynx_native_view_t*, void* user_data, float left, float top,
             float width, float height, float pixel_ratio));
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_properties_changed(
    lynx_native_view_t*, void (*)(lynx_native_view_t*, void* user_data,
                                  lynx_value attrs, lynx_value events));
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_motion_event(
    lynx_native_view_t*, void (*)(lynx_native_view_t*, void* user_data,
                                  native_view_motion_event_t*));
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_focus_changed(
    lynx_native_view_t*,
    void (*)(lynx_native_view_t*, void* user_data, bool focused, bool is_leaf));
LYNX_CAPI_EXPORT void lynx_native_view_bind_on_method_invoked(
    lynx_native_view_t*,
    void (*)(lynx_native_view_t*, void* user_data, const char* method,
             lynx_value attrs, lynx_native_view_callback callback,
             lynx_native_view_callback_info_t));
LYNX_CAPI_EXPORT void lynx_native_view_bind_is_scroll_enabled(
    lynx_native_view_t*, bool (*)(lynx_native_view_t*, void* user_data));
LYNX_CAPI_EXPORT void lynx_native_view_bind_is_surface_enabled(
    lynx_native_view_t*, bool (*)(lynx_native_view_t*, void* user_data));
LYNX_CAPI_EXPORT void lynx_native_view_bind_surface_buffer_mode(
    lynx_native_view_t*,
    lynx_surface_buffer_mode_t (*)(lynx_native_view_t*, void* user_data));

LYNX_CAPI_EXPORT bool lynx_native_view_present_surface(
    lynx_native_view_t*, int width, int height, const float* transform,
    lynx_surface_handle_t* handle);
// Returns a platform native graphics handle for the latest back buffer.
// Windows callers should import the returned D3D shared HANDLE with the
// graphics API/device that matches the active Windows backend.
LYNX_CAPI_EXPORT lynx_surface_handle_t* lynx_native_view_acquire_surface(
    lynx_native_view_t*, int width, int height);
LYNX_CAPI_EXPORT bool lynx_native_view_swap_back(lynx_native_view_t*);
LYNX_CAPI_EXPORT void lynx_native_view_trigger_event(lynx_native_view_t*,
                                                     const char* name,
                                                     lynx_value params);
LYNX_CAPI_EXPORT void lynx_native_view_request_focus(lynx_native_view_t*);
LYNX_CAPI_EXPORT void lynx_native_view_mark_dirty(lynx_native_view_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_VIEW_CAPI_H_
