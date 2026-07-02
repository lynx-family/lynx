# Component Guide

This directory is the Clay component-layer source of truth.

It is responsible for:

- component tree ownership and parent/child relationships
- component lifecycle dispatch
- layout-to-view propagation
- native-view embedding at the component layer
- component-facing specs that define the intended contract for the code

Typical entry points in this directory:

- `base_view.h` / `base_view.cc`
- `view_context.h` / `view_context.cc`
- `page_view.cc`
- `native_view.h` / `native_view.cc`

This directory does not contain the entire Android native-view stack by itself.
The full implementation spans:

- `lynx/clay/ui/component/`
- `lynx/clay/lynx_adaptor/`
- `clay/shell/platform/android/`
- `platform/android/lynx_clay/`

## Spec Structure

All component specs live under `spec/`.

Current spec files:

- `spec/native_view_spec.md`
  - Normative contract for generic Clay Android `NativeView`
  - Covers create, insert, attach/detach, props/events, layout, nodeReady,
    destroy, holder selection, logical-only rendering, and optional subtree
    raster snapshots
- `spec/map_view_spec.md`
  - Specialization built on top of `spec/native_view_spec.md`
  - Covers `x-map-ng`, `x-map-marker-ng`, map tag mapping, marker subtree layout,
    logical-only marker rendering, and marker bitmap snapshot flow
- `spec/ios_hybrid_composition_spec.md`
  - Normative contract for Clay iOS hybrid-composition overlay geometry
  - Covers visible overlay slices, local Metal backing surfaces, shared
    compositor mapping, and UIKit wrapper clipping

Relationship between the specs:

- `spec/native_view_spec.md` defines generic rules
- `spec/map_view_spec.md` is allowed to specialize those rules for the map
  stack
- `spec/ios_hybrid_composition_spec.md` defines generic iOS overlay geometry
- generic behavior must not be documented only in the map spec
- map-only behavior must not be pushed back into the generic spec unless it has
  become a shared contract

## Recommended Reading Order

When a task touches generic native-view behavior:

1. `lynx/clay/ui/component/AGENTS.md`
2. `lynx/clay/ui/component/spec/native_view_spec.md`
3. Relevant implementation files

When a task touches the map stack:

1. `lynx/clay/ui/component/AGENTS.md`
2. `lynx/clay/ui/component/spec/native_view_spec.md`
3. `lynx/clay/ui/component/spec/map_view_spec.md`
4. Relevant implementation files

When a task touches iOS hybrid-composition overlays:

1. `lynx/clay/ui/component/AGENTS.md`
2. `lynx/clay/ui/component/spec/ios_hybrid_composition_spec.md`
3. Relevant compositor, platform-overlay, and presenter implementation files

## Code Map

Generic native-view implementation is currently distributed across these files:

- `lynx/clay/ui/component/native_view.cc`
- `lynx/clay/ui/component/view_context.cc`
- `lynx/clay/lynx_adaptor/layout_context_clay.cc`
- `lynx/clay/lynx_adaptor/painting_context_clay.cc`
- `clay/shell/platform/android/plugins/native_view_service_android.cc`
- `clay/shell/platform/android/java_src/main/com/lynx/clay/embedding/engine/plugins/platformview/PlatformViewPlugin.java`
- `clay/shell/platform/android/java_src/main/com/lynx/clay/embedding/engine/plugins/platformview/InternalPlatformViewContext.java`
- `clay/shell/platform/android/java_src/main/com/lynx/clay/embedding/engine/plugins/platformview/InternalPlatformViewRegistry.java`
- `clay/shell/platform/android/java_src/main/com/lynx/clay/embedding/engine/plugins/platformview/views/InternalPlatformViewWrapper.java`
- `platform/android/lynx_clay/src/main/java/com/lynx/tasm/LynxUIClayUIBridgeImpl.java`

Map-specific implementation is currently concentrated in:

- `platform/android/x_element/x_element_map_ng/.../LynxMapView.java`
- `platform/android/x_element/x_element_map_ng/.../LynxMapMarker.java`

## Change Routing Rules

Use these rules when deciding where a change belongs.

Put the change in generic native-view code if it affects:

- all wrapped `NativeView` nodes
- holder selection or generic render-mode semantics
- generic layout relay
- generic nodeReady gating
- generic subtree snapshot infrastructure

Put the change in map-specific code if it affects:

- only `x-map-ng` or `x-map-marker-ng`
- tag mapping or event aliasing for map
- marker-specific logical-only policy
- marker-specific snapshot root assumptions
- marker-specific async bitmap generation, retry, or coalescing behavior

If a behavior starts as map-specific and later becomes reusable, first update
the generic spec, then move the implementation.

## Documentation Rules

- Keep all spec files in English.
- Treat spec files as normative contracts for the current implementation, not as
  temporary design notes.
- When code changes affect lifecycle, layout, hierarchy semantics, render-mode
  policy, or snapshot flow, update the relevant spec in the same change.
- Keep generic rules in `spec/native_view_spec.md`.
- Keep specialization rules in dedicated specialization specs such as
  `spec/map_view_spec.md`.
- Do not leave conflicting statements split across AGENTS and spec files. The
  spec is authoritative for behavior; this file is authoritative for reading
  order and ownership boundaries.

## For AI-Assisted Work

If you are using an LLM to modify this directory:

- start from the spec files before proposing structural changes
- verify whether the code still matches the spec before extending behavior
- if code and spec diverge, update both together instead of assuming either is
  implicitly correct
- avoid reviving discarded designs unless the new change intentionally replaces
  the current contract
- prefer explaining changes in terms of which layer owns the behavior:
  component, adaptor, JNI/platform service, Java platform-view glue, or
  `lynx_clay` bridge
