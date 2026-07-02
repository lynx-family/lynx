# Native View Spec (Clay Android)

This document describes the current Clay Android contract for `NativeView`.

It is a spec for the implemented behavior, not a historical design note.
Whenever the C++, JNI, Java holder, or bridge layers change in a way that
alters lifecycle, layout, rendering, or snapshot semantics, this document must
be updated in the same change.

Scope:

- generic `NativeView` behavior in Clay Android
- lifecycle alignment across C++, JNI, Java platform-view glue, and wrapped
  `LynxBaseUI` instances
- rendering mode and holder selection
- optional Clay subtree raster snapshot support

Out of scope:

- map-specific tag policy and marker bitmap behavior
- product-specific tag aliases beyond the generic wrapped-view mechanism

Those live in `map_view_spec.md`.

## 1. Design Goal

The intended lifecycle is:

`create -> insert -> attach -> props/events -> layout -> nodeReady -> detach -> destroy`

The lifecycle is C++-driven.
Java must not manufacture equivalent states from unrelated Android callbacks.

## 2. Terms

- `NativeView`
  - A Clay component node whose platform representation is managed by the
    Android platform-view pipeline.
- internal wrapped native view
  - A `NativeView` whose Java-side implementation is `InternalPlatformView`
    backed by a wrapped `LynxBaseUI`.
- external native view
  - A `NativeView` routed to a non-internal `PlatformViewContext`.
- `InternalPlatformViewWrapper`
  - The Java wrapper that creates a `LynxBaseUI`, applies props/events/layout,
    proxies lifecycle, and optionally installs subtree snapshot support.
- nodeReady patching
  - The ready-id flush sent from C++ through
    `PaintingContextClayRef::UpdateNodeReadyPatching(...)` and
    `ViewContext::UpdateNodeReadyPatching(...)`.
- platform-layer rendering
  - Android-side rendering performed through `PlatformViewWrapper`,
    `FlutterMutatorView`, VirtualDisplay, or equivalent holder infrastructure.
- platform `canDraw=false`
  - A mode where the platform view lifecycle and sizing still exist, but
    platform-layer attachment and drawing are suppressed.
- Clay subtree raster snapshot
  - A raster image generated from a Clay subtree through
    `PageView::MakeRasterSnapshot(...)`.

## 3. Architecture

### 3.1 C++ component layer

Primary classes:

- `clay::NativeView`
  - owns the component-side lifecycle
  - caches staged attributes
  - pushes layout changes to the platform plugin from `OnPainting()`
  - pushes a final layout update again from `OnNodeReady()` before dispatching
    ready
- `clay::ViewContext`
  - owns the view tree
  - establishes parent/child relationships through `AddChild(...)`
  - dispatches nodeReady patching to the correct `BaseView`
- `clay::PageView`
  - provides `MakeRasterSnapshot(...)`
- `LayoutContextClay`
  - owns layout-node creation and measurability policy

### 3.2 JNI / platform service layer

Primary class:

- `NativeViewPluginAndroid`
  - JNI bridge between `clay::NativeView` and Java `PlatformViewPlugin`
  - creates the Java plugin object
  - forwards create/insert/attach/detach/layout/nodeReady/measure/method calls
  - exposes `RequestSubtreeRasterSnapshot(...)` for optional subtree capture

### 3.3 Java platform-view layer

Primary classes:

- `PlatformViewPlugin`
  - Java entry point for each `NativeView`
  - chooses internal vs external `PlatformViewContext`
  - creates the holder path
  - applies platform draw policy such as `canDraw=false` for selected tags
- `PlatformViewHolder`
  - holder abstraction for texture, hybrid, or virtual-display composition
- `PlatformViewContext`
  - per-context interface for create/insert/layout/props/nodeReady/destroy
- `InternalPlatformViewContext`
  - internal context used for wrapped `LynxBaseUI` reuse
- `InternalPlatformViewRegistry`
  - registry of internal view factories
  - pre-registers internal wrapped tags so Java can choose the internal context
    and expose bootstrap tags and composition preferences to Clay
- `InternalPlatformView`
  - Java-side internal node abstraction
  - `onLayoutFinish()` remains only as a compatibility alias; the normative
    lifecycle signal is `onNodeReady()`
- `InternalPlatformViewWrapper`
  - generic wrapped-`LynxBaseUI` implementation
  - owns prop/event dispatch, layout-info relay, nodeReady gate, and optional
    snapshot-provider installation

### 3.4 `lynx_clay` bridge layer

Primary classes:

- `LynxUIClayUIBridge`
  - typed bridge interface used by Clay Java code
- `LynxUIClayUIBridgeImpl`
  - concrete bridge in `platform/android/lynx_clay`
  - creates `LynxBaseUI` instances from behavior tags
  - reuses existing Lynx runtime APIs for props, events, insert/remove,
    layout-info, lifecycle, and UI methods
  - optionally installs subtree snapshot providers for supported wrapped UIs

## 4. Create Contract

### 4.1 C++ create

`NativeView` creation begins in the constructor:

1. `NativeViewService::CreateNativeViewPlugin(...)`
2. `plugin.OnCreate(tag)`
3. plugin reports:
   - shared image sink, if any
   - hybrid-composition support
   - scrolling support
   - availability
4. `NativeView` configures `RenderExternalContent` accordingly

Important current behavior:

- if a shared image sink is available, `NativeView` registers an external
  texture and sets `RenderExternalContent::kExternalTexture`
- if no shared image sink exists but hybrid composition is supported,
  `RenderExternalContent` uses the platform-view id instead

### 4.2 Java create

Java create is driven by `PlatformViewPlugin.onCreateView(...)`:

1. resolve whether the tag is internal or external
2. choose the `PlatformViewContext`
3. `platformViewContext.createNode(...)`
4. `platformViewContext.getViewInNode(...)`
5. create a holder path
6. apply any tag-specific render-mode policy

For internal wrapped views the create path continues as:

1. `InternalPlatformViewRegistry.createView(...)`
2. `InternalPlatformViewWrapper.createView(...)`
3. `LynxUIClayUIBridge.createLynxUI(...)`
4. `LynxUIClayUIBridge.setSign(...)`
5. optionally install `LynxContext.ClaySnapshotProvider` through
   `tryInstallClaySubtreeRasterSnapshotProvider(...)`
6. `LynxUIClayUIBridge.getView(...)`

The create contract completes only after a real Android `View` exists.

## 5. Insert Contract

Insert is triggered only after the Clay parent/child relation has already been
established in C++:

1. `BaseView::AddChild(...)`
2. `NativeView::OnInsert(parentId, index)`
3. JNI `NativeViewPluginAndroid::OnInsert(...)`
4. Java `PlatformViewPlugin.onInsertNode(...)`
5. `PlatformViewContext.insertNode(child, parent, index)`

For internal wrapped views:

6. `InternalPlatformViewWrapper.insertChild(...)`
7. `LynxUIClayUIBridge.insertChild(parent, child, index)`

Current bridge behavior:

- calls `parent.insertChild(child, safeIndex)` to preserve logical Lynx parent
  ownership
- ports the relevant `LynxUIOwner` draw-list insertion logic so the wrapped
  child participates in Lynx-side draw ordering when applicable

Normative requirement:

- platform render policy must never break the logical insert relation

## 6. Attach / Detach Contract

### 6.1 Attach

Attach is driven from C++ tree attachment:

1. `NativeView::OnAttachToTree()`
2. `plugin.OnAttach()`
3. Java `PlatformViewPlugin.onViewAttached()`
4. holder attaches to `FlutterView` if its holder policy requires it
5. `PlatformViewContext.onViewAttached(viewId)`
6. internal wrapper forwards `LynxUIClayUIBridge.onAttach(...)`

### 6.2 Detach

Detach is symmetrical:

1. `NativeView::OnDetachFromTree()`
2. `plugin.OnDetach()`
3. Java `PlatformViewPlugin.onViewDetached()`
4. holder detaches from `FlutterView` if present
5. `PlatformViewContext.removeNode(viewId)`
6. internal wrapper forwards `LynxUIClayUIBridge.onDetach(...)`

Current wrapper contract:

- attach is one-shot per attach cycle, not per create
- detach may happen multiple times before destroy
- destroy must still issue a detach if the view is currently attached

## 7. Props / Events Contract

Props and events are batched from C++ staged attributes:

1. `NativeView::DidUpdateAttributes()`
2. `plugin.UpdatePlatformAttributes(staging_attrs, events)`
3. Java `PlatformViewPlugin.onPlatformViewAttributesUpdate(...)`
4. `PlatformViewContext.updateProps(...)`

For internal wrapped views:

5. `InternalPlatformViewWrapper.setAttribute(...)` for each prop
6. `InternalPlatformViewWrapper.setEvents(...)`
7. `InternalPlatformViewWrapper.onAttributesUpdated()`

Current wrapper semantics:

- `onAttributesUpdated()` does not dispatch nodeReady directly
- it only marks that a full props/events batch has completed
- nodeReady can fire only after both the native ready signal and the props batch
  are available

## 8. Layout Contract

### 8.1 C++ to Java layout flow

Platform layout comes from Clay bounds, not from Android `View#onLayout`:

1. `NativeView::OnPainting()`
2. `NativeView::ApplyUpdateChanged()`
3. `ContentBoundsInViewport()` is converted to platform units
4. `plugin.LayoutChanged(left, top, width, height)`
5. Java `PlatformViewPlugin.onLayout(...)`
6. holder updates layout / returns buffer size
7. `PlatformViewContext.onLayout(...)`

Padding is relayed separately through `plugin.UpdatePaddings(...)` and merged on
Java side.

### 8.2 Internal wrapped-view layout relay

For `InternalPlatformViewWrapper`:

- layout and padding values are cached independently
- `updateLayoutInfoIfNeeded(...)` merges cached values with current Android view
  fallbacks when needed
- missing values default to `0`
- duplicate dispatches are suppressed by comparing against the last dispatched
  tuple

Bridge-side behavior in `LynxUIClayUIBridgeImpl.updateLayoutInfo(...)`:

- call `updateLayoutSize(width, height)` first
- preserve margin, border, and bound fields not supplied by Clay
- call `updateLayout(...)` with Clay-provided position/size/padding plus the
  preserved fields

### 8.3 Layout before nodeReady

Current `NativeView` behavior guarantees a final layout push before ready:

- `NativeView::OnNodeReady()` first calls `ApplyUpdateChanged()`
- only then does it call `plugin.OnNodeReady()`

This means Java can observe `onLayout(...)` before `onNodeReady()` for the same
patch, even when the last size change happens right before ready flush.

## 9. NodeReady Contract

NodeReady is C++-driven.

Reference chain for wrapped internal native views:

1. `PaintingContextClayRef::UpdateNodeReadyPatching(...)`
2. `ViewContext::UpdateNodeReadyPatching(...)`
3. `NativeView::OnNodeReady()`
4. JNI `NativeViewPluginAndroid::OnNodeReady()`
5. Java `PlatformViewPlugin.onNodeReady()`
6. `PlatformViewContext.onNodeReady(viewId)`
7. `InternalPlatformViewWrapper.onNodeReady()`
8. `LynxUIClayUIBridge.onNodeReady(lynxUI)`
9. `LynxBaseUI.onNodeReady()`

`InternalPlatformViewWrapper` currently gates ready on three conditions:

- the native ready signal has arrived
- the wrapped view has been attached
- at least one props/events batch has completed

Additional rules:

- ready is one-shot per node instance
- Java must not synthesize it from attach, props, or Android layout callbacks
- `onLayoutFinish()` exists only as a compatibility alias for older call sites;
  new behavior must be described in terms of `onNodeReady()`

## 10. Destroy Contract

Destroy flow:

1. Java `PlatformViewPlugin.onDestroyView()`
2. holder `release()`
3. `PlatformViewContext.destroyNode(viewId)`
4. internal wrapper:
   - `LynxUIClayUIBridge.onNodeRemoved(...)`
   - `LynxUIClayUIBridge.destroy(...)`
5. C++ `NativeView::OnDestroy()` releases any registered drawable image

Current wrapper behavior:

- `onNodeRemoved(...)` is emitted at most once
- if the wrapped view is still attached, `onDetach(...)` is issued before
  removal/destroy

## 11. Rendering and Composition Modes

### 11.1 Holder paths

Current holder paths:

- external texture composition
  - `PlatformViewWrapperHolder`
  - `PlatformViewWrapper`
- hybrid composition
  - `PlatformViewHybridHolder`
  - `PlatformViewsController`
  - `FlutterMutatorView`
- virtual-display fallback
  - `PlatformViewVDHolder`

### 11.2 Logical-only render mode

Logical-only is a platform-render policy, not a lifecycle policy.

It means:

- the Java plugin and holder path still exist
- size and buffer bookkeeping still happen
- lifecycle and method routing still happen
- logical insertion still happens
- only platform-layer drawing is suppressed

It does **not** mean:

- deleting the native-view node
- skipping holder creation entirely
- skipping `LynxBaseUI` lifecycle
- changing Clay child paint order

Current implementation details:

- texture path
  - `PlatformViewPlugin` creates `PlatformViewWrapperHolder` with
    `canDraw=false`
  - the holder sets `PlatformViewWrapper.canDraw=false` during construction
  - `PlatformViewWrapperHolder.onViewAttached(...)` skips adding the wrapper to
    `FlutterView`
- hybrid path
  - `PlatformViewPlugin` creates `PlatformViewHybridHolder` with
    `canDraw=false`
  - the holder calls
    `PlatformViewsController.setPlatformViewCanDraw(viewId, false)`
  - `PlatformViewsController` tracks non-drawable platform view ids
  - `PlatformViewsController.initializePlatformViewIfNeeded(...)` skips adding
    non-drawable mutator views to `FlutterView`
  - `PlatformViewsController.attachToView(...)` skips re-adding non-drawable
    mutator views on controller reattach
  - if `canDraw=false` is applied after the mutator view is attached,
    `PlatformViewsController` removes it from `FlutterView`

Important current limitation:

- platform `canDraw=false` suppresses rendering at the platform layer, but it does not
  fully eliminate holder creation or all earlier allocation work in the create
  path

## 12. Layout and Measurement Responsibilities

Default `NativeView` behavior:

- native views are custom-measurable leaves unless specialized otherwise
- Yoga will treat such nodes as leaves and skip descendant layout

Specialization rule:

- if a specialization needs descendant layout, it must disable the measure
  function without turning the node into a virtual node

Current example:

- Android map tags do this through the internal platform-view tag policy:
  `ViewRegistry::CreateView(...)` creates `NativeView`, while
  `ViewRegistry::CreateShadowNode(...)` returns no `NativeViewShadowNode` for
  view-only tags such as `x-map-ng` and `x-map-marker-ng`

## 13. Optional Clay Subtree Raster Snapshot Extension

A wrapped internal native view may expose a Clay subtree raster snapshot
provider to Java.

Generic flow:

1. Java wrapper calls `requestSubtreeRasterSnapshot(scale, callback)` through
   `InternalPlatformView`
2. `PlatformViewPlugin.requestSubtreeRasterSnapshot(...)` forwards to JNI
3. native `RequestSubtreeRasterSnapshot(...)` chooses a snapshot root
4. `PageView::MakeRasterSnapshot(...)` renders the subtree into encoded bytes
5. the result is delivered asynchronously; Java consumers that touch Android UI
   state must switch to the UI thread themselves

Current Android implementation characteristics:

- callback success returns encoded PNG bytes plus output width/height
- callback failure is reported asynchronously as `onError()`
- the current Android implementation snapshots the requested `NativeView` root
  itself; map marker specialization relies on the marker `NativeView` root

Snapshot preconditions:

- a valid snapshot root must exist
- the root must have non-zero width and height
- the root must be or become a repaint boundary before snapshotting

## 14. Invariants

- `create`, `insert`, `attach`, `layout`, `nodeReady`, and `destroy` must remain
  C++-driven end-to-end.
- Java must not synthesize nodeReady from attach, props, or Android layout
  callbacks.
- `InternalPlatformViewWrapper` must not dispatch `LynxBaseUI.onNodeReady()`
  before attach and the first props/events batch are both satisfied.
- platform `canDraw=false` must be implemented by suppressing platform-layer
  attachment/drawing, not by redefining Clay child painting semantics.
- holder or render-mode policy must not break logical parent/child insertion.
- layout relay must preserve fields that Clay does not currently own.
- subtree snapshot callbacks must tolerate async completion and destruction
  races.
- specialization-specific rules belong in specialization specs such as
  `map_view_spec.md`, not in this generic file.
