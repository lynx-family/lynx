# Desktop output color spaces

Desktop embedders can opt into Display P3. The default remains sRGB. Both are SDR
RGB spaces with a D65 white point and the sRGB transfer function. A pixel format
such as BGRA8888 does not, by itself, identify the color space.

Page colors retain their existing sRGB interpretation. Rendering converts them,
images, and external textures into the output space before composition. Selecting
P3 output does not reinterpret existing page colors as P3 colors.

## Window output

For a windowed LynxView, call
`LynxView::Builder::SetColorSpace(kLynxColorSpaceDisplayP3)` before `Build()`.
The C equivalent is `lynx_view_builder_set_color_space`.

Set `FlutterEngine.enableDisplayP3` before starting a macOS Metal engine.
On Windows set `FlutterDesktopEngineProperties::enable_display_p3` before
creating the engine. Desktop software windows present in sRGB.

On macOS, the Metal render target and its IOSurface carry the same encoding.
Core Animation receives an IOSurface tagged with `kCGColorSpaceDisplayP3` and
converts it for the destination display. BGRA8Unorm remains the SDR storage format.

Windows uses a Display P3 working texture and converts it to linear extended
sRGB in an FP16 DirectComposition swap chain. The conversion preserves negative
and greater-than-one RGB channels and premultiplied alpha. The swap chain declares
`DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709`. The Windows compositor manages display
conversion. If the presenter cannot support this path, window output uses sRGB.
Software-rendered Windows windows also use sRGB. The P3 path requires D3D11.1,
DXGI 1.4 and the system D3D shader compiler. Capability detection and ANGLE import
validation run before rendering starts. Resizing keeps the selected color space.

The output choice is fixed during renderer initialization. Create a new renderer
to change it, so cached surfaces and partially repainted buffers keep one encoding.

## Windowless and C APIs

On macOS and Windows, call `LynxWindowlessRenderer::SetColorSpace` before
attaching the renderer to a view. The C entry point is
`lynx_windowless_renderer_set_color_space`. Software, GL, GLDirect and
Accelerated use the chosen space. The default is sRGB, and subsequent
changes are rejected once the renderer configuration is consumed. The host
interprets the pixels it receives in that space. Use the following Clay entry points.

- `ClayCreateSharedImageSinkWithColorSpace` for hardware output
- `ClaySoftwareRendererConfig::color_space` for software output
- `ClayOpenGLRendererConfig::color_space` for a host-owned FBO.

The fields appended to configuration structs are read according to `struct_size`.
Older configurations use sRGB. Existing callback layouts are unchanged.

## NativeView and external textures

A desktop `LynxNativeView` producer can declare its source encoding with
`SetSurfaceColorSpace` and read it with `GetSurfaceColorSpace`.
For `x-texture-view`, invoke `setsurfacecolorspace` with a `colorSpace` of
`"display-p3"` or `"srgb"`. A successful `acquiresurface` reports that
encoding alongside the handle. Finish the write with `swapbuffer` before
switching spaces. For `update`, declare the encoding of the incoming
IOSurface or D3D shared handle before importing it.

Override `NativePlatformView::color_space()` to declare the encoding of the
producer's pixels. On macOS and Windows a producer can call
`UpdateSurfaceColorSpace` between frames and update its declaration after
success. Serialize this operation with acquiring, presenting and swapping a
buffer. A pending write must finish first. Queued frames retain their encoding,
and newly allocated buffers need a full redraw.

For other producers, use `ClayCreateSharedImageWithColorSpace`,
`ClayCreateSharedImageFromHandleWithColorSpace`, or the sink creation API.
`ClaySharedImageGetColorSpace` retrieves the declaration. Importing a handle does
not convert its pixels or modify metadata on the producer's native object. Native
handles exported without their color-space declaration are insufficient for
portable color interpretation.

Windows D3D11 shared textures retain the declared space when allocated or
imported, including windowless output. D3D handles do not carry this metadata;
the host must propagate it separately. Display P3 shared textures do not fall
back to D3D9, which cannot preserve this contract.

SharedImage representations retain this metadata through NativeView composition.
P3 sources remain P3 on P3 output and convert to sRGB, with SDR gamut clipping, on
sRGB output. An sRGB producer keeps sRGB colors when drawn on P3 output.

Metal uses native texture import. macOS Skity GL contexts that cannot import an
IOSurface, including SwiftShader, use a CPU copy carrying the same source-space
metadata. This allows NativeView composition across the native/GL boundary.

## Images and caches

Clay preserves declared Skity source spaces when copying, resizing and uploading
decoded pixels. Render targets and raster caches use the output space. The
portable Skity PNG codec can read and write PNG chromaticities and gamma.
PNG images that specify Display P3 only through an ICC profile and JPEG ICC
profiles require additional decoder support. Skia uses its own image color
management.

## Validation

The macOS `color_space_unittests` target covers Metal rendering, buffer rotation,
partial repaint, external texture conversion and Core Animation presentation.
The SwiftShader tests cover software callbacks and older configuration sizes.

The Windows `native_view_color_space_unittests` target covers builder defaults,
texture method dispatch, D3D shared-handle import and buffer allocation failures.
On Windows 10+, `p3_presenter_unittests` uses D3D11 WARP to run the actual
conversion shader and read back FP16 output. It covers primaries outside sRGB,
premultiplied alpha, transfer-function decoding, immediate-context state
restoration, invalid targets, partial source updates and resized textures.
Display validation additionally requires a Windows wide-gamut display: compare
tagged P3 and sRGB content, transparent windows, resizing, and sRGB fallback.
