// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.base;

import android.annotation.SuppressLint;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

/**
 * Handles running cleanup tasks when an object becomes eligible for GC. Cleanup tasks
 * can execute on the main thread or reaper(worker) thread. In general, classes should not have
 * finalizers and likewise should not use this class for the same reasons. The
 * exception is where public APIs exist that require native side resources to be
 * cleaned up in response to java side GC of API objects. (Private/internal
 * interfaces should always favor explicit resource releases / destroy()
 * protocol for this rather than depend on GC to trigger native cleanup).
 */
public class CleanupReference extends PhantomReference<Object> {
  private static final String TAG = "CleanupReference";

  private static final boolean DEBUG = false; // Always check in as false!

  // The VM will enqueue CleanupReference instance onto sGcQueue when it becomes eligible for
  // garbage collection (i.e. when all references to the underlying object are nullified).
  // |sReaperThread| processes this queue by forwarding the references on to the UI thread
  // (via REMOVE_REF message) to perform cleanup.
  private static ReferenceQueue<Object> sGcQueue = new ReferenceQueue<Object>();
  private static Object sCleanupMonitor = new Object();

  private static final Thread sReaperThread = new Thread(TAG) {
    @Override
    @SuppressWarnings("WaitNotInLoop")
    public void run() {
      while (true) {
        try {
          CleanupReference ref = (CleanupReference) sGcQueue.remove();
          if (DEBUG)
            LLog.d(TAG, "removed one ref from GC queue");
          synchronized (sCleanupMonitor) {
            if (!ref.mCleanupOnUiThread) {
              sReaperThreadPendingRefs.offer(ref);
            } else {
              Message.obtain(LazyHolder.sHandler, REMOVE_REF, ref).sendToTarget();
              // Give the UI thread chance to run cleanup before looping around and
              // taking the next item from the queue, to avoid Message bombing it.
              sCleanupMonitor.wait(500);
            }
            while ((ref = (CleanupReference) sReaperThreadPendingRefs.poll()) != null) {
              TraceEvent.beginSection("CleanupReference.ReaperThread.runCleanupTask");
              ref.runCleanupTaskInternal(sReaperThreadRefs);
              TraceEvent.endSection("CleanupReference.ReaperThread.runCleanupTask");
            }
          }
        } catch (Exception e) {
          LLog.e(TAG, "Queue remove exception:" + e.toString());
        }
      }
    }
  };

  static {
    sReaperThread.setDaemon(true);
    sReaperThread.start();
  }

  // Message's sent in the |what| field to |sHandler|.

  // Add a new reference to sRefs. |msg.obj| is the CleanupReference to add.
  private static final int ADD_REF = 1;
  // Remove reference from sRefs. |msg.obj| is the CleanupReference to remove.
  private static final int REMOVE_REF = 2;

  /**
   * This {@link Handler} polls {@link #sUiThreadRefs}, looking for cleanup tasks that
   * are ready to run.
   * This is lazily initialized as ThreadUtils.getUiThreadLooper() may not be
   * set yet early in startup.
   */
  @SuppressLint("HandlerLeak")
  private static class LazyHolder {
    static final Handler sHandler = new Handler(Looper.getMainLooper()) {
      @Override
      public void handleMessage(Message msg) {
        try {
          TraceEvent.beginSection("CleanupReference.LazyHolder.handleMessage");
          CleanupReference ref = (CleanupReference) msg.obj;
          switch (msg.what) {
            case ADD_REF:
              sUiThreadRefs.add(ref);
              break;
            case REMOVE_REF:
              ref.runCleanupTaskInternal(sUiThreadRefs);
              break;
            default:
              LLog.e(TAG, String.format("Bad message=%d", msg.what));
              break;
          }

          if (DEBUG)
            LLog.d(TAG,
                String.format("will try and cleanup; max = %d",
                    sUiThreadRefs.size() + sReaperThreadRefs.size()));

          synchronized (sCleanupMonitor) {
            // Always run the cleanup loop here even when adding or removing refs, to
            // avoid falling behind on rapid garbage allocation inner loops.
            while ((ref = (CleanupReference) sGcQueue.poll()) != null) {
              if (!ref.mCleanupOnUiThread) {
                sReaperThreadPendingRefs.offer(ref);
              } else {
                ref.runCleanupTaskInternal(sUiThreadRefs);
              }
            }
            sCleanupMonitor.notifyAll();
          }
        } finally {
          TraceEvent.endSection("CleanupReference.LazyHolder.handleMessage");
        }
      }
    };
  }

  /**
   * Keep a strong reference to {@link CleanupReference} so that it will
   * actually get enqueued.
   * Only accessed on the UI thread.
   */
  private static Set<CleanupReference> sUiThreadRefs = new HashSet<CleanupReference>();

  /**
   * Keep a strong reference to {@link CleanupReference} so that it will
   * actually get enqueued.
   * Accessed on sReaperThread thread and any thread.
   */
  private static Set<CleanupReference> sReaperThreadRefs = ConcurrentHashMap.newKeySet();
  private static ConcurrentLinkedQueue sReaperThreadPendingRefs = new ConcurrentLinkedQueue();

  private final boolean mCleanupOnUiThread;

  private Runnable mCleanupTask;

  /**
   * @param obj the object whose loss of reachability should trigger the
   *            cleanup task.
   * @param cleanupTask the task to run once obj loses reachability.
   * @param cleanupOnUiThread run task on ui thread.
   */
  public CleanupReference(Object obj, Runnable cleanupTask, boolean cleanupOnUiThread) {
    super(obj, sGcQueue);
    if (DEBUG)
      LLog.d(TAG, "+++ CREATED ONE REF");
    mCleanupTask = cleanupTask;
    mCleanupOnUiThread = cleanupOnUiThread;
    if (!mCleanupOnUiThread) {
      sReaperThreadRefs.add(this);
    } else {
      handleOnUiThread(ADD_REF);
    }
  }

  /**
   * Clear the cleanup task {@link Runnable} so that nothing will be done
   * after garbage collection.
   */
  public void cleanupNow() {
    if (!mCleanupOnUiThread) {
      // When cleanup on worker thread is set, cleanupNow on ui thread will be ignored, instead will
      // cleanup on worker thread when gc. Caller ensure not to call on ui thread.
      if (Looper.getMainLooper().getThread() != Thread.currentThread()) {
        TraceEvent.beginSection("CleanupReference.InvokingThread.runCleanupTask");
        runCleanupTaskInternal(sReaperThreadRefs);
        TraceEvent.endSection("CleanupReference.InvokingThread.runCleanupTask");
      }
    } else {
      handleOnUiThread(REMOVE_REF);
    }
  }

  public boolean hasCleanedUp() {
    return mCleanupTask == null;
  }

  private void handleOnUiThread(int what) {
    Message msg = Message.obtain(LazyHolder.sHandler, what, this);
    if (Looper.myLooper() == msg.getTarget().getLooper()) {
      msg.getTarget().handleMessage(msg);
      msg.recycle();
    } else {
      msg.sendToTarget();
    }
  }

  private void runCleanupTaskInternal(Set<CleanupReference> refs) {
    if (DEBUG)
      LLog.d(TAG, "runCleanupTaskInternal");
    refs.remove(this);
    Runnable cleanupTask = mCleanupTask;
    mCleanupTask = null;
    if (cleanupTask != null) {
      if (DEBUG)
        LLog.i(TAG, "--- CLEANING ONE REF");
      cleanupTask.run();
    }
    clear();
  }
}
