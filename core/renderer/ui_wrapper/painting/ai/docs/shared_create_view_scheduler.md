---
id: shared-create-view-scheduler
type: submodule-design
status: active
title: Shared create-view preparation scheduler
tags: [android, painting, scheduling]
---

## Responsibility

`core/renderer/ui_wrapper/painting/create_view_scheduler.h` extracts the native
Android legacy create-view scheduling mechanism. `CreateViewScheduler<Result>`
is a per-painting-context preparation helper, not a general executor. Its result
parameter keeps JNI out of deterministic native tests; the Android caller uses
`base::android::ScopedGlobalJavaRef<jobject>` without a conversion adapter.

## Boundary

- `Schedule(prepare, defer = false)` returns an opaque `TaskRef`. The callback
  returns `Result`, including an empty JNI reference when the context has expired
  or creation is aborted. Every normally returning callback settles the promise.
  Callbacks must not throw; there is no cancellation or exception policy here.
- Default dispatch posts preparation to `PostTaskToConcurrentLoop` with
  `HIGH_PRIORITY`. Constructor dispatch injection is a deterministic test seam.
  Workers capture only the task, never the scheduler or painting context itself.
  Pipeline callbacks own their captured payloads and weak platform references.
- `DispatchDeferred()` drains context-free tasks in submission order. Even a
  deferred task can be claimed by UI consumption before attachment. Dispatch
  after that is harmless because `OnceTask` admits only one preparation.
- `TakeBatch()` atomically drains the scheduled queue in reverse submission
  order, returning null when empty. Later submissions remain for the next flush.
  The producer passes this snapshot to a high-priority UI operation calling
  `ActivateBatch(batch)`; taking the snapshot must not be delayed until that UI
  operation executes. `ResetBatch()` releases the active assistance snapshot.
- `Consume(task)` claims the result once, attempts preparation locally, assists
  LIFO preparations from the active snapshot while waiting, then gets only the
  requested future. It returns `optional<Result>`: an engaged empty JNI result
  is a completed abort; `nullopt` means the result is already claimed or its
  preparation is currently on this UI stack. In the latter case it remains
  available to consume after preparation returns. Assistance never consumes other
  results or invokes any main finalizer.

The producer owns scheduling, deferred dispatch, and snapshot creation. The UI
thread owns activation, reset, and consumption. Queue handoff and OnceTask claim
are thread-safe, but concurrent result consumption from multiple threads is not
supported. No scheduler lock is held while dispatch or preparation runs.

## Ownership and reentrancy

Consumption claims its task before invoking preparation, so reentrant consumption
of the same task cannot wait on itself or publish twice. Reentrant consumption
of a task currently being assisted on this UI stack also returns without waiting
or claiming its future; the caller can consume it once preparation returns.
Each wait retains its
task and the snapshot active at entry. Assistance advances the shared snapshot
cursor **before** running a callback, so nested waits do not reuse a stale cursor.
A reentrant flush may replace or reset the active snapshot without invalidating
an outer wait; that wait continues assisting its retained snapshot, not new work.

Scheduling queues are drained at attachment/flush, not accumulated across
batches. Only the active snapshot is retained by the scheduler; replaced snapshots
survive only while an in-flight wait or UI operation owns them. Assisted entries
release their task references when taken. Preparation captures are released after
execution, including aborts, even if a posted worker still owns the task. Result
ownership transfers to the sole consumer. Pending unconsumed results remain owned
by tasks until consumed or released. Each context has separate queues; there is
no global or cross-page assistance queue.

## Legacy integration and reuse

`PaintingContextAndroid` retains all existing environment/config enablement and
synchronous paths. Its async JNI callback performs preparation and error metadata
reporting; its enqueued UI operation calls `Consume` and invokes a non-null Java
runnable at the original queue position. A missing weak Java reference returns an
empty result rather than leaving a live promise unresolved.

`SetContextHasAttached` still clears the context-free flag and dispatches deferred
work. `BeforeFlush` still skips context-free mode, enqueues view-tree rebuilding,
and takes/activates the scheduled snapshot before normal UI operations.
`FinishTasmOperation` still enqueues snapshot reset at its original boundary.

Another painting pipeline can supply its own JNI preparation callback, retain the
`TaskRef`, and call `Consume` only at its first host dependency. It must own its own
scheduler, flags, snapshot lifecycle, main-thread finalization, and teardown
checks. The Android fragment consumer is described in
[`android-fragment-platform-rendering`](../../android/ai/docs/android-fragment-platform-rendering.md).

## Verification

`create_view_scheduler_unittest.cc` is included in
`create_view_scheduler_unittest_exec`. Deterministic dispatch and promise gates test
main/worker claims, move-only and null completion, repeated/reentrant consumption,
LIFO preparation-only assistance, snapshot boundaries, context-free dispatch,
reentrant flush ownership, per-context isolation, capture release, and scheduler
teardown during consumption. A production-dispatch test verifies the high-priority
worker loop; another verifies workers outliving the scheduler. JNI integration
requires an Android compile check; these portable native tests do not invoke Java.
