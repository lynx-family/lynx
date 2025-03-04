// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.eventreport;

import androidx.annotation.AnyThread;
import androidx.annotation.Keep;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.service.ILynxEventReporterService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

@Keep
public class LynxEventReporter {
  private static final String TAG = "LynxEventReporter";
  // Lynx SDK's Version, set by LynxEnv.
  public final static String PROP_NAME_LYNX_SDK_VERSION = "lynx_sdk_version";
  // The last thread strategy this lynxView is using, will be updated when the lynxView is init.
  public final static String PROP_NAME_THREAD_MODE = "thread_mode";
  // GeneralInfo props name:
  // The last loaded URL in this lynxView, will be updated when lynxView render new template.
  public final static String PROP_NAME_URL = "url";
  // the relative_path would be equivalent to the url to remove applicationExternalCacheDir,
  // applicationFilesDir and LocalDir.
  // It can be more accurate to filter info by relative_path than by url on the tea platform.
  public final static String PROP_NAME_RELATIVE_PATH = "relative_path";
  // Enable SSR.
  public final static String PROP_NAME_ENABLE_SSR = "enable_ssr";
  private final static String UPDATE_GENERIC_INFO = "LynxEventReporter::updateGenericInfo";
  private final static String REMOVE_GENERIC_INFO = "LynxEventReporter::removeGenericInfo";
  private final static String ON_EVENT = "LynxEventReporter::OnEvent";
  private final static String HANDLE_EVENT = "LynxEventReporter::handleEvent";
  /**
   * Universal parameters generated internally by Lynx and can be used for all events.
   *
   * Note: Must be accessed on report thread.
   */
  private HashMap<Integer, HashMap<String, Object>> mAllGenericInfos;
  /**
   * allExtraParams is used to store all the extended parameters related to LynxView.These extended
   * parameters are set through LynxView.setExtraParams:forKey.The storage structure uses the
   * address of LynxView as the key and ExtraParams as the value.
   *
   *  Note: Must be accessed on report thread.
   */
  private HashMap<Integer, HashMap<String, Object>> mAllExtraParams;
  /**
   * All observers of event report.
   */
  private final CopyOnWriteArrayList<ILynxEventReportObserver> mObserverList =
      new CopyOnWriteArrayList<>();

  // Generated in the LynxShell, id of template instance.
  // mInstanceId is a value greater than or equal to 0, the initial value is -1.
  public static final int INSTANCE_ID_UNKNOWN = -1;
  // Event name of lynx erorr.
  public static final String LYNX_SDK_ERROR_EVENT = "lynxsdk_error_event";

  private volatile boolean mIsNativeLibraryLoaded = false;
  /**
   * Builder for event properties.
   */
  public interface PropsBuilder {
    /**
     * Build event properties on report thread.
     * @return event props.
     */
    public Map<String, Object> build();
  }

  @AnyThread
  public static LynxEventReporter getInstance() {
    return LynxEventReporterSingletonHolder.sInstance;
  }

  /**
   * Report event with instance id.
   * Can run on any thread.
   * @param eventName name of event.
   * @param props properties of event.
   * @param instanceId id of template instance.
   */
  @AnyThread
  public static void onEvent(
      final String eventName, final Map<String, Object> props, final int instanceId) {
    if (eventName == null || (instanceId < 0 && props == null)) {
      // If instance id is unknown, props of event must be not null.
      return;
    }
    String sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = ON_EVENT;
      Map<String, String> args = new HashMap<>();
      args.put("eventName", eventName);
      args.put("instanceId", instanceId + "");
      TraceEvent.beginSection(sectionName, args);
    }
    runOnReportThread(
        () -> { LynxEventReporter.getInstance().handleEvent(instanceId, eventName, props); });
    TraceEvent.endSection(sectionName);
  }

  /**
   * Report event with instance id.
   * Can run on any thread.
   * @param eventName name of event.
   * @param instanceId id of template instance.
   * @param builder properties builder of event, call builder.build() on report thread.
   */
  @AnyThread
  public static void onEvent(
      final String eventName, final int instanceId, final PropsBuilder builder) {
    if (eventName == null || (instanceId < 0 && builder == null)) {
      // If instance id is unknown, props of event must be not null.
      return;
    }
    String sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = ON_EVENT;
      Map<String, String> args = new HashMap<>();
      args.put("eventName", eventName);
      args.put("instanceId", instanceId + "");
      TraceEvent.beginSection(sectionName, args);
    }
    runOnReportThread(() -> {
      Map<String, Object> props = null;
      if (builder != null) {
        props = builder.build();
      }
      if (instanceId < 0 && props == null) {
        // If instance id is unknown, props of event must be not null.
        return;
      }
      LynxEventReporter.getInstance().handleEvent(instanceId, eventName, props);
    });
    TraceEvent.endSection(sectionName);
  }

  /**
   *Update generic info of template instance.
   * Can run on any thread.
   * @param key  key of generic info.
   * @param value value of generic info.
   * @param instanceId id of template instance.
   */
  @AnyThread
  public static void updateGenericInfo(final String key, Object value, final int instanceId) {
    if (key == null || value == null || instanceId < 0) {
      return;
    }
    final String sectionName = UPDATE_GENERIC_INFO;
    if (TraceEvent.enableTrace()) {
      Map<String, String> args = new HashMap<>();
      args.put("key", key);
      args.put("instanceId", instanceId + "");
      args.put("value", value.toString());
      TraceEvent.beginSection(sectionName, args);
    }
    runOnReportThread(() -> {
      TraceEvent.beginSection(sectionName);
      getGenericInfo(instanceId).put(key, value);
      TraceEvent.endSection(sectionName);
    });
    TraceEvent.endSection(sectionName);
  }

  /**
   * Remove generic info of template instance by id.
   * @param instanceId id of template instance.
   */
  @AnyThread
  public static void removeGenericInfo(final int instanceId) {
    if (instanceId < 0) {
      return;
    }
    final String sectionName = REMOVE_GENERIC_INFO;
    if (TraceEvent.enableTrace()) {
      Map<String, String> args = new HashMap<>();
      args.put("instanceId", instanceId + "");
      TraceEvent.beginSection(sectionName, args);
    }

    runOnReportThread(() -> {
      TraceEvent.beginSection(sectionName);
      LynxEventReporter.getInstance().mAllGenericInfos.remove(instanceId);
      TraceEvent.endSection(sectionName);
    });
    TraceEvent.endSection(sectionName);
  }

  /**
   * Put extra parameters for reporting events, overriding old values if the parameters already
   * exist.
   * @param params common parameters for report events, these values will be merged with params of
   *     EventReporter.onEvent.
   * @param instanceId id of template instance.
   */
  @AnyThread
  public static void putExtraParams(final Map<String, Object> params, final int instanceId) {
    if (params == null || instanceId < 0) {
      return;
    }
    runOnReportThread(() -> {
      LynxEventReporter reporter = LynxEventReporter.getInstance();
      HashMap<String, Object> extraParams = reporter.mAllExtraParams.get(instanceId);
      if (extraParams == null) {
        reporter.mAllExtraParams.put(instanceId, new HashMap<>(params));
      } else {
        extraParams.putAll(params);
      }
    });
  }

  /**
   * Move extra parameters for reporting events from originInstance to targetInstanceId.
   */
  @AnyThread
  public static void moveExtraParams(final int originInstanceId, final int targetInstanceId) {
    if (originInstanceId < 0 || targetInstanceId < 0) {
      return;
    }
    runOnReportThread(() -> {
      LynxEventReporter reporter = LynxEventReporter.getInstance();
      HashMap<String, Object> extraParams = reporter.mAllExtraParams.get(originInstanceId);
      if (extraParams != null) {
        HashMap<String, Object> targetExtraParams = reporter.mAllExtraParams.get(targetInstanceId);
        if (targetExtraParams != null) {
          targetExtraParams.putAll(extraParams);
        } else {
          reporter.mAllExtraParams.put(targetInstanceId, extraParams);
        }
        reporter.mAllExtraParams.remove(originInstanceId);
      }
    });
  }

  /**
   * Move extra parameters for reporting events from originInstance to targetInstanceId.
   */
  @AnyThread
  public static void clearCache(final int instanceId) {
    if (instanceId < 0) {
      return;
    }
    runOnReportThread(() -> {
      LynxEventReporter reporter = LynxEventReporter.getInstance();
      reporter.mAllGenericInfos.remove(instanceId);
      reporter.mAllExtraParams.remove(instanceId);
    });
  }

  @AnyThread
  public static void addObserver(ILynxEventReportObserver observer) {
    LynxEventReporter.getInstance().addObserverInternal(observer);
  }

  private void addObserverInternal(ILynxEventReportObserver observer) {
    if (observer != null && !mObserverList.contains(observer)) {
      mObserverList.add(observer);
    }
  }

  @AnyThread
  public static void removeObserver(ILynxEventReportObserver observer) {
    LynxEventReporter.getInstance().removeObserverInternal(observer);
  }

  private void removeObserverInternal(ILynxEventReportObserver observer) {
    if (observer != null) {
      mObserverList.remove(observer);
    }
  }

  private void handleEvent(int instanceId, String eventName, Map<String, Object> props) {
    String sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = HANDLE_EVENT;
      Map<String, String> args = new HashMap<>();
      args.put("instanceId", instanceId + "");
      args.put("eventName", eventName);
      TraceEvent.beginSection(sectionName, args);
    }
    ILynxEventReporterService eventReporterService =
        LynxServiceCenter.inst().getService(ILynxEventReporterService.class);
    if (eventName == null) {
      LLog.e(TAG, "event name is null.");
      TraceEvent.endSection(sectionName);
      return;
    }
    HashMap<String, Object> propsData = new HashMap<>();
    HashMap<String, Object> genericInfo = mAllGenericInfos.get(instanceId);
    if (genericInfo != null) {
      propsData.putAll(genericInfo);
    }
    if (props != null) {
      propsData.putAll(props);
    }
    Map<String, Object> unmodifiableProps = Collections.unmodifiableMap(propsData);
    Map<String, Object> unmodifiableExtraData = null;
    HashMap<String, Object> extraParams = mAllExtraParams.get(instanceId);
    if (extraParams != null) {
      unmodifiableExtraData = Collections.unmodifiableMap(extraParams);
    }
    for (ILynxEventReportObserver observer : mObserverList) {
      observer.onReportEvent(eventName, instanceId, unmodifiableProps, unmodifiableExtraData);
    }
    TraceEvent.endSection(sectionName);
  }

  private static HashMap<String, Object> getGenericInfo(Integer instanceId) {
    HashMap<String, Object> genericInfo =
        LynxEventReporter.getInstance().mAllGenericInfos.get(instanceId);
    if (genericInfo == null) {
      genericInfo = new HashMap<>();
      genericInfo.put(PROP_NAME_LYNX_SDK_VERSION, LynxEnv.inst().getLynxVersion());
      LynxEventReporter.getInstance().mAllGenericInfos.put(instanceId, genericInfo);
    }
    return genericInfo;
  }

  /**
   * Execute runnable on reporter thread
   * @param runnable runnable to be executed
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  @AnyThread
  public static void runOnReportThread(Runnable runnable) {
    if (!LynxEventReporter.getInstance().mIsNativeLibraryLoaded) {
      LynxEventReporter.getInstance().mIsNativeLibraryLoaded =
          LynxEnv.inst().isNativeLibraryLoaded();
    }
    if (LynxEventReporter.getInstance().mIsNativeLibraryLoaded) {
      LynxEventReporter.getInstance().nativeRunOnReportThread(runnable);
    }
  }

  private static class LynxEventReporterSingletonHolder {
    private static final LynxEventReporter sInstance = new LynxEventReporter();
  }

  private LynxEventReporter() {
    mAllGenericInfos = new HashMap<>();
    mAllExtraParams = new HashMap<>();
    ILynxEventReporterService eventReporterService =
        LynxServiceCenter.inst().getService(ILynxEventReporterService.class);
    if (eventReporterService == null) {
      LLog.e(TAG, "eventReporter service not found or event name is null.");
    } else {
      addObserverInternal(eventReporterService);
    }
  }

  /**
   * Received native events.
   * The method can only be called by native on the report thread (C++).
   * @param instanceId id of template instance.
   * @param eventName name of event.
   * @param props props of event.
   */
  @CalledByNative
  protected static void onEvent(int instanceId, String eventName, ReadableMap props) {
    String sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = ON_EVENT;
      Map<String, String> args = new HashMap<>();
      args.put("instanceId", instanceId + "");
      args.put("eventName", eventName);
      TraceEvent.beginSection(sectionName, args);
    }
    LynxEventReporter.getInstance().handleEvent(instanceId, eventName, props.asHashMap());
    TraceEvent.endSection(sectionName);
  }

  /**
   * Native update generic info of template instance by id.
   * The method can only be called by native on the report thread (C++).
   * @param instanceId id of template instance.
   * @param props props of generic info.
   */
  @CalledByNative
  protected static void updateGenericInfo(int instanceId, ReadableMap props) {
    String sectionName = "";
    if (TraceEvent.enableTrace()) {
      sectionName = UPDATE_GENERIC_INFO;
      Map<String, String> args = new HashMap<>();
      args.put("instanceId", instanceId + "");
      TraceEvent.beginSection(sectionName, args);
    }
    getGenericInfo(instanceId).putAll(props.asHashMap());
    TraceEvent.endSection(sectionName);
  }

  /**
   * Execute runnable by native on the report thread.
   * The method can only be called by native on the report thread (C++).
   */
  @CalledByNative
  protected static void callRunnable(Object runnable) {
    if (runnable instanceof Runnable) {
      ((Runnable) runnable).run();
    } else {
      LLog.e(TAG,
          "runnable[" + (runnable == null ? "null" : runnable.getClass())
              + "] must be an instance of Runnable.");
    }
  }

  /**
   * Post runnable to native report thread.
   */
  private native void nativeRunOnReportThread(Object runnable);
}
