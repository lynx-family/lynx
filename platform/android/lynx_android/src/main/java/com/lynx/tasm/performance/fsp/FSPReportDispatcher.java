// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import androidx.annotation.Keep;
import com.lynx.tasm.base.CalledByNative;
import java.lang.ref.WeakReference;
import java.util.concurrent.ConcurrentLinkedQueue;

/**
 * Posts FSP snapshots to the report thread without creating a global JNI reference per snapshot.
 */
@Keep
final class FSPReportDispatcher {
  private static final class PendingSnapshot {
    final MeaningfulContentSnapshot rawSnapshot;
    final long currentTimestampUs;

    PendingSnapshot(MeaningfulContentSnapshot rawSnapshot, long currentTimestampUs) {
      this.rawSnapshot = rawSnapshot;
      this.currentTimestampUs = currentTimestampUs;
    }
  }

  private final WeakReference<FSPTracer> mTracerRef;
  private final ConcurrentLinkedQueue<PendingSnapshot> mPendingSnapshots =
      new ConcurrentLinkedQueue<>();
  private long mNativePtr;

  FSPReportDispatcher(FSPTracer tracer) {
    mTracerRef = new WeakReference<>(tracer);
    mNativePtr = nativeCreate(this);
  }

  synchronized boolean post(MeaningfulContentSnapshot rawSnapshot, long currentTimestampUs) {
    if (rawSnapshot == null || mNativePtr == 0) {
      return false;
    }
    PendingSnapshot pendingSnapshot = new PendingSnapshot(rawSnapshot, currentTimestampUs);
    mPendingSnapshots.offer(pendingSnapshot);
    if (nativePost(mNativePtr)) {
      return true;
    }
    mPendingSnapshots.remove(pendingSnapshot);
    return false;
  }

  synchronized void close() {
    if (mNativePtr == 0) {
      return;
    }
    long nativePtr = mNativePtr;
    mNativePtr = 0;
    nativeDestroy(nativePtr);
  }

  @CalledByNative
  void dispatchOne() {
    PendingSnapshot pendingSnapshot = mPendingSnapshots.poll();
    FSPTracer tracer = mTracerRef.get();
    if (pendingSnapshot == null || tracer == null) {
      return;
    }
    tracer.processSnapshotOnReportThread(
        pendingSnapshot.rawSnapshot, pendingSnapshot.currentTimestampUs);
  }

  private static native long nativeCreate(FSPReportDispatcher dispatcher);

  private static native boolean nativePost(long nativePtr);

  private static native void nativeDestroy(long nativePtr);
}
