// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import android.content.Context;
import androidx.annotation.Nullable;
import com.lynx.jsbridge.IModuleCreator;
import com.lynx.jsbridge.LynxEmbeddedModule;
import com.lynx.jsbridge.LynxModule;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.jsbridge.SharedContextFinder;
import com.lynx.jsbridge.SharedModuleCreator;
import com.lynx.tasm.DefaultLogicExecutor;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.ILynxEngine;
import com.lynx.tasm.ILynxLogicExecutor;
import com.lynx.tasm.IUIRendererCreator;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.LynxEngine;
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
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
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
  private volatile TemplateBundle templateBundle;

  // initial globalProps shared by multiple lynxViews config by the same LynxViewGroup;
  private TemplateData globalProps;

  private boolean enableCacheEngine;

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
  private boolean enableSharedModule = false;

  private Context mContext;
  private CountDownLatch countDownLatch = new CountDownLatch(1);

  private final List<LynxResourceCallback<TemplateBundle>> mFetchCallbacks =
      Collections.synchronizedList(new LinkedList<>());
  private volatile LynxResourceResponse<TemplateBundle> mFetchResult = null;

  private Map<String, BitmapSize> bitmapSizePool = new ConcurrentHashMap<>();

  /** Runtime Cache Manager **/
  private Future<Void> templateResultFutureTask;
  private LynxEngine cachedLynxEngine;

  /**
   * shared LynxModuleFactory among multiple lynxViews config by the same LynxViewGroup;
   */
  private LynxModuleFactory mSharedModuleFactory;

  @Override
  public boolean hasPresetMeasureSpec() {
    return hasPresetMeasureSpec;
  }

  LynxViewGroup(Context context, String url, TemplateBundle bundle, TemplateData globalProps,
      boolean enableCacheEngine, BehaviorRegistry registry,
      LynxBackgroundRuntimeOptions runtimeOptions, HashMap contextData,
      ThreadStrategyForRendering threadStrategy, boolean enableAutoExpose,
      boolean enableLayoutSafepoint, boolean enableUnifiedPipeline, boolean forceDarkAllowed,
      Float density, int screenWidth, int screenHeight, boolean enableMultiAsyncThread,
      boolean enableSyncFlush, boolean enablePendingJsTask, boolean enableAsyncHydration,
      boolean enableVSyncAlignedMessageLoop, boolean enableJSRuntime, boolean enableAirStrictMode,
      boolean debuggable, int presetWidthMeasureSpec, int presetHeightMeasureSpec, float fontScale,
      boolean enablePreUpdateData, IUIRendererCreator uiRendererCreator, int embeddedMode,
      boolean hasPresetMeasureSpec, ILynxLogicExecutor logicExecutor, boolean enableMTSModule,
      String tapSlop, boolean enableSharedModule) {
    this.mContext = context;
    this.url = url;
    this.templateBundle = bundle;
    this.globalProps = globalProps;
    this.enableCacheEngine = enableCacheEngine;
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
    this.enableSharedModule = enableSharedModule;

    init();
  }

  private void init() {
    if (this.lynxRuntimeOptions != null) {
      this.lynxRuntimeOptions.setGlobalProps(this.globalProps);
    }

    if (templateBundle == null) {
      this.fetchTemplateInternal();
    }

    if (enableSharedModule && mSharedModuleFactory == null) {
      mSharedModuleFactory = new LynxModuleFactory();
      // bind SharedContextFinder & SharedModuleCreator
      IModuleCreator sharedModuleCreator = new SharedModuleCreator(new SharedContextFinder());
      mSharedModuleFactory.bind(sharedModuleCreator);
      // register ModuleAuthValidator if it is set in LynxRuntimeOptions
      if (this.lynxRuntimeOptions != null
          && this.lynxRuntimeOptions.getModuleAuthValidator() != null) {
        mSharedModuleFactory.registerModuleAuthValidator(
            this.lynxRuntimeOptions.getModuleAuthValidator());
      }
    }
    if (logicExecutor instanceof DefaultLogicExecutor) {
      ((DefaultLogicExecutor) logicExecutor).init(this);
    } else if (this.logicExecutor == null) {
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
    if (this.threadStrategy != null) {
      return this.threadStrategy;
    }
    return ThreadStrategyForRendering.ALL_ON_UI;
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

  @Nullable
  @Override
  public TemplateBundle getTemplateBundleNonBlocking() {
    return templateBundle;
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
  public boolean isEngineCacheEnabled() {
    return enableCacheEngine;
  }

  @Override
  public void setLynxEngine(LynxEngine lynxEngine) {
    if (enableCacheEngine) {
      this.cachedLynxEngine = lynxEngine;
    }
  }

  @Override
  public LynxEngine getLynxEngine() {
    if (enableCacheEngine) {
      LynxEngine result = this.cachedLynxEngine;
      this.cachedLynxEngine = null;
      return result;
    }
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
    if (enableSharedModule) {
      mSharedModuleFactory.registerModule(name, module, param);
    } else {
      lynxRuntimeOptions.registerModule(name, module, param);
    }
  }

  private void setFetchResult(LynxResourceResponse<TemplateBundle> result) {
    synchronized (mFetchCallbacks) {
      if (mFetchResult != null) {
        LLog.e(TAG, "internal error: fetch result should be set once");
        return;
      }
      mFetchResult = result;
      if (result.getState() == LynxResourceResponse.ResponseState.SUCCESS) {
        templateBundle = result.getData();
      }
      if (countDownLatch.getCount() > 0) {
        countDownLatch.countDown();
      }

      for (LynxResourceCallback<TemplateBundle> callback : mFetchCallbacks) {
        callback.onResponse(result);
      }
      mFetchCallbacks.clear();

      if (logicExecutor instanceof DefaultLogicExecutor) {
        ((DefaultLogicExecutor) logicExecutor).init(this);
      }
    }
  }

  /**
   * Fetching template result as soon as possible.
   */
  private void fetchTemplateInternal() {
    Runnable runnable = () -> {
      LynxTemplateResourceFetcher fetcher = lynxRuntimeOptions.getTemplateResourceFetcher();
      if (fetcher == null) {
        LLog.e(
            TAG, "failed to fetch template bundle because no TemplateResourceFetcher was provided");
        setFetchResult(LynxResourceResponse.onFailed(
            new RuntimeException("no TemplateResourceFetcher was provided")));
        return;
      }
      LynxResourceRequest request = new LynxResourceRequest(
          url, LynxResourceRequest.LynxResourceType.LynxResourceTypeTemplate);
      if (lynxRuntimeOptions.getTemplateResourceFetcher() != null) {
        lynxRuntimeOptions.getTemplateResourceFetcher().fetchTemplate(request, response -> {
          if (response.getState() == LynxResourceResponse.ResponseState.FAILED) {
            // request failed
            setFetchResult(LynxResourceResponse.onFailed(response.getError()));
            return;
          }

          TemplateBundle bundle = null;
          TemplateProviderResult result = response.getData();
          if (result != null) {
            if (result.getTemplateBundle() != null) {
              bundle = result.getTemplateBundle();
            } else if (result.getTemplateBinary() != null) {
              bundle = TemplateBundle.fromTemplate(result.getTemplateBinary());
            }
          }
          if (bundle == null) {
            // unknown error
            setFetchResult(
                LynxResourceResponse.onFailed(new RuntimeException("Template bundle is null")));
            return;
          }
          // success
          setFetchResult(LynxResourceResponse.onSuccess(bundle));
        });
      }
    };
    if (lynxRuntimeOptions != null) {
      LynxThreadPool.getAsyncServiceExecutor().execute(runnable);
    }
  }

  @Override
  public void fetchTemplateBundle(LynxResourceCallback<TemplateBundle> callback) {
    if (mFetchResult != null) {
      callback.onResponse(mFetchResult);
      return;
    }
    // not ready yet
    synchronized (mFetchCallbacks) {
      if (mFetchResult != null) {
        // double check
        callback.onResponse(mFetchResult);
        return;
      }
      mFetchCallbacks.add(callback);
    }
  }

  @Override
  public LynxModuleFactory getSharedModuleFactory() {
    if (!enableSharedModule) {
      return null;
    }
    return mSharedModuleFactory;
  }

  public void release() {
    if (templateBundle != null) {
      templateBundle.release();
    }
    if (logicExecutor != null) {
      logicExecutor.destroy();
    }
    setFetchResult(
        LynxResourceResponse.onFailed(new RuntimeException("This LynxViewGroup released")));
  }
}
