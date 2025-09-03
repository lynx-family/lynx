// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.base.service;

import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.concurrent.ConcurrentHashMap;

public class LynxBaseServiceCenter {
  private static final String TAG = "LynxBaseServiceCenter";

  private final ConcurrentHashMap<Class, IBaseServiceProvider> mServiceMap =
      new ConcurrentHashMap<>();

  private static volatile LynxBaseServiceCenter instance = null;

  public static LynxBaseServiceCenter inst() {
    if (instance == null) {
      synchronized (LynxBaseServiceCenter.class) {
        if (instance == null) {
          instance = new LynxBaseServiceCenter();
        }
      }
    }
    return instance;
  }

  @Nullable
  public <T extends IBaseServiceProvider> T getService(@NonNull Class<T> clazz) {
    T service = (T) mServiceMap.get(clazz);
    if (service == null) {
      Log.i(TAG, clazz.getSimpleName() + " is unregistered");
    }
    return service;
  }

  /**
   * register service regardless of whether the original service instance exists
   */
  public void registerService(@NonNull IBaseServiceProvider instance) {
    Class<? extends IBaseServiceProvider> clazz = instance.getServiceClass();
    if (!clazz.isInstance(instance)) {
      Log.e(TAG, "Incorrect service type: " + clazz.getSimpleName());
      return;
    }
    IBaseServiceProvider service = mServiceMap.put(clazz, instance);
    if (service != null) {
      Log.w(TAG, service.getServiceClass().getSimpleName() + " is already registered");
    }
  }

  public void unregisterService(Class<? extends IBaseServiceProvider> clazz) {
    mServiceMap.remove(clazz);
  }

  public void unregisterAllService() {
    mServiceMap.clear();
  }
}
