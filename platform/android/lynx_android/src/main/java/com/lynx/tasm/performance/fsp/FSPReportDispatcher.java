// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import com.lynx.tasm.base.CalledByNative;
import java.lang.ref.WeakReference;
import java.util.ArrayDeque;

/** Receives one snapshot at a time on the Lynx report thread. */
abstract class FSPReportProcessor {
  abstract void processSnapshotOnReportThread(
      MeaningfulContentSnapshot rawSnapshot, long currentTimestampUs);
}

/**
 * Serializes FSP snapshot processing on the Lynx report thread.
 *
 * <p>Posting every snapshot through {@code LynxEventReporter.runOnReportThread} would create a JNI
 * global reference for every pending {@code Runnable}. If the report thread falls behind the UI
 * thread, those references retain all pending snapshots until their individual tasks run.
 *
 * <p>This dispatcher instead keeps snapshots in a Java queue and lets all native tasks share one
 * global reference to this dispatcher. Every accepted snapshot has one native task at the same
 * position in the report-thread queue. This one-to-one mapping is important: a hard-timeout task
 * posted after several snapshots must run after those snapshots, rather than overtaking snapshots
 * that are waiting only in Java. The dispatch protocol is:
 *
 * <ol>
 *   <li>{@link #post} appends a snapshot and posts its corresponding native task.
 *   <li>{@link #dispatchOne} is invoked by that task and processes exactly one queued snapshot.
 *   <li>{@link #close} rejects new snapshots, drops queued snapshots, and releases the native
 *       owner's reference.
 * </ol>
 *
 * <p>All mutable fields are guarded by this object's monitor. Snapshot processing deliberately
 * runs outside the monitor because it may be expensive and may close the owning FSP session.
 */
final class FSPReportDispatcher {
  private static final class PendingSnapshot {
    final MeaningfulContentSnapshot rawSnapshot;
    final long currentTimestampUs;

    PendingSnapshot(MeaningfulContentSnapshot rawSnapshot, long currentTimestampUs) {
      this.rawSnapshot = rawSnapshot;
      this.currentTimestampUs = currentTimestampUs;
    }
  }

  // Native code keeps the dispatcher reachable until every posted callback finishes. Keeping the
  // processor weak prevents that temporary JNI lifetime from also retaining the FSP session and
  // its PerformanceController after the session has closed.
  private final WeakReference<FSPReportProcessor> mProcessorRef;
  private final ArrayDeque<PendingSnapshot> mPendingSnapshots = new ArrayDeque<>();

  // Non-zero while the dispatcher accepts work. This pointer owns the native reference returned
  // by nativeCreate; nativeDestroy releases that owner exactly once.
  private long mNativePtr;

  FSPReportDispatcher(FSPReportProcessor processor) {
    mProcessorRef = new WeakReference<>(processor);
    mNativePtr = nativeCreate(this);
  }

  synchronized boolean post(MeaningfulContentSnapshot rawSnapshot, long currentTimestampUs) {
    if (rawSnapshot == null || mNativePtr == 0) {
      return false;
    }
    mPendingSnapshots.addLast(new PendingSnapshot(rawSnapshot, currentTimestampUs));
    // Post one report-thread task for every snapshot. Native tasks share the dispatcher's single
    // global reference, so preserving report-thread ordering does not recreate the original
    // one-GlobalRef-per-Runnable problem.
    if (nativePost(mNativePtr)) {
      return true;
    }
    // Enqueue happens before nativePost so that a very fast callback can always find its snapshot.
    // If nativePost fails, no callback exists for the just-appended tail item. Roll it back to keep
    // the Java queue in one-to-one correspondence with successfully posted native tasks.
    mPendingSnapshots.removeLast();
    return false;
  }

  synchronized void close() {
    if (mNativePtr == 0) {
      return;
    }
    long nativePtr = mNativePtr;
    mNativePtr = 0;
    mPendingSnapshots.clear();
    // Callbacks already posted to the report thread each own an additional native reference and
    // may still enter dispatchOne(). They will find an empty queue and become harmless no-ops.
    nativeDestroy(nativePtr);
  }

  @CalledByNative
  void dispatchOne() {
    PendingSnapshot pendingSnapshot;
    FSPReportProcessor processor;
    synchronized (this) {
      pendingSnapshot = mPendingSnapshots.poll();
      processor = mProcessorRef.get();
    }
    // Never hold the queue lock while running FSP computation. Apart from reducing contention,
    // this permits the processor to close the current session without re-entering this monitor.
    if (pendingSnapshot != null && processor != null) {
      processor.processSnapshotOnReportThread(
          pendingSnapshot.rawSnapshot, pendingSnapshot.currentTimestampUs);
    }
  }

  private static native long nativeCreate(FSPReportDispatcher dispatcher);

  private static native boolean nativePost(long nativePtr);

  private static native void nativeDestroy(long nativePtr);
}
