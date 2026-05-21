// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.app.Application;
import android.util.Log;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * The manager of [IServiceProvider] instants.
 */
public class LynxServiceCenter {
  static final class ServiceHolder {
    final IServiceProvider service;
    boolean initialized;

    ServiceHolder(IServiceProvider service) {
      this(service, false);
    }

    ServiceHolder(IServiceProvider service, boolean initialized) {
      this.service = service;
      this.initialized = initialized;
    }
  }

  private static final String TAG = "LynxServiceCenter";
  private static volatile LynxServiceCenter instance;

  private Application context;
  private final Map<Class<?>, ServiceHolder> serviceMap = new ConcurrentHashMap<>();

  private LynxServiceCenter() {}

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

  public <T extends IServiceProvider> T getService(Class<T> clazz) {
    ServiceHolder holder = serviceMap.get(clazz);
    if (holder == null) {
      return null;
    }
    if (!holder.initialized) {
      Log.w(TAG, "Service " + clazz + " hasn't been initialized");
      return null;
    }
    @SuppressWarnings("unchecked") T result = (T) holder.service;
    return result;
  }

  public boolean hasService(Class<? extends IServiceProvider> clazz) {
    return serviceMap.containsKey(clazz);
  }

  /**
   * use `void registerService(@NonNull IServiceProvider instance)` instead
   */
  @Deprecated
  public <T extends IServiceProvider> void registerService(Class<? extends T> clazz, T instance) {
    if (!clazz.isInstance(instance)) {
      Log.e(TAG, "Incorrect service type: " + clazz.getSimpleName());
      return;
    }
    ServiceHolder holder = new ServiceHolder(instance);
    if (context != null) {
      /* context provided means initialize() has been invoked */
      holder.initialized = true;
      holder.service.onInitialize(context);
    }
    serviceMap.put(clazz, holder);
  }

  public <T extends IServiceProvider> void registerService(T instance) {
    registerService(instance.getServiceClass(), instance);
  }

  public void unregisterService(Class<? extends IServiceProvider> clazz) {
    serviceMap.remove(clazz);
  }

  public void unregisterAllService() {
    serviceMap.clear();
  }

  /**
   * Triggers all services' IServiceProvider.onInitialize
   * <p>
   * This method can be invoked multiple times, while each service's
   * IServiceProvider.onInitialize will be guaranteed by LynxServiceCenter
   * to be invoked only once per service. Therefore, the frameworks like
   * lynx can safely call this method as needed.
   */
  public void initialize(Application context) {
    this.context = context;
    synchronized (ServiceHolder.class) {
      for (Map.Entry<Class<?>, ServiceHolder> entry : serviceMap.entrySet()) {
        ServiceHolder holder = entry.getValue();
        if (!holder.initialized) {
          holder.initialized = true;
          holder.service.onInitialize(context);
        }
      }
    }
  }
}
