---
id: android-fragment-platform-rendering
type: module-design
status: active
title: Android fragment platform rendering
depends-on: [shared-create-view-scheduler]
---

## Responsibility

The Android fragment renderer adapts components without native fragment hosts through LynxUI. Eligible adapted UIs are prepared concurrently with element-tree flush; native renderers can exist before their Java hosts. This overlaps construction and safe initial-property processing without delaying native-only layout bookkeeping.

## Threading boundary

Eligibility requires the existing environment and owner create-view-async switches plus Behavior opt-in. Native fragment hosts, root reuse, context-free owners and fallback view reuse remain synchronous. Behavior opt-in remains responsible for component-specific construction and property-setter safety.

Each `PlatformRendererContext` owns a shared `CreateViewScheduler<ScopedGlobalJavaRef<jobject>>`, following the `shared-create-view-scheduler` contract. A renderer retains its opaque `TaskRef`. The first operation requiring a Java host calls `Consume`, which claims work that has not started or assists LIFO preparations from the active flush snapshot while waiting, then finalizes only the requested result on main exactly once. Assistance never invokes Java finalizers or publishes another renderer. An engaged empty result completes an abort; `nullopt` can reject preparation already on the same UI stack, so the renderer keeps its TaskRef until successful consumption or teardown. It clears the pending reference before Java finalization. Parent/child attachment requires both hosts. Waiting only inside the display-list callback is insufficient because rebuilding sublayers can attach views first. Workers cannot depend on main-thread completion, and every outcome settles the future. Failed background preparation can fall back synchronously; an unfinished successful task is never replaced speculatively.

## Flush snapshots

`NativePaintingCtxAndroid::BeforeFlush`, called by `Flush`, drains `TakeBatch` on the producer before handing operations to `DynamicUIOperationQueue`. The first normal operation of each producer flush reserves a snapshot slot. Before transfer, that slot receives the snapshot and a high-priority UI operation activates it. The normal-operation marker reactivates the same snapshot because the dynamic queue may coalesce multiple producer flushes, running all their high-priority operations before any normal operations. Without this marker, later work could replace the assistance snapshot for an earlier host dependency.

A normal reset operation ends every producer flush, including layout-only flushes without `FinishTasmOperation`. `FinishTasmOperation` retains its Java notification and queue-status ordering; resetting only there would either miss layout-only batches or precede later host dependencies. Empty snapshots also establish a boundary. There is no cross-flush queue accumulation: submissions after `TakeBatch` remain for the next producer flush. A nested flush may replace/reset active assistance, but the shared scheduler retains the snapshot belonging to an already-running wait. Queue operations and in-flight consumption retain the scheduler, not a raw painting-context pointer; workers retain only their task and JNI payload. Context destruction resets active assistance and continues to reject host publication.

## Initial properties

Adapted initial properties currently arrive as named PropBundle properties, not compact style buffers. Preparation splits out setters that publish shared state: accessibility element lists and exclusive focus, exposure identity, intersection observers, shared elements and Hero transitions. Those setters run on main before after-props hooks, gesture registration and owner publication. Safe setters and shadow-proxy preparation run in the preparation task. The complete original property map remains available to after-props and animation processing.

New shared-state setters must join this deferral boundary. Introducing compact initial styles requires equivalent filtering before enabling background processing of that representation. Legacy LynxUI creation is unchanged.

## Lifetime

JNI references retain preparation inputs across flush and consumption. Teardown and owner replacement prevent prepared completions from publishing into obsolete UI state. Discarding an unpublished or partially initialized preparation releases its task/JNI references without adding a `LynxUI.destroy()` call or a component lifecycle callback. This preserves legacy asynchronous-creation behavior; `supportCreateAsync` does not authorize an additional abort lifecycle. Already-published UIs retain the normal owner destruction path. Releasing preparation references alone does not guarantee reclamation of component-private resources. Renderer binding, hierarchy changes and destruction remain main-thread operations. Finalization may invoke application callbacks that synchronously destroy the view: native operations retain their renderers across that boundary and stop after teardown, while Java rechecks owner/context validity before further setters or publication. No FCP improvement is inferred from unit-test success; latency claims require device measurements.

## Verification

`DeferredRendererPreparationTest` uses the real native renderer with injected scheduler dispatch. It covers native-only registration/layout caching, first-host-use finalization, reentrant and late teardown, coalesced flush snapshots, preparation-only LIFO assistance, retry after same-stack preparation rejection, and reset after a layout-only flush. The batch tests exercise `NativePaintingCtxAndroid` and `DynamicUIOperationQueue`; deterministic native scheduler tests separately cover worker ownership and nested-wait mechanics. JNI tests require a rebuilt Android native test library before execution results apply to this integration.
