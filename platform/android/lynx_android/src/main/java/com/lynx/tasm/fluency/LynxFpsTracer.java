// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.fluency;

import android.os.SystemClock;
import android.view.Choreographer;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.DeviceUtils;
import java.lang.ref.WeakReference;
import java.util.LinkedList;
import java.util.List;

public class LynxFpsTracer {
  private static final String TAG = "LynxFpsTracer";

  private final WeakReference<LynxContext> mContext;

  private volatile boolean mFPSState = false;

  private IFluencyCallback mIFluencyCallback;
  private Choreographer.FrameCallback mFrameCallback;

  private LinkedList<Long> mFrameCostList;

  private long mStartTimeNanos = -1L;
  private long mLastFrameNanos = -1L;
  private int mCounter = 0;
  private long mStartTime;
  private long mMonitorDur;
  private int mMaxRefreshRate = -1;

  private static final int MIN_DROP_FRAME = 0;
  private static final double mDesiredFrameInterval = 1.0 / 60.0;
  private static final int OFFSET_TO_MS = 100;
  private static final double TIME_S_TO_MS = 1000d;
  private static final double TIME_MS_TO_NS = 1000000d;

  public LynxFpsTracer(LynxContext context) {
    mContext = new WeakReference<>(context);
    mFrameCostList = new LinkedList<>();
  }

  public void setFluencyCallback(IFluencyCallback fluencyCallback) {
    this.mIFluencyCallback = fluencyCallback;
  }

  public interface IFluencyCallback {
    /**
     * callback monitor stop every time
     */
    void report(LynxFpsRawMetrics rawMetrics);
  }

  public static class LynxFpsRawMetrics {
    public int frames;
    public int fps;
    public int maximumFrames;
    public long duration;
    public int drop1;
    public long drop1Duration;
    public int drop3;
    public long drop3Duration;
    public int drop7;
    public long drop7Duration;
    public int drop25;
    public long drop25Duration;

    public LynxFpsRawMetrics() {}
  }

  public void start() {
    if (mFPSState) {
      return;
    }
    startInternal();
    mFPSState = true;
  }

  public void stop() {
    if (!mFPSState) {
      return;
    }
    endInternal();
    mFPSState = false;
  }

  private void startInternal() {
    mFrameCostList.clear();
    // reset
    mStartTimeNanos = -1L;
    mLastFrameNanos = -1L;
    mCounter = 0;
    mFrameCallback = new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        if (mStartTimeNanos == -1) {
          mStartTimeNanos = frameTimeNanos;
        }
        // if you need frame information, send callback here

        ++mCounter;
        if (mFPSState) {
          Choreographer.getInstance().postFrameCallback(this);
        }

        doDropCompute(mLastFrameNanos, frameTimeNanos);

        mLastFrameNanos = frameTimeNanos;
      }
    };
    try {
      mStartTime = SystemClock.elapsedRealtime();
      Choreographer.getInstance().postFrameCallback(mFrameCallback);
    } catch (Exception ignore) {
      mFPSState = false;
      mStartTimeNanos = -1L;
      mLastFrameNanos = -1L;
      mCounter = 0;
      mFrameCallback = null;
      mStartTime = -1L;
    }
  }

  private void endInternal() {
    mMonitorDur = SystemClock.elapsedRealtime() - mStartTime;
    if (mFrameCallback != null) {
      Choreographer.getInstance().removeFrameCallback(mFrameCallback);
    }
    doReport();
  }

  /**
   * record frame-drop count of every time
   * @param lastFrameNanos
   * @param frameNanos
   */
  private void doDropCompute(final long lastFrameNanos, final long frameNanos) {
    if (mLastFrameNanos <= 0) {
      return;
    }
    // milliseconds
    long cost = (long) ((frameNanos - lastFrameNanos) / TIME_MS_TO_NS);
    if (cost <= 0) {
      return;
    }
    if (mMaxRefreshRate == -1) {
      mMaxRefreshRate = getRefreshRate();
    }
    // Record it in the time-consuming list. When the sliding stops, calculate the delay of
    // this sliding.
    if (mFrameCostList.size() > 20000) {
      mFrameCostList.poll();
    }
    // The unit of the recorded value is milliseconds * 100
    long costMillis100 = (long) ((frameNanos - lastFrameNanos) / TIME_MS_TO_NS * OFFSET_TO_MS);
    mFrameCostList.add(costMillis100);
  }

  /**
   * report drop-count -> hit-times
   */
  private void doReport() {
    LynxFpsRawMetrics results = new LynxFpsRawMetrics();
    results.duration = mMonitorDur;

    // calculate fps
    long interval = mLastFrameNanos - mStartTimeNanos;
    if (interval <= 0 || mCounter <= 1) {
      return;
    }
    long fps = (long) ((mCounter - 1) * TIME_MS_TO_NS * TIME_S_TO_MS / interval);

    results.fps = (int) fps;
    results.maximumFrames = mMaxRefreshRate;
    mMaxRefreshRate = -1;

    // calculate drop frame
    if (mFrameCostList.isEmpty()) {
      return;
    }
    final List<Long> reportList = mFrameCostList;
    mFrameCostList = new LinkedList<>();
    if (!reportList.isEmpty()) {
      // The number of dropped frames is always calculated based on 60Hz
      double frameIntervalMillis = mDesiredFrameInterval * TIME_S_TO_MS;
      int maxDropFrame = results.maximumFrames - 1;

      for (Long cost : reportList) {
        // Calculate the number of dropped frames. [0, maxDropFrame)
        int droppedCount =
            Math.max(Math.min(getDroppedCount(cost, frameIntervalMillis), maxDropFrame), 0);
        long dropDuration = cost / OFFSET_TO_MS - (long) frameIntervalMillis;
        // Count the calculation results of the number of dropped frames.
        results.frames++;
        if (droppedCount >= 1) {
          results.drop1++;
          results.drop1Duration += dropDuration;
        } else {
          continue;
        }
        if (droppedCount >= 3) {
          results.drop3++;
          results.drop3Duration += dropDuration;
        } else {
          continue;
        }
        if (droppedCount >= 7) {
          results.drop7++;
          results.drop7Duration += dropDuration;
        } else {
          continue;
        }
        if (droppedCount >= 25) {
          results.drop25++;
          results.drop25Duration += dropDuration;
        }
      }
    }

    // do report
    if (mIFluencyCallback != null) {
      mIFluencyCallback.report(results);
    }
  }

  // cost is ms * 100
  private int getDroppedCount(Long cost, double frameInterval) {
    int refreshRate = (int) (frameInterval * OFFSET_TO_MS);
    return (int) ((cost + (refreshRate - 1)) / refreshRate - 1);
  }

  int getRefreshRate() {
    LynxContext context = mContext.get();
    if (context != null) {
      return getRoundedRate(DeviceUtils.getRefreshRate(context));
    }
    return 60;
  }

  private int getRoundedRate(float refreshRate) {
    float ROUNDING_THRESHOLD = 5.1f;
    // Assuming that the ROM has been modified, it may return a value of 62.00 or 57.00
    if (Math.abs(refreshRate - 60) < ROUNDING_THRESHOLD) {
      return 60;
    } else if (Math.abs(refreshRate - 90) < ROUNDING_THRESHOLD) {
      return 90;
    } else if (Math.abs(refreshRate - 120) < ROUNDING_THRESHOLD) {
      return 120;
    }
    return (int) refreshRate;
  }
}
