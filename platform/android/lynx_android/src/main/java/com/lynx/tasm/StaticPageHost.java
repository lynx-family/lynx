// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.CallStackUtil;
import java.lang.ref.WeakReference;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;

/** Connects one Lynx page to its statically compiled page instance. */
public final class StaticPageHost {
  private static final String TAG = "StaticPageHost";
  private static final ConcurrentHashMap<Integer, WeakReference<StaticPageHost>> HOSTS =
      new ConcurrentHashMap<>();

  private final Object mLock = new Object();
  private final Executor mOwnerExecutor;
  private int mInstanceId = -1;
  private boolean mRegistered;
  private long mGeneration;
  @NonNull private Map<String, Object> mCurrentData = Collections.emptyMap();
  @Nullable private Map<String, Object> mCurrentGlobalProps;
  @Nullable private StaticPageInstance mPageInstance;

  StaticPageHost(@NonNull Executor ownerExecutor) {
    mOwnerExecutor = ownerExecutor;
  }

  /**
   * Attaches the generated page instance to the Lynx page with {@code instanceId}.
   *
   * @return whether the instance was attached.
   */
  public static boolean attach(int instanceId, @NonNull StaticPageInstance instance) {
    StaticPageHost host = find(instanceId);
    return host != null && host.attachPageInstance(instance);
  }

  @Nullable
  Map<String, Object> register(int instanceId, @Nullable TemplateData data,
      @Nullable TemplateData currentGlobalProps, @Nullable TemplateData groupGlobalProps,
      @Nullable TemplateData loadGlobalProps) {
    Map<String, Object> mergedGlobalProps =
        mergeGlobalProps(currentGlobalProps, groupGlobalProps, loadGlobalProps);
    synchronized (mLock) {
      unregisterLocked();
      mGeneration++;
      Map<String, Object> platformData = getTemplateDataMap(data);
      if (platformData != null) {
        if (mCurrentData.isEmpty()) {
          mCurrentData = platformData;
        } else {
          HashMap<String, Object> merged = new HashMap<>(platformData);
          merged.putAll(mCurrentData);
          mCurrentData = merged;
        }
      }
      if (mergedGlobalProps != null) {
        if (mCurrentGlobalProps == null) {
          mCurrentGlobalProps = mergedGlobalProps;
        } else {
          // Metadata set before load takes precedence over load-time global props.
          HashMap<String, Object> merged = new HashMap<>(mergedGlobalProps);
          merged.putAll(mCurrentGlobalProps);
          mCurrentGlobalProps = merged;
        }
      }
      mInstanceId = instanceId;
      mPageInstance = null;
      mRegistered = instanceId >= 0;
      if (mRegistered) {
        HOSTS.put(instanceId, new WeakReference<>(this));
      }
      return mCurrentGlobalProps;
    }
  }

  boolean isRegistered() {
    synchronized (mLock) {
      return mRegistered;
    }
  }

  @Nullable
  Map<String, Object> updateMetaData(
      @Nullable TemplateData data, @Nullable TemplateData globalPropsUpdate) {
    final StaticPageInstance instance;
    final long generation;
    Map<String, Object> mergedData = null;
    Map<String, Object> mergedGlobalProps = null;
    synchronized (mLock) {
      Map<String, Object> platformData = getTemplateDataMap(data);
      if (platformData != null) {
        if (mCurrentData.isEmpty()) {
          mCurrentData = platformData;
        } else {
          HashMap<String, Object> merged = new HashMap<>(mCurrentData);
          merged.putAll(platformData);
          mCurrentData = merged;
        }
        mergedData = mCurrentData;
      }
      Map<String, Object> globalPropsUpdateData = getTemplateDataMap(globalPropsUpdate);
      if (globalPropsUpdateData != null) {
        if (mCurrentGlobalProps == null) {
          mCurrentGlobalProps = globalPropsUpdateData;
        } else {
          HashMap<String, Object> merged = new HashMap<>(mCurrentGlobalProps);
          merged.putAll(globalPropsUpdateData);
          mCurrentGlobalProps = merged;
        }
        mergedGlobalProps = mCurrentGlobalProps;
      }
      instance = mRegistered ? mPageInstance : null;
      generation = mGeneration;
    }
    if (instance != null) {
      final Map<String, Object> dataSnapshot = mergedData;
      final Map<String, Object> globalPropsSnapshot = mergedGlobalProps;
      dispatch("updateMetaData", () -> {
        if (isCurrentPageInstance(instance, generation)) {
          instance.updateMetaData(dataSnapshot, globalPropsSnapshot);
        }
      });
    }
    return mergedGlobalProps;
  }

  static boolean isStaticPageDataOrNull(@Nullable TemplateData data) {
    return data == null || TemplateData.isForStaticPage(data);
  }

  void clear() {
    final StaticPageInstance instance;
    synchronized (mLock) {
      unregisterLocked();
      mGeneration++;
      mRegistered = false;
      mInstanceId = -1;
      instance = mPageInstance;
      mPageInstance = null;
      mCurrentData = Collections.emptyMap();
      mCurrentGlobalProps = null;
    }
    if (instance != null) {
      dispatch("destroy", instance::destroy);
    }
  }

  @CalledByNative
  private static boolean renderPageForNative(int instanceId) {
    StaticPageHost host = find(instanceId);
    return host != null && host.renderPage();
  }

  @Nullable
  private static StaticPageHost find(int instanceId) {
    WeakReference<StaticPageHost> reference = HOSTS.get(instanceId);
    if (reference == null) {
      return null;
    }
    StaticPageHost host = reference.get();
    if (host == null) {
      HOSTS.remove(instanceId, reference);
    }
    return host;
  }

  private boolean attachPageInstance(@NonNull StaticPageInstance instance) {
    synchronized (mLock) {
      if (!mRegistered || mPageInstance != null) {
        return false;
      }
      mPageInstance = instance;
      return true;
    }
  }

  private boolean isCurrentPageInstance(@NonNull StaticPageInstance instance, long generation) {
    synchronized (mLock) {
      return mRegistered && mPageInstance == instance && mGeneration == generation;
    }
  }

  private boolean renderPage() {
    final StaticPageInstance instance;
    final Map<String, Object> data;
    final Map<String, Object> globalProps;
    synchronized (mLock) {
      if (!mRegistered || mPageInstance == null) {
        return false;
      }
      instance = mPageInstance;
      data = mCurrentData;
      globalProps = mCurrentGlobalProps;
    }
    try {
      instance.renderPage(data, globalProps);
      return true;
    } catch (RuntimeException error) {
      logFailure("renderPage", error);
      return false;
    }
  }

  private void dispatch(String operation, Runnable task) {
    try {
      mOwnerExecutor.execute(() -> {
        try {
          task.run();
        } catch (RuntimeException error) {
          logFailure(operation, error);
        }
      });
    } catch (RuntimeException error) {
      logFailure(operation + " dispatch", error);
    }
  }

  private void unregisterLocked() {
    if (mInstanceId < 0) {
      return;
    }
    WeakReference<StaticPageHost> reference = HOSTS.get(mInstanceId);
    if (reference != null && reference.get() == this) {
      HOSTS.remove(mInstanceId, reference);
    }
  }

  @Nullable
  private static Map<String, Object> mergeGlobalProps(@Nullable TemplateData currentGlobalProps,
      @Nullable TemplateData groupGlobalProps, @Nullable TemplateData loadGlobalProps) {
    Map<String, Object> mergedGlobalProps = null;
    TemplateData[] sources = {currentGlobalProps, groupGlobalProps, loadGlobalProps};
    for (TemplateData source : sources) {
      Map<String, Object> sourceData = getTemplateDataMap(source);
      if (sourceData == null) {
        continue;
      }
      if (mergedGlobalProps == null) {
        mergedGlobalProps = sourceData;
      } else {
        HashMap<String, Object> merged = new HashMap<>(mergedGlobalProps);
        merged.putAll(sourceData);
        mergedGlobalProps = merged;
      }
    }
    return mergedGlobalProps;
  }

  @Nullable
  @SuppressWarnings("unchecked")
  private static Map<String, Object> getTemplateDataMap(@Nullable TemplateData data) {
    // Static-page data returns its retained platform map. Standard TemplateData is accepted only
    // for load-time global props and is materialized here at the direct-load boundary.
    return data == null ? null : (Map<String, Object>) (Map<?, ?>) data.toMap();
  }

  private static void logFailure(String operation, Throwable error) {
    LLog.e(TAG,
        "Static page direct " + operation + " failed:\n"
            + CallStackUtil.getStackTraceStringTrimmed(error));
  }
}
