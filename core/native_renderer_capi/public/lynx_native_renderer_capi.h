// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Native renderer C API for embedders that build the element tree
// directly from a host language (e.g. Rust via Whisker), bypassing
// ReactLynx / Lepus template loading entirely.
//
// Usage pattern:
//
//   1. Host obtains a LynxView (Java on Android, Obj-C on iOS) and
//      passes the platform pointer to lynx_shell_from_view().
//   2. Host calls lynx_shell_run_on_tasm_thread() to schedule work
//      on the TASM thread; everything below must run inside that
//      callback.
//   3. Host creates fiber elements via lynx_create_fiber_*(),
//      composes them with lynx_element_append_child(), and registers
//      the root with lynx_shell_set_root_element().
//   4. lynx_shell_flush() commits the tree to layout + paint.
//   5. lynx_element_release() / lynx_shell_release() drop the
//      strong references when done.
//
// All handles are opaque — internally they wrap a LynxShell* / an
// fml::RefPtr<FiberElement> respectively. The C++ types are hidden
// behind the C ABI so the embedder is insulated from name-mangling
// drift across compiler versions.

#ifndef CORE_NATIVE_RENDERER_CAPI_PUBLIC_LYNX_NATIVE_RENDERER_CAPI_H_
#define CORE_NATIVE_RENDERER_CAPI_PUBLIC_LYNX_NATIVE_RENDERER_CAPI_H_

#include <stdbool.h>
#include <stdint.h>

// Self-contained export macros. The rest of Lynx uses
// `platform/embedder/public/capi/lynx_export.h` for the same purpose,
// but this header has to compile in embedder builds (Whisker) where
// `platform/embedder/` isn't on the include path — the prebuilt
// LynxAndroid AAR / Lynx.xcframework only ship the `core/`, `base/`,
// and `service_api/` subtrees. Keeping the macros inline avoids any
// cross-subtree include from a public header.
#if defined(__GNUC__) || defined(__clang__)
#define LYNX_NATIVE_RENDERER_CAPI_EXPORT \
  __attribute__((visibility("default")))
#else
#define LYNX_NATIVE_RENDERER_CAPI_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ----- Opaque handle types --------------------------------------------------

typedef struct lynx_shell_t lynx_shell_t;
typedef struct lynx_fiber_element_t lynx_fiber_element_t;

// ----- Element tag enum -----------------------------------------------------
//
// Mirrors the subset of Lynx's built-in fiber element types Whisker's
// native renderer use case currently exercises. Adding a new tag here
// is the supported way to expose more element kinds.

typedef enum lynx_element_tag_e {
  LYNX_ELEMENT_TAG_PAGE = 0,
  LYNX_ELEMENT_TAG_VIEW = 1,
  LYNX_ELEMENT_TAG_TEXT = 2,
  LYNX_ELEMENT_TAG_RAW_TEXT = 3,
  LYNX_ELEMENT_TAG_IMAGE = 4,
  LYNX_ELEMENT_TAG_SCROLL_VIEW = 5,
} lynx_element_tag_e;

// ----- Shell wrapping + lifecycle -------------------------------------------

// Wrap a raw native shell pointer (already extracted by the embedder
// from a Java or Obj-C LynxView) into an opaque handle.
//
// `native_shell_ptr` is what `LynxTemplateRender.mNativePtr` holds on
// the Java side (a `jlong` cast back to a pointer) or what
// `LynxView`'s `_shell_` ivar holds on the Obj-C side. Doing the JNI
// reflection / ivar dance stays on the embedder side so this header
// is platform-independent and never needs JNIEnv.
//
// Returns NULL if `native_shell_ptr` is NULL. The handle wraps but
// does NOT own the underlying shell — callers must not outlive the
// LynxView that created it. Pair with lynx_shell_release() when done.
LYNX_NATIVE_RENDERER_CAPI_EXPORT lynx_shell_t* lynx_shell_from_native_ptr(
    void* native_shell_ptr);

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_shell_release(lynx_shell_t* shell);

// ----- Thread dispatch ------------------------------------------------------

typedef void (*lynx_tasm_callback_t)(void* user_data);

// Schedule `callback(user_data)` to run on the shell's TASM thread.
// On first call, the shell is configured for fiber-arch mode and the
// ElementManager becomes available; subsequent calls just dispatch.
// Returns false if the shell is null or the callback is null.
LYNX_NATIVE_RENDERER_CAPI_EXPORT bool lynx_shell_run_on_tasm_thread(
    lynx_shell_t* shell,
    lynx_tasm_callback_t callback,
    void* user_data);

// ----- Element creation -----------------------------------------------------

// Create a fiber element of the given tag. Must be called from within
// a callback dispatched via lynx_shell_run_on_tasm_thread (the
// ElementManager isn't guaranteed alive otherwise). Returns NULL on
// failure or if the tag is unknown.
//
// The returned handle owns one strong reference; release with
// lynx_element_release() when the embedder no longer needs it.
LYNX_NATIVE_RENDERER_CAPI_EXPORT lynx_fiber_element_t* lynx_create_fiber_element(
    lynx_shell_t* shell,
    lynx_element_tag_e tag);

// Create a fiber element by string tag name. Lets embedders allocate
// custom Lynx elements (any tag registered against Lynx's behaviour
// registry — e.g. `"x-input"`, `"x-refresh"`, third-party-registered
// elements) without going through the closed `lynx_element_tag_e`
// enum. Returns NULL if the tag isn't registered or `tag_name` is
// NULL / empty.
//
// Must be called from within a callback dispatched via
// lynx_shell_run_on_tasm_thread, same as the enum-tag variant. The
// returned handle has the same ownership semantics as
// `lynx_create_fiber_element`'s — caller releases via
// `lynx_element_release`.
LYNX_NATIVE_RENDERER_CAPI_EXPORT lynx_fiber_element_t*
lynx_create_fiber_element_by_name(lynx_shell_t* shell, const char* tag_name);

LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_release(lynx_fiber_element_t* element);

// Return the stable per-element id (Lynx calls this `impl_id`).
// Useful for routing events back from the platform event emitter to
// the embedder's registry. Returns 0 if `element` is NULL.
LYNX_NATIVE_RENDERER_CAPI_EXPORT int32_t lynx_element_id(lynx_fiber_element_t* element);

// ----- Element manipulation -------------------------------------------------

// Set a string-valued attribute. UTF-8. Both `key` and `value` must
// be non-null.
LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_set_attribute(
    lynx_fiber_element_t* element,
    const char* key,
    const char* value);

// Set raw inline CSS (as if `style="..."` were declared in template).
LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_set_inline_styles(
    lynx_fiber_element_t* element,
    const char* css);

// Append `child` to `parent`'s children list.
LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_append_child(
    lynx_fiber_element_t* parent,
    lynx_fiber_element_t* child);

// Remove `child` from `parent`'s children list.
LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_element_remove_child(
    lynx_fiber_element_t* parent,
    lynx_fiber_element_t* child);

// ----- Pipeline -------------------------------------------------------------

// Install `page` as the shell's root PageElement. `page` MUST have
// been created with LYNX_ELEMENT_TAG_PAGE.
LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_shell_set_root_element(
    lynx_shell_t* shell,
    lynx_fiber_element_t* page);

// Commit the current element tree — flush fiber actions, run patch
// finalization, schedule layout + paint. Must be called from inside
// the TASM thread callback.
LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_shell_flush(lynx_shell_t* shell);

// ----- subsecond ASLR anchor ------------------------------------------------

// No-op function whose address serves as a well-known anchor for
// subsecond-style hot-patch frameworks (Whisker) to compute the
// runtime ASLR slide for liblynx.so. Embedders that don't use
// subsecond can ignore this; calling it has no side effects.
//
// Stable across Lynx versions by contract — DO NOT remove or
// rename. This symbol exists *because* a hot-patch framework needs
// some reliably-exported Lynx symbol to dlsym against.
LYNX_NATIVE_RENDERER_CAPI_EXPORT void lynx_aslr_reference(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CORE_NATIVE_RENDERER_CAPI_PUBLIC_LYNX_NATIVE_RENDERER_CAPI_H_
