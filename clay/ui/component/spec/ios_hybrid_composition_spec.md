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
- selecting the backing-surface size for each overlay
- mapping global Clay coordinates into the rendered surface
- positioning and clipping the UIKit wrapper

## 2. Terms

- `overlay_rect`
  - The visible Clay region presented by one overlay surface.
  - It is expressed in global Clay canvas coordinates and physical pixels.
- full-frame backing surface
  - A GPU surface whose size equals `CompositorState::GetFrameSize()`.
- local backing surface
  - A GPU surface whose size equals `overlay_rect.size`.
- backing-surface policy
  - The compositor may select either a full-frame or local backing surface.
  - The selection is independent of the visible meaning of `overlay_rect`.
- overlay view
  - The `FlutterOverlayView` whose `CAMetalLayer` presents the selected backing
    surface.
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

When content represented by the overlay slice is removed from the background
canvas, the same visible rect is used for the difference clip.

An explicit overlay entry can obtain its visible rect directly from the
overlay view bounds instead of a platform-view intersection. This discovery
choice does not redefine the backing-surface policy.

Backing allocation must not be encoded by replacing the visible rect with a
full-frame rect. Doing that would remove unrelated background content and
break multiple platform-view composition.

Primary code paths:

- `lynx/clay/flow/view_slicer.h`
- `lynx/clay/flow/view_slicer.cc`
- `lynx/clay/flow/view_slicer_unittests.cc`

## 4. Backing-Surface Mapping

Every overlay keeps `overlay_rect` as the visible global region. The compositor
selects a backing size and applies this canvas mapping:

```text
surface_size = selected_backing_size
surface_clip = (0, 0, overlay_rect.width, overlay_rect.height)
canvas_translation = -overlay_rect.origin
```

The selected backing size may be either:

```text
full-frame policy: surface_size = CompositorState::GetFrameSize()
local policy:      surface_size = overlay_rect.size
```

The recorded slice keeps global Clay coordinates. The translation moves the
requested global region to the backing surface origin before rasterization.
With a full-frame backing, only the top-left `overlay_rect.size` portion is
populated. With a local backing, that portion is the complete surface.

These values and the UIKit overlay-view frame form one geometry contract:

```text
surface allocation <-> canvas clip <-> canvas translation
                   <-> overlay-view frame
```

Changing only one value causes stretching, misplaced pixels, or an empty
overlay.

`CompositorService` applies the mapping as follows:

1. select the full-frame or local backing-surface policy
2. acquire a surface with the selected size
3. clip to `overlay_rect.size` at the surface origin
4. translate by `-overlay_rect.origin`
5. render the slice into the translated canvas

A full-frame backing remains stable when only the overlay rect changes. A local
backing is not recreated when only the global origin changes because
`AcquireFrame(...)` receives width and height; an actual width or height change
can resize its drawable.

Primary code paths:

- `lynx/clay/shell/common/services/compositor/compositor_service.cc`
- `lynx/clay/shell/common/services/compositor/platform_overlay_service.h`
- `clay/shell/platform/darwin/ios/framework/Source/platform_overlay_service_ios.h`
- `clay/shell/platform/darwin/ios/framework/Source/platform_overlay_service_ios.mm`

## 5. iOS UIKit Presentation

`PresenterServiceIOS::UpdateOverlay(...)` maps physical Clay geometry into the
UIKit hierarchy.

For a regular hybrid-composition platform view, the wrapper frame is the
visible slice converted to points:

```text
wrapper.frame = overlay_rect / UIScreen.scale
```

The wrapper exposes and clips the visible slice.

For a registered system-overlay host, the wrapper is attached to that host and
uses the host bounds. Host selection and backing-surface allocation remain
separate decisions.

For both backing-surface policies, the inner view derives its point size from
the backing surface that was acquired and submitted for the current frame:

```text
overlay_view.frame.origin = (0, 0)
overlay_view.frame.size = CAMetalLayer.drawableSize / CALayer.contentsScale
```

This makes the submitted Metal drawable the source of truth for the inner-view
size, whether the compositor selected a full-frame or local backing.
`use_platform_overlay` selects the UIKit host and wrapper placement; it does
not independently infer the backing-surface policy. If the layer does not
expose drawable size, the presenter falls back to wrapper-local bounds.

Primary code paths:

- `clay/shell/platform/darwin/ios/framework/Source/presenter_service_ios.mm`
- `clay/shell/platform/darwin/ios/framework/Source/platform_overlay_service_ios.mm`
- `clay/shell/platform/darwin/ios/framework/Source/FlutterOverlayView.h`

## 6. Geometry Examples

### 6.1 Visible slice with a full-frame backing

For a three-times-scale device:

```text
overlay_rect:       (0, 1984, 1206, 296) px
screen scale:       3
full frame size:    (1206, 2556) px
surface size:       (1206, 2556) px
wrapper frame:      (0, 661.33, 402, 98.67) pt
overlay view frame: (0, 0, 402, 852) pt
canvas translation: (0, -1984) px
```

The requested slice is rendered into the top-left portion of the full-frame
drawable. The wrapper clips that portion and places it at the slice's global
screen position.

### 6.2 The same visible slice with a local backing

For the same three-times-scale device:

```text
overlay_rect:       (0, 1984, 1206, 296) px
screen scale:       3
surface size:       (1206, 296) px
wrapper frame:      (0, 661.33, 402, 98.67) pt
overlay view frame: (0, 0, 402, 98.67) pt
canvas translation: (0, -1984) px
```

The local drawable and the inner view have the same point-space size. The
wrapper still owns placement and clipping.

## 7. Failure Modes

### 7.1 Local drawable with a larger overlay view

Symptoms:

- content appears enlarged or stretched
- padding and controls extend outside the expected area

Cause:

- UIKit scales an `overlay_rect`-sized drawable across a view representing a
  different region

### 7.2 Full-frame drawable with a slice-sized overlay view

Symptoms:

- content is compressed into a thin strip
- the whole page appears inside one overlay slice

Cause:

- UIKit scales a full-frame drawable into the wrapper-local slice instead of
  clipping a full-frame overlay view

### 7.3 Inconsistent canvas mapping

Symptoms:

- content appears at the wrong location
- the wrapper is visible but its expected pixels are empty

Cause:

- the overlay canvas does not use both the local clip and
  `-overlay_rect.origin` translation

### 7.4 Full-frame `overlay_rect`

Symptoms:

- unrelated background content disappears
- multiple overlays duplicate or reorder Clay content

Cause:

- a visible intersection was replaced with allocation geometry

## 8. Invariants

- `OverlayData.rect` represents visible content in global Clay coordinates.
- background difference clipping uses the same visible rect when the overlay
  slice is removed from the background.
- backing-surface allocation is separate from visible-rect discovery.
- the compositor owns the full-frame or local backing-surface policy.
- the overlay canvas clips to `overlay_rect.size` at the surface origin.
- the overlay canvas translates by `-overlay_rect.origin`.
- the iOS overlay view size follows the submitted drawable size.
- the UIKit wrapper owns host selection, placement, and clipping.
- raster allocation and UIKit presentation must use the same backing size.
- moving a same-sized local overlay does not require a new backing allocation.
- changing a local overlay's width or height may resize the drawable.
- platform overlay geometry must not leak into map or marker implementations.
- component-specific scale or offset workarounds must not compensate for a
  broken generic overlay geometry contract.

## 9. Verification

Required shared tests:

- `SliceViews(...)` returns only the actual intersection rect.
- explicit overlay-layer presentation uses the node's final bounds.
- multiple platform views preserve independent visible overlay rects.
- clip and translation render the requested global slice at surface origin.
- both full-frame and local backing-surface policies preserve the visible
  overlay geometry.

Required iOS runtime checks:

- compare wrapper frame, overlay view frame, and `CAMetalLayer.drawableSize`
- verify a full-frame drawable uses a full-frame inner view inside a clipping
  wrapper
- verify a local drawable uses a matching local inner view
- verify content at the top, center, and bottom of the screen
- verify a same-sized moving overlay remains aligned
- verify a real width or height change resizes without stretching
- verify opacity and visibility changes do not alter geometry
- verify map content above the native map remains aligned and clipped
- verify explicit system overlays match their host bounds
- verify multiple interleaved platform views preserve background and z-order
- use a real device for final scale, Metal composition, memory, and performance
  validation
