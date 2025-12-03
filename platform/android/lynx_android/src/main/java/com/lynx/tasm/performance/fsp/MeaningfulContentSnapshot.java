// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import android.graphics.Bitmap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.utils.BitmapUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/// MeaningfulContentSnapshot is a snapshot of meaningful painting areas.
public class MeaningfulContentSnapshot {
  private List<MeaningfulPaintingArea> mMeaningfulPaintingAreas = null;
  private int mContainerWidth = 0;
  private int mContainerHeight = 0;
  private long mTraceCurrentTimestampUs = 0;

  public MeaningfulContentSnapshot(int containerWidth, int containerHeight,
      List<MeaningfulPaintingArea> meaningfulPaintingAreas) {
    mContainerWidth = containerWidth;
    mContainerHeight = containerHeight;
    mMeaningfulPaintingAreas = meaningfulPaintingAreas;
  }

  public void takeScreenshot(ILynxUIRenderer uiRenderer) {
    if (!TraceEvent.isTracingStarted()) {
      return;
    }
    final long traceCurrentTimestampUs = System.nanoTime() / 1000;
    mTraceCurrentTimestampUs = traceCurrentTimestampUs;
    if (uiRenderer == null || !LynxEnv.inst().isFspScreenshotEnabled()) {
      return;
    }
    final Bitmap lynxViewBitmap = uiRenderer.getBitmapOfView();
    LynxEventReporter.runOnReportThread(() -> {
      final Map<String, String> props = new HashMap<>();
      if (lynxViewBitmap != null) {
        String base64String = BitmapUtils.bitmapToFormattedBase64(lynxViewBitmap, 30);
        props.put("data", base64String);
      }
      UIThreadUtils.runOnUiThread(() -> {
        TraceEvent.instant(TraceEventDef.CATEGORY_DEFAULT,
            TraceEventDef.FSP_SNAPSHOT_BASE_64_BITMAP, traceCurrentTimestampUs, props);
      });
    });
  }

  public int getContainerWidth() {
    return mContainerWidth;
  }

  public int getContainerHeight() {
    return mContainerHeight;
  }

  public long getTraceCurrentTimestampUs() {
    return mTraceCurrentTimestampUs;
  }

  public List<MeaningfulPaintingArea> getMeaningfulPaintingAreas() {
    return mMeaningfulPaintingAreas;
  }
}
