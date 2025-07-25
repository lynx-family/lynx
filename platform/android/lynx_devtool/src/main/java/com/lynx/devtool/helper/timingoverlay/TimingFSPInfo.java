// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.helper.timingoverlay;

import android.graphics.Color;
import android.graphics.Point;
import android.os.SystemClock;
import android.text.TextUtils;
import com.lynx.tasm.performance.fsptracing.FSPResult;
import com.lynx.tasm.performance.fsptracing.FSPTracer;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

/**
 * Information about FSP (First Screen Paint) tracking.
 * Android equivalent of iOS LynxTimingFSPInfo.
 */
public class TimingFSPInfo {
  public enum FSPState { NONE, TRACKING, STABLE, SUCCESS, FAILURE }

  private FSPState mState = FSPState.NONE;
  private long mStartTimestampNs;
  private FSPTracer.StopReason mStopReason = null;
  private WeakReference<FSPTracer> mTracer = null;
  private FSPResult mResult = null;
  private Point mContentSize = new Point(0, 0);

  public TimingFSPInfo() {
    this.mStartTimestampNs = SystemClock.elapsedRealtimeNanos();
  }

  /**
   * Get the sign of the UI element to highlight.
   * @return UI element sign or -1 if none
   */
  public int getHighlightElementSign() {
    if (mResult != null) {
      return mResult.getLastUISign();
    }
    return -1;
  }

  /**
   * Get the background color for the overlay based on FSP state.
   * @return Color to use for the overlay background
   */
  public int getBackgroundColor() {
    switch (mState) {
      case NONE:
        return Color.argb(0, 0, 0, 0); // Clear color
      case TRACKING:
        return Color.argb(64, 0, 128, 128); // Translucent teal (similar to iOS)
      case STABLE:
        return Color.argb(64, 0, 0, 128); // Translucent blue
      case SUCCESS:
        return Color.argb(64, 0, 128, 0); // Translucent green
      case FAILURE:
        return Color.argb(64, 128, 0, 0); // Translucent red
      default:
        return Color.argb(0, 0, 0, 0); // Clear color
    }
  }

  /**
   * Get information lines to display in the overlay.
   * @return List of strings with FSP information
   */
  public List<String> getInformationLines() {
    ArrayList<String> lines = new ArrayList<>();

    String fspDescStr = "";
    switch (mState) {
      case NONE:
        return lines;
      case TRACKING:
        fspDescStr = "Tracking";
        break;
      case STABLE:
        if (mResult != null) {
          fspDescStr = String.format("Stable: %d ms", getDurationMs());
        } else {
          fspDescStr = "Stable";
        }
        break;
      case SUCCESS:
        if (mResult != null) {
          fspDescStr = String.format("Reached: %d ms", getDurationMs());
        } else {
          fspDescStr = "Success";
        }
        break;
      case FAILURE:
        fspDescStr = "Failed";
        if (mStopReason == FSPTracer.StopReason.TIMEDOUT) {
          fspDescStr += " (Timeout)";
        } else if (mStopReason == FSPTracer.StopReason.USER_INTERACTION) {
          fspDescStr += " (User Action)";
        }
        break;
    }

    lines.add(String.format("FSP: %s", fspDescStr));

    if (mContentSize.x > 0 && mContentSize.y > 0) {
      lines.add(String.format(" Content: %dx%d", mContentSize.x, mContentSize.y));
    }

    if (mResult != null && mResult.getLastUISign() != -1) {
      lines.add(String.format(" Last UI: %d", mResult.getLastUISign()));
    }

    return lines;
  }

  /**
   * Get snapshot detail lines for debugging.
   * @return List of strings with snapshot details
   */
  public List<String> getSnapshotDetailLines() {
    if (mState != FSPState.STABLE && mState != FSPState.SUCCESS) {
      return null;
    }

    FSPTracer tracerRef = mTracer.get();
    if (tracerRef == null) {
      return null;
    }

    ArrayList<String> lines = new ArrayList<>();
    lines.add("FSP Snapshot:");
    lines.add(TextUtils.join("\n  ", tracerRef.inspectCurrentSnapshot()));
    return lines;
  }

  /**
   * Check if FSP tracking is active.
   * @return true if tracking, false otherwise
   */
  public boolean isTracking() {
    return mState == FSPState.TRACKING;
  }

  /**
   * Get the current FSP state.
   * @return Current FSP state
   */
  public FSPState getState() {
    return mState;
  }

  /**
   * Get the duration since FSP tracking started.
   * @return Duration in milliseconds
   */
  public long getDurationMs() {
    long durationNs = mResult.getLastChangeTimestampNs() - mStartTimestampNs;
    double durationMs = (double) durationNs / 1000_000.0;
    return (long) durationMs;
  }

  /**
   * Set the current FSP state.
   * @param state New FSP state
   */
  public void setState(FSPState state) {
    this.mState = state;
  }

  /**
   * Set the FSP result.
   * @param mResult The FSP tracing result
   */
  public void setResult(FSPResult mResult) {
    this.mResult = mResult;
  }

  /**
   * Set the FSP tracer.
   * @param tracer
   */
  public void setTracer(FSPTracer tracer) {
    this.mTracer = new WeakReference<>(tracer);
  }

  /**
   * Set the stop reason for FSP tracking.
   * @param reason The reason why FSP tracking stopped
   */
  public void setStopReason(FSPTracer.StopReason reason) {
    this.mStopReason = reason;
  }
}
