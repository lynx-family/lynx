// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.os.Bundle;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.BuildConfig;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.lepus.LynxLepusModule;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.event.EventTargetBase;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.core.JSProxy;
import com.lynx.tasm.core.LynxEngineProxy;
import com.lynx.tasm.event.LynxEvent;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.event.LynxInternalEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
import com.lynx.tasm.theme.LynxTheme;
import com.lynx.tasm.utils.CallStackUtil;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/* package */
@SuppressWarnings("JniMissingFunction")
public class NativeFacade implements EventEmitter.LynxEventReporter {
  public interface Callback {
    void onLoaded(int templateSize);

    void onSSRHydrateFinished();

    void onRuntimeReady();

    void onDataUpdated();

    void onPageChanged(boolean isFirstScreen);

    void onDynamicComponentPerfReady(HashMap<String, Object> perf);

    void onErrorOccurred(LynxError error);

    String translateResourceForTheme(String resId, String themedKey);

    void onThemeUpdatedByJs(LynxTheme theme);

    // issue: #1510
    void onModuleFunctionInvoked(String module, String method, int error_code);

    void onPageConfigDecoded(PageConfig config);

    void onJSBInvoked(Map<String, Object> jsbInfo);

    void onCallJSBFinished(Map<String, Object> timing);

    void onUpdateDataWithoutChange();

    void onTemplateBundleReady(TemplateBundle bundle);

    void onReceiveMessageEvent(ReadableMap event);

    void onTASMFinishedByNative();

    void onUpdateI18nResource(String key, String bytes, int status);

    void onUIMethodInvoked(final int cb, JavaOnlyMap res);

    void onClearAllNativeTimingInfo();
  }

  private Callback mCallback;
  private LynxViewClient mClient;
  private String mUrl;
  private WeakReference<JSProxy> mJSProxy;
  private int mSize;

  protected WeakReference<LynxEngineProxy> mEngineProxy;
  private WeakReference<LynxContext> mLynxContext;

  private static final String TAG = "NativeFacade";

  private boolean mEnableAutoExpose;
  @Nullable private PageConfig mPageConfig;

  private boolean mEnableJSRuntime = true;

  private LynxModuleFactory mModuleFactory = null;

  public NativeFacade(boolean enableAutoExpose, boolean enableJSRuntime) {
    mEnableAutoExpose = enableAutoExpose;
    mEnableJSRuntime = enableJSRuntime;
  }

  public void setCallback(Callback callback) {
    mCallback = callback;
  }

  public void setTemplateLoadClient(LynxViewClient client) {
    mClient = client;
  }

  public void setUrl(String url) {
    mUrl = url;
  }

  public void setJSProxy(JSProxy jsProxy) {
    mJSProxy = new WeakReference<>(jsProxy);
  }

  public void setSize(int size) {
    mSize = size;
  }

  public void setEngineProxy(LynxEngineProxy engineProxy) {
    mEngineProxy = new WeakReference<>(engineProxy);
  }

  public void setLynxContext(LynxContext context) {
    mLynxContext = new WeakReference<>(context);
  }

  public WeakReference<LynxContext> getLynxContext() {
    return mLynxContext;
  }

  public boolean getEnableJSRuntime() {
    return mEnableJSRuntime;
  }

  public void setModuleFactory(LynxModuleFactory moduleFactory) {
    mModuleFactory = moduleFactory;
  }

  public LynxModuleFactory getModuleFactory() {
    return mModuleFactory;
  }

  public boolean onLynxEvent(LynxEvent event) {
    if (mClient == null) {
      return false;
    }
    LynxContext context = mLynxContext.get();
    if (context == null || context.getLynxUIOwner() == null) {
      return false;
    }

    if (event.getType() == LynxEvent.LynxEventType.kTouch && event instanceof LynxTouchEvent
        && ((LynxTouchEvent) event).getIsMultiTouch()) {
      JavaOnlyMap uiTouchMap = ((LynxTouchEvent) event).getUITouchMap();
      for (Iterator<Map.Entry<String, Object>> it = uiTouchMap.entrySet().iterator();
           it.hasNext();) {
        Map.Entry<String, Object> entry = it.next();
        EventTarget target = (EventTarget) ((LynxTouchEvent) event)
                                 .getActiveTargetMap()
                                 .get(Integer.parseInt(entry.getKey()));
        if (target == null) {
          continue;
        }

        HashMap<Integer, LynxTouchEvent.Point> touchMap = new HashMap<>();
        for (Object touch : (JavaOnlyArray) entry.getValue()) {
          touchMap.put((Integer) ((JavaOnlyArray) touch).get(0),
              new LynxTouchEvent.Point(
                  (Float) ((JavaOnlyArray) touch).get(5), (Float) ((JavaOnlyArray) touch).get(6)));
        }
        LynxTouchEvent touchEvent = new LynxTouchEvent(target.getSign(), event.getName(), touchMap);
        LynxEventDetail detail =
            new LynxEventDetail(touchEvent, (EventTargetBase) target, context.getLynxView());
        detail.setMotionEvent(((LynxTouchEvent) event).getMotionEvent());

        if (target.dispatchEvent(detail)) {
          it.remove();
        } else {
          mClient.onLynxEvent(detail);
        }
      }
      if (0 == uiTouchMap.size()) {
        return true;
      }
      return false;
    }

    EventTarget target = (EventTarget) event.getTarget();
    if (target == null) {
      return false;
    }
    LynxEventDetail detail =
        new LynxEventDetail(event, (EventTargetBase) target, context.getLynxView());
    if (event.getType() == LynxEvent.LynxEventType.kTouch && event instanceof LynxTouchEvent) {
      detail.setMotionEvent(((LynxTouchEvent) event).getMotionEvent());
    }
    if (target.dispatchEvent(detail)) {
      return true;
    }
    mClient.onLynxEvent(detail);
    return false;
  }

  public String getPageVersion() {
    if (null == mPageConfig) {
      LLog.e(TAG, "PageConfig is null.GetPageVersion get default error;");
      return "error";
    } else {
      return mPageConfig.getPageVersion();
    }
  }
  public PageConfig getPageConfig() {
    return mPageConfig;
  }

  // TODO(songshourui.null): Remove this API later. First, remove the function implementation. If
  // subsequent verification confirms that API removal does not cause a build break, then delete the
  // API.
  public void onInternalEvent(@NonNull LynxInternalEvent event) {}

  @CalledByNative
  public String translateResourceForTheme(final String resId, final String themedKey) {
    if (mCallback != null) {
      return mCallback.translateResourceForTheme(resId, themedKey);
    }
    return null;
  }

  @CalledByNative
  private void onConfigUpdatedByJS(String type, Object hashMapObj) {
    if (type == null || hashMapObj == null || !(hashMapObj instanceof HashMap)) {
      return;
    }

    HashMap<String, String> hashMap = (HashMap<String, String>) hashMapObj;
    if ("theme".equals(type)) {
      LynxTheme theme = new LynxTheme();
      for (Map.Entry<String, String> entry : hashMap.entrySet()) {
        theme.update(entry.getKey(), entry.getValue());
      }
      if (mCallback != null) {
        mCallback.onThemeUpdatedByJs(theme);
      }
    }
  }

  public void destroyAnyThreadPart() {
    if (mModuleFactory != null) {
      // If mEnableJSRuntime is true, mModuleFactory's destroy will be called when JSRuntime is
      // destroyed.
      if (mEnableJSRuntime) {
        mModuleFactory.retainJniObject();
      } else {
        mModuleFactory.destroy();
      }
      mModuleFactory = null;
    }
  }

  public void destroyUiThreadPart() {
    if (mClient != null) {
      mClient.onDestroy();
      mClient = null;
    }
  }

  @CalledByNative
  private void onRuntimeReady() {
    if (mCallback != null) {
      mCallback.onRuntimeReady();
    }
  }

  @CalledByNative
  private void onDataUpdated() {
    if (mCallback != null) {
      mCallback.onDataUpdated();
    }
  }

  @CalledByNative
  private void onPageChanged(boolean isFirstScreen) {
    if (mCallback != null) {
      mCallback.onPageChanged(isFirstScreen);
    }
  }

  @CalledByNative
  public void reportError(LynxError error) {
    if (mCallback != null) {
      mCallback.onErrorOccurred(error);
    }
  }

  @CalledByNative
  private void dispatchOnLoaded() {
    if (mCallback != null) {
      mCallback.onLoaded(mSize);
    }
  }

  @CalledByNative
  private void onSSRHydrateFinished() {
    if (mCallback != null) {
      mCallback.onSSRHydrateFinished();
    }
  }

  @CalledByNative
  private void onTASMFinishedByNative() {
    if (mCallback != null) {
      mCallback.onTASMFinishedByNative();
    }
  }

  @CalledByNative
  private void onTimingSetup(ReadableMap timingMap) {
    if (mClient == null) {
      return;
    }
    // Call the client's onTimingSetup method
    mClient.onTimingSetup(timingMap.asHashMap());
  }

  @CalledByNative
  private void onTimingUpdate(
      ReadableMap timingMap, ReadableMap updateTimingMap, String updateFlag) {
    if (mClient == null) {
      return;
    }
    // Now, the second parameter of the onTimingUpdate callback for
    // iOS and Android is different.
    // The format for iOS is {updateFlag: {timingKey: timingValue}},
    // while for Android it is {timingKey: timingValue}.
    // Therefore, on the Android layer, we need to extract {timingKey: timingValue}
    // from the first level of the map.
    HashMap<String, Object> originalUpdateMap = updateTimingMap.asHashMap();
    Object updateMapObject = originalUpdateMap.get(updateFlag);
    if (!(updateMapObject instanceof HashMap)) {
      return;
    }
    // The LynxViewClient.onTimingUpdate method's second parameter
    // accepts a Map<String, Long> argument.
    // The object that we directly receive from the C++ callback is
    // a HashMap<String, Object> parameter.
    // In this case, we need to convert the HashMap<String, Object>
    // into a Map<String, Long>
    HashMap<?, ?> updateMap = (HashMap) updateMapObject;
    // Initialize a new Map for Long values
    HashMap<String, Long> updateLongMap = new HashMap<>();
    // Iterate over the entry set of the updateMap
    for (Map.Entry<?, ?> entry : updateMap.entrySet()) {
      Object key = entry.getKey();
      Object value = entry.getValue();
      // Only put Long values into the updateLongMap
      if (key instanceof String && value instanceof Long) {
        updateLongMap.put((String) key, (Long) value);
      }
    }
    // Call the client's onTimingUpdate method
    mClient.onTimingUpdate(timingMap.asHashMap(), updateLongMap, updateFlag);
  }

  @CalledByNative
  private void onDynamicComponentPerfReady(ReadableMap perf) {
    if (mCallback != null) {
      mCallback.onDynamicComponentPerfReady(perf.asHashMap());
    }
  }

  @CalledByNative
  private void onModuleFunctionInvoked(String module, String method, int errorCode) {
    if (mCallback != null) {
      try {
        mCallback.onModuleFunctionInvoked(module, method, errorCode);
      } catch (Exception e) {
        LynxError error = new LynxError(LynxSubErrorCode.E_NATIVE_MODULES_EXCEPTION,
            "Callback 'onModuleFunctionInvoked' called after method '" + method + "' in module '"
                + module + "' threw an exception: " + e.getMessage());
        error.setCallStack(CallStackUtil.getStackTraceStringWithLineTrimmed(e));
        reportError(error);
      }
    }
  }

  @CalledByNative
  private void onUpdateDataWithoutChange() {
    if (mCallback != null) {
      mCallback.onUpdateDataWithoutChange();
    }
  }

  @CalledByNative
  public void onPageConfigDecoded(ReadableMap map) {
    mPageConfig = new PageConfig(map);
    if (mCallback != null) {
      mCallback.onPageConfigDecoded(mPageConfig);
    }
  }

  @CalledByNative
  public ByteBuffer triggerLepusBridge(String methodName, Object args) {
    ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(
        LynxLepusModule.triggerLepusBridge(methodName, args, mModuleFactory));
    return buffer;
  }

  @CalledByNative
  public void triggerLepusBridgeAsync(String methodName, Object args) {
    LynxEngineProxy engineProxy = mEngineProxy != null ? mEngineProxy.get() : null;
    LynxLepusModule.triggerLepusBridgeAsync(methodName, args, engineProxy, mModuleFactory);
  }

  @CalledByNative
  private void flushJSBTiming(ReadableMap timing) {
    // Temporarily return directly under lite.
    // This function will increase the loading time in the UIThread.
    // TODO(zhangkaijie.9): remove this after performance issue is solved
    if (BuildConfig.enable_lite) {
      return;
    }

    if (mCallback == null || timing == null) {
      return;
    }
    mCallback.onJSBInvoked(JavaOnlyMap.from(timing.getMap("info").asHashMap()));
    if (timing.getMap("info").getInt("jsb_status_code", 0) != 1) {
      return;
    }
    mCallback.onCallJSBFinished(timing.asHashMap());
  }

  @CalledByNative
  public void getI18nResourceByNative(final String channelOrUrl, String fallbackUrl) {
    LynxContext context = mLynxContext.get();
    if (context != null) {
      LynxResourceProvider<Bundle, String> i18nProvider =
          context.getProviderRegistry().getProviderByKey(
              LynxProviderRegistry.LYNX_PROVIDER_TYPE_I18N_TEXT);
      if (i18nProvider == null) {
        context.reportResourceError(LynxSubErrorCode.E_RESOURCE_I18N, channelOrUrl, "I18nResource",
            "no i18n provider found");
        return;
      }
      Bundle requestParams = new Bundle();
      requestParams.putString("fallbackUrl", fallbackUrl);
      LynxResourceRequest<Bundle> i18nRequest =
          new LynxResourceRequest<>(channelOrUrl.toLowerCase(), requestParams);
      i18nProvider.request(i18nRequest, new LynxResourceCallback<String>() {
        final WeakReference<NativeFacade> weakFacade = new WeakReference<>(NativeFacade.this);
        @Override
        public void onResponse(@NonNull final LynxResourceResponse<String> response) {
          super.onResponse(response);
          if (!UIThreadUtils.isOnUiThread()) {
            UIThreadUtils.runOnUiThread(new Runnable() {
              @Override
              public void run() {
                callbackResponse(response);
              }
            });
          } else {
            callbackResponse(response);
          }
        }

        void callbackResponse(LynxResourceResponse<String> response) {
          NativeFacade facade = weakFacade.get();
          if (facade != null) {
            if (TextUtils.isEmpty(response.getData())) {
              LynxContext ctx = facade.mLynxContext.get();
              if (ctx != null) {
                ctx.reportResourceError(LynxSubErrorCode.E_RESOURCE_I18N, channelOrUrl,
                    "I18nResource", "empty i18n resource return");
                facade.mCallback.onUpdateI18nResource(channelOrUrl, "", -1);
                return;
              }
            }
            facade.mCallback.onUpdateI18nResource(
                channelOrUrl, response.getData(), response.getCode());
          }
        }
      });
    }
  }

  @CalledByNative
  private void InvokeUIMethod(
      LynxGetUIResult ui_result, String method, JavaOnlyMap params, final int cb) {
    if (mLynxContext == null) {
      return;
    }
    LynxContext context = mLynxContext.get();
    if (context != null && context.getLynxUIOwner() != null) {
      com.lynx.react.bridge.Callback callback = new com.lynx.react.bridge.Callback() {
        @Override
        public void invoke(Object... args) {
          if (cb < 0) {
            return; // invalid callback
          }
          JavaOnlyMap res = new JavaOnlyMap(); // {code: 2, data: "xxx"}
          res.putInt("code", (Integer) args[0]);
          if (args.length > 1) {
            res.put("data", args[1]);
          }
          if (mCallback != null) {
            mCallback.onUIMethodInvoked(cb, res);
          }
        }
      };
      // ui_result is already checked in lynx engine
      context.getLynxUIOwner().invokeUIMethodForSelectorQuery(
          ui_result.getUiArray().getInt(0), method, params, callback);
    }
  }

  public boolean getAutoExpose() {
    if (mEnableAutoExpose) {
      if (null == mPageConfig) {
        LLog.e(TAG, "PageConfig is null.GetAutoExpose get default true!");
        return true; // default true
      } else {
        return mPageConfig.isAutoExpose();
      }
    }
    return false;
  }

  public int getInstanceId() {
    LynxContext context = mLynxContext.get();
    if (context == null) {
      return LynxContext.INSTANCE_ID_DEFAULT;
    }
    return context.getInstanceId();
  }

  @CalledByNative
  private void onTemplateBundleReady(TemplateBundle bundle) {
    if (mCallback != null) {
      mCallback.onTemplateBundleReady(bundle);
    }
  }

  @CalledByNative
  private void onReceiveMessageEvent(ReadableMap event) {
    if (mCallback != null) {
      mCallback.onReceiveMessageEvent(event);
    }
  }

  public void clearAllNativeTimingInfo() {
    if (mCallback != null) {
      mCallback.onClearAllNativeTimingInfo();
    }
  }
}
