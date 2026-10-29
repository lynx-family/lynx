# iOS Hybrid Composition Overlay Spec

This document defines the geometry and ownership contract for Clay iOS hybrid
composition overlays.

It covers Clay content rendered above an embedded UIKit platform view and
explicit Clay overlay layers rendered into a platform host. It does not define
the embedded platform view lifecycle, which remains owned by `NativeView`,
`EmbeddedViewParams`, and the iOS platform-view controller.

## 1. Design Goal

Clay must preserve this visual order:

```text
Clay content below the platform view
-> UIKit platform view
-> Clay content above the platform view
```

The last item cannot remain in the background Clay surface because UIKit draws
the embedded view above that surface. Clay records the content above the native
view into a separate overlay surface and places that surface above the UIKit
view.

The implementation keeps these concerns separate:

- finding the visible overlay slice
- rendering that slice into a local backing surface
- mapping global Clay coordinates into the local surface
- positioning and clipping the UIKit wrapper

## 2. Terms

- `overlay_rect`
  - The visible Clay region presented by one overlay surface.
  - It is expressed in global Clay canvas coordinates and physical pixels.
- local backing surface
  - A GPU surface whose size equals `overlay_rect.size`.
  - Its local origin `(0, 0)` represents `overlay_rect.origin` in the global
    Clay canvas.
- presentation kind
  - `kPlatformView` represents content sliced above an embedded UIKit view.
  - `kOverlayLayer` represents content that explicitly owns an overlay layer.
  - The kind remains internal to `EmbeddedViewParams`; `OverlayData` does not
    need to carry it because all overlays use the same local surface mapping.
- overlay view
  - The `FlutterOverlayView` whose `CAMetalLayer` presents the local surface.
- overlay wrapper
  - A UIKit view that owns placement and clipping in the selected host.

## 3. Overlay Discovery

### 3.1 Layer input

The shared flow starts from:

- `EmbeddedViewParams`, which contains each embedded view's final bounding rect
- `EmbedderViewSlice`, which records Clay drawing after the platform view in
  composition order
- the platform-view composition order

Primary code paths:

- `lynx/clay/flow/embedded_views.h`
- `lynx/clay/flow/layers/platform_view_layer.cc`
- `lynx/clay/flow/view_slicer.cc`

### 3.2 `SliceViews(...)`

`SliceViews(...)` searches the recorded slice for drawn regions that intersect
a platform view. The intersected regions are joined and rounded out to produce
`OverlayData.rect`.

Normative meaning:

```text
OverlayData.rect == overlay_rect == visible intersection in global coordinates
```

When `EmbeddedViewParams` explicitly requires an overlay layer, its final
bounding rect becomes the overlay rect. The same rect is removed from the
background canvas with a difference clip.

The presentation kind only determines how `SliceViews(...)` obtains the visible
rect. It must not be encoded by replacing that rect with a full-frame rect.
Doing that would remove unrelated background content and break multiple
platform-view composition.

Primary code paths:

- `lynx/clay/flow/view_slicer.h`
- `lynx/clay/flow/view_slicer.cc`
- `lynx/clay/flow/view_slicer_unittests.cc`

## 4. Local Surface Mapping

Every overlay request uses one local mapping:

```text
surface_size = overlay_rect.size
surface_clip = (0, 0, overlay_rect.width, overlay_rect.height)
canvas_translation = -overlay_rect.origin
```

The recorded slice keeps global Clay coordinates. The translation moves the
requested global region to the local surface origin before rasterization.

These values form one geometry contract:

```text
surface allocation <-> canvas clip <-> canvas translation
```

Changing only one value causes stretching, misplaced pixels, or an empty
overlay.

The mapping is platform independent and remains in `CompositorService`:

1. acquire a surface with `overlay_rect.size`
2. clip to the local surface bounds
3. translate by `-overlay_rect.origin`
4. render the slice into the translated canvas

A local surface is not recreated when only the global origin changes because
`AcquireFrame(...)` receives width and height. A width or height change can
resize the drawable. Opacity and visibility changes do not change allocation
geometry.

Primary code paths:

- `lynx/clay/shell/common/services/compositor/compositor_service.cc`
- `lynx/clay/shell/common/services/compositor/platform_overlay_service.h`
- `clay/shell/platform/darwin/ios/framework/Source/platform_overlay_service_ios.h`
- `clay/shell/platform/darwin/ios/framework/Source/platform_overlay_service_ios.mm`

## 5. iOS UIKit Presentation

`PresenterServiceIOS::UpdateOverlay(...)` maps physical Clay geometry into the
UIKit hierarchy.

For both a regular hybrid-composition platform view and a registered
system-overlay host, the wrapper frame is the visible slice converted to
points:

```text
wrapper.frame = overlay_rect / UIScreen.scale
```

The host selection is independent from this geometry conversion. A regular
hybrid-composition overlay is attached to the Flutter view, while a system
overlay is attached to its registered host. The system host reports offset
`(0, 0)` and its own dimensions back to Clay, so its explicit overlay rect
converted to points is equal to the host bounds.

In both cases the inner overlay view describes exactly the wrapper-local
surface:

```text
overlay_view.frame = overlay_view_wrapper.bounds
```

The Metal drawable and the overlay view therefore have matching aspect ratios
and coordinate ownership. The wrapper controls placement and clipping; the
inner view does not reintroduce global frame coordinates.

`GetSystemOverlayHostView(...)` selects the UIKit host. It does not select a
separate wrapper-frame or backing-surface policy. The same service reports the
system host's offset and dimensions back to Clay through `NotifyGeometryChanged`.

Primary code paths:

- `clay/shell/platform/darwin/ios/framework/Source/presenter_service_ios.mm`
- `clay/shell/platform/darwin/ios/framework/Source/platform_overlay_service_ios.mm`
- `clay/shell/platform/darwin/ios/framework/Source/FlutterOverlayView.h`
- `clay/shell/platform/darwin/ios/services/overlay_service_ios.mm`

## 6. Geometry Examples

### 6.1 Platform-view slice

For a three-times-scale device:

```text
overlay_rect:       (0, 1984, 1206, 296) px
screen scale:       3
surface size:       (1206, 296) px
wrapper frame:      (0, 661.33, 402, 98.67) pt
overlay view frame: (0, 0, 402, 98.67) pt
canvas translation: (0, -1984) px
```

The local drawable fills the local overlay view without UIKit scaling it to a
different region. The wrapper places the resulting slice at its global screen
position.

### 6.2 System overlay

For a two-times-scale device with an `828 x 1792 px` host:

```text
overlay_rect:       (0, 0, 828, 1792) px
surface size:       (828, 1792) px
wrapper frame:      (0, 0, 414, 896) pt
overlay view frame: (0, 0, 414, 896) pt
```

The surface is still local. It is full-window only because the system host's
own local bounds cover the complete window.

## 7. Failure Modes

### 7.1 Local drawable with a larger overlay view

Symptoms:

- content appears enlarged or stretched
- padding and controls extend outside the expected area

Cause:

- UIKit scales an `overlay_rect`-sized drawable across a view representing a
  different region

### 7.2 Inconsistent canvas mapping

Symptoms:

- content appears at the wrong location
- the wrapper is visible but its expected pixels are empty

Cause:

- the local surface does not use both the local clip and
  `-overlay_rect.origin` translation

### 7.3 Full-frame `overlay_rect`

Symptoms:

- unrelated background content disappears
- multiple overlays duplicate or reorder Clay content

Cause:

- a visible intersection was replaced with allocation geometry

## 8. Invariants

- `OverlayData.rect` represents visible content in global Clay coordinates.
- background difference clipping uses the same visible rect.
- every overlay backing surface uses `overlay_rect.size`.
- the overlay canvas clips to local surface bounds.
- the overlay canvas translates by `-overlay_rect.origin`.
- the iOS overlay view uses its wrapper's local bounds.
- the UIKit wrapper owns host selection, placement, and clipping.
- presentation kind does not change the local surface coordinate contract.
- moving a same-sized overlay does not require a new backing allocation.
- actual width or height changes may resize the drawable.
- platform overlay geometry must not leak into map or marker implementations.
- component-specific scale or offset workarounds must not compensate for a
  broken generic overlay geometry contract.

## 9. Verification

Required shared tests:

- `SliceViews(...)` returns only the actual intersection rect.
- explicit overlay-layer presentation uses the node's final bounds.
- multiple platform views preserve independent visible overlay rects.
- local clip and translation render the requested global slice at local origin.

Required iOS runtime checks:

- compare wrapper frame, overlay view frame, and `CAMetalLayer.drawableSize`
- verify content at the top, center, and bottom of the screen
- verify a same-sized moving overlay remains aligned
- verify a real width or height change resizes without stretching
- verify opacity and visibility changes do not alter geometry
- verify map content above the native map remains aligned and clipped
- verify explicit system overlays match their host bounds
- verify multiple interleaved platform views preserve background and z-order
- use a real device for final scale, Metal composition, memory, and performance
  validation

## 10. SharedImage Delegate Output

### 10.1 Configuration and attachment

Configure output on the main thread through `DelegateRenderConfig` or the
`FlutterView` fields before attachment. The default is display output, three
buffers and an opaque white background. SharedImage accepts two or three
buffers and requires Metal with IOSurface support on iOS 11 or later.
`backgroundColor` uses unpremultiplied ARGB, including alpha.

A new configuration object is applied before the next output attachment and
replaces the view's output mode, buffer count and background color. Direct view
overrides made after that application remain effective while the configuration
object identity is unchanged. Assigning an engine configuration during an active
attachment schedules it for the next attachment. A nil engine configuration
leaves settings to the view. Changing the view's output mode or buffer count
while attached raises an exception.

SharedImage output keeps the UIView subtree hidden. The view remembers the
caller's requested hidden state and restores it when display output is selected.
Supported UIKit content and Clay overlays are composited into SharedImage;
native GPU views require a texture adapter because bitmap capture cannot render
arbitrary UIKit layers.

The output states are:

```text
configured and detached -> attached display or SharedImage -> detached
```

Initial attachment requires a foreground window and a valid viewport. Foreground
GPU-surface recovery preserves the current attachment's view settings and Sink;
it does not consume a pending configuration.

### 10.2 Borrowed Sink lifetime

`getSharedImageSink` returns a borrowed native handle for one in-process
consumer. Call it on the main thread after SharedImage attachment and acquire
an owning native reference synchronously before scheduling asynchronous work.
The getter raises unless the requesting view is the current owner with an
attached SharedImage output. Registering a consumer wakes an otherwise static
scene so its initial frame can be acquired.

Stop using the previous attachment's callbacks and release its consumer
references when detaching. Reattachment creates a new Sink; retaining an old
Sink does not make it the output of the new attachment.

### 10.3 Page reuse barrier

Reuse starts on the main thread after the old output surface is detached.
Concurrent reuse requests are rejected. Retaining the previous provider or its
UIKit views is allowed, so correctness must not depend on their deallocation.

Before publishing a new provider, reset must complete in this order:

1. Invalidate the old native-view generation, remove its views, detach the old
   nested scroll observer and reset presenter state.
2. Clear the old page's UI and shadow nodes on the UI runner.
3. Clear the retained raster layer tree and retained frames on the raster runner.
4. Drain earlier platform work, reset page gesture results, outer scroll direction
   and native fold-scroll state, and finish native-view reuse.
5. Install the new provider and its page service manager, then invoke completion
   on the main thread.

Create the next page's painting/layout contexts and nodes only in completion or
later. Old providers and plugins are invalid owners as soon as reuse starts.
Delayed destruction of an old painting or layout context must preserve the new
page's nodes and delegates. Page resource consumers resolve services from the
current page manager. A retained old scroll must neither remain observed nor
change the new page's gesture state; the new page can bind its own scroll.
