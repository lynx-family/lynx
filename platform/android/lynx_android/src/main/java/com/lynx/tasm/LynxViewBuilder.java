// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.jsbridge.LynxModule;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import com.lynx.tasm.behavior.LynxUIRenderer;
import com.lynx.tasm.component.DynamicComponentFetcher;
import com.lynx.tasm.image.model.LynxImageFetcher;
import com.lynx.tasm.loader.LynxFontFaceLoader;
import com.lynx.tasm.provider.AbsTemplateProvider;
import com.lynx.tasm.provider.LynxResourceFetcher;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.LynxViewConfigProcessor;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class LynxViewBuilder {
  AbsTemplateProvider templateProvider;
  BehaviorRegistry behaviorRegistry;
  Object lynxModuleExtraData;
  boolean enableLayoutSafepoint;
  Float densityOverride;
  DynamicComponentFetcher fetcher;
  LynxResourceFetcher resourceFetcher;
  LynxFontFaceLoader.Loader fontLoader;
  LynxImageFetcher imageFetcher;
  static Float defaultDensity = null;

  boolean enableAutoExpose = true; // switch for LynxView onShow onHide event, default true.
  boolean enableLayoutOnly = LynxEnv.inst().isLayoutOnlyEnabled();

  boolean enableMultiAsyncThread = true;
  boolean enableSyncFlush = false;
  boolean enablePendingJsTask = false;
  boolean enableAutoConcurrency = false;
  boolean enableVSyncAlignedMessageLoop = false;
  boolean forceDarkAllowed = false;

  /**
   * enable async hydration of ssr.
   */
  boolean enableAsyncHydration = false;

  /**
   * enableJSRuntime、enableAirStrictMode both determine whether js thread will be enabled.
   * Change enableJSRuntime & enableAirStrictMode private and add enableJSRuntime() to expose for
   * usage.
   */
  boolean enableJSRuntime = true;
  /**
   * Add switch for Air Mode.
   * Currently only contributes to decide whether js thread will be enabled,
   * and further optimized logics may be involved.
   */
  boolean enableAirStrictMode = false;
  boolean debuggable = false;
  ThreadStrategyForRendering threadStrategy = ThreadStrategyForRendering.ALL_ON_UI;
  int presetWidthMeasureSpec;
  int presetHeightMeasureSpec;
  int screenWidth = DisplayMetricsHolder.UNDEFINE_SCREEN_SIZE_VALUE;
  int screenHeight = DisplayMetricsHolder.UNDEFINE_SCREEN_SIZE_VALUE;
  float fontScale = 1.0f;
  private HashMap<String, Object> mContextData;

  private Map<String, String> mImageCustomParam;

  Map<String, String> lynxViewConfig;

  boolean enablePreUpdateData = false;

  LynxBooleanOption enableGenericResourceFetcher = LynxBooleanOption.UNSET;

  /**
   * enable LynxResourceService loader injection
   */
  boolean enableLynxResourceServiceLoaderInjection = false;
  LynxBackgroundRuntimeOptions lynxRuntimeOptions;
  LynxBackgroundRuntime lynxBackgroundRuntime;

  private ILynxUIRenderer lynxUIRenderer;

  public LynxViewBuilder() {
    LynxEnv.inst().lazyInitIfNeeded();
    lynxRuntimeOptions = new LynxBackgroundRuntimeOptions();
    behaviorRegistry = new BehaviorRegistry(LynxEnv.inst().getBehaviorMap());
    templateProvider = LynxEnv.inst().getTemplateProvider();
    densityOverride = null;
    if (defaultDensity != null) {
      densityOverride = defaultDensity;
    }
    lynxUIRenderer = new LynxUIRenderer();
  }

  @Deprecated
  public LynxViewBuilder(Context context) {
    this();
  }

  public LynxViewBuilder setTemplateProvider(@Nullable AbsTemplateProvider provider) {
    templateProvider = provider;
    return this;
  }

  public void setCustomBehaviorRegistry(@Nullable BehaviorRegistry registry) {
    behaviorRegistry = registry;
  }

  /**
   * Use {@link #addBehaviors(List)} instead.
   */
  @Deprecated
  public LynxViewBuilder setBehaviors(@Nullable List<Behavior> bundle) {
    if (bundle != null) {
      behaviorRegistry.addBehaviors(bundle);
    }
    return this;
  }

  public LynxViewBuilder addBehaviors(@NonNull List<Behavior> bundle) {
    behaviorRegistry.addBehaviors(bundle);
    return this;
  }

  public LynxViewBuilder addBehavior(@NonNull Behavior bundle) {
    behaviorRegistry.addBehavior(bundle);
    return this;
  }

  @Deprecated
  public LynxViewBuilder setUIRunningMode(boolean ui) {
    if (ui) {
      threadStrategy = ThreadStrategyForRendering.ALL_ON_UI;
    } else {
      threadStrategy = ThreadStrategyForRendering.PART_ON_LAYOUT;
    }
    return this;
  }

  public LynxViewBuilder setPresetMeasuredSpec(int widthMeasureSpec, int heightMeasureSpec) {
    presetHeightMeasureSpec = heightMeasureSpec;
    presetWidthMeasureSpec = widthMeasureSpec;
    return this;
  }

  /**
   * You can set a virtual screen size to lynxview by this way.
   * Generally, you don't need to set it.The screen size of lynx is the real device size by default.
   * It will be useful for the split-window, this case, it can make some css properties based on rpx
   * shows better.
   * When the given screen size is not set, Lynx will be initialized with screen metrics
   * from activity context or real screen metrics.
   * @param width(px) screen width
   * @param height(px) screen screen
   */
  public LynxViewBuilder setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    return this;
  }

  /**
   * LynxView will send onShow&onHide automatically. If it's not what you want,
   * you can shut it down, by set enableAutoExpose to False.
   * @param enableAutoExpose
   * @return
   */
  public LynxViewBuilder enableAutoExpose(boolean enableAutoExpose) {
    this.enableAutoExpose = enableAutoExpose;
    return this;
  }

  /**
   * Enable user bytecode. Controls bytecode for files other than lynx_core.js, such as
   * app-service.js.
   * @param enableUserBytecode
   * @return
   */
  public LynxViewBuilder setEnableUserBytecode(boolean enableUserBytecode) {
    lynxRuntimeOptions.setEnableUserBytecode(enableUserBytecode);
    return this;
  }

  /**
   * Set user bytecode source url.
   *
   * @param url Source url of the template.
   *            SourceUrl will be used to mark js files in bytecode.
   *            If the js file changes while url remains the same, code
   *            cache knows the js file is updated and will remove the
   *            cache generated by the former version of the js file.
   */
  public LynxViewBuilder setBytecodeSourceUrl(String url) {
    lynxRuntimeOptions.setBytecodeSourceUrl(url);
    return this;
  }

  /**
   * Deprecated, please use {@link #setEnableUserBytecode(boolean)} instead.
   */
  @Deprecated
  public LynxViewBuilder setEnableUserCodeCache(boolean enableUserBytecode) {
    setEnableUserBytecode(enableUserBytecode);
    return this;
  }

  /**
   * Deprecated, please use {@link #setBytecodeSourceUrl(boolean)} instead.
   */
  @Deprecated
  public LynxViewBuilder setCodeCacheSourceUrl(String url) {
    setBytecodeSourceUrl(url);
    return this;
  }

  /**
   * Experimental API.
   * ATTENTION: Currently density will be applied globally, and each process should have only have
   * one density. Every lynx view should be initialized with this function with the same parameter
   * or never call this function. This restriction may be removed in future.
   *
   * Will use real screen density by default.
   * @param density density used by lynx, its value means number of actual physical pixels
   *                corresponding to unit PX in Lynx.
   *                ATTENTION the density input should be logical density but not DPI.
   */
  public LynxViewBuilder setDensity(float density) {
    this.densityOverride = density;
    return this;
  }

  /**
   * Experimental API.
   *
   * Set a default overriding density which will be applied to all LynxView constructed after it is
   * set. Will use screen density if default density is not set.
   * @param density overriding density by default, set it to null to reset the default density.
   */
  static public void setDefaultDensity(Float density) {
    defaultDensity = density;
  }

  public LynxViewBuilder setThreadStrategyForRendering(ThreadStrategyForRendering strategy) {
    if (null != strategy) {
      threadStrategy = strategy;
    }
    return this;
  }

  public ThreadStrategyForRendering getThreadStrategy() {
    return threadStrategy;
  }

  public LynxViewBuilder setLynxGroup(@Nullable LynxGroup group) {
    lynxRuntimeOptions.setLynxGroup(group);
    return this;
  }

  /**
   * Pass extra data to LynxModule, the usage of data depends on module's implementation
   *
   * @param data Data passed to LynxModule
   */
  public LynxViewBuilder setLynxModuleExtraData(Object data) {
    lynxModuleExtraData = data;
    return this;
  }

  /**
   * Register module with module name and module class.
   *
   * @param name   Module's name, will be used in JS code.
   * @param module module class.
   */
  public void registerModule(String name, Class<? extends LynxModule> module) {
    registerModule(name, module, null);
  }

  /**
   * Register module with module name and module class.
   *
   * @param name   Module's name, will be used in JS code.
   * @param module module class, containing a no-parameter constructor.
   * @param param  the param will be provided to the module class's constructor
   *               when creating the module object.
   */
  public void registerModule(String name, Class<? extends LynxModule> module, Object param) {
    lynxRuntimeOptions.registerModule(name, module, param);
  }

  /**
   * Whether to allow synchronous retrieval of asynchronous layout results during the onMeasure
   * phase.
   *
   * This switch can effectively reduce content flickering issues in LynxView
   * when rendering templates or updating data in long list scenarios using the asynchronous layout
   * model.
   *
   * @see ThreadStrategyForRendering#PART_ON_LAYOUT
   */
  public LynxViewBuilder setEnableLayoutSafepoint(boolean enable) {
    enableLayoutSafepoint = enable;
    return this;
  }

  @Deprecated
  public LynxViewBuilder setEnableCreateViewAsync(boolean enable) {
    return this;
  }

  /**
   * Experimental API.
   *
   * Enable auto concurrency or not.
   *
   * For this mode, use MULTI_THREADS and EnableMultiAsyncThread by default.
   *
   * Only switch to PART_ON_LAYOUT when absolutely necessary,
   * like load template or render list child.
   *
   * @see ThreadStrategyForRendering#MULTI_THREADS
   * @see ThreadStrategyForRendering#PART_ON_LAYOUT.
   * @see LynxViewBuilder#setEnableMultiAsyncThread
   */
  public LynxViewBuilder setEnableAutoConcurrency(boolean enable) {
    enableAutoConcurrency = enable;
    return this;
  }

  /**
   * Experimental API.
   *
   * Enable vsync-aligned message loop for ui thread or not.
   * If enabled, the task inside message loop of ui thread will be executed aligned with vsync.
   */
  public LynxViewBuilder setEnableVSyncAlignedMessageLoop(boolean enable) {
    enableVSyncAlignedMessageLoop = enable;
    return this;
  }

  /**
   * Each lynxview has its own TASM/Layout thread, For Thread Mode of MultiThread
   * @param enableMultiAsyncThread
   * @return
   */
  public LynxViewBuilder setEnableMultiAsyncThread(boolean enableMultiAsyncThread) {
    this.enableMultiAsyncThread = enableMultiAsyncThread;
    return this;
  }

  /**
   * Experimental API.
   *
   * enable sync flush ui operations when onMeasure or not.
   *
   * only use for MULTI_THREADS, avoid flash or shake when
   * RenderTemplate or UpdateData.
   *
   * if use MULTI_THREADS for feed scene, each cell is a
   * LynxView, you'd better enable here, or must disable.
   *
   * @see ThreadStrategyForRendering#MULTI_THREADS
   */
  public LynxViewBuilder setEnableSyncFlush(boolean enable) {
    enableSyncFlush = enable;
    return this;
  }

  /**
   * Run the hydration process in a async thread.
   */
  public LynxViewBuilder setEnableAsyncHydration(boolean enable) {
    enableAsyncHydration = enable;
    return this;
  }

  public LynxViewBuilder setEnablePendingJsTask(boolean enablePendingJsTask) {
    this.enablePendingJsTask = enablePendingJsTask;
    return this;
  }

  public LynxViewBuilder setEnableJSRuntime(boolean enable) {
    enableJSRuntime = enable;
    return this;
  }

  public LynxViewBuilder setEnableAirStrictMode(boolean enable) {
    enableAirStrictMode = enable;
    return this;
  }

  public LynxViewBuilder setDynamicComponentFetcher(DynamicComponentFetcher fetcher) {
    this.fetcher = fetcher;
    return this;
  }

  public LynxViewBuilder setResourceProvider(String key, LynxResourceProvider provider) {
    lynxRuntimeOptions.setResourceProviders(key, provider);
    return this;
  }

  /**
   * Sets whether or not to allow force dark to apply to lynx context, default is false.
   * @param allowed Whether or not to allow force dark
   * @return
   */
  public LynxViewBuilder setForceDarkAllowed(boolean allowed) {
    forceDarkAllowed = allowed;
    return this;
  }

  /**
   * For generic resource fetching.
   * @param resourceFetcher need to implement the request interface and requestSync interface in
   *     LynxResourceFetcher.
   * @return
   */
  public LynxViewBuilder setResourceFetcher(LynxResourceFetcher resourceFetcher) {
    this.resourceFetcher = resourceFetcher;
    return this;
  }

  public LynxViewBuilder setFontLoader(LynxFontFaceLoader.Loader fontLoader) {
    this.fontLoader = fontLoader;
    return this;
  }

  public LynxViewBuilder setFontScale(float scale) {
    fontScale = scale;
    return this;
  }

  public LynxViewBuilder setImageFetcher(LynxImageFetcher imageFetcher) {
    this.imageFetcher = imageFetcher;
    return this;
  }

  /**
   * Supports passing custom parameters to image network requests,
   * currently only used in image compliance scenarios.
   * @param imageCustomParams default is null
   * @return
   */
  public LynxViewBuilder setImageCustomParam(Map<String, String> imageCustomParams) {
    mImageCustomParam = imageCustomParams;
    return this;
  }

  /**
   * Build a LynxView using a pre-created LynxBackgroundRuntime, if the runtime is
   * non-null, LynxView will use this runtime.
   *
   * @important Once the builder is used to create a LynxView, this runtime is
   * consumed and set to null
   *
   * @param runtime LynxBackgroundRuntime will be used by LynxView
   */
  public LynxViewBuilder setLynxBackgroundRuntime(LynxBackgroundRuntime runtime) {
    lynxBackgroundRuntime = runtime;
    return this;
  }

  /**
   * set lynx view can be debug or not
   * when switch enableDevTool is disabled and
   * switch enableDevToolForDebuggableView is enabled
   */
  public LynxViewBuilder setDebuggable(boolean enable) {
    debuggable = enable;
    return this;
  }

  @Deprecated
  public LynxViewBuilder setEnableRadonCompatible(boolean enableRadonCompatible) {
    return this;
  }
  public LynxViewBuilder setEnableLayoutOnly(boolean enableLayoutOnly) {
    return this;
  };

  public LynxView build(@NonNull Context context) {
    TraceEvent.beginSection("CreateLynxView");
    LynxViewConfigProcessor.parseForLynxViewBuilder(lynxViewConfig, this);
    LynxView lynxView = new LynxView(context, this);
    TraceEvent.endSection("CreateLynxView");

    return lynxView;
  }

  public LynxViewBuilder registerContextData(String key, Object value) {
    if (this.mContextData == null) {
      this.mContextData = new HashMap<String, Object>();
    }
    this.mContextData.put(key, value);
    return this;
  }

  public HashMap getContextData() {
    return this.mContextData;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public Map<String, String> getImageCustomParam() {
    return this.mImageCustomParam;
  }

  /**
   * enableJSRuntime、enableAirStrictMode both determine whether js thread is enabled.
   * Change enableJSRuntime & enableAirStrictMode private and add enableJSRuntime() to expose for
   * usage.
   */
  public Boolean enableJSRuntime() {
    if (enableAirStrictMode) {
      return false;
    } else {
      return enableJSRuntime;
    }
  }

  /**
   * Control whether updateData can take effect before loadTemplate
   *
   * @param enable whether updateData could take effect before loadTemplate
   */
  public LynxViewBuilder setEnablePreUpdateData(boolean enable) {
    enablePreUpdateData = enable;
    return this;
  }

  /**
   * set resource provider for lynx generic resources;
   * @param fetcher
   */
  public void setGenericResourceFetcher(@NonNull LynxGenericResourceFetcher fetcher) {
    this.lynxRuntimeOptions.genericResourceFetcher(fetcher);
  }

  /**
   * set resource provider for lynx media resources;
   * @param fetcher
   */
  public void setMediaResourceFetcher(@NonNull LynxMediaResourceFetcher fetcher) {
    this.lynxRuntimeOptions.mediaResourceFetcher(fetcher);
  }

  /**
   * set resource fetcher for lynx template resources.
   * @param fetcher
   */
  public void setTemplateResourceFetcher(@NonNull LynxTemplateResourceFetcher fetcher) {
    this.lynxRuntimeOptions.templateResourceFetcher(fetcher);
  }

  /**
   * set enableGenericResourceFetcher or not.
   * @param BooleanOption: enabled
   */
  public void setEnableGenericResourceFetcher(LynxBooleanOption enabled) {
    this.enableGenericResourceFetcher = enabled;
  }

  /**
   * Support container to pass in LynxViewConfig for direct parsing and processing within Lynx.
   * @param map schema map
   */
  public LynxViewBuilder setLynxViewConfig(Map<String, String> map) {
    lynxViewConfig = map;
    return this;
  }

  public LynxViewBuilder setLynxUIRenderer(ILynxUIRenderer renderer) {
    lynxUIRenderer = renderer;
    return this;
  }

  public ILynxUIRenderer lynxUIRenderer() {
    return lynxUIRenderer;
  }
}
