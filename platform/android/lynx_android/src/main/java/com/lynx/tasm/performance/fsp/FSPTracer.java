// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import android.graphics.Rect;
import androidx.annotation.AnyThread;
import androidx.annotation.UiThread;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.ui.ILynxUIMeaningfulContent;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.PerformanceController;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.BitSet;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * FSP (First Stable Paint) Tracer implementation for Android platform.
 * Based on C++ core implementation and iOS interface.
 */
public class FSPTracer {
  // Status enum for type-safe state representation
  public enum ResultStatus {
    SUCCESS("success"),
    CANCEL_BY_USER_INTERACTION("cancelByUserInteraction"),
    CANCEL_BY_TIMEOUT("timeout"),
    STOP("stop"),
    ERROR("error");

    private final String value;

    ResultStatus(String value) {
      this.value = value;
    }

    public String getValue() {
      return value;
    }

    // Helper method to get enum from string value
    public static ResultStatus fromValue(String value) {
      for (ResultStatus status : ResultStatus.values()) {
        if (status.value.equals(value)) {
          return status;
        }
      }
      return null;
    }
  }

  private static final String TAG = "FSPTracer";
  // HashMap keys for FSP timing info
  private static final String KEY_FSP_STATUS = "fspStatus";
  private static final String KEY_CONTENT_FILL_PERCENTAGE_X = "contentFillPercentageX";
  private static final String KEY_CONTENT_FILL_PERCENTAGE_Y = "contentFillPercentageY";
  private static final String KEY_CONTENT_FILL_PERCENTAGE_TOTAL_AREA =
      "contentFillPercentageTotalArea";
  private static final String KEY_CONTAINER_FILL_PERCENTAGE_CONTAINER_AREA =
      "containerFillPercentageContainerArea";
  // Tracer fields
  private final FSPConfig mConfig = new FSPConfig();
  private final AtomicBoolean mIsRunning = new AtomicBoolean(false);
  private volatile FSPSnapshot mPreviousSnapshot;
  private WeakReference<PerformanceController> mPerfControllerRef = new WeakReference<>(null);

  /**
   * Constructor with configuration
   * @param config FSP configuration
   */
  public FSPTracer(PerformanceController perfController) {
    if (perfController != null) {
      mPerfControllerRef = new WeakReference<>(perfController);
    }
  }

  /**
   * Start monitoring for FSP
   * @param callback Completion callback to be invoked when FSP is detected
   */
  @UiThread
  public void start(IMeaningfulContentSnapshotCaptureHandler captureHandler) {
    if (captureHandler == null || !mConfig.enable || mIsRunning.get()) {
      return;
    }
    // 1. setup config.
    mConfig.parse();
    mIsRunning.set(true);
    // 2. start snapshot timer
    scheduleNextCapture(captureHandler);
    // 3. Start hard timeout if configured
    startHardTimeout();
  }

  /**
   * Stop monitoring
   * @param currentTimestampUs Current timestamp in microseconds
   */
  @UiThread
  public void stop() {
    if (!mConfig.enable || !mIsRunning.get()) {
      return;
    }
    internalStop(ResultStatus.STOP);
  }

  /**
   * Cancel monitoring due to user interaction
   * @param currentTimestampUs Current timestamp in microseconds
   */
  @UiThread
  public void cancelledByUserInteraction() {
    if (!mConfig.enable || !mIsRunning.get()) {
      return;
    }
    internalStop(ResultStatus.CANCEL_BY_USER_INTERACTION);
  }

  @UiThread
  private void scheduleNextCapture(final IMeaningfulContentSnapshotCaptureHandler captureHandler) {
    if (mConfig.snapshotIntervalMs <= 0 || captureHandler == null) {
      return;
    }
    // Use a weak reference to prevent memory leaks
    final WeakReference<FSPTracer> weakSelf = new WeakReference<>(this);
    UIThreadUtils.runOnUiThread(() -> {
      FSPTracer tracer = weakSelf.get();
      if (tracer == null || !tracer.mIsRunning.get() || captureHandler == null) {
        return;
      }
      // Capture the snapshot
      MeaningfulContentSnapshot snapshot = captureHandler.capture();
      if (snapshot == null) {
        // Schedule next capture even if current capture returns null
        tracer.scheduleNextCapture(captureHandler);
        return;
      }
      tracer.handleSnapshotCapture(snapshot);
      // For now, we'll just schedule the next capture
      // In a real implementation, we would convert the snapshot and process it
      tracer.scheduleNextCapture(captureHandler);
    }, mConfig.snapshotIntervalMs);
  }

  @UiThread
  private void startHardTimeout() {
    if (mConfig.hardTimeoutMs <= 0) {
      return;
    }
    LynxEventReporter.delayRunOnReportThread(() -> {
      if (!mConfig.enable || !mIsRunning.get()) {
        return;
      }
      internalStop(ResultStatus.CANCEL_BY_TIMEOUT);
    }, mConfig.hardTimeoutMs);
  }

  @AnyThread
  private void internalStop(ResultStatus status) {
    mIsRunning.set(false);
    long currentTimestampUs = PerformanceController.currentSystemTimeMicroseconds();
    if (currentTimestampUs <= 0) {
      return;
    }
    handleFSPResult(status, mPreviousSnapshot, currentTimestampUs);
  }

  @UiThread
  /// Process raw meaningful content snapshot to fsp snapshot, and check if it is valuable and
  /// stable.
  private void handleSnapshotCapture(final MeaningfulContentSnapshot rawSnapshot) {
    if (rawSnapshot.getContainerWidth() <= 0 || rawSnapshot.getContainerHeight() <= 0
        || rawSnapshot.getMeaningfulPaintingAreas() == null
        || rawSnapshot.getMeaningfulPaintingAreas().isEmpty()) {
      return;
    }
    long currentTimestampUs = PerformanceController.currentSystemTimeMicroseconds();
    final WeakReference<FSPTracer> weakSelf = new WeakReference<>(this);
    LynxEventReporter.runOnReportThread(() -> {
      FSPTracer tracer = weakSelf.get();
      if (tracer == null) {
        return;
      }
      List<MeaningfulPaintingArea> areaList = rawSnapshot.getMeaningfulPaintingAreas();
      if (areaList == null) {
        return;
      }
      FSPSnapshot snapshot = new FSPSnapshot(
          rawSnapshot.getContainerWidth(), rawSnapshot.getContainerHeight(), currentTimestampUs);
      for (MeaningfulPaintingArea area : areaList) {
        boolean isPresented = area.getMeaningfulContentStatus()
            == ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED;

        snapshot.fillContentToSnapshot(isPresented,
            new Rect(area.getOffsetX(), area.getOffsetY(), area.getOffsetX() + area.getWidth(),
                area.getOffsetY() + area.getHeight()),
            area.getFirstMeaningfulContentPresentedTimestampMicros());
      }
      snapshot.traceCurrentTimestampUs = rawSnapshot.getTraceCurrentTimestampUs();
      tracer.onCaptureSnapshot(snapshot);
    });
  }

  /// @note Run on ReportThread
  /// Process captured fsp snapshot
  public void onCaptureSnapshot(FSPSnapshot snapshot) {
    if (!mConfig.enable || !mIsRunning.get()) {
      return;
    }

    // Check if the snapshot is valuable
    if (!isSnapshotValuable(snapshot, mConfig)) {
      // If not valuable, reset previous snapshot
      mPreviousSnapshot = null;
      return;
    }
    if (TraceEvent.isTracingStarted()) {
      TraceEvent.instant(TraceEventDef.CATEGORY_DEFAULT,
          TraceEventDef.FSP_TRACER_ON_VALUABLE_SNAPSHOT, snapshot.traceCurrentTimestampUs,
          snapshot.toMap());
    }
    // If previous snapshot exists, check stability
    if (mPreviousSnapshot != null && mPreviousSnapshot.getLastChangeTimestampUs() > 0
        && isSnapshotStable(snapshot, mPreviousSnapshot, mConfig)) {
      TraceEvent.beginSection(TraceEventDef.FSP_TRACER_SNAPSHOT_STABLE);
      long diffTUs =
          snapshot.getLastChangeTimestampUs() - mPreviousSnapshot.getLastChangeTimestampUs();
      long diffTMs = diffTUs / 1000;
      // Check if interval is greater than min interval
      if (diffTMs >= mConfig.minDiffIntervalMs) {
        // Generate FSP with previous snapshot
        mIsRunning.set(false);
        handleFSPResult(
            ResultStatus.SUCCESS, mPreviousSnapshot, mPreviousSnapshot.getLastChangeTimestampUs());
      }
      TraceEvent.endSection(TraceEventDef.FSP_TRACER_SNAPSHOT_STABLE);
      return;
    }

    // Update previous snapshot if valuable.
    mPreviousSnapshot = snapshot;
  }

  /// @note Run on ReportThread
  /// Check if snapshot is valuable for FSP calculation
  public boolean isSnapshotValuable(FSPSnapshot snapshot, FSPConfig config) {
    // Check X axis projection fill rate
    int totalProjectionW = snapshot.getXTotalContentProjections().cardinality();
    if (totalProjectionW <= 0) {
      return false;
    }

    int xProjectionCount = snapshot.getXProjections().cardinality();
    snapshot.setContentFillPercentageX(xProjectionCount * 100 / totalProjectionW);
    if (snapshot.getContentFillPercentageX() < config.minContentFillPercentageX) {
      return false;
    }

    // Check Y axis projection fill rate
    int totalProjectionH = snapshot.getYTotalContentProjections().cardinality();
    if (totalProjectionH <= 0) {
      return false;
    }

    int yProjectionCount = snapshot.getYProjections().cardinality();
    snapshot.setContentFillPercentageY(yProjectionCount * 100 / totalProjectionH);

    if (snapshot.getContentFillPercentageY() < config.minContentFillPercentageY) {
      return false;
    }

    // Check total area fill rate
    if (snapshot.getTotalContentArea() <= 0) {
      return false;
    }

    snapshot.setContentFillPercentageTotalArea(
        (int) (snapshot.getTotalPresentedContentArea() * 100 / snapshot.getTotalContentArea()));
    if (snapshot.getContentFillPercentageTotalArea() < config.minContentFillPercentageTotalArea) {
      return false;
    }

    snapshot.setContainerFillPercentageContainerArea(
        (int) (snapshot.getTotalPresentedContentArea() * 100 / snapshot.getContainerArea()));
    return snapshot.getContainerFillPercentageContainerArea()
        >= config.minContainerFillPercentageContainerArea;
  }

  /// @note Run on ReportThread
  /// Check if snapshot is stable compared to previous snapshot
  public boolean isSnapshotStable(FSPSnapshot current, FSPSnapshot previous, FSPConfig config) {
    if (previous.getLastChangeTimestampUs() <= 0 || current.getLastChangeTimestampUs() <= 0) {
      return false;
    }

    // Calculate time difference in milliseconds
    long diffTUs = current.getLastChangeTimestampUs() - previous.getLastChangeTimestampUs();
    long diffTMs = diffTUs / 1000;

    if (diffTMs <= 0) {
      return false;
    }

    // Check X projection change rate
    int changeRateW =
        Math.abs(current.getXProjections().cardinality() - previous.getXProjections().cardinality())
        * 1000 / (int) diffTMs;
    if (changeRateW > config.acceptablePixelDiffPerSec) {
      return false;
    }

    // Check Y projection change rate
    int changeRateH =
        Math.abs(current.getYProjections().cardinality() - previous.getYProjections().cardinality())
        * 1000 / (int) diffTMs;
    if (changeRateH > config.acceptablePixelDiffPerSec) {
      return false;
    }

    // Check total meaningful content area change rate
    int areaChangeRate = (int) Math.abs(current.getTotalPresentedContentArea()
                             - previous.getTotalPresentedContentArea())
        * 1000 / (int) diffTMs;
    if (areaChangeRate > config.acceptableAreaDiffPerSec) {
      return false;
    }

    // Check container area change rate
    int containerAreaChangeRate = (int) Math.abs(current.getContainerFillPercentageContainerArea()
                                      - previous.getContainerFillPercentageContainerArea())
        * 1000 / (int) diffTMs;
    if (containerAreaChangeRate > config.acceptableAreaDiffPerSec) {
      return false;
    }

    // Check XOR projection change rate
    BitSet xorX = (BitSet) current.getXProjections().clone();
    xorX.xor(previous.getXProjections());
    int xorChangeRateX = xorX.cardinality() * 1000 / (int) diffTMs;
    if (xorChangeRateX > config.acceptablePixelDiffPerSec) {
      return false;
    }

    BitSet xorY = (BitSet) current.getYProjections().clone();
    xorY.xor(previous.getYProjections());
    int xorChangeRateY = xorY.cardinality() * 1000 / (int) diffTMs;

    return xorChangeRateY <= config.acceptablePixelDiffPerSec;
  }

  @AnyThread
  /// Process fsp result and report to performance controller
  private void handleFSPResult(
      final ResultStatus status, final FSPSnapshot currentSnapshot, final long currentTimestampUs) {
    PerformanceController perfController = mPerfControllerRef.get();
    if (perfController == null) {
      return;
    }
    if (currentSnapshot == null) {
      HashMap<String, String> info = new HashMap<>(1);
      info.put(KEY_FSP_STATUS, status.getValue());
      perfController.setFSPTimingInfo(currentTimestampUs, info);
      if (TraceEvent.isTracingStarted()) {
        UIThreadUtils.runOnUiThread(() -> {
          TraceEvent.instant(
              TraceEventDef.CATEGORY_DEFAULT, TraceEventDef.FSP_TIMING_MARK_FSP_END, info);
        });
      }
      return;
    }
    HashMap<String, String> info = new HashMap<>(1);
    info.put(KEY_FSP_STATUS, status.getValue());
    info.put(
        KEY_CONTENT_FILL_PERCENTAGE_X, String.valueOf(currentSnapshot.getContentFillPercentageX()));
    info.put(
        KEY_CONTENT_FILL_PERCENTAGE_Y, String.valueOf(currentSnapshot.getContentFillPercentageY()));
    info.put(KEY_CONTENT_FILL_PERCENTAGE_TOTAL_AREA,
        String.valueOf(currentSnapshot.getContentFillPercentageTotalArea()));
    info.put(KEY_CONTAINER_FILL_PERCENTAGE_CONTAINER_AREA,
        String.valueOf(currentSnapshot.getContainerFillPercentageContainerArea()));
    perfController.setFSPTimingInfo(currentSnapshot.getLastChangeTimestampUs(), info);
    if (TraceEvent.isTracingStarted()) {
      UIThreadUtils.runOnUiThread(() -> {
        // Add 1 us to make sure this evnet is after the snapshot bitmap
        // event.
        TraceEvent.instant(TraceEventDef.CATEGORY_DEFAULT, TraceEventDef.FSP_TIMING_MARK_FSP_END,
            currentSnapshot.traceCurrentTimestampUs + 1, info);
      });
    }
  }
}
