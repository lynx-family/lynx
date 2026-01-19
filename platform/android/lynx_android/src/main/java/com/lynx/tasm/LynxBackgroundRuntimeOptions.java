// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.jsbridge.LynxModule;
import com.lynx.jsbridge.ParamWrapper;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class LynxBackgroundRuntimeOptions {
  // These flag align with LynxRuntime.h's LynxRuntimeFlags define.
  private static int RUNTIME_FLAG_INIT = 0;
  private static int RUNTIME_FLAG_ENABLE_USER_BYTECODE = 1 << 0;
  private static int RUNTIME_FLAG_ENABLE_JS_GROUP_THREAD = 1 << 1;
  private static int RUNTIME_FLAG_FORCE_RELOAD_CORE_JS = 1 << 2;
  private static int RUNTIME_FLAG_FORCE_USE_LIGHT_WEIGHT_JS_ENGINE = 1 << 3;
  private static int RUNTIME_FLAG_PENDING_CORE_JS_LOAD = 1 << 4;
  // Pending lynx_core.js load
  private static int RUNTIME_FLAG_PENDING_JS_TASK = 1 << 5;

  private boolean mEnableUserBytecode;
  private String mBytecodeSourceUrl;
  private @Nullable LynxGroup mLynxGroup;
  private final List<ParamWrapper> mWrappers;
  private final Map<String, LynxResourceProvider> mResourceProviders;
  private TemplateData mPresetData;
  private TemplateData mGlobalProps;
  LynxGenericResourceFetcher genericResourceFetcher;
  LynxMediaResourceFetcher mediaResourceFetcher;
  LynxTemplateResourceFetcher templateResourceFetcher;
  LynxBooleanOption enableGenericResourceFetcher = LynxBooleanOption.UNSET;
  // Pending lynx_core.js load
  private boolean mPendingCoreJsLoad;
  private LynxModule.AuthValidator mAuthValidator;

  public LynxBackgroundRuntimeOptions() {
    mResourceProviders = new HashMap<>();
    mWrappers = new ArrayList<>();
  }

  public LynxBackgroundRuntimeOptions(@NonNull LynxBackgroundRuntimeOptions other) {
    this.mEnableUserBytecode = other.mEnableUserBytecode;
    this.mBytecodeSourceUrl = other.mBytecodeSourceUrl;
    this.mLynxGroup = other.mLynxGroup;
    this.mWrappers = new ArrayList<>(other.mWrappers);
    this.mResourceProviders = new HashMap<>(other.mResourceProviders);
    this.mPresetData = other.mPresetData;
    this.mGlobalProps = other.mGlobalProps;
    this.genericResourceFetcher = other.genericResourceFetcher;
    this.mediaResourceFetcher = other.mediaResourceFetcher;
    this.templateResourceFetcher = other.templateResourceFetcher;
    this.enableGenericResourceFetcher = other.enableGenericResourceFetcher;
    this.mPendingCoreJsLoad = other.mPendingCoreJsLoad;
    this.mAuthValidator = other.mAuthValidator;
  }

  public void registerModule(String name, Class<? extends LynxModule> module, Object param) {
    ParamWrapper wrapper = new ParamWrapper();
    wrapper.setModuleClass(module);
    wrapper.setParam(param);
    wrapper.setName(name);
    mWrappers.add(wrapper);
  }

  public void registerModuleAuthValidator(LynxModule.AuthValidator authValidator) {
    mAuthValidator = authValidator;
  }

  public LynxModule.AuthValidator getModuleAuthValidator() {
    return mAuthValidator;
  }

  public boolean useQuickJSEngine() {
    return mLynxGroup != null ? !mLynxGroup.enableV8() : true;
  }

  public boolean isEnableUserBytecode() {
    return mEnableUserBytecode;
  }

  public void setEnableUserBytecode(boolean mEnableUserBytecode) {
    this.mEnableUserBytecode = mEnableUserBytecode;
  }

  public String getBytecodeSourceUrl() {
    return mBytecodeSourceUrl;
  }

  public void setBytecodeSourceUrl(String mBytecodeSourceUrl) {
    this.mBytecodeSourceUrl = mBytecodeSourceUrl;
  }

  // set pending lynx_core.js load
  public void setPendingCoreJsLoad(boolean pending) {
    this.mPendingCoreJsLoad = pending;
  }

  // if pending lynx_core.js load
  public boolean isPendingCoreJsLoad() {
    return mPendingCoreJsLoad;
  }

  @Nullable
  public LynxGroup getLynxGroup() {
    return mLynxGroup;
  }

  public void setLynxGroup(@Nullable LynxGroup lynxGroup) {
    this.mLynxGroup = lynxGroup;
  }

  public List<ParamWrapper> getWrappers() {
    return mWrappers;
  }

  // @Deprecated, use genericResourceFetcher/mediaResourceFetcher/templateResourceFetcher instead
  public void setResourceProviders(String key, LynxResourceProvider provider) {
    mResourceProviders.put(key, provider);
  }

  public LynxResourceProvider getResourceProvidersByKey(String key) {
    return mResourceProviders.get(key);
  }

  public Set<Map.Entry<String, LynxResourceProvider>> getAllResourceProviders() {
    return mResourceProviders.entrySet();
  }

  /**
   * Set readonly data for LynxBackgroundRuntime, FE can access this data
   * via `lynx.__presetData`
   * @important set data {@link TemplateData#markReadOnly()} readonly} to avoid copy
   */
  public void setPresetData(TemplateData data) {
    mPresetData = data;
  }

  TemplateData getPresetData() {
    return mPresetData;
  }

  /**
   * Set readonly data for LynxBackgroundRuntime, FE can access this data
   * via `lynx.__globalProps`
   * @important set data {@link TemplateData#markReadOnly()} readonly} to avoid copy
   */
  public void setGlobalProps(TemplateData data) {
    mGlobalProps = data;
  }

  TemplateData getGlobalProps() {
    return mGlobalProps;
  }

  public void setGenericResourceFetcher(@NonNull LynxGenericResourceFetcher fetcher) {
    this.genericResourceFetcher = fetcher;
  }

  public LynxGenericResourceFetcher getGenericResourceFetcher() {
    return this.genericResourceFetcher;
  }

  public void setMediaResourceFetcher(@NonNull LynxMediaResourceFetcher fetcher) {
    this.mediaResourceFetcher = fetcher;
  }

  public LynxMediaResourceFetcher getMediaResourceFetcher() {
    return this.mediaResourceFetcher;
  }

  public void setTemplateResourceFetcher(@NonNull LynxTemplateResourceFetcher fetcher) {
    this.templateResourceFetcher = fetcher;
  }

  public LynxTemplateResourceFetcher getTemplateResourceFetcher() {
    return this.templateResourceFetcher;
  }

  public void setEnableGenericResourceFetcher(LynxBooleanOption enabled) {
    this.enableGenericResourceFetcher = enabled;
  }

  public LynxBooleanOption isEnableGenericResourceFetcher() {
    return this.enableGenericResourceFetcher;
  }

  // There are 2 ways to use this method:
  // 1. merge LynxBackgroundRuntimeOptions as `other` into LynxBackgroundRuntime as `this`
  // 2. merge LynxBackgroundRuntimeOptions as `other` into LynxViewBuilder as `this`
  // for `1` LynxBackgroundRuntime's configurations are not set, any overwrite/assign is safe
  // for `2` We need follow the rules described below.
  void merge(LynxBackgroundRuntimeOptions other) {
    // Overwrite BackgroundRuntime configurations:
    // This part of configurations are used to create BackgroundRuntime and cannot be modified
    // after it attaches to LynxView. So we use the configurations inside runtime to overwrite
    // LynxViewBuilder
    this.mLynxGroup = other.mLynxGroup;
    this.mEnableUserBytecode = other.mEnableUserBytecode;
    this.mBytecodeSourceUrl = other.mBytecodeSourceUrl;
    // LynxView doesn't pending core js load.
    this.mPendingCoreJsLoad = false;
    this.mWrappers.addAll(other.mWrappers);

    // Merge these Fetchers only if they are unset:
    // This part of configurations are shared between runtime and platform-level of LynxView.
    // We need to keep the configurations on LynxViewBuilder if Fetchers are set.
    if (this.enableGenericResourceFetcher == LynxBooleanOption.UNSET) {
      this.enableGenericResourceFetcher = other.enableGenericResourceFetcher;
    }
    this.genericResourceFetcher = this.genericResourceFetcher != null
        ? this.genericResourceFetcher
        : other.genericResourceFetcher;
    this.mediaResourceFetcher =
        this.mediaResourceFetcher != null ? this.mediaResourceFetcher : other.mediaResourceFetcher;
    this.templateResourceFetcher = this.templateResourceFetcher != null
        ? this.templateResourceFetcher
        : other.templateResourceFetcher;

    for (Map.Entry<String, LynxResourceProvider> local : other.mResourceProviders.entrySet()) {
      if (!mResourceProviders.containsKey(local.getKey())) {
        mResourceProviders.put(local.getKey(), local.getValue());
      }
    }
  }

  /**
   * calc runtime flags for native side.
   * @param forceReloadJSCore
   * @param enablePendingJsTask
   * @return flags
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public int calcRuntimeFlags(boolean forceReloadJSCore, boolean enablePendingJsTask) {
    int flags = RUNTIME_FLAG_INIT;
    flags = setRuntimeFlag(flags, forceReloadJSCore, RUNTIME_FLAG_FORCE_RELOAD_CORE_JS);
    flags =
        setRuntimeFlag(flags, useQuickJSEngine(), RUNTIME_FLAG_FORCE_USE_LIGHT_WEIGHT_JS_ENGINE);
    flags = setRuntimeFlag(flags, enablePendingJsTask, RUNTIME_FLAG_PENDING_JS_TASK);
    flags = setRuntimeFlag(flags, isEnableUserBytecode(), RUNTIME_FLAG_ENABLE_USER_BYTECODE);
    LynxGroup group = getLynxGroup();
    if (group != null) {
      flags =
          setRuntimeFlag(flags, group.enableJSGroupThread(), RUNTIME_FLAG_ENABLE_JS_GROUP_THREAD);
    }
    flags = setRuntimeFlag(flags, isPendingCoreJsLoad(), RUNTIME_FLAG_PENDING_CORE_JS_LOAD);
    return flags;
  }

  private static int setRuntimeFlag(int currentFlag, boolean enable, int flag) {
    if (enable) {
      currentFlag |= flag;
    } else {
      currentFlag &= ~flag;
    }
    return currentFlag;
  }
}
