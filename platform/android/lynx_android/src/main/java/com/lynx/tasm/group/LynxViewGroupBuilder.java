// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.tasm.ILynxLogicExecutor;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;

public class LynxViewGroupBuilder extends LynxBaseConfigurator<LynxViewGroupBuilder> {
  // App Bundle url shared by multiple lynxViews config by the same LynxViewGroup;
  private String url;

  // initial globalProps shared by multiple lynxViews config by the same LynxViewGroup;
  private TemplateBundle templateBundle;

  // initial globalProps shared by multiple lynxViews config by the same LynxViewGroup;
  private TemplateData globalProps;

  private ILynxLogicExecutor logicExecutor;

  private Context mContext;
  private boolean enableSharedModule = false;

  private boolean enableCacheEngine;

  public LynxViewGroupBuilder setContext(@NonNull Context context) {
    this.mContext = context;
    return this;
  }

  public LynxViewGroupBuilder setUrl(@NonNull String url) {
    this.url = url;
    return this;
  }

  public LynxViewGroupBuilder setTemplateBundle(@NonNull TemplateBundle templateBundle) {
    this.templateBundle = templateBundle;
    return this;
  }

  public LynxViewGroupBuilder setGlobalProps(@NonNull TemplateData globalProps) {
    this.globalProps = globalProps;
    return this;
  }

  public LynxViewGroupBuilder setLogicExecutor(ILynxLogicExecutor logicExecutor) {
    this.logicExecutor = logicExecutor;
    return this;
  }

  public LynxViewGroupBuilder setEnableSharedModule(boolean enableSharedModule) {
    this.enableSharedModule = enableSharedModule;
    return this;
  }

  /**
   * Enables Engine Cache in Current LynxViewGroup;
   */
  public LynxViewGroupBuilder setEnableCacheEngine(boolean enableCacheEngine) {
    this.enableCacheEngine = enableCacheEngine;
    return this;
  }

  public ILynxViewGroup build() {
    LynxViewGroup group = new LynxViewGroup(mContext, url, templateBundle, globalProps,
        enableCacheEngine, behaviorRegistry, lynxRuntimeOptions, mContextData, threadStrategy,
        enableAutoExpose, enableLayoutSafepoint, enableUnifiedPipeline, forceDarkAllowed,
        densityOverride, screenWidth, screenHeight, enableMultiAsyncThread, enableSyncFlush,
        enablePendingJsTask, enableAsyncHydration, enableVSyncAlignedMessageLoop, enableJSRuntime,
        enableAirStrictMode, debuggable, presetWidthMeasureSpec, presetHeightMeasureSpec, fontScale,
        enablePreUpdateData, uiRendererCreator, embeddedMode, hasPresetMeasureSpec, logicExecutor,
        enableMTSModule, tapSlop, enableSharedModule);
    return group;
  }
}
