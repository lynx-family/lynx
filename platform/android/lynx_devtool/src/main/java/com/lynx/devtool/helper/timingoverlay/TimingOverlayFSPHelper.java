// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.helper.timingoverlay;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.performance.fsptracing.FSPResult;
import com.lynx.tasm.performance.fsptracing.FSPTracer;
import java.lang.ref.WeakReference;

/**
 * Helper class for FSP (First Screen Paint) tracking.
 * Android equivalent of iOS LynxTimingOverlayHelper+FSP category.
 */
public class TimingOverlayFSPHelper implements FSPTracer.IObserverCallback {
  private static final String TAG = "TimingOverlayFSPHelper";

  private final WeakReference<TimingOverlayHelper> mHelper;
  private TimingFSPInfo mFSPInfo;
  private FSPTracer mFSPTracer;
  private final Handler mMainHandler = new Handler(Looper.getMainLooper());

  /**
   * Constructor.
   * @param helper Parent TimingOverlayHelper
   */
  public TimingOverlayFSPHelper(@NonNull TimingOverlayHelper helper) {
    mHelper = new WeakReference<>(helper);
    mFSPInfo = new TimingFSPInfo();
  }

  /**
   * Set up FSP tracking with the given UI owner.
   * @param uiOwner LynxUIOwner to track
   */
  public void setupFSP(LynxUIOwner uiOwner) {
    if (uiOwner == null) {
      return;
    }

    mFSPInfo = new TimingFSPInfo();

    // Set this helper as the observer for FSP tracing
    mFSPTracer = uiOwner.getFSPTracer();
    if (mFSPTracer != null) {
      mFSPTracer.setObserverCallback(this);
      mFSPInfo.setTracer(mFSPTracer);
    }

    // Log FSP setup
    TimingOverlayHelper helper = mHelper.get();
    if (helper != null) {
      helper.log(LLog.INFO,
          "FSP tracking setup for view: " + uiOwner.getRootUI().getLynxContext().getLynxView());
    }
  }

  /**
   * Get the current FSP information.
   * @return Current FSP information
   */
  public TimingFSPInfo getFSPInfo() {
    return mFSPInfo;
  }

  // FSPTracer.ObserverCallback implementation

  @Override
  public void onStart() {
    mFSPInfo.setState(TimingFSPInfo.FSPState.TRACKING);

    TimingOverlayHelper helper = mHelper.get();
    if (helper != null) {
      helper.setNeedsUpdate();

      // Log FSP tracking start
      helper.log(LLog.INFO, "FSP tracking started");
    }
  }

  @Override
  public void onBecomeStable(boolean stable, @NonNull FSPResult result) {
    if (stable) {
      mFSPInfo.setState(TimingFSPInfo.FSPState.STABLE);
      mFSPInfo.setResult(result);

      TimingOverlayHelper helper = mHelper.get();
      if (helper != null) {
        // Log FSP stable state
        helper.log(LLog.INFO,
            String.format("FSP become stable, timestamp: %d ms", mFSPInfo.getDurationMs()));

        if (result.getLastUISign() != -1) {
          helper.log(LLog.INFO,
              String.format("Last ui element before stable: %d", result.getLastUISign()));
        }

        helper.log(LLog.VERBOSE,
            String.format(
                "FSP snapshot:\n  %s", String.join("\n  ", mFSPInfo.getSnapshotDetailLines())));
      }
    } else {
      mFSPInfo.setState(TimingFSPInfo.FSPState.TRACKING);
      mFSPInfo.setResult(null);
    }

    TimingOverlayHelper helper = mHelper.get();
    if (helper != null) {
      helper.setNeedsUpdate();
    }
  }

  @Override
  public void onStop(@NonNull FSPTracer.StopReason reason, @NonNull FSPResult result) {
    if (result.isSuccess()) {
      mFSPInfo.setState(TimingFSPInfo.FSPState.SUCCESS);
      mFSPInfo.setResult(result);
      mFSPInfo.setStopReason(reason);

      TimingOverlayHelper helper = mHelper.get();
      if (helper != null) {
        // Log FSP success
        helper.log(
            LLog.INFO, String.format("FSP reached, timestamp: %d ms", mFSPInfo.getDurationMs()));

        if (result.getLastUISign() != -1) {
          helper.log(
              LLog.INFO, String.format("Last ui element before stop: %d", result.getLastUISign()));
        }
      }
    } else {
      mFSPInfo.setState(TimingFSPInfo.FSPState.FAILURE);
      mFSPInfo.setResult(result);
      mFSPInfo.setStopReason(reason);

      TimingOverlayHelper helper = mHelper.get();
      if (helper != null) {
        // Log FSP failure
        if (reason == FSPTracer.StopReason.TIMEDOUT) {
          helper.log(LLog.ERROR, "FSP timeout");
        } else if (reason == FSPTracer.StopReason.USER_INTERACTION) {
          helper.log(LLog.ERROR, "FSP failed due to user action");
        }
      }
    }

    TimingOverlayHelper helper = mHelper.get();
    if (helper != null) {
      helper.setNeedsUpdate();
    }

    // Auto-hide the overlay after a delay for success or failure states
    if (mFSPInfo.getState() == TimingFSPInfo.FSPState.SUCCESS
        || mFSPInfo.getState() == TimingFSPInfo.FSPState.FAILURE) {
      mMainHandler.postDelayed(() -> {
        TimingOverlayHelper helperRef = mHelper.get();
        if (helperRef != null) {
          mFSPInfo.setState(TimingFSPInfo.FSPState.NONE);
          helperRef.log(LLog.INFO, "FSP tracking stopped");
          helperRef.setNeedsUpdate();
        }
      }, 1000); // 1 second delay
    }
  }

  @Override
  public void willStartGracefulTimeout(double timeout) {
    // Log FSP graceful timeout
    TimingOverlayHelper helper = mHelper.get();
    if (helper != null) {
      helper.log(
          LLog.INFO, String.format("FSP will start graceful timeout: %.2f ms", timeout * 1000));
    }
  }
}
