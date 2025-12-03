// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import java.util.HashMap;

// FSP configuration
public class FSPConfig {
  public final boolean enable = LynxEnv.inst().enableFSP();
  // Minimum content fill rate in X-axis direction, default is 30%.
  public int minContentFillPercentageX;
  // Minimum content fill rate in Y-axis direction, default is 30%.
  public int minContentFillPercentageY;
  // Minimum content fill rate in total area direction, default is 30%.
  public int minContentFillPercentageTotalArea;
  // Minimum meaningful content fill rate relative to container area, default is 1%.
  public int minContainerFillPercentageContainerArea;
  // Acceptable pixel change rate on projection axis per second, default is 10 pixels per second.
  public int acceptablePixelDiffPerSec;
  // Acceptable total pixel area change rate per second, default is 10*10 pixels per second.
  public int acceptableAreaDiffPerSec;
  // Minimum time interval to determine stability, default is 300ms.
  public int minDiffIntervalMs;
  // Hard timeout in milliseconds, default is 10 seconds.
  public int hardTimeoutMs;
  // Snapshot interval in milliseconds, default is 17ms.
  public int snapshotIntervalMs;

  private boolean mReady;
  private static final String MIN_CONTENT_FILL_PERCENTAGE_X_KEY = "min_content_fill_percentage_x";
  private static final String MIN_CONTENT_FILL_PERCENTAGE_Y_KEY = "min_content_fill_percentage_y";
  private static final String MIN_CONTENT_FILL_PERCENTAGE_TOTAL_AREA_KEY =
      "min_content_fill_percentage_total_area";
  private static final String MIN_CONTAINER_FILL_PERCENTAGE_CONTAINER_AREA_KEY =
      "min_container_fill_percentage_container_area";
  private static final String ACCEPTABLE_PIXEL_DIFF_PER_SEC_KEY = "acceptable_pixel_diff_per_sec";
  private static final String ACCEPTABLE_AREA_DIFF_PER_SEC_KEY = "acceptable_area_diff_per_sec";
  private static final String MIN_DIFF_INTERVAL_MS_KEY = "min_diff_interval_ms";
  private static final String HARD_TIMEOUT_MS_KEY = "hard_timeout_ms";
  private static final String SNAPSHOT_INTERVAL_MS_KEY = "snapshotIntervalMs";

  FSPConfig() {}

  public void parse() {
    if (mReady || !enable) {
      return;
    }
    TraceEvent.beginSection(TraceEvent.CATEGORY_DEFAULT, TraceEventDef.FSP_CONFIG_PARSE);
    HashMap<String, String> map = LynxEnv.inst().getFSPConfig();
    minContentFillPercentageX = parseInt(map, MIN_CONTENT_FILL_PERCENTAGE_X_KEY, 30);
    minContentFillPercentageY = parseInt(map, MIN_CONTENT_FILL_PERCENTAGE_Y_KEY, 30);
    minContentFillPercentageTotalArea =
        parseInt(map, MIN_CONTENT_FILL_PERCENTAGE_TOTAL_AREA_KEY, 30);
    minContainerFillPercentageContainerArea =
        parseInt(map, MIN_CONTAINER_FILL_PERCENTAGE_CONTAINER_AREA_KEY, 1);
    acceptablePixelDiffPerSec = parseInt(map, ACCEPTABLE_PIXEL_DIFF_PER_SEC_KEY, 10);
    acceptableAreaDiffPerSec = parseInt(map, ACCEPTABLE_AREA_DIFF_PER_SEC_KEY, 100);
    minDiffIntervalMs = parseInt(map, MIN_DIFF_INTERVAL_MS_KEY, 300);
    hardTimeoutMs = parseInt(map, HARD_TIMEOUT_MS_KEY, 10000);
    snapshotIntervalMs = parseInt(map, SNAPSHOT_INTERVAL_MS_KEY, 17);
    mReady = true;
    TraceEvent.endSection(TraceEvent.CATEGORY_DEFAULT, TraceEventDef.FSP_CONFIG_PARSE, map);
  }

  private int parseInt(HashMap<String, String> map, String key, int defaultValue) {
    if (map == null) {
      return defaultValue;
    }
    Object valueObj = map.get(key);
    if (valueObj == null) {
      return defaultValue;
    }
    int value = defaultValue;
    if (valueObj instanceof String) {
      String valueStr = (String) valueObj;
      try {
        value = Integer.parseInt(valueStr.trim());
      } catch (NumberFormatException e) {
      }
    } else if (valueObj instanceof Number) {
      Number number = (Number) valueObj;
      value = number.intValue();
    }
    return value;
  }
}
