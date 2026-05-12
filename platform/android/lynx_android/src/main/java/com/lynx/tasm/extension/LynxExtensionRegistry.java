// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.extension;

import android.app.Application;
import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.jsbridge.LynxModule;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.service.IServiceProvider;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

@Keep
public final class LynxExtensionRegistry {
  private static final String TAG = "LynxExtensionRegistry";
  private static final Set<String> sRegisteredGlobalProviders =
      Collections.newSetFromMap(new ConcurrentHashMap<String, Boolean>());

  @Nullable private final Context mContext;
  @Nullable private final LynxViewBuilder mBuilder;

  private LynxExtensionRegistry(@Nullable Context context, @Nullable LynxViewBuilder builder) {
    mContext = context;
    mBuilder = builder;
  }

  public static void setupGlobal(@NonNull Context context, @NonNull String[] providerClassNames) {
    LynxExtensionRegistry registry = new LynxExtensionRegistry(context, null);
    setupGlobal(registry, providerClassNames);
    Context appContext = context.getApplicationContext();
    if (appContext instanceof Application) {
      LynxServiceCenter.inst().initialize((Application) appContext);
    }
  }

  public static void setup(@NonNull LynxViewBuilder builder, @NonNull String[] providerClassNames) {
    setup(new LynxExtensionRegistry(null, builder), providerClassNames);
  }

  public void addBehaviors(@Nullable List<Behavior> behaviors) {
    if (behaviors == null || behaviors.isEmpty()) {
      return;
    }
    if (mBuilder != null) {
      mBuilder.addBehaviors(behaviors);
    } else {
      LynxEnv.inst().addBehaviors(behaviors);
    }
  }

  public void registerModule(
      @NonNull String name, @NonNull Class<? extends LynxModule> moduleClass) {
    registerModule(name, moduleClass, null);
  }

  public void registerModule(@NonNull String name, @NonNull Class<? extends LynxModule> moduleClass,
      @Nullable Object param) {
    if (mBuilder != null) {
      mBuilder.registerModule(name, moduleClass, param);
    } else {
      LynxEnv.inst().registerModule(name, moduleClass, param);
    }
  }

  public void registerService(@NonNull Class<? extends IServiceProvider> serviceClass) {
    IServiceProvider service = createService(serviceClass);
    if (service != null) {
      LynxServiceCenter.inst().registerService(service);
    }
  }

  public void registerService(@NonNull IServiceProvider service) {
    LynxServiceCenter.inst().registerService(service);
  }

  private static void setup(
      @NonNull LynxExtensionRegistry registry, @NonNull String[] providerClassNames) {
    for (String providerClassName : providerClassNames) {
      if (providerClassName == null || providerClassName.length() == 0) {
        continue;
      }
      setupProvider(registry, providerClassName);
    }
  }

  private static void setupGlobal(
      @NonNull LynxExtensionRegistry registry, @NonNull String[] providerClassNames) {
    for (String providerClassName : providerClassNames) {
      if (providerClassName == null || providerClassName.length() == 0) {
        continue;
      }
      if (!sRegisteredGlobalProviders.add(providerClassName)) {
        continue;
      }
      if (!setupProvider(registry, providerClassName)) {
        sRegisteredGlobalProviders.remove(providerClassName);
      }
    }
  }

  private static boolean setupProvider(
      @NonNull LynxExtensionRegistry registry, @NonNull String providerClassName) {
    try {
      Class<?> cls = Class.forName(providerClassName);
      Constructor<?> constructor = cls.getDeclaredConstructor();
      constructor.setAccessible(true);
      Object instance = constructor.newInstance();
      if (instance instanceof LynxExtensionProvider) {
        ((LynxExtensionProvider) instance).register(registry);
        return true;
      } else {
        LLog.e(TAG, providerClassName + " does not implement LynxExtensionProvider");
      }
    } catch (Throwable t) {
      LLog.w(TAG, "Skip unavailable Lynx extension provider " + providerClassName + ": " + t);
    }
    return false;
  }

  @Nullable
  private static IServiceProvider createService(
      @NonNull Class<? extends IServiceProvider> serviceClass) {
    try {
      Field instanceField = serviceClass.getField("INSTANCE");
      Object instance = instanceField.get(null);
      if (instance instanceof IServiceProvider) {
        return (IServiceProvider) instance;
      }
    } catch (NoSuchFieldException ignored) {
      // Java services usually expose a public no-arg constructor instead of Kotlin INSTANCE.
    } catch (Throwable t) {
      LLog.w(TAG, "Failed to read service singleton " + serviceClass.getName() + ": " + t);
    }

    try {
      Constructor<? extends IServiceProvider> constructor = serviceClass.getDeclaredConstructor();
      constructor.setAccessible(true);
      return constructor.newInstance();
    } catch (Throwable t) {
      LLog.e(TAG, "Failed to create service " + serviceClass.getName() + ": " + t);
      return null;
    }
  }

  @NonNull
  public Context getContextOrThrow() {
    if (mContext == null) {
      throw new IllegalStateException("Context is only available in setupGlobal().");
    }
    return mContext;
  }
}
