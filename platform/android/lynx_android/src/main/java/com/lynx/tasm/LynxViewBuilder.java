// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.jsbridge.LynxModule;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import com.lynx.tasm.component.DynamicComponentFetcher;
import com.lynx.tasm.group.ILynxViewConfigProvider;
import com.lynx.tasm.group.ILynxViewGroup;
import com.lynx.tasm.group.LynxBaseConfigurator;
import com.lynx.tasm.image.LynxImageConfig;
import com.lynx.tasm.image.model.LynxImageFetcher;
import com.lynx.tasm.loader.LynxFontFaceLoader;
import com.lynx.tasm.provider.AbsTemplateProvider;
import com.lynx.tasm.provider.LynxResourceFetcher;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.ILynxTrailServiceExtension;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class LynxViewBuilder
    extends LynxBaseConfigurator<LynxViewBuilder> implements ILynxViewConfigProvider {
  AbsTemplateProvider templateProvider;
  Object lynxModuleExtraData;
  DynamicComponentFetcher fetcher;
  LynxResourceFetcher resourceFetcher;
  LynxFontFaceLoader.Loader fontLoader;
  LynxImageFetcher imageFetcher;

  boolean enableLayoutOnly = LynxEnv.inst().isLayoutOnlyEnabled();
  Map<String, String> mImageCustomParam;
  LynxImageConfig mLynxImageConfig;
  Map<String, String> lynxViewConfig;
  LynxBackgroundRuntime lynxBackgroundRuntime;
  Uri uri = null;
  ILynxViewGroup lynxViewGroup;

  public LynxViewBuilder() {
    LynxEnv.inst().lazyInitIfNeeded();
    templateProvider = LynxEnv.inst().getTemplateProvider();
  }

  @Deprecated
  public LynxViewBuilder(Context context) {
    this();
  }

  public LynxViewBuilder setTemplateProvider(@Nullable AbsTemplateProvider provider) {
    templateProvider = provider;
    return this;
  }

  /**
   * @brief create a UIRenderer for UIBodyView and refactor the thread strategy.
   * @return UIRenderer
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public ILynxUIRenderer createLynxUIRenderer() {
    ILynxUIRenderer uiRenderer = getUIRendererCreator().createLynxUIRender();
    setThreadStrategyForRendering(uiRenderer.getSupportedThreadStrategy(getThreadStrategy()));
    return uiRenderer;
  }

  /**
   * @brief set the uri for LynxView, which will be parsed by ILynxTrailService in building
   * LynxView.
   * @param uri uri for LynxView
   * @return this
   */
  public LynxViewBuilder setUri(@Nullable Uri uri) {
    this.uri = uri;
    return this;
  }

  /**
   * @brief get the uri of LynxView
   * @return uri of LynxView
   */
  public Uri getUri() {
    return uri;
  }

  /**
   * insert a key-value pair into lynxViewConfig if the key does not exist, otherwise ignored.
   * @param key
   * @param value
   * @return
   */
  public LynxViewBuilder insertLynxViewConfig(String key, String value) {
    if (lynxViewConfig == null) {
      lynxViewConfig = new HashMap<>();
    }
    if (lynxViewConfig.get(key) == null) {
      lynxViewConfig.put(key, value);
    }
    return this;
  }

  public LynxViewBuilder setLynxViewGroup(ILynxViewGroup group) {
    this.lynxViewGroup = group;
    return this;
  }

  @Override
  public LynxGroup getLynxGroup() {
    // Prefers lynxGroup in lynxViewBuilder.
    LynxGroup lynxGroup = this.lynxRuntimeOptions.getLynxGroup();
    if (lynxGroup != null) {
      return lynxGroup;
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getLynxGroup();
    }
    return null;
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
   * @param resourceFetcher need to implement the request interface and requestSync interface in
   *     LynxResourceFetcher.
   */
  public LynxViewBuilder setResourceFetcher(LynxResourceFetcher resourceFetcher) {
    this.resourceFetcher = resourceFetcher;
    return this;
  }

  public LynxViewBuilder setFontLoader(LynxFontFaceLoader.Loader fontLoader) {
    this.fontLoader = fontLoader;
    return this;
  }

  @Deprecated
  public LynxViewBuilder setEnableRadonCompatible(boolean enableRadonCompatible) {
    return this;
  }

  @Deprecated
  public LynxViewBuilder setEnableLayoutOnly(boolean enableLayoutOnly) {
    return this;
  }

  public LynxViewBuilder setDynamicComponentFetcher(DynamicComponentFetcher fetcher) {
    this.fetcher = fetcher;
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
   * @see ThreadStrategyForRendering#PART_ON_LAYOUT
   * @see LynxViewBuilder#setEnableMultiAsyncThread
   */
  @Deprecated
  @Override
  public LynxViewBuilder setEnableAutoConcurrency(boolean enable) {
    return super.setEnableAutoConcurrency(enable);
  }

  public LynxViewBuilder setImageFetcher(LynxImageFetcher imageFetcher) {
    this.imageFetcher = imageFetcher;
    return this;
  }

  @Deprecated
  public LynxViewBuilder setUIRunningMode(boolean ui) {
    super.setUIRunningMode(ui);
    return this;
  }

  @Override
  public void setCustomBehaviorRegistry(@NonNull BehaviorRegistry registry) {
    super.setCustomBehaviorRegistry(registry);
  }

  @Override
  public LynxViewBuilder addBehaviors(@NonNull List<Behavior> behaviorList) {
    return super.addBehaviors(behaviorList);
  }

  @Override
  public LynxViewBuilder addBehavior(@NonNull Behavior behavior) {
    return super.addBehavior(behavior);
  }

  @Override
  public LynxViewBuilder setLynxGroup(@Nullable LynxGroup group) {
    return super.setLynxGroup(group);
  }

  @Override
  public LynxViewBuilder setEnableLayoutSafepoint(boolean enable) {
    return super.setEnableLayoutSafepoint(enable);
  }

  @Override
  public LynxViewBuilder setThreadStrategyForRendering(ThreadStrategyForRendering strategy) {
    return super.setThreadStrategyForRendering(strategy);
  }

  @Override
  public LynxViewBuilder setPresetMeasuredSpec(int widthMeasureSpec, int heightMeasureSpec) {
    return super.setPresetMeasuredSpec(widthMeasureSpec, heightMeasureSpec);
  }

  @Override
  public LynxViewBuilder setResourceProvider(String key, LynxResourceProvider provider) {
    return super.setResourceProvider(key, provider);
  }

  /**
   * Sets whether or not to allow force dark to apply to lynx context, default is false.
   * @param allowed Whether or not to allow force dark
   * @return
   */
  @Override
  public LynxViewBuilder setForceDarkAllowed(boolean allowed) {
    return super.setForceDarkAllowed(allowed);
  }

  @Override
  public void registerModule(String name, Class<? extends LynxModule> module) {
    super.registerModule(name, module);
  }

  @Override
  public void registerModule(String name, Class<? extends LynxModule> module, Object param) {
    super.registerModule(name, module, param);
  }

  @Override
  public void registerModuleAuthValidator(LynxModule.AuthValidator authValidator) {
    super.registerModuleAuthValidator(authValidator);
  }

  @Override
  public LynxViewBuilder setEnableUserCodeCache(boolean enableUserBytecode) {
    return super.setEnableUserCodeCache(enableUserBytecode);
  }

  @Override
  public LynxViewBuilder setCodeCacheSourceUrl(String url) {
    return super.setCodeCacheSourceUrl(url);
  }

  @Override
  public void setGenericResourceFetcher(@NonNull LynxGenericResourceFetcher fetcher) {
    super.setGenericResourceFetcher(fetcher);
  }

  @Override
  public void setMediaResourceFetcher(@NonNull LynxMediaResourceFetcher fetcher) {
    super.setMediaResourceFetcher(fetcher);
  }

  @Override
  public void setTemplateResourceFetcher(@NonNull LynxTemplateResourceFetcher fetcher) {
    super.setTemplateResourceFetcher(fetcher);
  }

  @Override
  public void setEnableGenericResourceFetcher(LynxBooleanOption enabled) {
    super.setEnableGenericResourceFetcher(enabled);
  }

  @Override
  public LynxViewBuilder setFontScale(float scale) {
    return super.setFontScale(scale);
  }

  @Override
  public LynxViewBuilder setScreenSize(int width, int height) {
    return super.setScreenSize(width, height);
  }

  @Override
  public LynxViewBuilder setEnablePendingJsTask(boolean enablePendingJsTask) {
    return super.setEnablePendingJsTask(enablePendingJsTask);
  }

  @Override
  public LynxViewBuilder setEnableJSRuntime(boolean enable) {
    return super.setEnableJSRuntime(enable);
  }

  @Override
  public LynxViewBuilder enableAutoExpose(boolean enableAutoExpose) {
    return super.enableAutoExpose(enableAutoExpose);
  }

  @Override
  public LynxViewBuilder setEnableUserBytecode(boolean enableUserBytecode) {
    return super.setEnableUserBytecode(enableUserBytecode);
  }

  @Override
  public LynxViewBuilder setEnableVSyncAlignedMessageLoop(boolean enable) {
    return super.setEnableVSyncAlignedMessageLoop(enable);
  }

  @Override
  public LynxViewBuilder setBytecodeSourceUrl(String url) {
    return super.setBytecodeSourceUrl(url);
  }

  @Override
  public LynxViewBuilder setEnableAirStrictMode(boolean enable) {
    return super.setEnableAirStrictMode(enable);
  }

  @Override
  public LynxViewBuilder setEnableMultiAsyncThread(boolean enableMultiAsyncThread) {
    return super.setEnableMultiAsyncThread(enableMultiAsyncThread);
  }

  @Override
  public LynxViewBuilder setEnableSyncFlush(boolean enable) {
    return super.setEnableSyncFlush(enable);
  }

  /**
   * Supports passing custom parameters to image network requests,
   * currently only used in image compliance scenarios.
   * @param imageCustomParams default is null
   */
  public LynxViewBuilder setImageCustomParam(Map<String, String> imageCustomParams) {
    mImageCustomParam = imageCustomParams;
    return this;
  }

  /**
   * Set the configuration for Lynx image.
   * @param imageConfig the configuration for Lynx image
   */
  public LynxViewBuilder setLynxImageConfig(LynxImageConfig imageConfig) {
    mLynxImageConfig = imageConfig;
    return this;
  }

  public LynxImageConfig getLynxImageConfig() {
    return mLynxImageConfig;
  }

  /**
   * Support container to pass in LynxViewConfig for direct parsing and processing within Lynx.
   * @param map schema map
   */
  public LynxViewBuilder setLynxViewConfig(Map<String, String> map) {
    lynxViewConfig = map;
    return this;
  }

  public Map<String, String> getLynxViewConfig() {
    return lynxViewConfig;
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

  @Override
  public BehaviorRegistry getBehaviorRegistry() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getBehaviorRegistry();
    }
    return this.behaviorRegistry;
  }

  @Override
  public boolean isEnableAutoExpose() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableAutoExpose();
    }
    return this.enableAutoExpose;
  }

  @Override
  public Float getDensity() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getDensity();
    }
    return this.densityOverride;
  }

  @Override
  public ThreadStrategyForRendering getThreadStrategy() {
    if (this.threadStrategy != null) {
      return this.threadStrategy;
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getThreadStrategy();
    }
    return ThreadStrategyForRendering.ALL_ON_UI;
  }

  @Override
  public boolean isEnableLayoutSafepoint() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableLayoutSafepoint();
    }
    return this.enableLayoutSafepoint;
  }

  @Override
  public boolean isEnableUnifiedPipeline() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableUnifiedPipeline();
    }
    return this.enableUnifiedPipeline;
  }

  @Override
  public HashMap getContextData() {
    if (this.mContextData != null) {
      return this.mContextData;
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getContextData();
    }
    return null;
  }

  @Override
  public LynxBackgroundRuntimeOptions getLynxRuntimeOptions() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getLynxRuntimeOptions();
    }
    return this.lynxRuntimeOptions;
  }

  LynxViewBuilder mergeLynxRuntimeOptions(LynxBackgroundRuntimeOptions other) {
    LynxBackgroundRuntimeOptions options = getLynxRuntimeOptions();
    if (options != null) {
      options.merge(other);
    }
    return this;
  }

  @Override
  public int getScreenWidth() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getScreenWidth();
    }
    return this.screenWidth;
  }

  @Override
  public int getScreenHeight() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getScreenHeight();
    }
    return this.screenHeight;
  }

  @Override
  public boolean getForceDarkAllowed() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getForceDarkAllowed();
    }
    return this.forceDarkAllowed;
  }

  @Override
  public boolean isEnableMultiAsyncThread() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableMultiAsyncThread();
    }
    return this.enableMultiAsyncThread;
  }

  @Override
  public boolean isEnableSyncFlush() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableSyncFlush();
    }
    return this.enableSyncFlush;
  }

  @Override
  @Deprecated
  public boolean isEnableAutoConcurrency() {
    return false;
  }

  @Override
  public boolean isEnableVSyncAlignedMessageLoop() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableVSyncAlignedMessageLoop();
    }
    return this.enableVSyncAlignedMessageLoop;
  }

  @Override
  public boolean isEnablePendingJsTask() {
    if (hasPendingJsTaskSet) {
      return this.enablePendingJsTask;
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnablePendingJsTask();
    }
    return false;
  }

  @Override
  public boolean isEnableAsyncHydration() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableAsyncHydration();
    }
    return this.enableAsyncHydration;
  }

  @Override
  public boolean isEnableJSRuntime() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableJSRuntime();
    }

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
  @Override
  public LynxViewBuilder setEnablePreUpdateData(boolean enable) {
    return super.setEnablePreUpdateData(enable);
  }

  @Override
  public boolean isEnableAirStrictMode() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableAirStrictMode();
    }
    return this.enableAirStrictMode;
  }

  @Override
  public boolean isDebuggable() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isDebuggable();
    }
    return this.debuggable;
  }

  @Override
  public boolean hasPresetMeasureSpec() {
    return this.hasPresetMeasureSpec;
  }

  @Override
  public int getPresetWidthMeasureSpec() {
    if (this.hasPresetMeasureSpec) {
      return this.presetWidthMeasureSpec;
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getPresetWidthMeasureSpec();
    }
    return this.presetWidthMeasureSpec;
  }

  @Override
  public int getPresetHeightMeasureSpec() {
    if (this.hasPresetMeasureSpec) {
      return this.presetHeightMeasureSpec;
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getPresetHeightMeasureSpec();
    }
    return this.presetHeightMeasureSpec;
  }

  @Override
  public float getFontScale() {
    if (hasFontScaleSet) {
      return this.fontScale;
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getFontScale();
    }
    return this.fontScale;
  }

  @Override
  public boolean isEnablePreUpdateData() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnablePreUpdateData();
    }
    return this.enablePreUpdateData;
  }

  @Override
  public IUIRendererCreator getUIRendererCreator() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getUIRendererCreator();
    }
    return this.uiRendererCreator;
  }

  @Override
  public int getEmbeddedMode() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getEmbeddedMode();
    }
    return this.embeddedMode;
  }

  @Override
  public boolean isEnableMTSModule() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableMTSModule();
    }
    return this.enableMTSModule;
  }

  @Override
  public LynxBooleanOption isEnableGenericResourceFetcher() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.isEnableGenericResourceFetcher();
    }
    return lynxRuntimeOptions.isEnableGenericResourceFetcher();
  }

  @Override
  public LynxGenericResourceFetcher getLynxGenericResourceFetcher() {
    if (lynxRuntimeOptions.getGenericResourceFetcher() != null) {
      return lynxRuntimeOptions.getGenericResourceFetcher();
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getLynxGenericResourceFetcher();
    }
    return null;
  }

  @Override
  public LynxMediaResourceFetcher getLynxMediaResourceFetcher() {
    if (lynxRuntimeOptions.getMediaResourceFetcher() != null) {
      return lynxRuntimeOptions.getMediaResourceFetcher();
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getLynxMediaResourceFetcher();
    }
    return null;
  }

  @Override
  public LynxTemplateResourceFetcher getLynxTemplateResourceFetcher() {
    if (lynxRuntimeOptions.getTemplateResourceFetcher() != null) {
      return lynxRuntimeOptions.getTemplateResourceFetcher();
    }
    if (lynxViewGroup != null) {
      return lynxViewGroup.getLynxTemplateResourceFetcher();
    }
    return null;
  }

  @Override
  public ILynxLogicExecutor getLogicExecutor() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getLogicExecutor();
    }
    return null;
  }

  @Override
  public String getTapSlop() {
    if (lynxViewGroup != null) {
      return lynxViewGroup.getTapSlop();
    }
    return this.tapSlop;
  }

  public LynxView build(@NonNull Context context) {
    TraceEvent.beginSection(TraceEventDef.LYNXVIEW_BUILDER_BUILD);
    ILynxTrailService trailService = LynxServiceCenter.inst().getService(ILynxTrailService.class);
    if (trailService instanceof ILynxTrailServiceExtension) {
      ((ILynxTrailServiceExtension) trailService).parseLynxViewBuilder(this);
    }

    LynxView lynxView = new LynxView(context, this);
    if (TraceEvent.enableTrace()) {
      HashMap<String, String> args = new HashMap<>();
      if (lynxView.getLynxContext() != null) {
        args.put(
            TraceEventDef.INSTANCE_ID, String.valueOf(lynxView.getLynxContext().getInstanceId()));
      }
      TraceEvent.endSection(
          TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNXVIEW_BUILDER_BUILD, args);
    }
    return lynxView;
  }
}
