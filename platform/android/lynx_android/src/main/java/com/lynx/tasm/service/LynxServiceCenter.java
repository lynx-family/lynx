// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.TraceEvent;
import java.util.ServiceLoader;
import java.util.concurrent.ConcurrentHashMap;

public class LynxServiceCenter extends LynxLazyInitializer {
  private static final String TAG = "LynxServiceCenter";
  private static final String TRACE_INIT_MESSAGE = "LynxServiceCenter.doInitialize";

  private final ConcurrentHashMap<Class, IServiceProvider> mServiceMap = new ConcurrentHashMap<>();

  private static volatile LynxServiceCenter instance = null;

  public static LynxServiceCenter inst() {
    if (instance == null) {
      synchronized (LynxServiceCenter.class) {
        if (instance == null) {
          instance = new LynxServiceCenter();
        }
      }
    }
    return instance;
  }

  private LynxServiceCenter() {
    initialize();
  }

  @Override
  protected boolean doInitialize() {
    try {
      TraceEvent.beginSection(TRACE_INIT_MESSAGE);
      ServiceLoader<IServiceProvider> loader = ServiceLoader.load(IServiceProvider.class);
      for (IServiceProvider serviceProvider : loader) {
        // auto-registered service will not replace the existing ones which is registered manually
        registerService(serviceProvider, false);
      }
    } catch (Throwable e) {
      Log.e(TAG, "failed to register services", e);
    } finally {
      TraceEvent.endSection(TRACE_INIT_MESSAGE);
    }
    return true;
  }

  @Nullable
  public <T extends IServiceProvider> T getService(@NonNull Class<T> clazz) {
    ensureInitialized();
    T service = (T) mServiceMap.get(clazz);
    if (service == null) {
      Log.i(TAG, clazz.getSimpleName() + " is unregistered");
    }
    return service;
  }

  /**
   * register service regardless of whether the original service instance exists
   */
  public void registerService(@NonNull IServiceProvider instance) {
    registerService(instance, true);
  }

  /**
   * use `void registerService(@NonNull IServiceProvider instance)` instead
   */
  @Deprecated
  public void registerService(
      Class<? extends IServiceProvider> clazz, @NonNull IServiceProvider instance) {
    registerService(instance);
  }

  /**
   * @param instance service instance
   * @param force whether force to replace the existing service
   */
  private void registerService(@NonNull IServiceProvider instance, boolean force) {
    Class<? extends IServiceProvider> clazz = instance.getServiceClass();
    if (!clazz.isInstance(instance)) {
      Log.e(TAG, "Incorrect service type: " + clazz.getSimpleName());
      return;
    }
    IServiceProvider service =
        force ? mServiceMap.put(clazz, instance) : mServiceMap.putIfAbsent(clazz, instance);
    if (service != null) {
      Log.w(TAG, service.getServiceClass().getSimpleName() + " is already registered");
    }
  }

  public void unregisterService(Class<? extends IServiceProvider> clazz) {
    mServiceMap.remove(clazz);
  }

  public void unregisterAllService() {
    mServiceMap.clear();
  }
}
