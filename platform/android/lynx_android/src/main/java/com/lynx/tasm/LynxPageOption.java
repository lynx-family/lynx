// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.tasm.behavior.LynxUIRendererCreator;
import com.lynx.tasm.behavior.TouchEventDispatcher;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.util.HashMap;
import java.util.Objects;

/** Creation options for one surface. Build a new option to change another page's configuration. */
public final class LynxPageOption {
  private final ThreadStrategyForRendering threadStrategy;
  private final boolean enableMultiAsyncThread;
  private final boolean enableJSRuntime;
  private final boolean enablePendingJsTask;
  private final boolean enableAutoExpose;
  private final boolean enableLayoutSafepoint;
  private final boolean enableUnifiedPipeline;
  private final boolean forceDarkAllowed;
  private final boolean enableVSyncAlignedMessageLoop;
  private final boolean enableAsyncHydration;
  private final boolean debuggable;
  private final boolean enablePreUpdateData;
  private final boolean enableMTSModule;
  private final Float density;
  private final int screenWidth;
  private final int screenHeight;
  private final float fontScale;
  private final LynxColorScheme colorScheme;
  private final IUIRendererCreator uiRendererCreator;
  private final String tapSlop;
  private final HashMap<String, Object> contextData;

  private LynxPageOption(Builder builder) {
    threadStrategy = Objects.requireNonNull(builder.threadStrategy);
    enableMultiAsyncThread = builder.enableMultiAsyncThread;
    enableJSRuntime = builder.enableJSRuntime;
    enablePendingJsTask = builder.enablePendingJsTask;
    enableAutoExpose = builder.enableAutoExpose;
    enableLayoutSafepoint = builder.enableLayoutSafepoint;
    enableUnifiedPipeline = builder.enableUnifiedPipeline;
    forceDarkAllowed = builder.forceDarkAllowed;
    enableVSyncAlignedMessageLoop = builder.enableVSyncAlignedMessageLoop;
    enableAsyncHydration = builder.enableAsyncHydration;
    debuggable = builder.debuggable;
    enablePreUpdateData = builder.enablePreUpdateData;
    enableMTSModule = builder.enableMTSModule;
    density = builder.density;
    screenWidth = builder.screenWidth;
    screenHeight = builder.screenHeight;
    fontScale = builder.fontScale;
    colorScheme = Objects.requireNonNull(builder.colorScheme);
    uiRendererCreator = Objects.requireNonNull(builder.uiRendererCreator);
    tapSlop = builder.tapSlop;
    contextData = builder.contextData == null ? null : new HashMap<>(builder.contextData);
  }

  public ThreadStrategyForRendering getThreadStrategy() {
    return threadStrategy;
  }

  public boolean isEnableMultiAsyncThread() {
    return enableMultiAsyncThread;
  }

  public boolean isEnableJSRuntime() {
    return enableJSRuntime;
  }

  public boolean isEnablePendingJsTask() {
    return enablePendingJsTask;
  }

  public boolean isEnableAutoExpose() {
    return enableAutoExpose;
  }

  public boolean isEnableLayoutSafepoint() {
    return enableLayoutSafepoint;
  }

  public boolean isEnableUnifiedPipeline() {
    return enableUnifiedPipeline;
  }

  public boolean getForceDarkAllowed() {
    return forceDarkAllowed;
  }

  public boolean isEnableVSyncAlignedMessageLoop() {
    return enableVSyncAlignedMessageLoop;
  }

  public boolean isEnableAsyncHydration() {
    return enableAsyncHydration;
  }

  public boolean isDebuggable() {
    return debuggable;
  }

  public boolean isEnablePreUpdateData() {
    return enablePreUpdateData;
  }

  public boolean isEnableMTSModule() {
    return enableMTSModule;
  }

  public Float getDensity() {
    return density;
  }

  public int getScreenWidth() {
    return screenWidth;
  }

  public int getScreenHeight() {
    return screenHeight;
  }

  public float getFontScale() {
    return fontScale;
  }

  public LynxColorScheme getColorScheme() {
    return colorScheme;
  }

  public IUIRendererCreator getUIRendererCreator() {
    return uiRendererCreator;
  }

  public String getTapSlop() {
    return tapSlop;
  }

  HashMap<String, Object> createContextData() {
    return contextData == null ? null : new HashMap<>(contextData);
  }

  /** Configures only page-owned runtime and rendering behavior. */
  public static final class Builder {
    private ThreadStrategyForRendering threadStrategy = ThreadStrategyForRendering.ALL_ON_UI;
    private boolean enableMultiAsyncThread = false;
    private boolean enableJSRuntime = true;
    private boolean enablePendingJsTask = false;
    private boolean enableAutoExpose = false;
    private boolean enableLayoutSafepoint = false;
    private boolean enableUnifiedPipeline = false;
    private boolean forceDarkAllowed = false;
    private boolean enableVSyncAlignedMessageLoop = false;
    private boolean enableAsyncHydration = false;
    private boolean debuggable = false;
    private boolean enablePreUpdateData = false;
    private boolean enableMTSModule = false;
    private Float density = null;
    private int screenWidth = DisplayMetricsHolder.UNDEFINE_SCREEN_SIZE_VALUE;
    private int screenHeight = DisplayMetricsHolder.UNDEFINE_SCREEN_SIZE_VALUE;
    private float fontScale = 1.0f;
    private LynxColorScheme colorScheme = LynxColorScheme.LIGHT;
    private IUIRendererCreator uiRendererCreator = new LynxUIRendererCreator();
    private String tapSlop = TouchEventDispatcher.mTapSlopDefault;
    private HashMap<String, Object> contextData = null;

    public Builder setThreadStrategyForRendering(ThreadStrategyForRendering value) {
      threadStrategy = value;
      return this;
    }

    public Builder setEnableMultiAsyncThread(boolean value) {
      enableMultiAsyncThread = value;
      return this;
    }

    public Builder setEnableJSRuntime(boolean value) {
      enableJSRuntime = value;
      return this;
    }

    public Builder setEnablePendingJsTask(boolean value) {
      enablePendingJsTask = value;
      return this;
    }

    public Builder enableAutoExpose(boolean value) {
      enableAutoExpose = value;
      return this;
    }

    public Builder setEnableLayoutSafepoint(boolean value) {
      enableLayoutSafepoint = value;
      return this;
    }

    public Builder setEnableUnifiedPipeline(boolean value) {
      enableUnifiedPipeline = value;
      return this;
    }

    public Builder setForceDarkAllowed(boolean value) {
      forceDarkAllowed = value;
      return this;
    }

    public Builder setEnableVSyncAlignedMessageLoop(boolean value) {
      enableVSyncAlignedMessageLoop = value;
      return this;
    }

    public Builder setEnableAsyncHydration(boolean value) {
      enableAsyncHydration = value;
      return this;
    }

    public Builder setDebuggable(boolean value) {
      debuggable = value;
      return this;
    }

    public Builder setEnablePreUpdateData(boolean value) {
      enablePreUpdateData = value;
      return this;
    }

    public Builder setEnableMTSModule(boolean value) {
      enableMTSModule = value;
      return this;
    }

    public Builder setDensity(Float value) {
      density = value;
      return this;
    }

    public Builder setFontScale(float value) {
      fontScale = value;
      return this;
    }

    public Builder setColorScheme(LynxColorScheme value) {
      colorScheme = value;
      return this;
    }

    public Builder setUIRendererCreator(IUIRendererCreator value) {
      uiRendererCreator = value;
      return this;
    }

    public Builder setTapSlop(String value) {
      tapSlop = value;
      return this;
    }

    public Builder setScreenSize(int width, int height) {
      screenWidth = width;
      screenHeight = height;
      return this;
    }

    public Builder registerContextData(String key, Object value) {
      if (contextData == null) {
        contextData = new HashMap<>();
      }
      contextData.put(key, value);
      return this;
    }

    public LynxPageOption build() {
      return new LynxPageOption(this);
    }
  }
}
