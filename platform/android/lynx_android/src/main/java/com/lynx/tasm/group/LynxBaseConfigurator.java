// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.jsbridge.LynxModule;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.IUIRendererCreator;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.BuiltInUIRegistry;
import com.lynx.tasm.behavior.LynxUIRendererCreator;
import com.lynx.tasm.behavior.TouchEventDispatcher;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.util.HashMap;
import java.util.List;

public class LynxBaseConfigurator<T extends LynxBaseConfigurator<T>> {
  protected boolean enableMultiAsyncThread = true;
  protected Float densityOverride;
  protected BehaviorRegistry behaviorRegistry;
  protected LynxBackgroundRuntimeOptions lynxRuntimeOptions;
  protected boolean enableAutoExpose;
  protected boolean enableLayoutSafepoint;
  protected boolean enableUnifiedPipeline;
  protected boolean forceDarkAllowed = false;
  protected boolean enableSyncFlush = false;
  @Deprecated protected boolean enableAutoConcurrency = false;
  protected boolean enableVSyncAlignedMessageLoop = false;
  protected boolean enablePendingJsTask = false;
  protected boolean hasPendingJsTaskSet = false;
  protected boolean hasPresetMeasureSpec = false;

  static Float defaultDensity = null;

  /**
   * enable async hydration of ssr.
   */
  protected boolean enableAsyncHydration = false;

  /**
   * enableJSRuntime、enableAirStrictMode both determine whether js thread will be enabled.
   * Change enableJSRuntime & enableAirStrictMode private and add enableJSRuntime() to expose for
   * usage.
   */
  protected boolean enableJSRuntime = true;

  /**
   * Add switch for Air Mode.
   * Currently only contributes to decide whether js thread will be enabled,
   * and further optimized logics may be involved.
   */
  protected boolean enableAirStrictMode = false;
  protected boolean debuggable = false;
  protected int presetWidthMeasureSpec;
  protected int presetHeightMeasureSpec;
  protected float fontScale = 1.0f;
  protected boolean hasFontScaleSet = false;

  protected boolean enablePreUpdateData = false;
  protected HashMap<String, Object> mContextData;

  protected ThreadStrategyForRendering threadStrategy = null;

  protected int screenWidth = DisplayMetricsHolder.UNDEFINE_SCREEN_SIZE_VALUE;
  protected int screenHeight = DisplayMetricsHolder.UNDEFINE_SCREEN_SIZE_VALUE;

  protected IUIRendererCreator uiRendererCreator;

  protected int embeddedMode = EmbeddedMode.UNSET;
  protected boolean enableMTSModule = false;

  protected String tapSlop = TouchEventDispatcher.mTapSlopDefault;

  public LynxBaseConfigurator() {
    LynxEnv.inst().lazyInitIfNeeded();
    lynxRuntimeOptions = new LynxBackgroundRuntimeOptions();
    behaviorRegistry = new BehaviorRegistry(LynxEnv.inst().getBehaviorMap());
    uiRendererCreator = new LynxUIRendererCreator();

    if (defaultDensity != null) {
      densityOverride = defaultDensity;
    }
  }

  /**
   * Experimental API.
   *
   * Set a default overriding density which will be applied to all LynxView constructed after it is
   * set. Will use screen density if default density is not set.
   * @param density overriding density by default, set it to null to reset the default density.
   */
  public static void setDefaultDensity(Float density) {
    defaultDensity = density;
  }

  /**
   * Set Custom BehaviorRegistry
   */
  public void setCustomBehaviorRegistry(@NonNull BehaviorRegistry registry) {
    this.behaviorRegistry = registry;
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
  public T setScreenSize(int width, int height) {
    this.screenWidth = width;
    this.screenHeight = height;
    return (T) this;
  }

  /**
   * Use {@link #addBehaviors(List)} instead.
   */
  @Deprecated
  public T setBehaviors(@Nullable List<Behavior> bundle) {
    if (bundle != null) {
      behaviorRegistry.addBehaviors(bundle);
    }
    return (T) this;
  }

  /**
   * Adding Behaviour Lists
   */
  public T addBehaviors(@NonNull List<Behavior> behaviorList) {
    this.behaviorRegistry.addBehaviors(behaviorList);
    return (T) this;
  }

  /**
   * Adding Single behavior
   */
  public T addBehavior(@NonNull Behavior behavior) {
    this.behaviorRegistry.addBehavior(behavior);
    return (T) this;
  }

  /**
   * To enable onShow/onHide lifecycle automatically
   *
   * @return this
   */
  public T enableAutoExpose(boolean enableAutoExpose) {
    this.enableAutoExpose = enableAutoExpose;
    return (T) this;
  }

  /**
   * Enable user bytecode. Controls bytecode for files other than lynx_core.js, such as
   * app-service.js.
   * @param enableUserBytecode whether to enableUserBytecode
   * @return this
   */
  public T setEnableUserBytecode(boolean enableUserBytecode) {
    this.lynxRuntimeOptions.setEnableUserBytecode(enableUserBytecode);
    return (T) this;
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
  public T setBytecodeSourceUrl(String url) {
    this.lynxRuntimeOptions.setBytecodeSourceUrl(url);
    return (T) this;
  }

  /**
   * Deprecated, please use {@link #setEnableUserBytecode(boolean)} instead.
   */
  @Deprecated
  public T setEnableUserCodeCache(boolean enableUserBytecode) {
    setEnableUserBytecode(enableUserBytecode);
    return (T) this;
  }

  /**
   * Deprecated, please use {@link #setBytecodeSourceUrl(String)} instead.
   */
  @Deprecated
  public T setCodeCacheSourceUrl(String url) {
    setBytecodeSourceUrl(url);
    return (T) this;
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
  public T setDensity(float density) {
    this.densityOverride = density;
    return (T) this;
  }

  public T setThreadStrategyForRendering(ThreadStrategyForRendering strategy) {
    if (null != strategy) {
      threadStrategy = strategy;
    }
    return (T) this;
  }

  public T setLynxGroup(@Nullable LynxGroup group) {
    lynxRuntimeOptions.setLynxGroup(group);
    return (T) this;
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
   *
   * @param authValidator {#{@link LynxModule.AuthValidator}} LynxMethod verification interface,
   *     used to verify whether the LynxMethod is allowed to be called.
   */
  public void registerModuleAuthValidator(LynxModule.AuthValidator authValidator) {
    lynxRuntimeOptions.registerModuleAuthValidator(authValidator);
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
  public T setEnableLayoutSafepoint(boolean enable) {
    enableLayoutSafepoint = enable;
    return (T) this;
  }

  /**
   * Set enableUnifiedPipeline explicitly;
   *
   * @param enableUnifiedPipeline whether to enableUnifiedPipeline or not;
   * @return this
   */
  public T setEnableUnifiedPipeline(boolean enableUnifiedPipeline) {
    this.enableUnifiedPipeline = enableUnifiedPipeline;
    return (T) this;
  }

  public T registerContextData(String key, Object value) {
    if (this.mContextData == null) {
      this.mContextData = new HashMap<String, Object>();
    }
    this.mContextData.put(key, value);
    return (T) this;
  }

  /**
   * set resource provider for lynx generic resources;
   * @param fetcher
   */
  public void setGenericResourceFetcher(@NonNull LynxGenericResourceFetcher fetcher) {
    this.lynxRuntimeOptions.setGenericResourceFetcher(fetcher);
  }

  /**
   * set resource provider for lynx media resources;
   * @param fetcher
   */
  public void setMediaResourceFetcher(@NonNull LynxMediaResourceFetcher fetcher) {
    this.lynxRuntimeOptions.setMediaResourceFetcher(fetcher);
  }

  /**
   * set resource fetcher for lynx template resources.
   * @param fetcher
   */
  public void setTemplateResourceFetcher(@NonNull LynxTemplateResourceFetcher fetcher) {
    this.lynxRuntimeOptions.setTemplateResourceFetcher(fetcher);
  }

  /**
   * set enableGenericResourceFetcher or not.
   * @param enabled
   */
  public void setEnableGenericResourceFetcher(LynxBooleanOption enabled) {
    this.lynxRuntimeOptions.setEnableGenericResourceFetcher(enabled);
  }

  public T setForceDarkAllowed(boolean allowed) {
    this.forceDarkAllowed = allowed;
    return (T) this;
  }

  /**
   * Each lynxview has its own TASM/Layout thread, For Thread Mode of MultiThread
   * @param enableMultiAsyncThread
   * @return
   */
  public T setEnableMultiAsyncThread(boolean enableMultiAsyncThread) {
    this.enableMultiAsyncThread = enableMultiAsyncThread;
    return (T) this;
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
  public T setEnableSyncFlush(boolean enable) {
    enableSyncFlush = enable;
    return (T) this;
  }

  public T setEnablePendingJsTask(boolean enablePendingJsTask) {
    this.enablePendingJsTask = enablePendingJsTask;
    this.hasPendingJsTaskSet = true;
    return (T) this;
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
   * @see ThreadStrategyForRendering#PART_ON_LAYOUT
   * @see LynxViewBuilder#setEnableMultiAsyncThread
   */
  @Deprecated
  public T setEnableAutoConcurrency(boolean enable) {
    enableAutoConcurrency = enable;
    return (T) this;
  }

  /**
   * Experimental API.
   *
   * Enable vsync-aligned message loop for ui thread or not.
   * If enabled, the task inside message loop of ui thread will be executed aligned with vsync.
   */
  public T setEnableVSyncAlignedMessageLoop(boolean enable) {
    enableVSyncAlignedMessageLoop = enable;
    return (T) this;
  }

  /**
   * Run the hydration process in a async thread.
   */
  public T setEnableAsyncHydration(boolean enable) {
    enableAsyncHydration = enable;
    return (T) this;
  }

  public T setEnableJSRuntime(boolean enable) {
    enableJSRuntime = enable;
    return (T) this;
  }

  public T setEnableAirStrictMode(boolean enable) {
    enableAirStrictMode = enable;
    return (T) this;
  }

  /**
   * set lynx view can be debug or not
   * when switch enableDevTool is disabled and
   * switch enableDevToolForDebuggableView is enabled
   */
  public T setDebuggable(boolean enable) {
    debuggable = enable;
    return (T) this;
  }

  @Deprecated
  public T setUIRunningMode(boolean ui) {
    if (ui) {
      threadStrategy = ThreadStrategyForRendering.ALL_ON_UI;
    } else {
      threadStrategy = ThreadStrategyForRendering.PART_ON_LAYOUT;
    }
    return (T) this;
  }

  public T setPresetMeasuredSpec(int widthMeasureSpec, int heightMeasureSpec) {
    presetHeightMeasureSpec = heightMeasureSpec;
    presetWidthMeasureSpec = widthMeasureSpec;
    hasPresetMeasureSpec = true;
    return (T) this;
  }

  public T setFontScale(float scale) {
    fontScale = scale;
    hasFontScaleSet = true;
    return (T) this;
  }

  /**
   * Control whether updateData can take effect before loadTemplate
   *
   * @param enable whether updateData could take effect before loadTemplate
   */
  public T setEnablePreUpdateData(boolean enable) {
    enablePreUpdateData = enable;
    return (T) this;
  }

  public T setUIRendererCreator(@NonNull IUIRendererCreator uiRendererCreator) {
    this.uiRendererCreator = uiRendererCreator;
    return (T) this;
  }

  /**
   * embeddedMode is an experimental switch. When embeddedMode is set,
   * we offer optimal performance for embedded scenarios,
   * but it will restrict business flexibility.
   * For now, you can just ignore this switch.
   * Please DO NOT enable this switch on your own for now.
   * Contact the Lynx team for more information.
   * @param embeddedMode
   * @return
   */
  public T setEmbeddedMode(@EmbeddedMode.Mode int embeddedMode) {
    this.embeddedMode = embeddedMode;
    if ((embeddedMode & EmbeddedMode.EMBEDDED_MODE_BASE) > 0) {
      behaviorRegistry.setBuiltInBehaviors(BuiltInUIRegistry.getInstance().getBuiltInUIBehaviors());
    }
    return (T) this;
  }

  public T setResourceProvider(String key, LynxResourceProvider provider) {
    lynxRuntimeOptions.setResourceProviders(key, provider);
    return (T) this;
  }

  public T setEnableMTSModule(boolean enable) {
    enableMTSModule = enable;
    return (T) this;
  }

  public T setTapSlop(String tapSlop) {
    this.tapSlop = tapSlop;
    return (T) this;
  }
}
