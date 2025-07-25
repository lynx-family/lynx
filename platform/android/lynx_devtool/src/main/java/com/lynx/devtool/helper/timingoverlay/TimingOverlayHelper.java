// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.helper.timingoverlay;

import android.graphics.Rect;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import com.lynx.devtool.DevToolPlatformAndroidDelegate;
import com.lynx.devtool.LynxDevtoolEnv;
import com.lynx.devtool.helper.timingoverlay.TimingOverlayFSPHelper;
import com.lynx.devtool.helper.timingoverlay.TimingOverlayView;
import com.lynx.devtoolwrapper.LogBoxLogLevel;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewClientV2;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.performance.performanceobserver.MetricActualFmpEntry;
import com.lynx.tasm.performance.performanceobserver.MetricFcpEntry;
import com.lynx.tasm.performance.performanceobserver.MetricFspEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceMetric;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * Helper class for displaying timing information overlay.
 * Android equivalent of iOS LynxTimingOverlayHelper.
 */
public class TimingOverlayHelper extends LynxViewClientV2 implements TimingOverlayView.DataSource {
  private static final String TAG = "TimingOverlayHelper";

  // Static list of all timing overlay helpers
  private static final List<WeakReference<TimingOverlayHelper>> sAllTimingOverlays =
      new ArrayList<>();

  // Instance variables
  private final WeakReference<DevToolPlatformAndroidDelegate> mDelegate;
  private WeakReference<LynxView> mView;
  private WeakReference<LynxUIOwner> mUIOwner;
  private TimingOverlayView mOverlay;
  private boolean mNeedsUpdate;
  private final Map<Integer, PerformanceMetric> mMetricInfos;
  private final TimingOverlayFSPHelper mFSPHelper;

  /**
   * Set timing overlay enabled/disabled for all instances.
   * @param enabled true to enable, false to disable
   */
  public static void setEnabled(boolean enabled) {
    LynxDevtoolEnv.inst().enableTimingOverlay(enabled);

    // Update all existing instances
    for (WeakReference<TimingOverlayHelper> ref : sAllTimingOverlays) {
      TimingOverlayHelper helper = ref.get();
      if (helper != null) {
        helper.updateEnabled(enabled);
      }
    }

    if (enabled) {
      // Remove any null references
      sAllTimingOverlays.removeIf(ref -> ref.get() == null);
    } else {
      sAllTimingOverlays.clear();
    }
  }

  /**
   * Constructor.
   * @param delegate DevTool platform delegate
   */
  public TimingOverlayHelper(@NonNull DevToolPlatformAndroidDelegate delegate) {
    mDelegate = new WeakReference<>(delegate);
    mMetricInfos = new TreeMap<>();
    mFSPHelper = new TimingOverlayFSPHelper(this);
  }

  /**
   * Attach to a LynxView and UIOwner.
   * @param view LynxView to attach to
   * @param uiOwner UIOwner to attach to
   */
  public void attachLynxView(@NonNull LynxView view, @NonNull LynxUIOwner uiOwner) {
    if (!LynxDevtoolEnv.inst().isTimingOverlayEnabled()) {
      return;
    }

    // Add to static list
    sAllTimingOverlays.add(new WeakReference<>(this));

    // Reset state
    mNeedsUpdate = false;
    mMetricInfos.clear();

    // Store references
    mView = new WeakReference<>(view);
    mUIOwner = new WeakReference<>(uiOwner);

    // Setup FSP tracking
    mFSPHelper.setupFSP(uiOwner);

    // Setup Performance Observer
    view.addLynxViewClientV2(this);

    // Create overlay view
    ViewGroup rootView =
        view.getRootView() instanceof ViewGroup ? (ViewGroup) view.getRootView() : null;

    if (rootView != null) {
      // Create overlay view with the target view (mView) that it should follow
      mOverlay = new TimingOverlayView(view.getContext(), this, view);

      // Add overlay to root view
      ViewGroup.MarginLayoutParams params = new ViewGroup.MarginLayoutParams(0, 0);
      rootView.addView(mOverlay, params);
    }
  }

  /**
   * Mark the overlay as needing an update.
   */
  public void setNeedsUpdate() {
    mNeedsUpdate = true;
    if (mOverlay != null) {
      mOverlay.setNeedsUpdate();
    }
  }

  /**
   * Log a message.
   * @param level Log level
   * @param message Message to log
   */
  public void log(int level, String message) {
    // Log to system
    switch (level) {
      case LLog.VERBOSE:
        LLog.v(TAG, message);
        break;
      case LLog.DEBUG:
        LLog.d(TAG, message);
        break;
      case LLog.INFO:
        LLog.i(TAG, message);
        break;
      case LLog.WARN:
        LLog.w(TAG, message);
        break;
      case LLog.ERROR:
        LLog.e(TAG, message);
        break;
    }

    // Send to DevTool delegate
    DevToolPlatformAndroidDelegate delegate = mDelegate.get();
    if (delegate != null) {
      LogBoxLogLevel logBoxLevel;
      switch (level) {
        case LLog.WARN:
          logBoxLevel = LogBoxLogLevel.Warn; // Warning
          break;
        case LLog.ERROR:
          logBoxLevel = LogBoxLogLevel.Error; // Error
          break;
        default:
          logBoxLevel = LogBoxLogLevel.Info; // Info
          break;
      }

      delegate.sendConsoleEvent(message, logBoxLevel.ordinal(), System.currentTimeMillis());
    }
  }

  /**
   * Handle a performance event.
   * @param entry Performance entry
   */
  public void onPerformanceEvent(@NonNull PerformanceEntry entry) {
    if (entry == null) {
      return;
    }

    if (entry instanceof MetricFcpEntry) {
      // First Contentful Paint
      MetricFcpEntry fcpEntry = (MetricFcpEntry) entry;
      mMetricInfos.put(0, fcpEntry.fcp);
      mMetricInfos.put(1, fcpEntry.lynxFcp);
      mMetricInfos.put(2, fcpEntry.totalFcp);
    } else if (entry instanceof MetricActualFmpEntry) {
      // Actual First Meaningful Paint
      MetricActualFmpEntry fmpEntry = (MetricActualFmpEntry) entry;
      mMetricInfos.put(3, fmpEntry.actualFmp);
      mMetricInfos.put(4, fmpEntry.lynxActualFmp);
      mMetricInfos.put(5, fmpEntry.totalActualFmp);
    } else if (entry instanceof MetricFspEntry) {
      // First Screen Paint
      MetricFspEntry fspEntry = (MetricFspEntry) entry;
      mMetricInfos.put(6, fspEntry.fsp);
      mMetricInfos.put(7, fspEntry.lynxFsp);
      mMetricInfos.put(8, fspEntry.totalFsp);
    }

    setNeedsUpdate();
  }

  /**
   * Update timing overlay enabled/disabled for this instance.
   * @param enabled true to enable, false to disable
   */
  private void updateEnabled(boolean enabled) {
    if (enabled) {
      if (mOverlay == null && mView != null && mView.get() != null && mUIOwner != null
          && mUIOwner.get() != null) {
        attachLynxView(mView.get(), mUIOwner.get());
      }
    } else {
      if (mOverlay != null) {
        ViewGroup parent = (ViewGroup) mOverlay.getParent();
        if (parent != null) {
          parent.removeView(mOverlay);
        }
        mOverlay = null;
        LynxView view = mView.get();
        if (view != null) {
          view.removeLynxViewClientV2(this);
        }
      }
    }
  }

  // TimingOverlayView.DataSource implementation

  @Override
  public boolean overlayViewIsHidden() {
    return !mFSPHelper.getFSPInfo().isTracking();
  }

  @Override
  public Integer overlayViewBackgroundColor() {
    return mFSPHelper.getFSPInfo().getBackgroundColor();
  }

  @Override
  public Rect overlayViewHighlightRect() {
    int sign = mFSPHelper.getFSPInfo().getHighlightElementSign();
    if (sign != -1) {
      LynxBaseUI ui = mUIOwner.get().findLynxUIBySign(sign);
      if (ui != null) {
        return ui.getBoundingClientRect();
      }
    }
    return null;
  }

  @NonNull
  @Override
  public List<String> overlayViewDescriptionLines() {
    // Get FSP information lines
    List<String> lines = new ArrayList<>(mFSPHelper.getFSPInfo().getInformationLines());

    // Add metric information lines
    if (!mMetricInfos.isEmpty()) {
      lines.add("------");
      lines.add("Perf Timing:");

      List<Integer> sortedKeys = new ArrayList<>(mMetricInfos.keySet());
      Collections.sort(sortedKeys);
      for (Integer key : sortedKeys) {
        PerformanceMetric metric = mMetricInfos.get(key);
        if (metric != null) {
          lines.add(String.format(" %s = %s ms", metric.name, metric.duration));
        }
      }
    }

    return lines;
  }
}
