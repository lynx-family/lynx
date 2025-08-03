// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.ILynxEngine;
import com.lynx.tasm.ILynxLogicExecutor;
import com.lynx.tasm.IUIRendererCreator;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.resourceprovider.template.TemplateProviderResult;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Future;
import java.util.concurrent.FutureTask;

/**
 * LynxViewGroup is used to build LynxView that shares the same Runtime Environment.
 */
class LynxViewGroup implements ILynxViewGroup, ILynxViewRuntimeCacheManager {
  static final String TAG = "LynxViewGroup";

  // App Bundle url shared by multiple lynxViews config by the same LynxViewGroup;
  private final String url;

  // TemplateBundle object shared by multiple lynxViews config by the same LynxViewGroup;
  private TemplateBundle templateBundle;

  // initial globalProps shared by multiple lynxViews config by the same LynxViewGroup;
  private TemplateData globalProps;

  private BehaviorRegistry behaviorRegistry;
  private LynxBackgroundRuntimeOptions lynxRuntimeOptions;
  private HashMap contextData;
  private ThreadStrategyForRendering threadStrategy = ThreadStrategyForRendering.ALL_ON_UI;
  private boolean enableAutoExpose;
  private boolean enableLayoutSafepoint;
  private boolean enableUnifiedPipeline;
  private boolean forceDarkAllowed;
  private Float density;
  private int screenWidth;
  private int screenHeight;

  private boolean enableMultiAsyncThread;
  private boolean enableSyncFlush;
  private boolean enableAutoConcurrency;
  private boolean enableVSyncAlignedMessageLoop;
  private boolean enablePendingJsTask;
  private boolean enableAsyncHydration;
  private boolean enableJSRuntime;
  private boolean enableAirStrictMode;
  private boolean debuggable;
  private int presetWidthMeasureSpec;
  private int presetHeightMeasureSpec;
  private float fontScale;
  private boolean enablePreUpdateData;
  private IUIRendererCreator uiRendererCreator;
  private int embeddedMode = EmbeddedMode.UNSET;
  private boolean hasPresetMeasureSpec = false;
  private ILynxLogicExecutor logicExecutor;

  /** Runtime Cache Manager **/
  private Future<Void> templateResultFutureTask;

  @Override
  public boolean hasPresetMeasureSpec() {
    return hasPresetMeasureSpec;
  }

  LynxViewGroup(String url, TemplateBundle bundle, TemplateData globalProps,
      BehaviorRegistry registry, LynxBackgroundRuntimeOptions runtimeOptions, HashMap contextData,
      ThreadStrategyForRendering threadStrategy, boolean enableAutoExpose,
      boolean enableLayoutSafepoint, boolean enableUnifiedPipeline, boolean forceDarkAllowed,
      Float density, int screenWidth, int screenHeight, boolean enableMultiAsyncThread,
      boolean enableSyncFlush, boolean enablePendingJsTask, boolean enableAsyncHydration,
      boolean enableAutoConcurrency, boolean enableVSyncAlignedMessageLoop, boolean enableJSRuntime,
      boolean enableAirStrictMode, boolean debuggable, int presetWidthMeasureSpec,
      int presetHeightMeasureSpec, float fontScale, boolean enablePreUpdateData,
      IUIRendererCreator uiRendererCreator, int embeddedMode, boolean hasPresetMeasureSpec,
      ILynxLogicExecutor logicExecutor) {
    this.url = url;
    this.templateBundle = bundle;
    this.globalProps = globalProps;
    this.behaviorRegistry = registry;
    this.lynxRuntimeOptions = runtimeOptions;
    this.contextData = contextData;
    this.threadStrategy = threadStrategy;
    this.enableAutoExpose = enableAutoExpose;
    this.enableLayoutSafepoint = enableLayoutSafepoint;
    this.enableUnifiedPipeline = enableUnifiedPipeline;
    this.forceDarkAllowed = forceDarkAllowed;
    this.density = density;
    this.screenWidth = screenWidth;
    this.screenHeight = screenHeight;
    this.enableMultiAsyncThread = enableMultiAsyncThread;
    this.enableSyncFlush = enableSyncFlush;
    this.enablePendingJsTask = enablePendingJsTask;
    this.enableAsyncHydration = enableAsyncHydration;
    this.enableAutoConcurrency = enableAutoConcurrency;
    this.enableVSyncAlignedMessageLoop = enableVSyncAlignedMessageLoop;
    this.enableJSRuntime = enableJSRuntime;
    this.enableAirStrictMode = enableAirStrictMode;
    this.debuggable = debuggable;
    this.presetWidthMeasureSpec = presetWidthMeasureSpec;
    this.presetHeightMeasureSpec = presetHeightMeasureSpec;
    this.fontScale = fontScale;
    this.enablePreUpdateData = enablePreUpdateData;
    this.uiRendererCreator = uiRendererCreator;
    this.embeddedMode = embeddedMode;
    this.hasPresetMeasureSpec = hasPresetMeasureSpec;
    this.logicExecutor = logicExecutor;

    init();
  }

  private void init() {
    if (templateBundle == null) {
      this.templateResultFutureTask = this.fetchTemplate();
    }
  }

  @Override
  public BehaviorRegistry getBehaviorRegistry() {
    return this.behaviorRegistry;
  }

  @Override
  public boolean isEnableAutoExpose() {
    return this.enableAutoExpose;
  }

  @Override
  public Float getDensity() {
    return this.density;
  }

  @Override
  public ThreadStrategyForRendering getThreadStrategy() {
    return this.threadStrategy;
  }

  @Override
  public boolean isEnableLayoutSafepoint() {
    return this.enableLayoutSafepoint;
  }

  @Override
  public boolean isEnableUnifiedPipeline() {
    return this.enableUnifiedPipeline;
  }

  @Override
  public HashMap getContextData() {
    return this.contextData;
  }

  @Override
  public LynxBackgroundRuntimeOptions getLynxRuntimeOptions() {
    return this.lynxRuntimeOptions;
  }

  @Override
  public int getScreenWidth() {
    return this.screenWidth;
  }

  @Override
  public int getScreenHeight() {
    return this.screenHeight;
  }

  @Override
  public boolean getForceDarkAllowed() {
    return this.forceDarkAllowed;
  }

  @Override
  public boolean isEnableMultiAsyncThread() {
    return this.enableMultiAsyncThread;
  }

  @Override
  public boolean isEnableSyncFlush() {
    return this.enableSyncFlush;
  }

  @Override
  public boolean isEnableAutoConcurrency() {
    return this.enableAutoConcurrency;
  }

  @Override
  public boolean isEnableVSyncAlignedMessageLoop() {
    return this.enableVSyncAlignedMessageLoop;
  }

  @Override
  public boolean isEnablePendingJsTask() {
    return this.enablePendingJsTask;
  }

  @Override
  public boolean isEnableAsyncHydration() {
    return this.enableAsyncHydration;
  }

  @Override
  public boolean isEnableJSRuntime() {
    if (enableAirStrictMode) {
      return false;
    } else {
      return enableJSRuntime;
    }
  }

  @Override
  public boolean isEnableAirStrictMode() {
    return this.enableAirStrictMode;
  }

  @Override
  public boolean isDebuggable() {
    return this.debuggable;
  }

  @Override
  public int getPresetWidthMeasureSpec() {
    return this.presetWidthMeasureSpec;
  }

  @Override
  public int getPresetHeightMeasureSpec() {
    return this.presetHeightMeasureSpec;
  }

  @Override
  public float getFontScale() {
    return this.fontScale;
  }

  @Override
  public boolean isEnablePreUpdateData() {
    return this.enablePreUpdateData;
  }

  @Override
  public IUIRendererCreator getUIRendererCreator() {
    return this.uiRendererCreator;
  }

  @Override
  public int getEmbeddedMode() {
    return this.embeddedMode;
  }

  @Override
  public LynxBooleanOption isEnableGenericResourceFetcher() {
    return lynxRuntimeOptions.isEnableGenericResourceFetcher();
  }

  @Override
  public LynxGenericResourceFetcher getLynxGenericResourceFetcher() {
    return lynxRuntimeOptions.getGenericResourceFetcher();
  }

  @Override
  public LynxMediaResourceFetcher getLynxMediaResourceFetcher() {
    return lynxRuntimeOptions.getMediaResourceFetcher();
  }

  @Override
  public LynxTemplateResourceFetcher getLynxTemplateResourceFetcher() {
    return lynxRuntimeOptions.getTemplateResourceFetcher();
  }

  @Override
  public String getUrl() {
    return this.url;
  }

  @Override
  public TemplateData getGlobalProps() {
    return this.globalProps;
  }

  /** methods for RuntimeCacheManager **/
  @Override
  public void setTemplateBundle(TemplateBundle templateBundle) {
    this.templateBundle = templateBundle;
  }

  public TemplateBundle getTemplateBundle() {
    if (templateBundle == null) {
      try {
        templateResultFutureTask.get();
      } catch (Exception e) {
        LLog.i(TAG, "getTemplateBundle failed.");
      }
    }
    return this.templateBundle;
  }

  @Override
  public void setLynxEngine(ILynxEngine lynxEngine) {
    // TODO(@huangweiwu): to impl this;
  }

  @Override
  public ILynxEngine getLynxEngine() {
    // TODO(@huangweiwu): to impl this;
    return null;
  }

  public ILynxLogicExecutor getLogicExecutor() {
    return logicExecutor;
  }

  /**
   * Fetching template result as soon as possible.
   * @return A future object to get the result.
   */
  private Future<Void> fetchTemplate() {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        LynxResourceRequest request = new LynxResourceRequest(
            url, LynxResourceRequest.LynxResourceType.LynxResourceTypeTemplate);
        lynxRuntimeOptions.getTemplateResourceFetcher().fetchTemplate(
            request, new LynxResourceCallback<TemplateProviderResult>() {
              @Override
              public void onResponse(LynxResourceResponse<TemplateProviderResult> response) {
                TemplateProviderResult result = response.getData();
                if (result != null) {
                  if (result.getTemplateBundle() != null) {
                    templateBundle = result.getTemplateBundle();
                  } else if (result.getTemplateBinary() != null) {
                    templateBundle = TemplateBundle.fromTemplate(result.getTemplateBinary());
                  }
                }
              }
            });
      }
    };
    FutureTask<Void> resultFuture = new FutureTask<>(runnable, null);
    if (lynxRuntimeOptions != null) {
      LynxThreadPool.getAsyncServiceExecutor().execute(new Runnable() {
        @Override
        public void run() {
          resultFuture.run();
        }
      });
    }
    return resultFuture;
  }

  public void release() {
    if (templateBundle != null) {
      templateBundle.release();
    }
  }
}
