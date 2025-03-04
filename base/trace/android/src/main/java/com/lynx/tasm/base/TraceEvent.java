// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import android.os.Trace;
import com.lynx.trace.BuildConfig;
import java.util.Map;
import java.util.Random;

public class TraceEvent {
  public static final long CATEGORY_DEFAULT = 0;
  public static final long CATEGORY_VITALS = 1L;
  public static final long CATEGORY_SCREENSHOTS = 2L;
  public static final long CATEGORY_FPS = 3L;
  public static final String DEFAULT_INSTANT_COLOR = "#FF0000";

  public static final String[] defualt_categories = {"lynx", "vitals", "screenshot", "fps"};

  private static boolean sTraceEnvInited = false;
  private static boolean sDebugModeEnabled = false;

  private static boolean sPerfettoTraceEnabled = false;
  private static boolean sSystemTraceEnabled = false;

  private static String getRandomColor() {
    try {
      StringBuilder result = new StringBuilder();
      result.append('#');
      for (int i = 0; i < 6; i++) {
        result.append(Integer.toHexString(new Random().nextInt(16)));
      }
      return result.toString().toUpperCase();
    } catch (Exception e) {
      return DEFAULT_INSTANT_COLOR;
    }
  }

  public static void markTraceEnvInited(boolean isInit) {
    sTraceEnvInited = isInit;
  }

  public static void markTraceDebugMode(boolean isDebugModeEnabled) {
    sDebugModeEnabled = isDebugModeEnabled;
  }

  public static void beginSection(String sectionName) {
    beginSection(CATEGORY_DEFAULT, sectionName);
  }

  public static void endSection(String sectionName) {
    endSection(CATEGORY_DEFAULT, sectionName);
  }

  @Deprecated
  public static void beginSection(long category, String sectionName) {
    beginSection(defualt_categories[(int) category], sectionName);
  }

  public static void beginSection(String category, String sectionName) {
    if (TraceController.isNativeTracingOnly()) {
      return;
    }
    if (enableTrace()) {
      if (enablePerfettoTrace()) {
        nativeBeginSection(category, sectionName);
      } else {
        Trace.beginSection(sectionName);
      }
    }
  }

  @Deprecated
  public static void endSection(long category, String sectionName) {
    endSection(defualt_categories[(int) category], sectionName);
  }

  public static void endSection(String category, String sectionName) {
    if (TraceController.isNativeTracingOnly()) {
      return;
    }
    if (enableTrace()) {
      if (enablePerfettoTrace()) {
        nativeEndSection(category, sectionName);
      } else {
        Trace.endSection();
      }
    }
  }

  public static void beginSection(String sectionName, Map<String, String> props) {
    beginSection(CATEGORY_DEFAULT, sectionName, props);
  }

  @Deprecated
  public static void beginSection(long category, String sectionName, Map<String, String> props) {
    beginSection(defualt_categories[(int) category], sectionName, props);
  }

  public static void beginSection(String category, String sectionName, Map<String, String> props) {
    if (TraceController.isNativeTracingOnly()) {
      return;
    }
    if (enableTrace()) {
      if (enablePerfettoTrace()) {
        nativeBeginSectionWithProps(category, sectionName, props);
      } else {
        Trace.beginSection(sectionName);
      }
    }
  }

  @Deprecated
  public static void endSection(long category, String sectionName, Map<String, String> props) {
    endSection(defualt_categories[(int) category], sectionName, props);
  }

  public static void endSection(String category, String sectionName, Map<String, String> props) {
    if (TraceController.isNativeTracingOnly()) {
      return;
    }
    if (enableTrace()) {
      if (enablePerfettoTrace()) {
        nativeEndSectionWithProps(category, sectionName, props);
      } else {
        Trace.endSection();
      }
    }
  }

  @Deprecated
  public static void instant(long category, String eventName) {
    instant(defualt_categories[(int) category], eventName, System.nanoTime() / 1000);
  }

  public static void instant(String category, String eventName) {
    instant(category, eventName, System.nanoTime() / 1000);
  }

  @Deprecated
  public static void instant(long category, String eventName, long timestamp) {
    instant(defualt_categories[(int) category], eventName, timestamp, getRandomColor());
  }

  public static void instant(String category, String eventName, long timestamp) {
    instant(category, eventName, timestamp, getRandomColor());
  }

  @Deprecated
  public static void instant(long category, String eventName, String color) {
    instant(defualt_categories[(int) category], eventName, System.nanoTime() / 1000, color);
  }

  public static void instant(String category, String eventName, String color) {
    instant(category, eventName, System.nanoTime() / 1000, color);
  }

  @Deprecated
  public static void instant(long category, String eventName, Map<String, String> props) {
    instant(defualt_categories[(int) category], eventName, props);
  }

  public static void instant(String category, String eventName, Map<String, String> props) {
    if (enableTrace()) {
      if (enablePerfettoTrace()) {
        nativeInstantWithProps(category, eventName, System.nanoTime() / 1000, props);
      } else {
        Trace.beginSection(eventName);
        Trace.endSection();
      }
    }
  }

  @Deprecated
  public static void counter(long category, String name, long counterValue) {
    counter(defualt_categories[(int) category], name, counterValue);
  }

  public static void counter(String category, String name, long counterValue) {
    if (TraceController.isNativeTracingOnly()) {
      return;
    }
    if (enableTrace()) {
      nativeCounter(category, name, counterValue);
    }
  }

  @Deprecated
  public static boolean registerTraceBackend(long ptr) {
    return false;
  }

  @Deprecated
  public static boolean categoryEnabled(long category) {
    return categoryEnabled(defualt_categories[(int) category]);
  }

  public static boolean categoryEnabled(String category) {
    return enableTrace() && nativeCategoryEnabled(category);
  }

  public static boolean enableTrace() {
    return BuildConfig.enable_trace == "perfetto" || BuildConfig.enable_trace == "systrace"
        || sDebugModeEnabled;
  }

  public static boolean enableSystemTrace() {
    if (!sSystemTraceEnabled && sTraceEnvInited) {
      sSystemTraceEnabled = nativeSystemTraceEnabled();
    }
    return sSystemTraceEnabled;
  }

  public static boolean enablePerfettoTrace() {
    if (!sPerfettoTraceEnabled && sTraceEnvInited) {
      sPerfettoTraceEnabled = nativePerfettoTraceEnabled();
    }
    return sPerfettoTraceEnabled;
  }

  private static void instant(String category, String eventName, long timestamp, String color) {
    if (enableTrace()) {
      if (enablePerfettoTrace()) {
        nativeInstant(category, eventName, timestamp, color);
      } else {
        Trace.beginSection(eventName);
        Trace.endSection();
      }
    }
  }

  private static void instant(
      String category, String eventName, long timestamp, Map<String, String> props) {
    if (enableTrace()) {
      if (enablePerfettoTrace()) {
        timestamp = timestamp > 0 ? timestamp : System.nanoTime() / 1000;
        nativeInstantWithProps(category, eventName, timestamp, props);
      } else {
        Trace.beginSection(eventName);
        Trace.endSection();
      }
    }
  }

  private static native void nativeBeginSection(String category, String sectionName);
  private static native void nativeBeginSectionWithProps(
      String category, String sectionName, Map<String, String> props);
  private static native void nativeEndSection(String category, String sectionName);
  private static native void nativeEndSectionWithProps(
      String category, String sectionName, Map<String, String> props);
  private static native void nativeInstant(
      String category, String sectionName, long timestamp, String color);
  private static native void nativeInstantWithProps(
      String category, String sectionName, long timestamp, Map<String, String> props);
  private static native void nativeCounter(String category, String name, long counterValue);
  private static native boolean nativeCategoryEnabled(String category);

  private static native boolean nativeSystemTraceEnabled();
  private static native boolean nativePerfettoTraceEnabled();
}
