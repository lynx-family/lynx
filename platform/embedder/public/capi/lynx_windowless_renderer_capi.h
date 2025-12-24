// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_WINDOWLESS_RENDERER_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_WINDOWLESS_RENDERER_CAPI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lynx_export.h"
#include "lynx_types.h"

LYNX_EXTERN_C_BEGIN

// The type of the windowless renderer.
typedef enum lynx_windowless_renderer_type_e {
  // The software renderer type that renders content on CPU.
  kRendererTypeSoftware = 0,
  // The GL renderer type that renders content with host OpenGL context.
  // Unlike kRendererTypeGLDirect, this renderer type will be running as a
  // bitmap uploader, underneath will render view using a SharedImageSink and
  // readback GPU memory to a CPU bitmap. It may be SLOW for large views, but
  // all components will work well.
  kRendererTypeGL,
  // The GL renderer type that renders content with host OpenGL context.
  // This renderer type will be running directly on the host OpenGL context, it
  // can speed normal UI rendering, but components using external textures like
  // video or canvas maybe NOT work.
  kRendererTypeGLDirect,
  // The accelerated renderer type that uses shared texture.
  kRendererTypeAccelerated,
} lynx_windowless_renderer_type_e;

// Forward declaration of the windowless renderer structure.
// This is an opaque type that represents the internal implementation
// of the windowless renderer. Users should not directly access its
// fields but instead use the provided API functions to interact with it.
typedef struct lynx_windowless_renderer_t lynx_windowless_renderer_t;

// Creates a new windowless handler instance.
// This function is the entry point for initializing a windowless handler
// with the specified type and optional user data. The finalizer function
// will be called when the renderer is released.
LYNX_CAPI_EXPORT lynx_windowless_renderer_t*
lynx_windowless_renderer_create_with_finalizer(
    lynx_windowless_renderer_type_e type, void* user_data,
    void (*finalizer)(lynx_windowless_renderer_t*, void*));

LYNX_CAPI_EXPORT void* lynx_windowless_renderer_get_user_data(
    lynx_windowless_renderer_t*);

/* ----- Callbacks for GL renderer type ----- */
// The callback function type for making the current context. Returns true if
// the context is made successfully.
typedef bool (*on_gl_make_current)(lynx_windowless_renderer_t*);
// The callback function type for clearing the current context. Returns true if
// the context is cleared successfully.
typedef bool (*on_gl_clear_current)(lynx_windowless_renderer_t*);
// The callback function type for presenting the rendered content. The return
// value indicates success of the present call.
typedef bool (*on_gl_present)(lynx_windowless_renderer_t*);
// The callback function type for creating a framebuffer object. Returns the
// framebuffer object if the creation is successful.
typedef uint32_t (*on_gl_create_fbo)(lynx_windowless_renderer_t*, int w, int h);
// The callback function type for resolving the GL procedure. Returns the
// procedure address if the resolution is successful.
typedef void* (*on_gl_proc_resolver)(lynx_windowless_renderer_t*, const char*);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_on_gl_make_current(
    lynx_windowless_renderer_t*, on_gl_make_current);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_on_gl_clear_current(
    lynx_windowless_renderer_t*, on_gl_clear_current);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_on_gl_present(
    lynx_windowless_renderer_t*, on_gl_present);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_on_gl_create_fbo(
    lynx_windowless_renderer_t*, on_gl_create_fbo);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_on_gl_proc_resolver(
    lynx_windowless_renderer_t*, on_gl_proc_resolver);
/* ----- Callbacks for GL renderer type ----- */

/* ----- Callbacks for Software renderer type ----- */
// The callback function type for presenting the rendered content when the
// renderer is in software mode. The return value indicates success of the
// present call.
typedef bool (*on_software_present)(lynx_windowless_renderer_t*,
                                    const void* allocation, size_t row_bytes,
                                    size_t height);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_on_software_present(
    lynx_windowless_renderer_t*, on_software_present);
/* ----- Callbacks for Software renderer type ----- */

/* ----- Callbacks for accelerated renderer type ----- */
typedef bool (*on_accelerated_present)(lynx_windowless_renderer_t*);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_on_accelerated_present(
    lynx_windowless_renderer_t*, on_accelerated_present);

// Gets the accelerated paint info of the windowless renderer. The accelerated
// paint info used for presenting on screen is generally obtained by calling
// from the host's render thread. Returns true if the call is successful, false
// otherwise.
LYNX_CAPI_EXPORT bool lynx_windowless_renderer_get_accelerated_paint_info(
    lynx_windowless_renderer_t*, lynx_accelerated_paint_info_t* paint_info);
/* ----- Callbacks for accelerated renderer type ----- */

// The callback function type for posting a task to the main thread.
// The task is a function pointer that will be executed on the main thread.
// The interval_nanoseconds is the time interval in nanoseconds at which the
// task should be executed.
typedef void (*on_post_task)(lynx_windowless_renderer_t*, lynx_task_t task,
                             uint64_t interval_nanoseconds);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_on_post_task(
    lynx_windowless_renderer_t*, on_post_task);

// Runs a task on the main thread.
LYNX_CAPI_EXPORT void lynx_windowless_renderer_run_task(
    lynx_windowless_renderer_t*, lynx_task_t task);

// Sends a pointer event to the windowless renderer.
LYNX_CAPI_EXPORT void lynx_windowless_renderer_send_pointer_event(
    lynx_windowless_renderer_t*, lynx_pointer_event_t* event);
// Sends a key event to the windowless renderer.
LYNX_CAPI_EXPORT void lynx_windowless_renderer_send_key_event(
    lynx_windowless_renderer_t*, lynx_key_event_t* event);

/* ----- Callbacks for platform function ----- */
typedef const char* (*get_clipboard_data)(lynx_windowless_renderer_t*);
typedef void (*set_clipboard_data)(lynx_windowless_renderer_t*,
                                   const char* data);
typedef void (*activate_system_cursor)(lynx_windowless_renderer_t*,
                                       lynx_cursor_type_e type,
                                       const char* path);
typedef void (*show_text_input)(lynx_windowless_renderer_t*, bool show);
typedef void (*set_marked_text_rect)(lynx_windowless_renderer_t*, float x,
                                     float y, float width, float height);
typedef void (*set_editable_transform)(lynx_windowless_renderer_t*,
                                       const float transform_matrix[16]);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_get_clipboard_data(
    lynx_windowless_renderer_t*, get_clipboard_data);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_set_clipboard_data(
    lynx_windowless_renderer_t*, set_clipboard_data);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_activate_system_cursor(
    lynx_windowless_renderer_t*, activate_system_cursor);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_show_text_input(
    lynx_windowless_renderer_t*, show_text_input);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_set_marked_text_rect(
    lynx_windowless_renderer_t*, set_marked_text_rect);
LYNX_CAPI_EXPORT void lynx_windowless_renderer_bind_set_editable_transform(
    lynx_windowless_renderer_t*, set_editable_transform);
/* ----- Callbacks for platform function ----- */

LYNX_CAPI_EXPORT void lynx_windowless_renderer_release(
    lynx_windowless_renderer_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_WINDOWLESS_RENDERER_CAPI_H_
