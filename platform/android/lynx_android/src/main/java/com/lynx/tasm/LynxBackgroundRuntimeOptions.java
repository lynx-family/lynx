// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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
  private boolean mEnableUserBytecode;
  private String mBytecodeSourceUrl;
  private @Nullable LynxGroup mLynxGroup;
  private final List<ParamWrapper> mWrappers;
  private final Map<String, LynxResourceProvider> mResourceProviders;
  private TemplateData mPresetData;
  LynxGenericResourceFetcher genericResourceFetcher;
  LynxMediaResourceFetcher mediaResourceFetcher;
  LynxTemplateResourceFetcher templateResourceFetcher;

  public LynxBackgroundRuntimeOptions() {
    mResourceProviders = new HashMap<>();
    mWrappers = new ArrayList<>();
  }

  public void registerModule(String name, Class<? extends LynxModule> module, Object param) {
    ParamWrapper wrapper = new ParamWrapper();
    wrapper.setModuleClass(module);
    wrapper.setParam(param);
    wrapper.setName(name);
    mWrappers.add(wrapper);
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

  public void genericResourceFetcher(@NonNull LynxGenericResourceFetcher fetcher) {
    this.genericResourceFetcher = fetcher;
  }

  public void mediaResourceFetcher(@NonNull LynxMediaResourceFetcher fetcher) {
    this.mediaResourceFetcher = fetcher;
  }

  public void templateResourceFetcher(@NonNull LynxTemplateResourceFetcher fetcher) {
    this.templateResourceFetcher = fetcher;
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

    // Merge these Fetchers only if they are unset:
    // This part of configurations are shared between runtime and platform-level of LynxView.
    // We need to keep the configurations on LynxViewBuilder if Fetchers are set.
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
}
