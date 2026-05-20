// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CAPI_H_

#include "lynx_export.h"
#include "lynx_load_meta_capi.h"
#include "lynx_runtime_lifecycle_observer_capi.h"
#include "lynx_update_meta_capi.h"
#include "lynx_view_builder_capi.h"
#include "lynx_view_client_capi.h"
#include "lynx_vsync_monitor_capi.h"

LYNX_EXTERN_C_BEGIN

// LynxView is the main entry point for rendering and interacting with Lynx.
// It provides methods for loading and rendering templates, updating data, and
// sending events. Users should call lynx_view_release() to release the LynxView
// when it is no longer needed.
typedef struct lynx_view_t lynx_view_t;

// Create lynx view with builder.
LYNX_CAPI_EXPORT lynx_view_t* lynx_view_create(lynx_view_builder_t* builder,
                                               void* user_data);
LYNX_CAPI_EXPORT void* lynx_view_get_user_data(lynx_view_t*);

// Get the fixed WebView2 runtime path configured by the builder. Returns an
// empty string if no fixed runtime path was configured.
LYNX_CAPI_EXPORT const char* lynx_view_get_webview2_fixed_runtime_path(
    lynx_view_t*);

// Register lifecycle event observer for LynxView. The client passed in is the
// structure implemented by user and registered to the LynxView instance is used
// to obtain the callbacks of each process in the LynxView lifecycle.
LYNX_CAPI_EXPORT void lynx_view_add_client(lynx_view_t*,
                                           lynx_view_client_t* client);
LYNX_CAPI_EXPORT void lynx_view_remove_client(lynx_view_t*,
                                              lynx_view_client_t* client);

// Register a runtime lifecycle observer for the LynxView.
// This function allows you to associate a runtime lifecycle observer with a
// specific LynxView instance. The observer will be notified of various runtime
// lifecycle events, such as runtime attachment, and detachment. This can be
// useful for monitoring and responding to changes in the runtime state.
LYNX_CAPI_EXPORT void lynx_view_register_runtime_lifecycle_observer(
    lynx_view_t*, lynx_runtime_lifecycle_observer_t*);

// Using LynxLoadMeta to render LynxView, it is the main entrance for the client
// to load Lynx templates.
LYNX_CAPI_EXPORT void lynx_view_load_template(lynx_view_t*,
                                              lynx_load_meta_t* data);

// Using LynxUpdateMeta to update LynxView, it is the main entrance for the
// client to update template data.
LYNX_CAPI_EXPORT void lynx_view_update_data(lynx_view_t*,
                                            lynx_update_meta_t* data);

// Reload the template of the LynxView with the given data. This function
// reloads the template of the specified LynxView using the provided template
// data and global properties. It can be used when you need to refresh the view
// with updated data or different template configurations.
LYNX_CAPI_EXPORT void lynx_view_reload_template(
    lynx_view_t*, lynx_template_data_t* data,
    lynx_template_data_t* global_props);

// Send global events to the front end through the client, and the front end can
// listen to the event through GlobalEventEmitter.
LYNX_CAPI_EXPORT void lynx_view_send_global_event(lynx_view_t*,
                                                  const char* name,
                                                  const char* json);

// Update the screen metrics of the LynxView. This function allows you to modify
// the screen-related properties of an existing LynxView, including the screen
// width, height, and pixel ratio.
LYNX_CAPI_EXPORT void lynx_view_update_screen_metrics(lynx_view_t*,
                                                      const float& width,
                                                      const float& height,
                                                      const float& pixel_ratio);

// Updates the position and size of the LynxView. This function allows you to
// change the position and dimensions of an existing LynxView. The position is
// defined by the `x` and `y` coordinates of the top-left corner of the view,
// and the size is defined by the `width` and `height` parameters.
LYNX_CAPI_EXPORT void lynx_view_set_frame(lynx_view_t*, const float& x,
                                          const float& y, const float& width,
                                          const float& height);

// Changing the font scaling ratio in client settings will automatically change
// the text size.
LYNX_CAPI_EXPORT void lynx_view_set_font_scale(lynx_view_t*,
                                               const float& font_scale);

// Set the parent window of the LynxView.
LYNX_CAPI_EXPORT void lynx_view_set_parent(lynx_view_t*, NativeWindow parent);

// Get the native window of the LynxView.
LYNX_CAPI_EXPORT NativeWindow lynx_view_get_native_window(lynx_view_t*);

// Get the generic resource fetcher of the LynxView. This function increases the
// reference count of the returned fetcher. The caller assumes ownership and is
// responsible for calling `lynx_generic_resource_fetcher_release` to release
// it when no longer needed.
LYNX_CAPI_EXPORT lynx_generic_resource_fetcher_t*
lynx_view_get_generic_resource_fetcher(lynx_view_t*);

// Instruct the LynxView to enter the foreground state. This function should be
// called when the LynxView becomes visible or active again after being in the
// background. It may trigger necessary operations such as resuming animations,
// reloading resources, or updating the UI.
LYNX_CAPI_EXPORT void lynx_view_enter_foreground(lynx_view_t*);

// Instruct the LynxView to enter the background state. This function should be
// called when the LynxView is about to become hidden or inactive, such as when
// the app is sent to the background. It may trigger operations like pausing
// animations, releasing resources, or saving the current state to ensure
// efficient resource usage and a smooth transition when the view returns to the
// foreground.
LYNX_CAPI_EXPORT void lynx_view_enter_background(lynx_view_t*);

// Send bubble event to lynx view. This function is only used by test bench to
// replay the bubble event.
LYNX_CAPI_EXPORT void lynx_view_inject_bubble_event(lynx_view_t*,
                                                    const char* params);
// Register instance-level view factory.
LYNX_CAPI_EXPORT void lynx_view_register_native_view(
    lynx_view_t*, const char* name, lynx_native_view_creator creator,
    void* opaque);

// Register Input Method Editor (IME) handler for the LynxView.
// When registering IME (handler != NULL), the provided handler will be invoked
// to handle keyboard events. When releasing IME (handler == NULL), the IME
// will be hidden and no further keyboard events will be sent.
LYNX_CAPI_EXPORT void lynx_view_register_ime_handler(lynx_view_t* view,
                                                     void* handler,
                                                     void* opaque);

// Set a custom vsync monitor to request a synchronous vsync signal
LYNX_CAPI_EXPORT void lynx_view_set_custom_vsync_monitor(
    lynx_view_t*, lynx_vsync_monitor_t* monitor);

// Users should call lynx_view_release() to release the LynxView when it is no
// longer needed.
LYNX_CAPI_EXPORT void lynx_view_release(lynx_view_t* view);

// Callback type for event simulation (devtool).
typedef void (*lynx_emulate_touch_fn)(void* context, const char* event_type,
                                      int x, int y, const char* button,
                                      float delta_x, float delta_y,
                                      int modifiers, int click_count);

// Set event simulation proxy for devtool. The callback will be invoked to
// emulate touch/mouse events. Pass NULL callback to clear the proxy.
LYNX_CAPI_EXPORT void lynx_view_set_event_simulation_proxy(
    lynx_view_t* view, lynx_emulate_touch_fn callback, void* context);

LYNX_CAPI_EXPORT void lynx_view_send_touch_event(lynx_view_t* view,
                                                 const char* name, int32_t id,
                                                 float x, float y,
                                                 float client_x, float client_y,
                                                 float page_x, float page_y);

LYNX_CAPI_EXPORT int lynx_view_get_node_for_location(lynx_view_t* view, int x,
                                                     int y);

LYNX_CAPI_EXPORT void lynx_view_emulate_mouse_event(lynx_view_t* view,
                                                    const char* event_name,
                                                    float x, float y,
                                                    float delta_x,
                                                    float delta_y);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CAPI_H_
