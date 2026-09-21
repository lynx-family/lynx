---
id: android-fragment-platform-rendering
type: module-design
status: active
title: Android fragment platform rendering
depends-on: [shared-create-view-scheduler]
---

## Responsibility

The Android fragment renderer adapts components without native fragment hosts through LynxUI. This layer provides preparation/publication and deferred native-host capabilities, including direct test injection of preparation tasks. Production activation is not included: `NativePaintingCtxAndroid` still queues synchronous renderer creation and does not call `PrepareRenderer` or `CreatePreparedRenderer`. Native renderers supplied with a preparation task can exist before their Java hosts. Native-only layout caching already exists and is unchanged.

## Threading boundary

Eligibility requires the existing environment and owner create-view-async switches plus Behavior opt-in. Native fragment hosts, root reuse, context-free owners and fallback view reuse remain synchronous. Behavior opt-in remains responsible for component-specific construction and property-setter safety.

Each `PlatformRendererContext` owns a shared `CreateViewScheduler<ScopedGlobalJavaRef<jobject>>`, following the `shared-create-view-scheduler` contract. A renderer retains its opaque `TaskRef`. The first operation requiring a Java host calls `Consume`, which claims work that has not started or assists LIFO preparations from the active flush snapshot while waiting, then finalizes only the requested result on main exactly once. Assistance never invokes Java finalizers or publishes another renderer. An engaged empty result completes an abort; `nullopt` can reject preparation already on the same UI stack, so the renderer keeps its TaskRef until successful consumption or teardown. It clears the pending reference before Java finalization. Parent/child attachment requires both hosts. Waiting only inside the display-list callback is insufficient because rebuilding sublayers can attach views first. Workers cannot depend on main-thread completion, and every outcome settles the future. Failed background preparation can fall back synchronously; an unfinished successful task is never replaced speculatively.

## Flush snapshots (not activated)

The shared scheduler exposes snapshot taking, activation, assistance and reset, but this layer does not wire those operations into `NativePaintingCtxAndroid::Enqueue`, `BeforeFlush` or `Flush`. Production creation does not submit preparation tasks. Direct capability tests inject tasks without relying on painting-queue activation. The subsequent activation layer must take snapshots on the producer, activate before host-dependent operations, preserve each producer flush's snapshot when the dynamic queue coalesces flushes, and reset even for layout-only flushes. Batch/reset integration tests belong with that activation, not this capability layer.

## Initial properties

Preparation follows the legacy C++-scheduled `createViewAsync` sequence: construct the UI, attach events and gestures, and process the complete initial properties on the preparation thread (worker or assisting UI thread). Adapted initial properties arrive as named PropBundle properties, not compact style buffers. One `StylesDiffMap` backed by the original property map is reused for construction, the single property pass, and after-props processing; there is no property-name partition, routing copy or per-property wrapper.

After-props hooks, transition and keyframe animation completion, and owner publication run on main. Behavior opt-in is responsible for setter safety, including inherited setters that affect shared registries, as in legacy asynchronous creation. Publication checks do not roll back preparation-time registry side effects. Legacy LynxUI creation is unchanged.

## Lifetime

JNI references retain preparation inputs across flush and consumption. Teardown and Java UI owner replacement prevent prepared completions from publishing into obsolete UI state. Discarding an unpublished or partially initialized preparation releases its task/JNI references without adding a `LynxUI.destroy()` call or a component lifecycle callback. This preserves legacy asynchronous-creation behavior; `supportCreateAsync` does not authorize an additional abort lifecycle. Already-published UIs retain the normal owner destruction path. Releasing preparation references alone does not guarantee reclamation of component-private resources. Renderer binding, hierarchy changes and destruction remain main-thread operations. Renderer operations, including component construction, property setters and after-props hooks, must not call `LynxView.destroy()`. Enforcement of access to host lifecycle APIs and general reentrant hierarchy-teardown protection are outside this module. Java rechecks owner/context validity before further setters or publication. No FCP improvement is inferred from unit-test success; latency claims require device measurements.

Element IDs are unique, with one native renderer per ID; same-ID renderer replacement is not a supported contract. Queued work arriving after teardown is rejected at entry. Destruction of an initialized renderer uses `DestroyPlatformRenderer`; a renderer without an initialized host only unregisters and drops its pending preparation, without invoking public host destruction or adding an unpublished UI destruction callback.

## Verification

`DeferredRendererPreparationTest` uses the real native renderer with injected scheduler dispatch. Capability tests cover native-only registration/layout caching, first-host-use finalization, teardown before first host use, and reentrant finalization and late teardown. Coalesced flush snapshots, preparation-only LIFO assistance through the painting queue, retry after same-stack preparation rejection in that batch, and layout-only flush reset tests are deferred until production activation. Deterministic native scheduler tests separately cover worker ownership and nested-wait mechanics. JNI tests require a rebuilt Android native test library before execution results apply to this integration.
