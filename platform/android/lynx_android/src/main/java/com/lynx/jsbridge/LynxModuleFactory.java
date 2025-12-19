// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * LynxModuleFactory is a factory class for creating LynxModules.has three main functions
 * 1. Managing registered Module Classes.use {@link #registerModule(String, Class, Object)} to
 * register a module class. and use {@link #mWrappers}  to cache the module class.
 * 2. Use {@link #getModule(String)} to create a LynxModuleWrapper through {@link #mModuleCreator}.
 * 3. Interacting with Native's LynxModuleFactory object
 */
public class LynxModuleFactory {
  private static final String TAG = "LynxModuleFactory";
  /**
   * When we use LynxBackgroundRuntime Standalone to create LynxView, JS Thread may
   * access LynxModule meanwhile new modules are registered during LynxView creation.
   * We use `ConcurrentHashMap` instead of `Map` here, because `putIfAbsent` on
   * `ConcurrentHashMap` requires API level 1, but on `Map ` requires API level 24.
   */
  private final ConcurrentHashMap<String, ParamWrapper> mWrappers;
  /**
   * native ptr of module factory
   */
  private long mNativePtr = 0;
  /**
   * Used for assembling Module instances, including CommonModuleCreator and SharedModuleCreator.
   */
  private IModuleCreator mModuleCreator;
  /**
   * Extra data that will be passed to Every LynxModule {@link LynxModule#mExtraData}.
   */
  private Object mLynxModuleExtraData;
  /**
   * Whether LynxModuleFactory is destroyed.
   */
  private boolean mHasDestroyed = false;

  /**
   * AuthValidator is only valid for the current LynxBackgroundRuntime Options.
   */
  private LynxModule.AuthValidator mAuthValidator;

  @Nullable AbstractLifecycleListener mLifecycleListener;

  /**
   * AbstractLifecycleListener is a listener for LynxModuleFactory's lifecycle.
   */
  public static abstract class AbstractLifecycleListener {
    public void onCreate(){};
    public void onModuleRegistered(@NonNull String name, @NonNull ParamWrapper wrapper){};
    public void onModuleCreated(@NonNull String name, @NonNull LynxModuleWrapper wrapper){};

    /**
     * called on module execute thread.like JS Thread
     */
    public abstract void onDestroy();
  }

  public LynxModuleFactory() {
    mWrappers = new ConcurrentHashMap<>();
  }

  public void setLifecycleListener(@Nullable AbstractLifecycleListener listener) {
    mLifecycleListener = listener;
  }

  /**
   * bind IModuleCreator to LynxModuleFactory. if creator is null, LynxModuleFactory will only use
   * to cache module class.
   */
  public void bind(IModuleCreator creator) {
    mModuleCreator = creator;
  }

  protected Map<String, ParamWrapper> getWrappers() {
    return mWrappers;
  }

  public void setLynxModuleExtraData(Object data) {
    mLynxModuleExtraData = data;
  }

  public void registerModule(
      @NonNull String name, @NonNull Class<? extends LynxModule> module, @Nullable Object param) {
    ParamWrapper wrapper = new ParamWrapper();
    wrapper.setName(name);
    wrapper.setModuleClass(module);
    wrapper.setParam(param);

    ParamWrapper oldWrapper = mWrappers.get(name);
    if (oldWrapper != null) {
      LLog.e(
          TAG, "Duplicated LynxModule For Name: " + name + ", " + oldWrapper + " will be override");
    }
    mWrappers.put(name, wrapper);
    LLog.v(TAG, "method registerModule , registered module with name: " + name + " class" + module);
  }

  public void addModuleParamWrapper(List<ParamWrapper> wrappers) {
    if (wrappers == null || wrappers.isEmpty()) {
      return;
    }
    for (ParamWrapper w : wrappers) {
      String name = w.getName();
      ParamWrapper oldWrapper = this.mWrappers.get(name);
      if (oldWrapper != null) {
        LLog.e(TAG,
            "Duplicated LynxModule For Name: " + name + ", " + oldWrapper + " will be override");
      }
      this.mWrappers.put(name, w);
      LLog.v(TAG,
          "method addModuleParamWrapper ,  registered module with name: " + name + " class" + w);
    }
  }

  /**
   * Only used in LynxBackgroundRuntime Standalone to create LynxView, we already register
   * some modules in RuntimeOptions and we don't want the modules on LynxViewBuilder overwrite it.
   * @param wrappers the module params to be registered.
   */
  public void addModuleParamWrapperIfAbsent(List<ParamWrapper> wrappers) {
    if (wrappers == null || wrappers.isEmpty()) {
      return;
    }
    for (ParamWrapper w : wrappers) {
      String name = w.getName();
      if (this.mWrappers.containsKey(name)) {
        LLog.w(TAG, "Duplicated LynxModule For Name: " + name + ", will be ignored");
      }
      this.mWrappers.putIfAbsent(name, w);
      LLog.v(TAG,
          "method addModuleParamWrapperIfAbsent , registered module with name: " + name + " class"
              + w);
    }
  }

  public void registerModuleAuthValidator(LynxModule.AuthValidator authValidator) {
    mAuthValidator = authValidator;
  }

  public IContextFinder currentContextFinder() {
    if (mModuleCreator == null) {
      return null;
    }
    return mModuleCreator.currentContextFinder();
  }

  public LynxModuleWrapper getModule(String name) {
    if (mModuleCreator == null) {
      return null;
    }
    LynxModuleWrapper moduleWrapper = mModuleCreator.create(name, mWrappers);
    if (moduleWrapper != null) {
      moduleWrapper.getModule().setExtraData(mLynxModuleExtraData);
      moduleWrapper.setAuthValidator(mAuthValidator);
    }
    return moduleWrapper;
  }

  public void retainJniObject() {
    if (!nativeRetainJniObject(mNativePtr)) {
      LLog.e("LynxModuleFactory", "LynxModuleFactory try to retainJniObject failed");
      destroy();
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public long getNativePtr() {
    return mNativePtr;
  }

  // Called by native code.
  @CalledByNative
  private LynxModuleWrapper moduleWrapperForName(String name) {
    LynxModuleWrapper module = getModule(name);
    return module;
  }

  @CalledByNative
  private void setNativePtr(long nativePtr) {
    mNativePtr = nativePtr;
  }

  @CalledByNative
  public void destroy() {
    if (mHasDestroyed) {
      return;
    }
    if (mLifecycleListener != null) {
      mLifecycleListener.onDestroy();
    }
    if (mModuleCreator != null) {
      mModuleCreator.destroy();
    }
    mWrappers.clear();
    mNativePtr = 0;
    mHasDestroyed = true;
  }

  private native boolean nativeRetainJniObject(long nativePtr);
}
