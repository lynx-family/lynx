# Map View Spec (Clay Android and iOS)

This document defines how the map stack reuses the generic native-view contract
from `native_view_spec.md`.

Scope:

- `x-map-ng`
- `x-map-marker-ng`
- Clay subtree snapshot integration used by Android `LynxMapMarker` and iOS
  `BDXLynxMapMarkerNG`

This document only records map-specific behavior.
Generic native-view lifecycle and holder rules are defined in
`native_view_spec.md`.
Generic iOS hybrid-composition overlay geometry is defined in
`ios_hybrid_composition_spec.md`.

## 1. Reuse Goal

The map integration reuses the existing platform map implementations instead of
forking a separate Clay-only map system.

Reused wrapped classes:

- Android `LynxMapView`
- Android `LynxMapMarker`
- iOS `BDXLynxMapNG`
- iOS `BDXLynxMapMarkerNG`

Reused contracts:

- existing props
- existing events
- existing UI methods
- existing marker SDK integration in the wrapped platform marker classes

Clay is responsible for:

- hosting these wrapped UIs through `NativeView`
- computing layout for the marker widget subtree
- rasterizing the marker widget subtree when a Clay snapshot provider is used

## 2. Tag and Behavior Mapping

Current mapping:

| Clay tag | Wrapped behavior tag | Android wrapped UI | iOS wrapped UI |
| --- | --- | --- | --- |
| `x-map-ng` | `x-map-ng` | `LynxMapView` | `BDXLynxMapNG` |
| `x-map-marker-ng` | `x-map-marker-ng` | `LynxMapMarker` | `BDXLynxMapMarkerNG` |

Current implementation details:

- Android `InternalPlatformViewRegistry` bootstrap-registers
  `x-map-marker-ng` as an `InternalPlatformViewWrapper` with
  `ViewDrawType.LOGICAL_ONLY`; `x-map-ng` is admitted dynamically from the
  wrapped behavior tags as a normal drawable platform view
- iOS custom behavior reuse discovers the wrapped map tags from
  `LynxUIClayUIBridge` and routes them through internal platform-view wrappers
- Android and iOS C++ `InternalPlatformViewTags` reserve `x-map-ng`; Android
  and iOS register `x-map-marker-ng` in the C++ Clay element registry
- `x-map-ng` creates the default `NativeView`
- `x-map-marker-ng` creates `MapMarkerView`, a marker-specific `NativeView`
  subclass whose render object is excluded from normal frame layer-tree
  attachment
- `x-map-ng` is intentionally omitted from `InternalPlatformViewShadowNodeTags`
  and `x-map-marker-ng` is registered with no shadow node; both are normal
  layout nodes instead of custom-measurable shadow-node leaves
- `LynxMapView` accepts both legacy Android map event names and Clay-side map
  event names, then emits whichever name was actually bound

## 3. Structural Model

The map stack is intentionally split into two different layers.

### 3.1 Wrapped platform-backed nodes

Wrapped `LynxBaseUI` nodes:

- Android `x-map-ng` -> `LynxMapView`
- Android `x-map-marker-ng` -> `LynxMapMarker`
- iOS `x-map-ng` -> `BDXLynxMapNG`
- iOS `x-map-marker-ng` -> `BDXLynxMapMarkerNG`

These nodes follow the generic internal wrapped `NativeView` contract.

### 3.2 Clay-rendered marker widget subtree

Under `x-map-marker-ng`, normal visual content such as:

- `view`
- `image`
- `text`
- virtual descendants such as `raw-text`

stays in the normal Clay C++ view tree.

Important consequence:

- marker descendants are **not** wrapped into extra `LynxBaseUI` instances
- only the marker root itself is a wrapped native view
- marker bitmap generation snapshots the Clay subtree rooted at the marker
  native view itself

This is the key distinction from earlier discarded approaches that attempted to
wrap the entire marker subtree into nested platform views.

## 4. Layout Model

### 4.1 Why map tags are special

Default internal native views are custom-measurable leaves, which would prevent
marker descendants from participating in layout.

That is not acceptable for the map stack because marker bitmap generation needs
real descendant layout sizes.

### 4.2 Current specialization

Android and iOS C++ tag policy keeps both map nodes without
`NativeViewShadowNode` ownership:

- `ViewRegistry::CreateView(...)` creates `NativeView` for `x-map-ng` through
  `InternalPlatformViewTags`
- the Clay element registry creates `MapMarkerView` for `x-map-marker-ng`
- `ViewRegistry::CreateShadowNode(...)` returns no shadow node for both tags
- `ViewRegistry::GetTagInfo(...)` reports them as normal layout nodes instead
  of custom layout nodes

Required outcome:

- the `x-map-ng` map root still keeps its own layout box
- the marker root still keeps its own layout box
- marker descendants still participate in Yoga layout
- virtual descendants remain virtual only if their underlying shadow node is
  virtual

### 4.3 Marker root layout relay

Android `LynxMapMarker.updateLayout(...)` currently treats Clay as the source of
width and height but intentionally resets position to `(0, 0)` when forwarding
to the wrapped `UIView` base logic.

iOS `LynxClayWrappedPlatformView` also treats Clay as the source of marker
layout, but marker snapshots may be requested before the wrapped marker has a
stable non-zero x-element size. For `x-map-marker-ng`, it may fill missing
layout dimensions from the last non-zero layout, the wrapped `LynxUI` size, or
the content size of wrapped children before forwarding layout to the wrapped
UI.

Practical consequence:

- marker widget size matters for bitmap generation
- marker widget absolute position inside the page does not define the rendered
  bitmap content

## 5. Lifecycle Specialization

### 5.1 Create

Create follows the generic wrapped native-view chain, with map-specific behavior
in the bridge and registry:

- registry pre-registration guarantees the map tags are recognized as internal
  native views
- Android `InternalPlatformViewWrapper` installs a Clay subtree snapshot
  provider into the wrapped UI's `LynxContext`; `LynxMapMarker` reads the
  generic `ClaySnapshotProvider` from context when it needs marker bitmap data
- iOS `LynxUIContext` exposes a generic `claySnapshotProvider`; the provider
  resolves the wrapped `ClayPlatformView` by UI/sign and requests the marker
  subtree snapshot

### 5.2 Insert establishes map ownership

Insert chain:

1. `BaseView::AddChild(...)`
2. `NativeView::OnInsert(...)`
3. `PlatformViewPlugin.onInsertNode(...)`
4. `InternalPlatformViewContext.insertNode(...)`
5. `InternalPlatformViewWrapper.insertChild(...)`
6. `LynxUIClayUIBridgeImpl.insertChild(...)`

Why this matters for maps:

- `LynxMapView.onInsertChild(...)` binds inserted markers back to the map view
- specifically, Android `LynxMapMarker.onAttachedToMapView(...)` stores the map
  owner and retries any pending annotation update
- iOS `BDXLynxMapNG.insertChild(...)` binds
  `BDXLynxMapMarkerNG.map = self`

That binding must exist before an async marker snapshot result is finally
applied.

On iOS, hybrid composition can expose a different ordering from Android: the
marker can be inserted into the map before the marker's `annotation` prop has
arrived. The iOS map implementation must therefore tolerate a marker insert
with no annotation model, and the marker `annotation` setter must enqueue the
annotation for insertion if the map binding already exists.

### 5.3 Layout before marker ready work

Because generic `NativeView::OnNodeReady()` pushes a final `ApplyUpdateChanged()`
before dispatching ready, Java-side marker code can receive up-to-date layout
before `LynxMapMarker.onNodeReady()` runs.

This ordering is relied on by the marker snapshot flow.

### 5.4 NodeReady remains C++-driven

Map nodes do not redefine ready timing.

Current Android chain:

- `PaintingContextClayRef::UpdateNodeReadyPatching(...)`
- `ViewContext::UpdateNodeReadyPatching(...)`
- `NativeView::OnNodeReady()`
- `PlatformViewPlugin.onNodeReady()`
- `InternalPlatformViewWrapper.onNodeReady()`
- `LynxUIClayUIBridgeImpl.onNodeReady()`
- `LynxBaseUI.onNodeReady()`
- `LynxMapView.onNodeReady()` / `LynxMapMarker.onNodeReady()`

Current iOS chain still starts from the Clay native-view lifecycle, but the
wrapped x-element lifecycle is observed through
`LynxClayCustomBehaviorSupport`. The bridge schedules marker snapshot refresh
work from create, insert, layout, nodeReady, child insertion, and explicit
`refreshMarker` method calls.

Map code must not replace the Clay lifecycle with Android `View#onLayout`, iOS
`UIView` layout callbacks, or other platform-only signals.

### 5.5 Marker heavy work is deferred to the next frame

When a Clay snapshot provider is installed, Android
`LynxMapMarker.onNodeReady()` does not immediately generate the marker bitmap.

Current Android policy:

- consume the pending annotation update in `onNodeReady()`
- if running in Clay snapshot mode, request a bitmap through
  `ClaySnapshotProvider`; the provider schedules the actual subtree raster
  snapshot on the next frame
- otherwise use the normal non-Clay path immediately

This keeps ready one-shot and C++-driven while moving snapshot work off the
first ready frame.

Current iOS policy:

- marker snapshot refresh requests are scheduled asynchronously on the main
  queue
- only one marker snapshot may be in flight at a time
- late layout, late annotation delivery, or explicit `refreshMarker` calls
  schedule another request or mark the current request as pending

## 6. Marker Rendering Policy

### 6.1 Current policy

- `x-map-ng` uses normal platform rendering
- `x-map-marker-ng` keeps the full native-view lifecycle and method-routing path
- Android marks the wrapped platform holder as logical-only so platform-layer
  rendering is suppressed
- all platforms create `x-map-marker-ng` with a marker-specific snapshot render
  object; its repaint-boundary root keeps a normal offset layer for subtree
  snapshots, but that layer is not appended to the parent layer during normal
  frame painting

Rationale:

- the marker still needs normal lifecycle, insertion, method routing, and size
  bookkeeping
- but platform-layer rendering and normal-frame Clay painting must be suppressed
  to avoid duplicate marker visuals outside the map SDK-managed marker drawing
  path

### 6.2 Android texture holder behavior

When the marker goes through texture composition:

- `PlatformViewPlugin` creates `PlatformViewWrapperHolder`
- it sets `PlatformViewWrapper.canDraw=false`
- `PlatformViewWrapperHolder.onViewAttached(...)` skips adding the wrapper to
  `FlutterView`

### 6.3 Android hybrid holder behavior

When the marker goes through hybrid composition:

- `PlatformViewPlugin` creates `PlatformViewHybridHolder`
- the holder constructor calls
  `PlatformViewsController.setPlatformViewCanDraw(viewId, false)`
- `PlatformViewsController` records the view id as non-drawable before the
  mutator view is initialized
- `PlatformViewsController` skips adding the mutator view to `FlutterView`; if
  the mutator view is already attached, it is removed when `canDraw=false` is
  applied

Current nuance:

- the hybrid path still keeps the platform-view registration and mutator-view
  infrastructure alive
- platform `canDraw=false` is therefore a render suppression policy, not a
  lifecycle removal policy
- platform `canDraw=false` suppresses only the Android platform layer; Clay
  normal-frame painting is controlled separately by the Clay detached-marker
  render policy
- the bitmap rendered by the map SDK is the marker on the map and follows map
  zoom; any complete marker widget painted outside the map SDK layer is
  Clay-scene leakage

### 6.4 Non-negotiable rule

Android platform `canDraw=false` must only suppress platform-layer rendering.
It must not alter Clay-side child painting semantics. If a native-view-backed
component needs to participate in snapshot rendering but not normal-scene
rendering, that must be represented by a Clay paint policy, not by changing
platform-view attachment behavior.

### 6.5 Clay marker layer-tree policy

`x-map-marker-ng` is created as `MapMarkerView`, a `NativeView` subclass whose
native-view lifecycle is unchanged. Its only render difference is that
`MapMarkerView` marks its normal `RenderExternalContent` render object as
excluded from normal frame layer-tree attachment through
`RenderObject::SetShouldBuildIntoLayerTree(false)`. The root still
owns a normal repaint-boundary offset layer so `PageView::MakeRasterSnapshot`
can build a subtree layer tree from it, but normal frame composition skips
attaching that layer to the parent layer.

Implementation boundary:

- `clay_elements.cc` registers `x-map-marker-ng` as `MapMarkerView` on Android
  and iOS only, with no shadow node; other internal native-view tags still
  create `NativeView`
- `MapMarkerView` uses the normal `NativeView` constructor, then sets
  `RenderObject::SetShouldBuildIntoLayerTree(false)`, so attach, detach,
  layout, insert, nodeReady, platform methods, and snapshot-provider
  installation keep the native-view behavior
- `RenderObject::ShouldBuildIntoLayerTree()` exposes the normal-frame
  layer-tree inclusion decision used by main-frame composition and detached
  snapshot sizing
- `PaintingContext::RepaintCompositedChild(...)` still creates the normal
  `PendingOffsetLayer` for repaint-boundary children
- `PaintingContext::CompositeChild(...)` repaints the marker child layer, then
  returns before parent-layer attachment and normal-frame offset updates when
  `ShouldBuildIntoLayerTree()` is false
- `RenderObject::PaintWithContext(...)` remains generic and always delegates to
  the render object's normal `Paint(...)` implementation
- `RenderExternalContent::Paint(...)` always paints the root `RenderBox`; for a
  detached marker it then paints the Clay children and returns before emitting
  a platform-view layer, drawable-image layer, or punch-hole layer
- `PageView::MakeRasterSnapshot(...)` reads the target render object's layer,
  creates a temporary `FrameBuilder`, calls `BuildSubtreeFrame(layer)`, and
  rasterizes that isolated subtree
- for detached marker roots, the encoded bitmap is still rasterized from
  the physical layer tree, but the callback width and height are forced to the
  marker root's layout size; map SDK consumers use that layout size for marker
  annotation sizing and anchor/offset calculations instead of inferring it from
  device pixels

Required outcome:

- marker layout, native-view lifecycle, insert, nodeReady, and snapshot request
  flow stay intact
- the marker snapshot root layer does not flush to the normal screen frame
- marker Clay descendants still paint when the snapshot pipeline explicitly
  rasterizes the marker root

## 7. Marker Snapshot Pipeline

### 7.1 Provider installation

Android provider installation happens during wrapped marker create:

1. `InternalPlatformViewWrapper.createView(...)`
2. `LynxUIClayUIBridge.tryInstallClaySubtreeRasterSnapshotProvider(...)`
3. `LynxUIClayUIBridgeImpl` installs a `LynxContext.ClaySnapshotProvider`
4. `LynxMapMarker` reads the provider from its `LynxContext`

This keeps Clay Java code free of direct dependencies on specific wrapped UI
classes.

iOS does not install an Android-style Java provider. Instead,
`LynxUIContext.claySnapshotProvider` exposes a generic snapshot request method,
and `LynxClayCustomBehaviorSupport` resolves the target wrapped platform view
for `x-map-marker-ng` by UI/sign before requesting the snapshot.

### 7.2 Native snapshot chain

Current Android chain:

1. `LynxMapMarker` requests a snapshot through the installed provider
2. `InternalPlatformView.requestSubtreeRasterSnapshot(...)`
3. `PlatformViewPlugin.requestSubtreeRasterSnapshot(...)`
4. JNI `RequestSubtreeRasterSnapshot(...)`
5. C++ selects the marker `NativeView` root as the snapshot root
6. `PageView::MakeRasterSnapshot(...)`
7. PNG bytes plus output size return asynchronously
8. `LynxMapMarker` switches back to the UI thread before reading or mutating
   marker state

The Android native callback keeps the asynchronous Java callback as a global
reference. JNI class and byte-array references created while delivering each
result are scoped local references, so success, error, and early-return paths do
not accumulate entries in the thread's local-reference table.

Current iOS chain:

1. `LynxClayCustomBehaviorSupport` observes marker create, insert, layout,
   nodeReady, and attribute updates
2. the marker bridge requests a generic subtree raster snapshot through
   `LynxUIContext.claySnapshotProvider`; because iOS
   hybrid composition can miss parts of the wrapped x-element lifecycle, snapshot
   readiness is validated by the empty-result retry path instead of requiring a
   completed marker layout callback first
3. the provider resolves the wrapped `ClayPlatformView` by UI/sign, and the
   `ClayPlatformView` internal category forwards the request through
   `NativeViewPluginIOS`
4. C++ selects the marker `NativeView` root as the snapshot root
5. `PageView::MakeRasterSnapshot(...)` returns PNG bytes and output size
   asynchronously; for detached map marker snapshots the output size is the marker
   layout size, not the encoded image pixel size
6. the iOS bridge decodes the bitmap, derives `UIImage.scale` from the decoded
   image pixels divided by the current marker layout size, sets the snapshot
   image on the wrapped marker `UIImageView`, and invokes `refreshMarker`

Current root-selection rule:

- marker `NativeView` root

Rationale:

- the marker root size is the bitmap contract passed back to the map SDK
- child-local offsets under the marker root must be preserved
- the root's page/global offset should be normalized by the snapshot path rather
  than copied into the bitmap content
- normal Clay descendants under the marker must remain in the C++ component tree
  instead of becoming nested wrapped platform nodes

The marker is a repaint boundary, and `RepaintCompositedChild(...)` establishes
its `PendingOffsetLayer`. `PageView::MakeRasterSnapshot(...)` relies on this
rendering invariant directly; it does not identify the layer through its debug
name.

### 7.3 Snapshot preconditions

A marker snapshot can only succeed if:

- the marker has a Clay child subtree when visual content is expected
- the marker root has non-zero width and height
- descendant layout has already run
- the marker has not been destroyed during the async operation

### 7.4 Snapshot paint pass

`PageView::MakeRasterSnapshot(...)` does not depend on a special global paint
pass. It snapshots by starting from the target view's own repaint-boundary layer:

Current code boundary:

- normal frame: `PaintingContext::CompositeChild(...)` skips appending the
  detached marker child layer to its parent, so the marker subtree is not part
  of the screen layer tree
- screenshot: `PageView::MakeRasterSnapshot(...)` resets the target layer
  offset, creates a temporary `FrameBuilder`, calls
  `FrameBuilder::BuildSubtreeFrame(layer)`, and rasterizes that isolated layer
  tree

This local subtree build is what allows `x-map-marker-ng` to render through
marker snapshots without painting into the main scene.

Layer tree construction details:

1. Normal painting calls `RepaintCompositedChild(...)` for the marker repaint
   boundary, so the marker owns a `PendingOffsetLayer` even though it will not
   be attached to the visible parent layer.
2. `RenderObject::PaintWithContext(...)` follows the generic paint path and
   invokes `RenderExternalContent::Paint(...)`.
3. `RenderExternalContent::Paint(...)` records the marker root's box and Clay
   descendants, but skips external platform content when
   `ShouldBuildIntoLayerTree()` is false.
4. `PaintingContext::CompositeChild(...)` returns after repainting the pending
   layer when `ShouldBuildIntoLayerTree()` is false. This is the normal-frame
   attachment suppression point.
5. `PageView::MakeRasterSnapshot(...)` takes that same pending layer as the
   snapshot root, saves its current offset, normalizes the offset to `(0, 0)`,
   wraps the subtree in a temporary transform layer for the requested scale,
   and restores the original offset after building the subtree frame.
6. `FrameBuilder::BuildSubtreeFrame(...)` updates add-to-frame state from that
   subtree root and calls `PendingLayer::AddToFrame(...)`; unlike
   `BuildFrame(...)`, it does not require the root layer to have no parent,
   because snapshot roots are normally owned by the live page render tree.
7. The temporary `LayerTree` is passed to the raster snapshot delegate and is
   never submitted as the page frame.

## 8. Marker Async Update Policy

### 8.1 Unified refresh behavior

Current Android `LynxMapMarker.refreshMarker(...)` behavior:

- annotation, layout, attachment, and explicit refresh all converge on the same
  asynchronous `updateAnnotation(...)` and `requestMarkerBitmap(...)` path
- `requestMarkerBitmap(...)` is the only place that selects the bitmap source:
  a context-installed Clay provider is asynchronous, while the normal Android
  view-tree bitmap is generated synchronously and returned through the same
  callback
- if the marker SDK object does not exist yet but its annotation and map owner
  are available, rebuild it through that same update path; callers do not branch
  on render mode

Current iOS `BDXLynxMapMarkerNG.refreshMarker(...)` behavior:

- map insertion, annotation updates, and explicit refresh use the same
  `refreshMarkerWithCallback:` path; no render-mode state is cached on the map
  or marker
- provider selection is confined to that request stage: without a provider,
  normal UIKit layout completes and the callback returns immediately; with a
  provider, the marker waits for the asynchronous Clay subtree image
- the marker image is produced by the Clay subtree snapshot path and stored on
  the marker `UIImageView`
- `refreshMarker` reports an invalid state instead of succeeding when the map
  owner, annotation model, or renderable marker content is still missing
- before refreshing the SDK annotation image, `BDXLynxMapNG` flushes any pending
  add/remove/update batches that were queued by insert or late annotation
  delivery
- custom annotations are refreshed through
  `BDXMapOverlayUpdateTypeRefresh`; the marker view is kept bound to the map
  SDK annotation while the `UIImageView` image changes underneath it
- this preserves the successful MapKit custom-view refresh path while still
  tolerating iOS hybrid composition ordering where map binding, annotation
  delivery, and snapshot readiness can arrive in different frames

### 8.2 Coalescing and in-flight rules

Current Android policy:

- `LynxMapMarker` keeps only marker readiness and annotation state; it does not
  own next-frame scheduling, in-flight tracking, or snapshot retry state
- `ClaySnapshotProvider` posts each raster snapshot request to the next frame
  and retries empty/error results up to the shared retry limit
- PNG decode remains in the Clay bridge; marker code receives only the async
  bitmap callback and applies it through `updateAnnotationWithBitmap(...)`

Current iOS policy in `LynxClayWrappedPlatformView`:

- only one subtree snapshot request may be in flight at a time
- additional requests while in flight are collapsed into a pending flag
- an empty snapshot result retries on the next frame up to the current retry
  limit
- successful snapshot application resets the retry counter and invokes
  `refreshMarker`

### 8.3 Prerequisite waiting

Before applying a snapshot result, the marker waits for:

- Android `mLynxMapView` or iOS `BDXLynxMapMarkerNG.map` to be available
- the snapshot provider to exist when the Clay snapshot path is selected
- valid PNG bytes and non-zero output size
- on iOS, a wrapped platform view that can be resolved by marker UI/sign
- on iOS, an output pixel size that is consistent with the marker point size

The map-owner wait depends on the normal insert chain described earlier.

The iOS bridge must not use decoded `UIImage.size` as a layout fallback for
`x-map-marker-ng`: decoded snapshot images are pixel-backed, while the marker
custom annotation view expects UIKit point size. For detached map marker
snapshots, `PageView::MakeRasterSnapshot(...)` returns physical PNG bytes and
reports the marker layout size in Clay logical pixels. iOS converts that
reported size directly to UIKit points, then derives `UIImage.scale` from
decoded pixel width divided by the target point width.

On iOS, if snapshot application succeeds but `refreshMarker` still reports that
the map owner, annotation model, or renderable marker content is not ready, the
Clay bridge retries the refresh on the next frame. Late map binding and late
annotation delivery also trigger another refresh when a snapshot image is
already present. The wrapped `BDXLynxMapMarkerNG` keeps this refresh as an
idempotent readiness check so the map insertion and marker annotation setters do
not depend on the short retry window in `LynxClayCustomBehaviorSupport`.

### 8.4 Retry policy

If snapshot data is not ready yet:

- retry on the next frame
- maximum retry count is currently `3`

### 8.5 Decode/apply threading

Current Android threading split:

- decode PNG bytes on a single-thread background executor
- switch back to UI thread through `UIThreadUtils.runOnUiThread(...)`
- only then call `updateAnnotationWithBitmap(...)`

Current iOS threading split:

- the Clay bridge schedules and retries the wrapped marker `refreshMarker`
  UI method on the main queue
- `BDXLynxMapMarkerNG.refreshMarker` requests snapshot bytes through
  `LynxUIContext.claySnapshotProvider`
- snapshot bytes are delivered back to the main queue by the
  `ClayPlatformView` internal category
- the wrapped marker decodes bytes into a `UIImage`, stores it on the marker
  `UIImageView`, and then asks `BDXLynxMapNG` to refresh the annotation model

## 9. Non-Clay Fallback

On Android, if no Clay snapshot provider is installed:

- `LynxMapMarker` falls back to the normal `genBitmap()` implementation
- bitmap content comes from the Android-side view tree instead of a Clay
  subtree

On iOS, pages that do not enter the Clay marker snapshot bridge keep the normal
wrapped map-marker behavior and do not use `requestSubtreeRasterSnapshot...`.

This preserves backward compatibility for non-Clay or mixed environments.

## 10. Map-Specific Invariants

- Do not fork map behavior away from the wrapped platform map classes unless
  the reuse strategy is intentionally being abandoned.
- `x-map-ng` must keep reusing the wrapped platform map behavior.
- `x-map-marker-ng` must remain a wrapped platform marker native view; its
  descendants remain normal Clay nodes.
- marker descendants must participate in Clay layout so snapshot size is not
  zero.
- insert must establish the Android `mLynxMapView` or iOS
  `BDXLynxMapMarkerNG.map` before async marker snapshot results are applied.
- iOS marker insert must not require the annotation model to exist yet; late
  annotation delivery must enqueue an add batch and be flushed before refresh.
- marker bitmap generation must not block the first nodeReady frame.
- Android platform `canDraw=false` must suppress duplicate platform-layer
  marker rendering without removing the underlying lifecycle or sizing path.
- `x-map-marker-ng` must be created as `MapMarkerView` through the Clay element
  registry, not as a tagged special case inside `ViewRegistry` or the generic
  `NativeView` constructor.
- marker snapshot render objects must suppress normal-scene Clay leakage by
  skipping parent layer attachment without suppressing
  `PageView::MakeRasterSnapshot(...)`.
- generic `RenderObject` painting must not contain map-marker snapshot policy;
  external-self suppression belongs to `RenderExternalContent`.
- detached marker painting must preserve the root box and Clay descendants while
  omitting platform-view, drawable-image, and punch-hole content.
- snapshot offset normalization must restore the live repaint-boundary layer's
  original offset after the temporary subtree frame is built.
- iOS map overlays depend on the generic HC contract: `OverlayData.rect`
  remains the visible global-coordinate slice, while the local Metal backing
  surface, local overlay view, and clipping UIKit wrapper describe that slice.
- map and marker implementations must not compensate for an HC surface/view
  geometry mismatch with component-specific scale, offset, or overlay bounds.
- if tag mapping, map event compatibility, platform draw policy, Clay draw policy,
  snapshot-root selection, retry policy, or async apply rules change, update
  this spec together with the code.

## 11. Validation Scope

Validation for this specialization should cover the component contracts rather
than a particular application, URL, build artifact, or test environment.

The required coverage is:

- `x-map-marker-ng` retains the normal `NativeView` lifecycle while remaining
  detached from the visible Clay layer tree.
- The marker subtree participates in layout and can be rasterized after its
  repaint-boundary layer is ready.
- Snapshot generation returns encoded image data together with the dimensions
  defined by the platform snapshot contract.
- The map component applies the snapshot as the marker annotation image and
  does not also paint the marker subtree into the Clay scene.
- Marker insertion, annotation delivery, layout changes, and snapshot
  completion can occur asynchronously without losing the latest refresh.
- Invalid or not-yet-ready snapshot state fails without leaving an in-flight
  request permanently pending.

## 12. Verification Invariants

### 12.1 Android

- Clay installs a subtree snapshot provider on the wrapped marker UI.
- A logical-only marker does not contribute visible platform-view drawing.
- Snapshot bytes decode to a non-empty bitmap before the map annotation is
  created or refreshed.
- The callback dimensions for a detached marker use its logical layout bounds,
  while the encoded image retains the physical pixel density.
- Repeated marker updates replace the existing annotation image instead of
  creating duplicate visible markers.

### 12.2 iOS

- The wrapped platform view forwards snapshot requests to the marker's Clay
  render object.
- Snapshot image data contains physical pixels, while the reported detached
  marker dimensions preserve the logical layout size.
- The image scale represents physical pixel density, and the annotation view
  size remains expressed in UIKit points.
- Snapshot completion refreshes the existing map annotation after both the map
  and annotation model are available.
- A pending refresh is replayed after the current snapshot completes, without
  duplicating the marker outside the map.
- Hybrid-composition overlay validation follows
  `ios_hybrid_composition_spec.md`, including local drawable/view agreement and
  wrapper placement and clipping.
