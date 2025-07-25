// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing;

import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.performance.fsptracing.FSPContentInfo;
import com.lynx.tasm.performance.fsptracing.FSPResult;
import com.lynx.tasm.performance.fsptracing.IFSPTracerImpl;
import com.lynx.tasm.performance.fsptracing.TraceEventDef;
import com.lynx.tasm.performance.fsptracing.area.FSPAreaConfig;
import com.lynx.tasm.performance.fsptracing.area.FSPAreaTracerImpl;
import com.lynx.tasm.performance.fsptracing.axial.FSPAxialConfig;
import com.lynx.tasm.performance.fsptracing.axial.FSPAxialTracerImpl;
import java.lang.FunctionalInterface;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPTracer {
  @FunctionalInterface
  public interface ICompletionCallback {
    void onComplete(@NonNull FSPResult result);
  }

  public interface IObserverCallback {
    void onStart();
    void onBecomeStable(boolean stable, @NonNull FSPResult result);
    void onStop(@NonNull StopReason reason, @NonNull FSPResult result);
    void willStartGracefulTimeout(double timeout);
  }

  public enum StopReason { SUCCESS, USER_INTERACTION, TIMEDOUT }

  private final Handler mMainHandler = new Handler(Looper.getMainLooper());
  private final FSPConfig mConfig;
  private final LynxContext mContext;
  private final IFSPTracerImpl mTracerImpl;

  private FSPSnapshot mCurrentSnapshot;
  private FSPSnapshot mPreviousSnapshot;
  private boolean mIsTracing = false;
  private boolean mHasLoadingUI = false;

  private ICompletionCallback mCompletionCallback;
  private IObserverCallback mObserverCallback;

  private FSPResult mLastResult = null;
  private Runnable mHardTimeoutRunnable;
  private Runnable mGracefulTimeoutRunnable;
  private Runnable mSnapshotRunnable;
  private long mBeginTimestampNs;

  private static int gEnabled = -1;
  private static FSPConfig gConfig = null;
  public static boolean isEnabled() {
    if (gEnabled == -1) {
      gEnabled =
          LynxEnv.getBooleanFromExternalEnv(LynxEnvKey.ENABLE_FIRST_STABLE_PAINT, true) ? 1 : 0;
    }
    return gEnabled == 1;
  }

  public static void setEnabled(boolean enabled, @Nullable FSPConfig config) {
    gEnabled = enabled ? 1 : 0;
    if (config != null) {
      gConfig = config.clone();
    }
  }

  /**
   * Create a tracer instance with the appropriate configuration.
   */
  public static FSPTracer create(@NonNull LynxContext context) {
    if (!isEnabled()) {
      return null;
    }

    TraceEvent.beginSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_CREATE);
    IFSPTracerImpl tracerImpl;
    FSPConfig config = gConfig != null ? gConfig.clone() : new FSPAxialConfig();

    if (config instanceof FSPAxialConfig) {
      tracerImpl = new FSPAxialTracerImpl((FSPAxialConfig) config);
    } else if (config instanceof FSPAreaConfig) {
      tracerImpl = new FSPAreaTracerImpl((FSPAreaConfig) config);
    } else {
      // Default to axial tracer with default config
      config = new FSPAxialConfig();
      tracerImpl = new FSPAxialTracerImpl((FSPAxialConfig) config);
    }

    FSPTracer tracer = new FSPTracer(config, context, tracerImpl);
    TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_CREATE);
    return tracer;
  }

  public FSPTracer(FSPConfig config, LynxContext context, IFSPTracerImpl tracerImpl) {
    this.mConfig = config;
    this.mContext = context;
    this.mTracerImpl = tracerImpl;
  }

  public void beginTracing(ICompletionCallback callback) {
    TraceEvent.beginSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_BEGIN_TRACING);

    mCompletionCallback = callback;

    if (!mIsTracing) {
      mIsTracing = true;
      mBeginTimestampNs = SystemClock.elapsedRealtimeNanos();
      if (mSnapshotRunnable != null) {
        mMainHandler.removeCallbacks(mSnapshotRunnable);
      }
      mSnapshotRunnable = new Runnable() {
        @Override
        public void run() {
          if (captureSnapshot()) {
            mMainHandler.postDelayed(this, (long) (mConfig.getSnapshotInterval() * 1000));
          }
        }
      };
      mMainHandler.postDelayed(mSnapshotRunnable, (long) (mConfig.getSnapshotInterval() * 1000));

      beginHardTimeout();

      if (mObserverCallback != null) {
        mObserverCallback.onStart();
      }
    }
    TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_BEGIN_TRACING);
  }

  public void stopTracing(StopReason reason) {
    stopTracing(reason, null);
  }

  public ArrayList<String> inspectCurrentSnapshot() {
    if (mCurrentSnapshot == null) {
      return new ArrayList<>();
    }

    TraceEvent.beginSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_IMPL_INSPECT_SNAPSHOT);
    ArrayList<String> result = mTracerImpl.inspectSnapshot(mCurrentSnapshot, mPreviousSnapshot);
    TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_IMPL_INSPECT_SNAPSHOT);
    return result;
  }

  private void stopTracing(StopReason reason, FSPResult result) {
    TraceEvent.beginSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_STOP_TRACING);
    if (true == mContext.setTracingFSP(false)) {
      mIsTracing = false;

      if (result == null) {
        result = new FSPResult();
      }

      if (mObserverCallback != null) {
        mObserverCallback.onStop(reason, result);
      }

      if (mCompletionCallback != null) {
        mCompletionCallback.onComplete(result);
      }
    }

    mLastResult = null;
    mCompletionCallback = null;

    if (mHardTimeoutRunnable != null) {
      mMainHandler.removeCallbacks(mHardTimeoutRunnable);
      mHardTimeoutRunnable = null;
    }

    if (mGracefulTimeoutRunnable != null) {
      mMainHandler.removeCallbacks(mGracefulTimeoutRunnable);
      mGracefulTimeoutRunnable = null;
    }

    mCurrentSnapshot = null;
    mPreviousSnapshot = null;
    TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_STOP_TRACING);
  }

  public void setObserverCallback(IObserverCallback callback) {
    mObserverCallback = callback;
  }

  // Return true to schedule next capturing.
  private boolean captureSnapshot() {
    if (!mIsTracing) {
      return false;
    }

    FSPResult result = null;
    boolean hasChange = mContext.checkPendingMeaningfulContentChanges();
    if (!hasChange) {
      if (mLastResult != null) {
        // No change since last snapshot, copy last result and change it to stable
        // without the need of another round of capturing and diffing.
        mLastResult.setSuccess(true);
        result = mLastResult;

        TraceEvent.beginSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_CAPTURE_SNAPSHOT);
      } else {
        // Skip this snapshot, stablility detection requires at least one
        // meaningful change.
        return true;
      }
    } else {
      TraceEvent.beginSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_CAPTURE_SNAPSHOT);

      // Captures changes inside the UI tree.
      swapSnapshots();
      updateSnapshot();
      result = diffSnapshots();
    }

    boolean wasStable = mLastResult != null && mLastResult.isSuccess();

    // FSP was stable, reverting back to unstable and canceling graceful timeout block.
    if (result == null || !result.isSuccess()) {
      mLastResult = result;

      if (wasStable) {
        // Cancel graceful timeout block if any.
        if (mGracefulTimeoutRunnable != null) {
          mMainHandler.removeCallbacks(mGracefulTimeoutRunnable);
        }

        if (mObserverCallback != null) {
          mObserverCallback.onBecomeStable(false, result);
        }
      }

      TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_CAPTURE_SNAPSHOT);
      return true;
    }

    // FSP turned stable, notify observer and begin timeout if needed.
    if (!wasStable) {
      if (mObserverCallback != null) {
        mObserverCallback.onBecomeStable(true, result);
      }

      double timeout = mConfig.getGracefulTimeout();
      // If extended graceful timeout is set, use it.
      if (mConfig.getExtendedGracefulTimeout() > 0.0) {
        boolean isLoading = mContext.checkPendingMeaningfulContentLoading() || mHasLoadingUI;
        if (isLoading) {
          // If there are still loading UIs, use the extended graceful timeout.
          timeout = mConfig.getExtendedGracefulTimeout();
        }
      }

      if (timeout > 0.0) {
        // Wait for graceful timeout to complete with the pending result.
        mLastResult = result;
        beginGracefulTimeout(timeout);

        TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_CAPTURE_SNAPSHOT);
        return true;
      } else {
        // No graceful timeout, complete immediately.
        stopTracing(StopReason.SUCCESS, result);

        TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_CAPTURE_SNAPSHOT);
        return true;
      }
    }

    // Repeated stability, wait for previous timeout.
    TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_CAPTURE_SNAPSHOT);
    return true;
  }

  private void swapSnapshots() {
    if (mCurrentSnapshot == null) {
      // First snapshot.
      mCurrentSnapshot = mTracerImpl.createSnapshot();
    } else if (mPreviousSnapshot == null) {
      // Second snapshot.
      mPreviousSnapshot = mCurrentSnapshot;
      mCurrentSnapshot = mTracerImpl.createSnapshot();
    } else {
      // Subsequent snapshots.
      FSPSnapshot temp = mCurrentSnapshot;
      mCurrentSnapshot = mPreviousSnapshot;
      mPreviousSnapshot = temp;
      mCurrentSnapshot.reset();
    }
  }

  private void updateSnapshot() {
    TraceEvent.beginSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_UPDATE_SNAPSHOT);

    if (mCurrentSnapshot == null) {
      return;
    }

    LynxUIOwner uiOwner = mContext.getLynxUIOwner();
    UIBody rootUI = uiOwner != null ? uiOwner.getRootUI() : null;
    if (rootUI == null) {
      return;
    }

    mCurrentSnapshot.setContainerSize(rootUI.getWidth(), rootUI.getHeight());

    boolean hasLoadingUI = false;
    for (LynxBaseUI ui : uiOwner.getAllUIs()) {
      switch (ui.getMeaningfulContentStatus()) {
        case LOADED:
          Rect contentRect = ui.getMeaningfulContentRect();
          if (contentRect.isEmpty()) {
            contentRect = ui.getBoundingClientRect();
          }

          long changeTs = ui.getMeaningfulContentChangeTimestampNs();
          FSPContentInfo contentInfo = new FSPContentInfo(ui.getSign(), changeTs, contentRect);
          mTracerImpl.fillSnapshot(mCurrentSnapshot, contentInfo);
          break;
        case LOADING:
          hasLoadingUI = true;
          break;
        default:
          break;
      }
    }

    mHasLoadingUI = hasLoadingUI;
    TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_UPDATE_SNAPSHOT);
  }

  private FSPResult diffSnapshots() {
    if (mCurrentSnapshot == null || mPreviousSnapshot == null) {
      return null;
    }

    TraceEvent.beginSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_IMPL_DIFF_SNAPSHOTS);
    boolean isStable = mTracerImpl.diffSnapshots(mCurrentSnapshot, mPreviousSnapshot);
    FSPResult result = new FSPResult(isStable, mCurrentSnapshot.getLastChangeUISign(),
        mCurrentSnapshot.getLastChangeTimestampNs(), mCurrentSnapshot.getContainerWidth(),
        mCurrentSnapshot.getContainerHeight());

    TraceEvent.endSection(TraceEventDef.CATEGORY, TraceEventDef.FSP_TRACER_IMPL_DIFF_SNAPSHOTS);
    return result;
  }

  private void beginHardTimeout() {
    if (mConfig.getHardTimeout() > 0.0) {
      if (mHardTimeoutRunnable != null) {
        mMainHandler.removeCallbacks(mHardTimeoutRunnable);
      }

      WeakReference<FSPTracer> weakSelf = new WeakReference<>(this);
      mHardTimeoutRunnable = () -> {
        FSPTracer strongSelf = weakSelf.get();
        if (strongSelf == null) {
          return;
        }

        strongSelf.stopTracing(StopReason.TIMEDOUT);
      };
      mMainHandler.postDelayed(mHardTimeoutRunnable, (long) (mConfig.getHardTimeout() * 1000));
    }
  }

  private void beginGracefulTimeout(double timeout) {
    if (mObserverCallback != null) {
      mObserverCallback.willStartGracefulTimeout(timeout);
    }

    // Cancel any existing graceful timeout block.
    if (mGracefulTimeoutRunnable != null) {
      mMainHandler.removeCallbacks(mGracefulTimeoutRunnable);
    }

    // Create a new cancellable block for graceful timeout.
    if (mGracefulTimeoutRunnable == null) {
      WeakReference<FSPTracer> weakSelf = new WeakReference<>(this);
      mGracefulTimeoutRunnable = () -> {
        FSPTracer strongSelf = weakSelf.get();
        if (strongSelf == null) {
          return;
        };

        strongSelf.onGracefulTimeout(timeout);
      };
    }

    // Schedule the graceful timeout block to run after the timeout.
    mMainHandler.postDelayed(mGracefulTimeoutRunnable, (long) (timeout * 1000));
  }

  private void onGracefulTimeout(double timeout) {
    double extension = mConfig.getExtendedGracefulTimeout() - timeout;
    if (extension > 0 && mContext.checkPendingMeaningfulContentLoading()) {
      // Check again for loading UIs. If there are still loading UIs since last
      // snapshot, extend the timeout.
      beginGracefulTimeout(extension);
      return;
    }

    if (mLastResult != null && mLastResult.isSuccess()) {
      stopTracing(StopReason.SUCCESS, mLastResult);
    }
  }
}
