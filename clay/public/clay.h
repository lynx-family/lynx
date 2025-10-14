// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_PUBLIC_CLAY_H_
#define CLAY_PUBLIC_CLAY_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef CLAY_LIBRARY_IMPLEMENTATION

// Add visibility/export annotations when building the library.
#ifdef _WIN32
#define CLAY_EXPORT __declspec(dllexport)
#else
#define CLAY_EXPORT __attribute__((visibility("default")))
#endif

#else  // CLAY_LIBRARY_IMPLEMENTATION

#ifdef _WIN32
#define CLAY_EXPORT __declspec(dllimport)
#else
#define CLAY_EXPORT
#endif

#endif  // CLAY_LIBRARY_IMPLEMENTATION

#if defined(__cplusplus)
#define CLAY_EXTERN_C extern "C"
#define CLAY_EXTERN_C_BEGIN CLAY_EXTERN_C {
#define CLAY_EXTERN_C_END }
#else  // defined(__cplusplus)
#define CLAY_EXTERN_C
#define CLAY_EXTERN_C_BEGIN
#define CLAY_EXTERN_C_END
#endif  // defined(__cplusplus)

CLAY_EXTERN_C_BEGIN

//------------------------------------------------------------------------------
// Handles
//------------------------------------------------------------------------------

// IOSurfaceRef on Darwin
// D3DSharedHandle on Win
typedef void* ClaySharedImageNativeHandle;

/// id<MTLDevice> in Metal
typedef void* ClayMetalDeviceHandle;

/// id<MTLCommandQueue> in Metal
typedef void* ClayMetalCommandQueueHandle;

/// ID3D11Device* in D3D
typedef void* ClayD3DDeviceHandle;

/// id<MTLTexture> in Metal
typedef void* ClayMetalTextureHandle;

/// ID3D11Texture2D* in D3D
typedef void* ClayD3DTextureHandle;

//------------------------------------------------------------------------------
// Callbacks
//------------------------------------------------------------------------------
typedef void (*ClayVoidCallback)(void* /* user data */);
typedef bool (*ClayBoolCallback)(void* /* user data */);
typedef void (*ClayDestructionCallback)(const void* /* ptr */,
                                        void* /* user_data */);
typedef void (*ClayKeyEventCallback)(bool /* handled */, void* /* user_data */);
typedef void (*ClayNetLoadResultCallback)(size_t request_seq, bool success,
                                          const uint8_t* data, size_t length);

//------------------------------------------------------------------------------
// Enumerations
//------------------------------------------------------------------------------
typedef enum {
  kClaySharedImageBackingPixelFormatNative8888,  // BGRA8888
  kClaySharedImageBackingPixelFormatRGBA8,
} ClaySharedImageBackingPixelFormat;

typedef enum {
  kClaySharedImageBackingTypeIOSurface,      /// Darwin
  kClaySharedImageBackingTypeCVPixelBuffer,  /// Darwin
  kClaySharedImageBackingTypeD3DTexture,     /// Windows
  kClaySharedImageBackingTypeNativeImage,    /// Harmony
  kClaySharedImageBackingTypeShmImage,       /// Linux
} ClaySharedImageBackingType;

typedef enum {
  /// CGL on OSX
  /// ANGLE on Windows, DO NOT use unless you are using the same ANGLE library
  /// EGL on Android
  kClaySharedImageRepresentationTypeGL,
  kClaySharedImageRepresentationTypeMetal,  /// OSX and iOS
  kClaySharedImageRepresentationTypeD3D,    /// Windows
  kClaySharedImageRepresentationTypeVulkan,
  kClaySharedImageRepresentationTypeShm,
} ClaySharedImageRepresentationType;

typedef enum {
  // In single buffer mode
  // the user must call ClaySharedImageSinkReleaseFront after read
  kClaySharedImageSinkBufferModeSingleBuffer,
  kClaySharedImageSinkBufferModeDoubleBuffer,
  kClaySharedImageSinkBufferModeTripleBuffer,
} ClaySharedImageSinkBufferMode;

/// The format of image ecodeing.
typedef enum { kClayImageFormatPNG, kClayImageFormatJPEG } ClayImageFormat;

//------------------------------------------------------------------------------
// The headless of embebbder.
typedef enum {
  kClayPointerPhaseCancel,
  /// The pointer, which must have been down (see kDown), is now up.
  ///
  /// For touch, this means that the pointer is no longer in contact with the
  /// screen. For a mouse, it means the last button was released. Note that if
  /// any other buttons are still pressed when one button is released, that
  /// should be sent as a kMove rather than a kUp.
  kClayPointerPhaseUp,
  /// The pointer, which must have been been up, is now down.
  ///
  /// For touch, this means that the pointer has come into contact with the
  /// screen. For a mouse, it means a button is now pressed. Note that if any
  /// other buttons are already pressed when a new button is pressed, that
  /// should be sent as a kMove rather than a kDown.
  kClayPointerPhaseDown,
  /// The pointer moved while down.
  ///
  /// This is also used for changes in button state that don't cause a kDown or
  /// kUp, such as releasing one of two pressed buttons.
  kClayPointerPhaseMove,
  /// The pointer is now sending input to Clay. For instance, a mouse has
  /// entered the area where the Clay content is displayed.
  ///
  /// A pointer should always be added before sending any other events.
  kClayPointerPhaseAdd,
  /// The pointer is no longer sending input to Clay. For instance, a mouse
  /// has left the area where the Clay content is displayed.
  ///
  /// A removed pointer should no longer send events until sending a new kAdd.
  kClayPointerPhaseRemove,
  /// The pointer moved while up.
  kClayPointerPhaseHover,
  /// A pan/zoom started on this pointer.
  kClayPointerPhasePanZoomStart,
  /// The pan/zoom updated.
  kClayPointerPhasePanZoomUpdate,
  /// The pan/zoom ended.
  kClayPointerPhasePanZoomEnd,
} ClayPointerPhase;

typedef enum {
  kClayPointerSignalKindNone,
  kClayPointerSignalKindScroll,
  kClayPointerSignalKindScrollInertiaCancel,
  kClayPointerSignalKindScale,
} ClayPointerSignalKind;

typedef enum {
  kClayPointerDeviceKindMouse = 1,
  kClayPointerDeviceKindTouch,
  kClayPointerDeviceKindStylus,
  kClayPointerDeviceKindTrackpad,
} ClayPointerDeviceKind;

/// Flags for the `buttons` field of `ClayPointerEvent` when `device_kind`
/// is `kClayPointerDeviceKindMouse`.
typedef enum {
  kClayPointerMouseButtonsMousePrimary = 1 << 0,
  kClayPointerMouseButtonsMouseSecondary = 1 << 1,
  kClayPointerMouseButtonsMouseMiddle = 1 << 2,
  kClayPointerMouseButtonsMouseBack = 1 << 3,
  kClayPointerMouseButtonsMouseForward = 1 << 4,
  /// If a mouse has more than five buttons, send higher bit shifted values
  /// corresponding to the button number: 1 << 5 for the 6th, etc.
} ClayPointerMouseButtons;

typedef enum {
  kClayKeyEventTypeUp = 1,
  kClayKeyEventTypeDown,
  kClayKeyEventTypeRepeat,
} ClayKeyEventType;

typedef enum {
  kClayEventTypeUnknown = 0,

  // touch events
  kClayEventTypeTouchStart,
  kClayEventTypeTouchMove,
  kClayEventTypeTouchCancel,
  kClayEventTypeTouchEnd,
  kClayEventTypeTap,
  kClayEventTypeLongPress,

  // mouse events
  kClayEventTypeMouseDown,
  kClayEventTypeMouseUp,
  kClayEventTypeMouseMove,
  kClayEventTypeMouseClick,
  kClayEventTypeMouseDoubleClick,
  kClayEventTypeMouseLongPress,
  kClayEventTypeMouseEnter,
  kClayEventTypeMouseOver,
  kClayEventTypeMouseLeave,
  // mouse drag and drop events
  kClayEventTypeDragEnter,
  kClayEventTypeDragOver,
  kClayEventTypeDragLeave,
  kClayEventTypeDrop,

  // wheel event
  kClayEventTypeWheel,

  // trackpad event
  kClayEventTypePanZoom,

  // key events
  kClayEventTypeKeyDown,
  kClayEventTypeKeyUp,
  // animation events
  kClayEventTypeAnimationStart,
  kClayEventTypeAnimationRepeat,
  kClayEventTypeAnimationEnd,
  kClayEventTypeAnimationCancel,
  kClayEventTypeTransitionStart,
  kClayEventTypeTransitionEnd,
} ClayEventType;

//------------------------------------------------------------------------------
// structs
//------------------------------------------------------------------------------
typedef struct ClaySize {
  uint32_t width;
  uint32_t height;
} ClaySize;

typedef struct ClayDataHolder {
  size_t size;
  const void* ptr;
  void* user_data;
  ClayDestructionCallback destruction_callback;
} ClayDataHolder;

typedef struct ClayPointer {
  typedef enum {
    kClayPointerTypeVoidPtr = 0,  // Internal usage. Invalid pointer
    kClayPointerTypeTransform,
    kClayPointerTypeExternal,
    kClayPointerTypeFilter,
    kClayPointerTypeBoxShadow,
  } ClayPointerType;

  // Type of the pointer.
  ClayPointerType type;
  void* pointer;
} ClayPointer;

typedef struct ClayTransformation {
  /// horizontal scale factor
  float scaleX;
  /// horizontal skew factor
  float skewX;
  /// horizontal translation
  float transX;
  /// vertical skew factor
  float skewY;
  /// vertical scale factor
  float scaleY;
  /// vertical translation
  float transY;
  /// input x-axis perspective factor
  float pers0;
  /// input y-axis perspective factor
  float pers1;
  /// perspective scale factor
  float pers2;
} ClayTransformation;

/// The container of native handle
typedef struct ClayCanvasView* ClayCanvasViewRef;

typedef struct ClaySharedImage* ClaySharedImageRef;

typedef struct ClayFenceSync* ClayFenceSyncRef;

typedef struct ClaySharedImageSink* ClaySharedImageSinkRef;

typedef struct ClaySharedImageSinkAccessor* ClaySharedImageSinkAccessorRef;

typedef struct ClaySharedImageGLRepresentationConfig {
  /// The size of this struct. Must be
  /// sizeof(ClaySharedImageGLRepresentationConfig).
  size_t struct_size;
  /// An optional parameter used on Windows ANGLE to support multiple ANGLE
  /// binaries, must be `eglGetProcAddress` proc address
  void* get_proc_address;
} ClaySharedImageGLRepresentationConfig;

typedef struct ClaySharedImageVKRepresentationConfig {
  /// The size of this struct. Must be
  /// sizeof(ClaySharedImageVKRepresentationConfig).
  size_t struct_size;
  void* physical_device;
  void* device;
  void* queue;
} ClaySharedImageVKRepresentationConfig;

typedef struct ClaySharedImageMetalRepresentationConfig {
  /// The size of this struct. Must be
  /// sizeof(ClaySharedImageMetalRepresentationConfig).
  size_t struct_size;
  ClayMetalDeviceHandle device;
  ClayMetalCommandQueueHandle command_queue;
} ClaySharedImageMetalRepresentationConfig;

typedef struct ClaySharedImageD3DRepresentationConfig {
  /// The size of this struct. Must be
  /// sizeof(ClaySharedImageD3DRepresentationConfig).
  size_t struct_size;
  ClayD3DDeviceHandle device;
} ClaySharedImageD3DRepresentationConfig;

typedef struct ClaySharedImageShmRepresentationConfig {
  /// The size of this struct. Must be
  /// sizeof(ClaySharedImageShmRepresentationConfig).
  size_t struct_size;
} ClaySharedImageShmRepresentationConfig;

typedef struct ClaySharedImageRepresentationConfig {
  /// The size of this struct. Must be
  /// sizeof(ClaySharedImageRepresentationConfig).
  size_t struct_size;
  ClaySharedImageRepresentationType type;
  union {
    ClaySharedImageGLRepresentationConfig gl_config;
    ClaySharedImageMetalRepresentationConfig metal_config;
    ClaySharedImageD3DRepresentationConfig d3d_config;
    ClaySharedImageVKRepresentationConfig vk_config;
    ClaySharedImageShmRepresentationConfig shm_config;
  };
} ClaySharedImageRepresentationConfig;

typedef struct ClayMetalTexture {
  /// The size of this struct. Must be sizeof(ClayMetalTexture).
  size_t struct_size;
  /// id<MTLTexture> in Metal
  /// The value is owned by engine, user must retain to persist it.
  ClayMetalTextureHandle texture;
  /// User data to be returned on the invocation of the destruction callback.
  void* user_data;
  /// The callback invoked by the engine when it no longer needs this backing
  /// store.
  ClayVoidCallback destruction_callback;
} ClayMetalTexture;

typedef struct ClayD3DTexture {
  /// The size of this struct. Must be sizeof(ClayD3DTexture).
  size_t struct_size;
  /// ID3D11Texture2D* in D3D
  /// The value is owned by engine, user must retain to persist it.
  ClayD3DTextureHandle texture;
  /// User data to be returned on the invocation of the destruction callback.
  void* user_data;
  /// The callback invoked by the engine when it no longer needs this backing
  /// store.
  ClayVoidCallback destruction_callback;
} ClayD3DTexture;

typedef struct ClayOpenGLTexture {
  /// The size of this struct. Must be sizeof(ClayOpenGLTexture).
  size_t struct_size;
  /// Target texture of the active texture unit (example GL_TEXTURE_2D or
  /// GL_TEXTURE_RECTANGLE).
  uint32_t target;
  /// The name of the texture.
  uint32_t name;
  /// The texture format (example GL_RGBA8).
  uint32_t format;
  /// User data to be returned on the invocation of the destruction callback.
  void* user_data;
  /// The callback invoked by the engine when it no longer needs this backing
  /// store.
  ClayVoidCallback destruction_callback;
  /// Optional parameters for texture height/width, default is 0, non-zero means
  /// the texture has the specified width/height. Usually, when the texture type
  /// is GL_TEXTURE_RECTANGLE, we need to specify the texture width/height to
  /// tell the embedder to scale when rendering.
  ClaySize size;
} ClayOpenGLTexture;

typedef struct ClayVulkanImage {
  /// The size of this struct. Must be sizeof(ClayVulkanImage).
  size_t struct_size;
  void* vulkan_image;
  /// User data to be returned on the invocation of the destruction callback.
  void* user_data;
  /// The callback invoked by the engine when it no longer needs this backing
  /// store.
  ClayVoidCallback destruction_callback;
} ClayVulkanImage;

typedef struct ClayOpenGLFramebuffer {
  /// The size of this struct. Must be sizeof(ClayOpenGLFramebuffer).
  size_t struct_size;

  /// The target of the color attachment of the frame-buffer. For example,
  /// GL_TEXTURE_2D or GL_RENDERBUFFER.
  uint32_t target;

  /// The name of the framebuffer.
  uint32_t name;

  /// User data to be returned on the invocation of the destruction callback.
  void* user_data;

  /// The callback invoked by the engine when it no longer needs this backing
  /// store.
  ClayVoidCallback destruction_callback;
} ClayOpenGLFramebuffer;

typedef struct ClaySharedMemoryImage {
  /// The size of this struct. Must be sizeof(ClaySharedMemoryImage).
  size_t struct_size;

  /// The fd of the shared memory
  int shm_fd;

  /// The size of the shared image.
  int32_t width;
  int32_t height;

  /// User data to be returned on the invocation of the destruction callback.
  void* user_data;

  /// The callback invoked by the engine when it no longer needs this backing
  /// store.
  ClayVoidCallback destruction_callback;
} ClaySharedMemoryImage;

typedef struct ClaySharedImageReadResult {
  /// The size of this struct. Must be sizeof(ClaySharedImageReadResult).
  size_t struct_size;
  ClaySharedImageRepresentationType type;
  union {
    ClayOpenGLTexture opengl_texture;
    ClayMetalTexture metal_texture;
    ClayD3DTexture d3d_texture;
    ClayVulkanImage vulkan_image;
    ClaySharedMemoryImage shm_image;
  };
} ClaySharedImageReadResult;

typedef struct ClaySharedImageWriteResult {
  /// The size of this struct. Must be sizeof(ClaySharedImageWriteResult).
  size_t struct_size;
  ClaySharedImageRepresentationType type;
  union {
    ClayOpenGLFramebuffer opengl_framebuffer;
    ClayMetalTexture metal_texture;
    ClayD3DTexture d3d_texture;
    ClaySharedMemoryImage shm_image;
  };
} ClaySharedImageWriteResult;

/// Currently only support RGBA8 bitmap with premultiplied alpha
typedef struct ClayBitmap {
  size_t struct_size;
  uint32_t width;
  uint32_t height;
  ClayDataHolder pixels;
} ClayBitmap;

typedef struct ClayPointerEvent {
  /// The size of this struct. Must be sizeof(ClayPointerEvent).
  size_t struct_size;
  ClayPointerPhase phase;
  /// The timestamp at which the pointer event was generated.
  size_t timestamp;
  /// The x coordinate of the pointer event in physical pixels.
  double x;
  /// The y coordinate of the pointer event in physical pixels.
  double y;
  /// An optional device identifier. If this is not specified, it is assumed
  /// that the embedder has no multi-touch capability.
  int32_t device;
  ClayPointerSignalKind signal_kind;
  /// The x offset of the scroll in physical pixels.
  double scroll_delta_x;
  /// The y offset of the scroll in physical pixels.
  double scroll_delta_y;
  /// The type of the device generating this event.
  /// Backwards compatibility note: If this is not set, the device will be
  /// treated as a mouse, with the primary button set for `kDown` and `kMove`.
  /// If set explicitly to `ClayPointerDeviceKind::kMouse`, you must set the
  /// correct buttons.
  ClayPointerDeviceKind device_kind;
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
} ClayPointerEvent;

/// A structure to represent a key event.
typedef struct ClayKeyEvent {
  /// The size of this struct. Must be sizeof(ClayKeyEvent).
  size_t struct_size;
  /// The timestamp at which the key event was generated.
  double timestamp;
  /// The event kind.
  ClayKeyEventType type;
  /// The USB HID code for the physical key of the event.
  ///
  /// For the full definition and list of pre-defined physical keys, see
  /// `PhysicalKeyboardKey` from the framework.
  ///
  /// The only case that `physical` might be 0 is when this is an empty event.
  uint64_t physical;
  /// The key ID for the logical key of this event.
  ///
  /// For the full definition and a list of pre-defined logical keys, see
  /// `LogicalKeyboardKey` from `clay/ui/event/keyboard_key.h`.
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
  /// of events (as documented in `ClayKeyEvent`). An example is when a key
  /// up is missed due to loss of window focus, on a platform that provides
  /// query to key pressing status, the embedder might realize that the key has
  /// been released at the next key event, and should construct a synthesized up
  /// event immediately before the actual event.
  ///
  /// An event being synthesized means that the `timestamp` might greatly
  /// deviate from the actual time when the event occurs physically.
  bool synthesized;
} ClayKeyEvent;

/// This information is passed to the headless when requesting a frame buffer
/// object.
typedef struct ClayFrameInfo {
  /// The size of this struct. Must be sizeof(ClayFrameInfo).
  size_t struct_size;
  /// The size of the surface that will be backed by the fbo.
  uint32_t width;
  uint32_t height;
} ClayFrameInfo;

/// Callback for when a frame buffer object is requested.
typedef uint32_t (*ClayFrameInfoCallback)(
    void* /* user data */, const ClayFrameInfo* /* frame info */);
typedef void* (*ClayProcResolver)(void* /* user data */,
                                  const char* /* name */);

typedef bool (*ClaySoftwarePresentCallback)(void* /* user data */,
                                            const void* /* allocation */,
                                            size_t /* row bytes */,
                                            size_t /* height */);

typedef struct ClayOpenGLRendererConfig {
  /// The size of this struct. Must be sizeof(ClayOpenGLRendererConfig).
  size_t struct_size;
  ClayBoolCallback make_current;
  ClayBoolCallback clear_current;
  /// Specifying `present` is required. The return value indicates success of
  /// the present call.
  ClayBoolCallback present;
  /// Specifying the `fbo_callback` or is required. The return value indicates
  /// the id of the frame buffer object that will obtain the gl
  /// surface from.
  ClayFrameInfoCallback fbo_callback;
  ClayProcResolver gl_proc_resolver;

  /// ATTENTION
  /// Host GL mode is very limited. In most cases, you should use
  /// ClayHardwareRendererConfig instead of host gl mode.
  ///
  /// If enable_shared_image_sink is true, host GL will be running as a bitmap
  /// uploader, underneath will render view using a SharedImageSink
  /// and readback GPU memory to a CPU bitmap. It may be SLOW for large views,
  /// but all components will work well.
  ///
  /// Disable shared image sink will disable the CPU bitmap readback, it can
  /// speed normal UI rendering, but components using external textures like
  /// video or canvas can NOT work.
  bool enable_shared_image_sink;
  ClaySharedImageSinkBufferMode
      shared_image_sink_buffer_mode;  // double buffer by default
} ClayOpenGLRendererConfig;

typedef struct ClayHardwareRendererConfig {
  /// The size of this struct. Must be sizeof(ClayHardwareRendererConfig).
  size_t struct_size;
  ClaySharedImageSinkRef sink_ref;
  bool disable_partial_repaint;
} ClayHardwareRendererConfig;

typedef struct ClaySoftwareRendererConfig {
  /// The size of this struct. Must be sizeof(ClaySoftwareRendererConfig).
  size_t struct_size;
  /// The callback presented to the embedder to present a fully populated buffer
  /// to the user. The pixel format of the buffer is the native 32-bit RGBA
  /// format. The buffer is owned by the engine and must be copied in
  /// this callback if needed.
  ClaySoftwarePresentCallback present_callback;
} ClaySoftwareRendererConfig;

typedef enum {
  /// ClayOpenGLRendererConfig
  /// Use Host envrionment passed OpenGL callbacks
  kClayRendererTypeHostGL,

  /// ClayHardwareRendererConfig
  kClayRendererTypeOpenGL,  /// native GL
  /// Metal is only supported on Darwin platforms (macOS / iOS).
  /// iOS version >= 10.0 (device), 13.0 (simulator)
  /// macOS version >= 10.14
  kClayRendererTypeMetal,
  kClayRendererTypeVulkan,

  /// ClaySoftwareRendererConfig
  kClayRendererTypeSoftware,
} ClayRendererType;

typedef struct ClayHeadlessRendererConfig {
  ClayRendererType type;
  union {
    ClayOpenGLRendererConfig opengl;
    ClayHardwareRendererConfig hardware;
    ClaySoftwareRendererConfig software;
  };
} ClayHeadlessRendererConfig;

typedef struct ClayTaskRunner_* ClayTaskRunner;

typedef struct ClayTask {
  ClayTaskRunner runner;
  uint64_t task;
} ClayTask;

typedef void (*ClayTaskRunnerPostTaskCallback)(ClayTask /* task */,
                                               uint64_t /* target time nanos */,
                                               void* /* user data */);

/// An interface used by the Headless engine to execute tasks at the target time
/// on a specified thread. There should be a 1-1 relationship between a thread
/// and a task runner. It is undefined behavior to run a task on a thread that
/// is not associated with its task runner.
typedef struct ClayTaskRunnerDescription {
  /// The size of this struct. Must be sizeof(ClayTaskRunnerDescription).
  size_t struct_size;
  void* user_data;
  /// May be called from any thread. Should return true if tasks posted on the
  /// calling thread will be run on that same thread.
  ClayBoolCallback runs_task_on_current_thread_callback;
  /// May be called from any thread.
  ClayTaskRunnerPostTaskCallback post_task_callback;
  /// A unique identifier for the task runner. If multiple task runners service
  /// tasks on the same thread, their identifiers must match.
  size_t identifier;
} ClayTaskRunnerDescription;

//------------------------------------------------------------------------------
// SharedImage
//------------------------------------------------------------------------------
CLAY_EXPORT ClaySharedImageRef ClayCreateSharedImage(
    ClaySharedImageBackingType type,
    ClaySharedImageBackingPixelFormat pixel_format, const ClaySize* size);
CLAY_EXPORT ClaySharedImageRef ClayCreateSharedImageFromHandle(
    ClaySharedImageBackingType type,
    ClaySharedImageBackingPixelFormat pixel_format, const ClaySize* size,
    ClaySharedImageNativeHandle handle);
CLAY_EXPORT ClaySharedImageRef
ClayRetainSharedImage(ClaySharedImageRef shared_image_ref);
CLAY_EXPORT void ClayReleaseSharedImage(ClaySharedImageRef shared_image_ref);
/// Get backing type, format and the native handle from ClaySharedImage
/// The returned handle is not owned. Users must retain the value to persist it.
/// But it's safe to use the handle whenever the shared_image_ref is valid.
CLAY_EXPORT void ClaySharedImageGetBacking(
    ClaySharedImageRef shared_image_ref, ClaySharedImageBackingType* out_type,
    ClaySharedImageBackingPixelFormat* out_format,
    ClaySharedImageNativeHandle* out_handle);

CLAY_EXPORT void ClaySharedImageGetSize(ClaySharedImageRef shared_image_ref,
                                        ClaySize* out);

/// The transformation matrix maps (u, v, 1) to the final tex coord
/// We use OpenGL texture coordinates(Bottom-Left Origin, +Y up) as the
/// standard coordinate system.
///
/// By default, the transformation matrix is the identity matrix.
///
/// For example, to use the graphics buffer as an Skia Image
/// ```
/// auto image = SkImages::BorrowTextureFrom(
///   context, tex,
///   GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType,
///   kPremul_SkAlphaType, nullptr)
/// if (!transform.isIdentity()) {
///   sk_sp<SkShader> shader = image->makeShader(SkTileMode::kRepeat,
///     SkTileMode::kRepeat,
///     sampling,
///     transform);
///
///   SkPaint paintWithShader = paint;
///   paintWithShader.setShader(shader);
///   context.canvas->drawRect(SkRect::MakeWH(1, 1), paintWithShader);
/// } else {
///   context.canvas->drawImage(image, 0, 0, sampling, paint);
/// }
/// ```
CLAY_EXPORT void ClaySharedImageGetTransformation(
    ClaySharedImageRef shared_image_ref, ClayTransformation* out);
/// Write the transformation to the graphics buffer
/// usually used after a cpu or gpu write
CLAY_EXPORT void ClaySharedImageSetTransformation(
    ClaySharedImageRef shared_image_ref,
    const ClayTransformation* transformation);

CLAY_EXPORT ClayFenceSyncRef
ClayCreateExternalFenceSync(ClayVoidCallback wait_callback,
                            ClayVoidCallback delete_callback, void* user_data);

CLAY_EXPORT bool ClayFenceSyncClientWait(ClayFenceSyncRef fence_sync);

CLAY_EXPORT void ClayDestroyFenceSync(ClayFenceSyncRef);

CLAY_EXPORT void ClaySharedImageSetFenceSync(
    ClaySharedImageRef shared_image_ref, ClayFenceSyncRef fence_sync_ref);

CLAY_EXPORT ClayFenceSyncRef
ClaySharedImageGetFenceSync(ClaySharedImageRef shared_image_ref);

CLAY_EXPORT ClaySharedImageSinkRef ClayCreateSharedImageSink(
    ClaySharedImageSinkBufferMode buffer_mode, ClaySharedImageBackingType type,
    ClaySharedImageBackingPixelFormat pixel_format);

CLAY_EXPORT ClaySharedImageSinkRef
ClayRetainSharedImageSink(ClaySharedImageSinkRef sink_ref);

CLAY_EXPORT void ClayReleaseSharedImageSink(ClaySharedImageSinkRef sink_ref);

CLAY_EXPORT ClaySharedImageSinkBufferMode
ClaySharedImageSinkGetBufferMode(ClaySharedImageSinkRef sink_ref);

CLAY_EXPORT void ClaySharedImageSinkSetFrameAvailableCallback(
    ClaySharedImageSinkRef sink_ref, ClayVoidCallback callback,
    void* user_data);

/// It's better to use ClaySharedImageSinkAccessor instead of the raw interfaces
/// below

/// The out ClaySharedImageRef is owned by sink
/// User must call `ClayRetainSharedImage` to retain it
CLAY_EXPORT bool ClaySharedImageSinkUpdateFront(
    ClaySharedImageSinkRef sink_ref, ClayFenceSyncRef produced_fence_sync,
    ClaySharedImageRef* out);

/// Explicitly release the front buffer
/// This function is only required in single buffer mode
CLAY_EXPORT void ClaySharedImageSinkReleaseFront(
    ClaySharedImageRef sink_ref, ClayFenceSyncRef produced_fence_sync);

/// The out ClaySharedImageRef is owned by sink
/// User must call `ClayRetainSharedImage` to retain it
CLAY_EXPORT bool ClaySharedImageSinkAcquireBack(ClaySharedImageSinkRef sink_ref,
                                                const ClaySize* size,
                                                ClaySharedImageRef* out,
                                                uint32_t* out_buffer_age);
CLAY_EXPORT bool ClaySharedImageSinkTryAcquireBack(
    ClaySharedImageSinkRef sink_ref, const ClaySize* size,
    ClaySharedImageRef* out, uint32_t* out_buffer_age,
    ClaySharedImageNativeHandle handle = nullptr);

CLAY_EXPORT bool ClaySharedImageSinkSwapBack(ClaySharedImageSinkRef sink_ref,
                                             ClayFenceSyncRef fence_sync);

CLAY_EXPORT ClaySharedImageSinkAccessorRef ClayCreateSharedImageSinkAccessor(
    ClaySharedImageSinkRef sink_ref,
    const ClaySharedImageRepresentationConfig* config);

CLAY_EXPORT void ClayDestroySharedImageSinkAccessor(
    ClaySharedImageSinkAccessorRef sink_ref);

/// The out ClaySharedImageRef is owned by sink
/// User must call `ClayRetainSharedImage` to retain it
CLAY_EXPORT bool ClaySharedImageSinkRead(
    ClaySharedImageSinkAccessorRef sink_accessor_ref,
    ClaySharedImageRef* out_image, ClaySharedImageReadResult* out);

/// Explicitly notify the read is done
/// This function is only required in single buffer mode
CLAY_EXPORT void ClaySharedImageSinkEndRead(
    ClaySharedImageSinkAccessorRef sink_accessor_ref);

/// The out ClaySharedImageRef is owned by sink
/// User must call `ClayRetainSharedImage` to retain it
CLAY_EXPORT bool ClaySharedImageSinkBeginWrite(
    ClaySharedImageSinkAccessorRef sink_accessor_ref, const ClaySize* size,
    ClaySharedImageRef* out_image, ClaySharedImageWriteResult* out,
    uint32_t* out_buffer_age);

CLAY_EXPORT bool ClaySharedImageSinkEndWrite(
    ClaySharedImageSinkAccessorRef sink_accessor_ref);

//------------------------------------------------------------------------------
// Bitmap
//------------------------------------------------------------------------------
typedef void (*ClayBitmapDecodeCallback)(const char* /* error message */,
                                         const ClayBitmap* /* bitmap */,
                                         void* /* user_data */);
CLAY_EXPORT void ClayDecodeImage(const ClayDataHolder* data_holder,
                                 ClayBitmapDecodeCallback decode_callback,
                                 void* user_data);
CLAY_EXPORT bool ClayDecodeDataUrlImage(const char* data_url, size_t length,
                                        ClayBitmap* out);
CLAY_EXPORT bool ClayEncodeBitmap(const ClayBitmap* bitmap,
                                  ClayImageFormat encoding,
                                  float compress_ratio, ClayDataHolder* out);

CLAY_EXTERN_C_END

#endif  // CLAY_PUBLIC_CLAY_H_
