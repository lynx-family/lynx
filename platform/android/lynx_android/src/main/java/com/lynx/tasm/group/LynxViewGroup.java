// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import android.content.Context;
import com.lynx.jsbridge.LynxEmbeddedModule;
import com.lynx.jsbridge.LynxModule;
import com.lynx.tasm.DefaultLogicExecutor;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.ILynxEngine;
import com.lynx.tasm.ILynxLogicExecutor;
import com.lynx.tasm.IUIRendererCreator;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.TouchEventDispatcher;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.resourceprovider.template.TemplateProviderResult;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * LynxViewGroup is used to build LynxView that shares the same Runtime Environment.
 */
class LynxViewGroup implements ILynxViewGroup, ILynxViewRuntimeCacheManager {
  static final String TAG = "LynxViewGroup";

  private final AtomicInteger mViewIdGenerator = new AtomicInteger(0);
  private final ConcurrentHashMap<Integer, WeakReference<LynxView>> mLynxViewMap =
      new ConcurrentHashMap<>();
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
  private boolean enableMTSModule;
  private ILynxLogicExecutor logicExecutor;
  private String tapSlop = TouchEventDispatcher.mTapSlopDefault;

  private Context mContext;
  private CountDownLatch countDownLatch = new CountDownLatch(1);

  private Map<String, BitmapSize> bitmapSizePool = new ConcurrentHashMap<>();

  /** Runtime Cache Manager **/
  private Future<Void> templateResultFutureTask;

  @Override
  public boolean hasPresetMeasureSpec() {
    return hasPresetMeasureSpec;
  }

  LynxViewGroup(Context context, String url, TemplateBundle bundle, TemplateData globalProps,
      BehaviorRegistry registry, LynxBackgroundRuntimeOptions runtimeOptions, HashMap contextData,
      ThreadStrategyForRendering threadStrategy, boolean enableAutoExpose,
      boolean enableLayoutSafepoint, boolean enableUnifiedPipeline, boolean forceDarkAllowed,
      Float density, int screenWidth, int screenHeight, boolean enableMultiAsyncThread,
      boolean enableSyncFlush, boolean enablePendingJsTask, boolean enableAsyncHydration,
      boolean enableVSyncAlignedMessageLoop, boolean enableJSRuntime, boolean enableAirStrictMode,
      boolean debuggable, int presetWidthMeasureSpec, int presetHeightMeasureSpec, float fontScale,
      boolean enablePreUpdateData, IUIRendererCreator uiRendererCreator, int embeddedMode,
      boolean hasPresetMeasureSpec, ILynxLogicExecutor logicExecutor, boolean enableMTSModule,
      String tapSlop) {
    this.mContext = context;
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
    this.enableMTSModule = enableMTSModule;
    this.tapSlop = tapSlop;

    init();
  }

  private void init() {
    if (this.lynxRuntimeOptions != null) {
      this.lynxRuntimeOptions.setGlobalProps(this.globalProps);
    }

    if (templateBundle == null) {
      this.fetchTemplate();
    } else {
      if (logicExecutor instanceof DefaultLogicExecutor) {
        // init DefaultLogicExecutor if needed.
        ((DefaultLogicExecutor) logicExecutor)
            .init(templateBundle, lynxRuntimeOptions, mContext, LynxViewGroup.this, debuggable);
      }
    }

    if (this.logicExecutor == null) {
      registerModule(LynxEmbeddedModule.NAME, LynxEmbeddedModule.class, this);
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
  public LynxGroup getLynxGroup() {
    return this.lynxRuntimeOptions.getLynxGroup();
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
    return false;
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
  public boolean isEnableMTSModule() {
    return this.enableMTSModule;
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

  /**
   * Get the associated TemplateBundle with LynxViewGroup.
   * If templateBundle is not ready yet. It will block current thread
   * and waiting for the result.
   *
   * @return The Associated TemplateBundle
   */
  @Override
  public TemplateBundle getTemplateBundle() {
    if (templateBundle == null) {
      try {
        if (countDownLatch.await(3, TimeUnit.SECONDS)) {
          return this.templateBundle;
        }
      } catch (Exception e) {
        LLog.i(TAG, "getTemplateBundle failed.");
      }
    }
    return this.templateBundle;
  }

  /**
   * Check if the bundle in LynxViewGroup is ready yet.
   *
   * @return TemplateBundle ready or not.
   */
  @Override
  public boolean isTemplateBundleReady() {
    return templateBundle != null;
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

  @Override
  public void setBitmapSizeCache(String source, int width, int height) {
    String traceEvent = null;
    if (TraceEvent.isTracingStarted()) {
      traceEvent = "setBitmapSizeCache: " + source + ": " + width + " - " + height;
      TraceEvent.beginSection(traceEvent);
    }
    if (source == null) {
      if (TraceEvent.isTracingStarted()) {
        TraceEvent.endSection(traceEvent);
      }
      return;
    }
    this.bitmapSizePool.put(source, new BitmapSize(source, width, height));
    if (TraceEvent.isTracingStarted()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  @Override
  public BitmapSize getBitmapSizeCache(String source) {
    String traceEvent = null;
    if (TraceEvent.isTracingStarted()) {
      traceEvent = "getBitmapSizeCache: " + source;
      TraceEvent.beginSection(traceEvent);
    }
    BitmapSize size = this.bitmapSizePool.get(source);
    if (TraceEvent.isTracingStarted()) {
      TraceEvent.endSection(traceEvent);
    }
    return size;
  }

  public ILynxLogicExecutor getLogicExecutor() {
    return logicExecutor;
  }

  public String getTapSlop() {
    return tapSlop;
  }

  @Override
  public int generateNextLynxViewID() {
    return mViewIdGenerator.getAndIncrement();
  }

  @Override
  public void addLynxView(int LynxViewId, LynxView view) {
    if (view != null) {
      mLynxViewMap.put(LynxViewId, new WeakReference<>(view));
    }
  }

  @Override
  public void removeLynxView(int LynxViewId) {
    mLynxViewMap.remove(LynxViewId);
  }

  @Override
  public LynxView getLynxViewById(int LynxViewId) {
    WeakReference<LynxView> ref = mLynxViewMap.get(LynxViewId);
    return ref != null ? ref.get() : null;
  }

  @Override
  public void registerModule(String name, Class<? extends LynxModule> module, Object param) {
    lynxRuntimeOptions.registerModule(name, module, param);
  }

  /**
   * Fetching template result as soon as possible.
   */
  private void fetchTemplate() {
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
                  if (templateBundle != null) {
                    if (logicExecutor instanceof DefaultLogicExecutor) {
                      ((DefaultLogicExecutor) logicExecutor)
                          .init(templateBundle, lynxRuntimeOptions, mContext, LynxViewGroup.this,
                              debuggable);
                    }
                  }

                  countDownLatch.countDown();
                }
              }
            });
      }
    };
    if (lynxRuntimeOptions != null) {
      LynxThreadPool.getAsyncServiceExecutor().execute(runnable);
    }
  }

  public void release() {
    if (templateBundle != null) {
      templateBundle.release();
    }
    if (logicExecutor != null) {
      logicExecutor.destroy();
    }
  }
}
