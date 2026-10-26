// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.core.resource.LynxResourceLoader;
import java.nio.ByteBuffer;
import java.util.Map;

/** JNI bindings between the Android renderer and the native engine. */
final class LynxJNI {
  private LynxJNI() {}

  static native long nativeGenerateViewId();

  static native long nativeCreate(long viewId, long runtimePtr, Object nativeFacade,
      Object performanceController, Object loader, int renderStrategy,
      boolean enableLayoutSafePoint, boolean enableLayoutOnly, int screenWidth, int screenHeight,
      float density, String locale, boolean enableJSRuntime, boolean enableMultiAsyncThread,
      boolean enablePreUpdateData, boolean enableVSyncAlignedMessageLoop,
      boolean enableAsyncHydration, boolean enableJSGroupThread, String jsGroupThreadName,
      Object tasmPlatformInvoker, long whiteboard, long uiDelegate, boolean useInvokeUIMethod,
      boolean longTaskMonitorDisabled, boolean forceLayoutOnBackgroundThread,
      boolean enableUnifiedPipeline, int embeddedMode, boolean has_logic_executor,
      boolean debuggable, long enginePtr, Object moduleFactory);

  static native void nativeRebuildLynxEngine(
      long ptr, long lifecycle, long uiDelegate, Object tasmPlatformInvoker, Object moduleFactory);

  static native void nativeDestroy(long ptr);

  static native long nativeLifecycleCreate();

  static native boolean nativeLifecycleTryTerminate(long lifecycle);

  static native void nativeLifecycleDestroy(long lifecycle);

  static native int nativeGetInstanceId(long ptr, long lifecycle);

  static native void nativeAttachRuntime(long ptr, long lifecycle, long backgroundRuntime);

  static native void nativeInitRuntime(long ptr, LynxResourceLoader resourceLoader,
      LynxModuleFactory moduleFactory, String groupId, String monitoringGroupLabel,
      String[] preloadJSPaths, String bytecodeSourceUrl, int runtimeFlags, long uiDelegate);

  static native void nativeOnLynxEngineCreated(long ptr, long uiDelegatePtr);

  static native byte[] nativeGetLynxUITree(long uiDelegatePtr);

  static native byte[] nativeGetUINodeInfo(long uiDelegatePtr, int id);

  static native int nativeSetUIStyle(long uiDelegatePtr, int id, String name, String content);

  static native void nativeStartRuntime(long ptr, long lifecycle);

  static native void nativeProcessRender(long ptr, long lifecycle);

  static native void nativeSetEnableUIFlush(long ptr, long lifecycle, boolean enableUIFlush);

  static native void nativeOnEnterForeground(long ptr, long lifecycle);

  static native void nativeOnEnterBackground(long ptr, long lifecycle);

  static native void nativePause(long ptr, long lifecycle);

  static native void nativeResume(long ptr, long lifecycle);

  // load template && component
  // FIXME(songshourui.null): only use templateData later
  static native void nativeLoadSSRDataByPreParsedData(long ptr, long lifecycle, String url,
      byte[] temp, long data, boolean readOnly, String processorName, TemplateData templateData);

  // FIXME(songshourui.null): only use templateData later
  static native void nativeLoadTemplateByPreParsedData(long ptr, long lifecycle, String url,
      byte[] temp, boolean isPrePainting, boolean enableRecycleTemplateBundle, long data,
      boolean readOnly, String processorName, TemplateData templateData, int options,
      ReadableMap timingOption);

  static native void nativeLoadLynxML(long ptr, long lifecycle, String url, byte[] source,
      long data, boolean readOnly, String processorName, TemplateData templateData,
      ReadableMap timingOption);

  // FIXME(songshourui.null): only use templateData later
  static native void nativeLoadTemplateBundleByPreParsedData(long ptr, long lifecycle, String url,
      long bundlePtr, boolean isPrePainting, long data, boolean readOnly, String processorName,
      TemplateData templateData, int options, ReadableMap timingOption);

  static native void nativeLoadTemplateBufferByPreParsedData(long ptr, long lifecycle, String url,
      ByteBuffer temp, boolean isPrePainting, boolean enableRecycleTemplateBundle, long data,
      boolean readOnly, String processorName, TemplateData templateData, int options,
      ReadableMap timingOption);

  static native void nativePreloadLazyBundles(long ptr, long lifecycle, String[] urls);

  static native boolean nativeRegisterLazyBundle(
      long ptr, long lifecycle, String url, long bundlePtr);

  // update data
  // FIXME(songshourui.null): only use templateData later
  static native void nativeUpdateDataByPreParsedData(long ptr, long lifecycle, long dataPtr,
      String processorName, boolean readOnly, TemplateData templateData, boolean is_reuse_engine);

  static native void nativeUpdateMetaData(long ptr, long lifecycle, long dataPtr,
      String processorName, boolean readOnly, TemplateData templateData, long globalPropsPtr);

  // FIXME(songshourui.null): only use templateData later
  static native void nativeResetDataByPreParsedData(long ptr, long lifecycle, long dataPtr,
      String processorName, boolean readOnly, TemplateData templateData);

  // FIXME(songshourui.null): only use templateData later
  static native void nativeReloadTemplate(long ptr, long lifecycle, long dataPtr, long propPtr,
      String dataProcessorName, boolean dataReadOnly, Object globalProps, TemplateData templateData,
      ReadableMap timingOption);

  static native void nativeUpdateConfig(long ptr, long lifecycle, ByteBuffer buffer, int length);

  static native void nativeUpdateGlobalProps(long ptr, long lifecycle, long data);

  static native void nativeUpdateScreenMetrics(
      long ptr, long lifecycle, int width, int height, float scale, long uiDelegate);

  static native void nativeSetFontScale(long ptr, long lifecycle, float scale);

  static native void nativeSetPlatformConfig(long ptr, long lifecycle, String jsonString);

  static native void nativeUpdateFontScale(long ptr, long lifecycle, float scale);

  static native void nativeUpdateColorScheme(
      long ptr, long lifecycle, int scheme, boolean useActLite);

  // layout
  static native void nativeUpdateViewport(long ptr, long lifecycle, int width, int widthMode,
      int height, int heightMode, float scale, long uiDelegate, boolean needLayout);

  static native void nativeSyncFetchLayoutResult(long ptr, long lifecycle);

  static native void nativeEnforceRelayoutOnCurrentThreadWithUpdatedViewport(
      long ptr, long lifecycle, int width, int widthMode, int height, int heightMode);

  static native void nativeSendGlobalEventToLepus(
      long ptr, long lifecycle, String name, ByteBuffer buffer, int length);

  static native void nativeSendSsrGlobalEvent(
      long ptr, long lifecycle, String name, ByteBuffer buffer, int length);

  static native void nativeTriggerEventBus(
      long ptr, long lifecycle, String name, ByteBuffer buffer, int length);

  // fetch data in native
  static native void nativeGetDataAsync(long ptr, long lifecycle, LynxGetDataCallback callback);

  static native Object nativeGetPageDataByKey(long ptr, long lifecycle, String[] keys);

  static native JavaOnlyMap nativeGetAllJsSource(long ptr, long lifecycle);

  static native boolean nativeTakeBTSHeapSnapshotToFile(
      long ptr, long lifecycle, String outputPath, Object callback);

  // list methods
  static native void nativeRenderChild(
      long ptr, long lifecycle, int listSign, int index, long operationId);

  static native void nativeUpdateChild(
      long ptr, long lifecycle, int listSign, int oldSign, int newIndex, long operationId);

  static native void nativeRemoveChild(long ptr, long lifecycle, int listSign, int childSign);

  static native int nativeObtainChild(long ptr, long lifecycle, int listSign, int index,
      long operationId, boolean enableReuseNotification);

  static native void nativeRecycleChild(long ptr, long lifecycle, int listSign, int childSign);

  static native void nativeObtainChildAsync(
      long ptr, long lifecycle, int listSign, int index, long operationId);

  static native void nativeRecycleChildAsync(long ptr, long lifecycle, int listSign, int childSign);

  static native void nativeScrollByListContainer(
      long ptr, long lifecycle, int sign, float x, float y, float originalX, float originalY);

  static native void nativeScrollToPosition(
      long ptr, long lifecycle, int sign, int position, float offset, int align, boolean smooth);

  static native void nativeScrollStopped(long ptr, long lifecycle, int sign);

  static native long nativeGetListEngineProxy(long ptr, long lifecycle);

  static native JavaOnlyMap nativeGetListPlatformInfo(long ptr, long lifecycle, int listSign);

  static native void nativeUpdateI18nResource(
      long ptr, long lifecycle, String key, String bytes, int status);

  static native void nativeMarkDirty(long ptr, long lifecycle);

  static native void nativeFlush(long ptr, long lifecycle);

  static native void nativeSyncPackageExternalPath(long ptr, String path);

  static native void nativeSetEnableBytecode(long ptr, long lifecycle, boolean enable, String url);

  static native void nativeDispatchMessageEvent(long ptr, long lifecycle, ReadableMap event);

  static native void nativeSetSSRTimingData(long ptr, long lifecycle, String url, long data_size);

  static native JavaOnlyMap nativeGetAllTimingInfo(long ptr, long lifecycle);

  static native void nativeClearPipelineTimingInfo(long ptr, long lifecycle);

  static native void nativeSetLongTaskMonitorDisabled(long ptr, long lifecycle, boolean disabled);

  static native void nativeSetSessionStorageItem(
      long ptr, long lifecycle, String key, long value, boolean readonly);

  static native void nativeGetSessionStorageItem(
      long ptr, long lifecycle, String key, PlatformCallBack callback);

  static native double nativeSubscribeSessionStorage(
      long ptr, long lifecycle, String key, PlatformCallBack callBack);

  static native void nativeUnsubscribeSessionStorage(
      long ptr, long lifecycle, String key, double id);

  static native void nativeAttachEngineToUIThread(long ptr, long lifecycle);

  static native void nativeDetachEngineFromUIThread(long ptr, long lifecycle);

  static native void nativeSetExtensionDelegate(long ptr, long lifecycle, long delegatePtr);

  static native void nativeSetContextHasAttached(long ptr, long lifecycle);

  static native void nativeReattachLynxEngineWrapper(
      long ptr, long lifecycle, long engine_ptr, long proxy_ptr);

  static native void nativeDetachLynxEngineWrapper(long ptr, long lifecycle);

  @CalledByNative
  private static void getDataBack(ByteBuffer buffer, LynxGetDataCallback callback) {
    Object object = LepusBuffer.INSTANCE.decodeMessage(buffer);
    if (object instanceof Map) {
      callback.onSuccess(JavaOnlyMap.from((Map) object));
    } else {
      callback.onFail("LynxView GetData Failed");
    }
  }

  @CalledByNative
  private static Object decodeByteBuffer(ByteBuffer buffer) {
    return buffer == null ? null : LepusBuffer.INSTANCE.decodeMessage(buffer);
  }

  @CalledByNative
  static void dispatchBTSHeapSnapshotResult(LynxConsumer<Boolean> callback, boolean success) {
    if (callback != null) {
      callback.accept(success);
    }
  }
}
