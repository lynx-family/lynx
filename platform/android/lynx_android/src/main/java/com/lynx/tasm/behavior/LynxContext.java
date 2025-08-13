// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static android.os.Build.VERSION.SDK_INT;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.RenderNode;
import android.os.Build;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import androidx.annotation.AnyThread;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwner;
import com.lynx.jsbridge.JSModule;
import com.lynx.jsbridge.LynxExtensionModule;
import com.lynx.jsbridge.RuntimeLifecycleListener;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.tasm.*;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.UIExposure;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.core.JSProxy;
import com.lynx.tasm.core.LynxLayoutProxy;
import com.lynx.tasm.fluency.FluencyTraceHelper;
import com.lynx.tasm.fontface.FontFace;
import com.lynx.tasm.image.model.LynxImageFetcher;
import com.lynx.tasm.loader.LynxFontFaceLoader;
import com.lynx.tasm.performance.PerformanceController;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceFetcher;
import com.lynx.tasm.provider.LynxResourceServiceProvider;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.utils.FontFaceParser;
import com.lynx.tasm.utils.LynxConstants;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONObject;

public abstract class LynxContext extends LynxBaseContext implements ExceptionHandler {
  private static final String TAG = "LynxContext";
  private static final String UIAPPEAREVENT = "uiappear";
  private static final String UIDISAPPEAREVENT = "uidisappear";

  private ImageInterceptor mImageInterceptor;
  private ImageInterceptor mAsyncImageInterceptor;
  private JavaOnlyMap mCSSKeyframes = null;
  private final Map<String, ReadableMap> mCSSFontFaces = new HashMap<>();
  private EventEmitter mEventEmitter;
  private TouchEventDispatcher mTouchEventDispatcher = null;
  private IListNodeInfoFetcher mListNodeInfoFetcher;
  private WeakReference<JSProxy> mJSProxy;
  private WeakReference<PerformanceController> mPerfController = null;

  private WeakReference<LynxLayoutProxy> mLayoutProxy;
  private UIBody mUIBody;
  private Map<String, FontFace> mParsedFontFace;
  private WeakReference<LynxUIOwner> mLynxUIOwner;
  private WeakReference<LynxIntersectionObserverManager> mIntersectionObserverManager;
  private String mTemplateUrl = null;
  private String mJSGroupThreadName = null;
  private Map<String, Object> mSharedData;
  private LynxViewClient mLynxViewClient = null;
  private WeakReference<UIBodyView> mBodyView = null;
  private WeakReference<ShadowNodeOwner> mShadowNodeOwnerRef;
  private DisplayMetrics mVirtualScreenMetrics;
  private Boolean mEnableAsyncLoadImage;
  private LynxProviderRegistry providerRegistry;
  private LynxFontFaceLoader.Loader fontLoader;
  private List<PatchFinishListener> mPatchFinishListeners;
  private UIExposure mExposure = null;
  // for fresco monitor
  private Object mFrescoCallerContext = null;
  // for fresco disk cache choice
  private boolean mEnableImageSmallDiskCache = false;

  private boolean mPrefetchImageOnCreate = false;
  // default value is false
  private boolean mEnableAsyncImageCallback = false;

  private boolean mEnableImageResourceHint = false;

  private LynxBooleanOption mLongTaskMonitorEnabled = LynxBooleanOption.UNSET;

  private Map<String, Object> mContextData;

  private int mPlatformMeasureWidthCache;
  private int mPlatformMeasureHeightCache;

  private boolean mInPreLoad;

  // This ID is generated within LynxShell and represents the template instance.
  // mInstanceId is always a non-negative value, initialized to -1.
  // The instance ID is a unique identifier for each LynxShell created, incrementing automatically
  // with each creation. Note that within the same process, each instance ID is unique, but across
  // multiple processes, instance IDs may repeat. The primary purpose of instanceID is to
  // differentiate performance data among different LynxView instances within the same process.
  public static final int INSTANCE_ID_DEFAULT = -1;
  private int mInstanceId = INSTANCE_ID_DEFAULT;

  // lynx_session_id is a unique identifier for all active LynxView instances, constructed using the
  // following format:
  // "$currentTimestamp-$lynxViewIdentify"
  // It is assigned during the LoadTemplate process and should not be modified elsewhere.
  // The primary purpose of lynx_session_id is to identify each LynxView instance for image memory
  // usage reporting. In different processes, lynx_session_id is also unique, mainly used to
  // differentiate performance metrics across various LynxView instances. Unlike instanceID, this
  // can differentiate performance data for LynxView instances across different processes.
  private String mLynxSessionId;
  // Flatten UI enable force dark mode
  private boolean mForceDarkAllowed;
  private static boolean sSupportUsageHint = true;
  private Map<String, String> mImageCustomParams;
  private Object mLynxExtraData;
  // Asynchronous image request switch controlled by client
  private boolean mForceImageAsyncRequest = false;

  private boolean mEnableAutoConcurrency;

  private FluencyTraceHelper mFluencyTraceHelper;

  // generic resource fetcher api
  private LynxGenericResourceFetcher genericResourceFetcher;
  private LynxMediaResourceFetcher mediaResourceFetcher;
  private LynxTemplateResourceFetcher templateResourceFetcher;

  private Map<String, LynxExtensionModule> mExtensionModules = new HashMap<>();
  private LynxImageFetcher mImageFetcher;
  private Float mFontScale;

  private boolean mEnableVSyncAligned;

  private boolean mEnableAutoExpose;

  private PageConfig mPageConfig;

  private int embeddedMode = EmbeddedMode.UNSET;

  private boolean isFallbackProcess = false;

  public LynxContext(Context base, DisplayMetrics screenMetrics) {
    super(base);
    mVirtualScreenMetrics = new DisplayMetrics();
    mVirtualScreenMetrics.setTo(screenMetrics);
  }

  public void setPlatformMeasureHeightCache(int platformMeasureHeightCache) {
    this.mPlatformMeasureHeightCache = platformMeasureHeightCache;
  }

  public void setPlatformMeasureWidthCache(int platformMeasureWidthCache) {
    this.mPlatformMeasureWidthCache = platformMeasureWidthCache;
  }

  public int getPlatformMeasureWidthCache() {
    return mPlatformMeasureWidthCache;
  }

  public int getPlatformMeasureHeightCache() {
    return mPlatformMeasureHeightCache;
  }

  /**
   * @brief Get the current font scale
   *
   * @details This method returns the current font scale. If `mFontScale` is null,
   * it initializes it with the system's font scale and marks it as initialized.
   *
   * @return float The current font scale
   */
  public float getFontScale() {
    if (mFontScale == null) {
      mFontScale = getResources().getConfiguration().fontScale;
    }
    return mFontScale;
  }

  /**
   * @brief check whether enable eventReporter
   * @return false if embedMode is open
   */
  public boolean enableEventReporter() {
    return !isEmbeddedModeOn();
  }

  /**
   * @brief check whether enable TimingAPI
   * @return false if and only if embedMode is open
   */
  public boolean enableTiming() {
    return !isEmbeddedModeOn();
  }

  /**
   * @brief check whether embedMode is open
   */
  public boolean isEmbeddedModeOn() {
    return (embeddedMode & EmbeddedMode.EMBEDDED_MODE_BASE) > 0;
  }

  public boolean isFallbackProcess() {
    return isFallbackProcess;
  }

  public void markFallbackProcess(boolean enable) {
    isFallbackProcess = enable;
    if (!enable && getUIBodyView() != null) {
      getUIBodyView().clearNodeIndexImageMap();
      getUIBodyView().markNeedRemoveExistingViews();
      getUIBodyView().removeExistingViews();
    }
  }

  public boolean isLayoutInElementModeOn() {
    return (embeddedMode & EmbeddedMode.LAYOUT_IN_ELEMENT) > 0;
  }

  /**
   * @brief check whether enables use the native `Fragment` based UI renderer
   */
  public boolean isFragmentLayerRenderOn() {
    return (embeddedMode & EmbeddedMode.FRAGMENT_LAYER_RENDER) > 0;
  }

  /**
   * @brief check whether engine pool is enabled
   */
  public boolean isEnginePoolEnabled() {
    return EmbeddedMode.isEnginePoolEnable(embeddedMode);
  }

  /**
   * @brief embedMode setter
   * @param embeddedMode switch, normally set from LynxViewBuilder
   */
  public void setEmbeddedMode(@EmbeddedMode.Mode int embeddedMode) {
    this.embeddedMode = embeddedMode;
  }

  public void setEnableAsyncLoadImage(boolean b) {
    this.mEnableAsyncLoadImage = b;
  }

  public boolean isEnableAsyncLoadImage() {
    return mEnableAsyncLoadImage != null && mEnableAsyncLoadImage;
  }

  public LynxBooleanOption getLongTaskMonitorEnabled() {
    return mLongTaskMonitorEnabled;
  }

  public void setLongTaskMonitorEnabled(LynxBooleanOption enabled) {
    mLongTaskMonitorEnabled = enabled;
  }

  public void setPerfController(PerformanceController perfController) {
    if (perfController != null) {
      mPerfController = new WeakReference<>(perfController);
    }
  }

  public PerformanceController getPerfController() {
    if (mPerfController == null) {
      return null;
    }
    return mPerfController.get();
  }

  public void onPageConfigDecoded(PageConfig config) {
    mPageConfig = config;
  }

  /**
   * @brief switch for LynxView onShow onHide event
   */
  public void setEnableAutoExpose(boolean enableAutoExpose) {
    mEnableAutoExpose = enableAutoExpose;
  }

  /**
   * @brief switch for LynxView onShow onHide event
   */
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

  /**
   * @brief get PageVersion from PageConfig
   */
  public String getPageVersion() {
    if (null == mPageConfig) {
      LLog.e(TAG, "PageConfig is null.GetPageVersion get default error;");
      return "error";
    } else {
      return mPageConfig.getPageVersion();
    }
  }

  /**
   * Get the probability of enabling Lynx Fluency Monitor in this context.
   *
   * @return double value of the probability
   */
  public double getEnableLynxScrollFluency() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableLynxScrollFluency();
    }
    return -1.0;
  }

  /**
   * @brief switch for enable VsyncAlignedFlush
   */
  public boolean getEnableVsyncAlignedFlush() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableVsyncAlignedFlush();
    }
    return false;
  }

  public boolean isAsyncRedirect() {
    if (null != mPageConfig) {
      return mPageConfig.isAsyncRedirect();
    }
    return false;
  }

  public boolean isSyncImageAttach() {
    if (null != mPageConfig) {
      return mPageConfig.isSyncImageAttach();
    }
    return true;
  }

  public boolean isPrefetchImageOnCreate() {
    return mPrefetchImageOnCreate;
  }

  public boolean isEnableCheckLocalImage() {
    if (null != mPageConfig) {
      return mPageConfig.isEnableCheckLocalImage();
    }
    return true;
  }

  public boolean isEnableAsyncRequestImage() {
    if (null != mPageConfig) {
      return mPageConfig.isEnableAsyncRequestImage();
    }
    return false;
  }

  public boolean isEnableAsyncImageCallback() {
    return mEnableAsyncImageCallback;
  }

  public boolean isForceImageAsyncRequest() {
    return mForceImageAsyncRequest;
  }

  public boolean isUseImagePostProcessor() {
    if (null != mPageConfig) {
      return mPageConfig.isUseImagePostProcessor();
    }
    return false;
  }

  public boolean getEnableLoadImageFromService() {
    if (null != mPageConfig) {
      return mPageConfig.isEnableLoadImageFromService();
    }
    return true;
  }

  public boolean getDefaultOverflowVisible() {
    if (null != mPageConfig) {
      return mPageConfig.getDefaultOverflowVisible();
    }
    return false;
  }

  public boolean isAsyncInitTTVideoEngine() {
    if (null != mPageConfig) {
      return mPageConfig.isAsyncInitTTVideoEngine();
    }
    return false;
  }

  public boolean getEnableFiberArch() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableFiberArc();
    }
    return false;
  }

  public boolean getCssAlignWithLegacyW3c() {
    if (null != mPageConfig) {
      return mPageConfig.isCssAlignWithLegacyW3c();
    }
    return false;
  }

  public boolean isEnableNewGesture() {
    if (null != mPageConfig) {
      return mPageConfig.isEnableNewGesture();
    }
    return false;
  }

  public FluencyTraceHelper getFluencyTraceHelper() {
    if (mFluencyTraceHelper == null) {
      mFluencyTraceHelper = new FluencyTraceHelper(this);
    }
    return mFluencyTraceHelper;
  }

  public DisplayMetrics getScreenMetrics() {
    return mVirtualScreenMetrics;
  }

  public void updateScreenSize(int screenWidth, int screenHeight) {
    mVirtualScreenMetrics.widthPixels = screenWidth;
    mVirtualScreenMetrics.heightPixels = screenHeight;
    if (mExposure != null) {
      mExposure.updateWindowSize(this);
    }
  }

  public void setImageInterceptor(ImageInterceptor imageInterceptor) {
    mImageInterceptor = imageInterceptor;
  }

  public void setAsyncImageInterceptor(ImageInterceptor imageInterceptor) {
    mAsyncImageInterceptor = imageInterceptor;
  }

  public ImageInterceptor imageInterceptor() {
    return mImageInterceptor;
  }

  public ImageInterceptor getAsyncImageInterceptor() {
    return mAsyncImageInterceptor;
  }

  public boolean getEnableImageSmallDiskCache() {
    return mEnableImageSmallDiskCache;
  }

  public void setEnableImageSmallDiskCache(boolean enable) {
    this.mEnableImageSmallDiskCache = enable;
  }

  public void setPrefetchImageOnCreate(boolean enable) {
    this.mPrefetchImageOnCreate = enable;
  }

  public void setEnableAsyncImageCallback(boolean enable) {
    this.mEnableAsyncImageCallback = enable;
  }

  public void setForceImageAsyncRequest(boolean enable) {
    this.mForceImageAsyncRequest = enable;
  }

  public void setLynxViewClient(LynxViewClient lynxViewClient) {
    this.mLynxViewClient = lynxViewClient;
  }

  public LynxViewClient getLynxViewClient() {
    return mLynxViewClient;
  }

  private void updateLynxSessionID(UIBodyView bodyView) {
    TraceEvent.beginSection(TraceEventDef.LYNX_CONTEXT_UPDATE_SESSION_ID);
    String currentTimestamp = String.valueOf(System.currentTimeMillis());
    String lynxViewIdentify = String.valueOf(System.identityHashCode(bodyView));
    // sessionID would be like "$currentTimestamp-$lynxViewIdentify"
    // Use StringBuilder to replace String format for String format is significantly slower than
    // StringBuilder in this case.
    StringBuilder builder = new StringBuilder();
    builder.append(currentTimestamp);
    builder.append("-");
    builder.append(lynxViewIdentify);
    mLynxSessionId = builder.toString();
    TraceEvent.endSection(TraceEventDef.LYNX_CONTEXT_UPDATE_SESSION_ID);
  }

  /**
   * use {@link #setUIBodyView(UIBodyView)} instead
   */
  @Deprecated
  public void setLynxView(LynxView lynxview) {
    setUIBodyView(lynxview);
  }

  public void setUIBodyView(UIBodyView bodyView) {
    setHasLynxViewAttached(bodyView != null);
    LLog.i("LynxUIRenderer", "lynxContext setUIBodyView" + bodyView + this);
    this.mBodyView = new WeakReference<>(bodyView);
    // Prevent generate lynxSessionID multiple times
    if (TextUtils.isEmpty(mLynxSessionId)) {
      updateLynxSessionID(bodyView);
    }
  }

  /**
   * Provide unique LynxView sessionID like "$currentTimestamp-$lynxViewIdentify" format
   * @return LynxSessionID generated in LynxTemplateRender
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  @NonNull
  @AnyThread
  public String getLynxSessionID() {
    if (mLynxSessionId == null) {
      return "";
    }
    return mLynxSessionId;
  }

  public LynxView getLynxView() {
    UIBodyView bodyView = getUIBodyView();
    return bodyView instanceof LynxView ? (LynxView) bodyView : null;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public UIBodyView getUIBodyView() {
    return mBodyView != null ? mBodyView.get() : null;
  }

  /**
   * use {@link #reportResourceError(String, String, String)} instead
   */
  @Deprecated
  public void reportResourceError(String errMsg) {
    if (this.mLynxViewClient != null) {
      this.mLynxViewClient.onReceivedError(
          new LynxError(errMsg, LynxSubErrorCode.E_RESOURCE_CUSTOM));
    }
  }

  public void reportResourceError(String src, String type, String error_msg) {
    reportResourceError(LynxSubErrorCode.E_RESOURCE_CUSTOM, src, type, error_msg);
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public void reportResourceError(int code, String src, String type, String error_msg) {
    LynxError error = new LynxError(code, error_msg);
    reportResourceError(src, type, error);
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public void reportResourceError(String src, String type, LynxError error) {
    if (this.mLynxViewClient == null || error == null) {
      return;
    }
    error.setTemplateUrl(mTemplateUrl);
    error.addCustomInfo(LynxError.LYNX_ERROR_KEY_RESOURCE_URL, src);
    error.addCustomInfo(LynxError.LYNX_ERROR_KEY_RESOURCE_TYPE, type);
    this.mLynxViewClient.onReceivedError(error);
  }

  public void setTouchEventDispatcher(TouchEventDispatcher dispatcher) {
    mTouchEventDispatcher = dispatcher;
  }

  public TouchEventDispatcher getTouchEventDispatcher() {
    return mTouchEventDispatcher;
  }

  public void onGestureRecognized() {
    if (mTouchEventDispatcher != null) {
      mTouchEventDispatcher.onGestureRecognized();
    }
  }

  public void onGestureRecognized(int sign) {
    if (mTouchEventDispatcher != null) {
      mTouchEventDispatcher.onGestureRecognized(sign);
    }
  }

  public void onGestureRecognized(LynxBaseUI ui) {
    if (mTouchEventDispatcher != null) {
      mTouchEventDispatcher.onGestureRecognized(ui);
    }
  }

  public void onPropsChanged(LynxBaseUI ui) {
    if (mTouchEventDispatcher != null) {
      mTouchEventDispatcher.onPropsChanged(ui);
    }
  }

  public boolean isTouchMoving() {
    if (mTouchEventDispatcher != null) {
      return mTouchEventDispatcher.isTouchMoving();
    }

    return false;
  }

  public void setEventEmitter(EventEmitter eventEmitter) {
    mEventEmitter = eventEmitter;
  }

  public EventEmitter getEventEmitter() {
    return mEventEmitter;
  }

  public IListNodeInfoFetcher getListNodeInfoFetcher() {
    return mListNodeInfoFetcher;
  }

  public void setListNodeInfoFetcher(IListNodeInfoFetcher listNodeInfoFetcher) {
    this.mListNodeInfoFetcher = listNodeInfoFetcher;
  }

  public void setIntersectionObserverManager(LynxIntersectionObserverManager manager) {
    mIntersectionObserverManager = new WeakReference<>(manager);
  }

  public LynxIntersectionObserverManager getIntersectionObserverManager() {
    if (mIntersectionObserverManager == null) {
      return null;
    }
    return mIntersectionObserverManager.get();
  }

  public Long getRuntimeId() {
    JSProxy proxy = mJSProxy.get();
    return proxy != null ? proxy.getRuntimeId() : null;
  }

  public void setJSProxy(JSProxy proxy) {
    mJSProxy = new WeakReference<>(proxy);
  }

  public void setLayoutProxy(LynxLayoutProxy proxy) {
    mLayoutProxy = new WeakReference<>(proxy);
  }

  public JSModule getJSModule(String module) {
    if (mJSProxy == null) {
      return null;
    }
    JSProxy proxy = mJSProxy.get();
    if (proxy != null) {
      return proxy.getJSModule(module);
    }
    return null;
  }

  public void sendKeyEvent(int keyCode, String name) {
    JSModule eventEmitter = getJSModule("GlobalEventEmitter");
    if (eventEmitter == null) {
      LLog.e(LynxConstants.TAG, "sendGlobalEvent error, can't get GlobalEventEmitter");
      return;
    }

    JavaOnlyArray args = new JavaOnlyArray();
    args.pushString(name);
    JavaOnlyArray param = new JavaOnlyArray();
    param.pushString(name);
    param.pushInt(keyCode);
    args.pushArray(param);
    eventEmitter.fire("emit", args);
  }

  public void sendGlobalEvent(String name, JavaOnlyArray params) {
    LLog.i(TAG, "LynxContext sendGlobalEvent " + name + " with this: " + this.toString());

    JSModule eventEmitter = getJSModule("GlobalEventEmitter");
    if (eventEmitter == null) {
      LLog.e(TAG,
          "LynxContext sendGlobalEvent failed since eventEmitter is null with this: "
              + this.toString());
      return;
    }
    JavaOnlyArray args = new JavaOnlyArray();
    args.pushString(name);
    args.pushArray(params);
    if (eventEmitter != null) {
      eventEmitter.fire("emit", args);
    } else {
      LLog.e(LynxConstants.TAG, "sendGlobalEvent error, can't get GlobalEventEmitter");
    }
  }

  // now override when create by LynxTemplateRender
  public abstract void handleException(Exception e);
  public void handleException(Exception e, JSONObject userDefinedInfo) {}

  /**
   * @deprecated The error code of an exception should always be LYNX_ERROR_EXCEPTION
   * instead of being manually assigned. Please use handleException(Exception).
   */
  @Deprecated
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void handleException(Exception e, int errCode) {}

  /**
   * @deprecated The error code of an exception should always be LYNX_ERROR_EXCEPTION
   * instead of being manually assigned. Please use handleException(Exception, JSONObject).
   */
  @Deprecated
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void handleException(Exception e, int errCode, JSONObject userDefinedInfo) {}

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void handleLynxError(LynxError error) {}

  public void removeAnimationKeyframe(String removeName) {
    if (mCSSKeyframes != null) {
      mCSSKeyframes.remove(removeName);
    }
  }

  public void setKeyframes(ReadableMap keyframes) {
    if (mCSSKeyframes == null) {
      mCSSKeyframes = new JavaOnlyMap();
    }
    if (keyframes != null) {
      mCSSKeyframes.merge(keyframes);
    }
  }

  public void setFontFaces(ReadableMap fontFaces) {
    if (fontFaces == null) {
      return;
    }
    synchronized (mCSSFontFaces) {
      ReadableMapKeySetIterator iterator = fontFaces.keySetIterator();
      while (iterator.hasNextKey()) {
        String key = iterator.nextKey();
        ReadableMap map = fontFaces.getMap(key);
        if (map == null) {
          continue;
        }
        mCSSFontFaces.put(key, map);
      }
    }
  }

  public ReadableMap getKeyframes(String name) {
    // map <%,<id, value>>
    if (mCSSKeyframes == null) {
      return null;
    }
    return mCSSKeyframes.hasKey(name) ? mCSSKeyframes.getMap(name) : null;
  }

  public Map getFontFaces(String name) {
    synchronized (mCSSFontFaces) {
      ReadableMap fontFace = null;
      if (mCSSFontFaces.containsKey(name)) {
        fontFace = mCSSFontFaces.get(name);
      }
      if (fontFace != null) {
        return fontFace.asHashMap();
      } else {
        return null;
      }
    }
  }

  public FontFace getFontFace(String fontFamily) {
    // TOOD(zhouzhuangzhuang):remove parse logic from LynxContext.
    String[] fonts = fontFamily.split(",");
    for (String family : fonts) {
      family = FontFaceParser.trim(family);
      if (TextUtils.isEmpty(family)) {
        continue;
      }
      synchronized (FontFaceParser.class) {
        if (mParsedFontFace == null) {
          mParsedFontFace = new HashMap<>();
        }
        FontFace fontFace = mParsedFontFace.get(family);
        if (fontFace == null) {
          fontFace = FontFaceParser.parse(this, family);
        } else {
          return fontFace;
        }
        if (fontFace != null) {
          mParsedFontFace.put(family, fontFace);
          return fontFace;
        }
      }
    }
    return null;
  }

  public String getTemplateUrl() {
    return mTemplateUrl;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  @Nullable
  public String getJSGroupThreadName() {
    return mJSGroupThreadName;
  }

  public void setTemplateUrl(String url) {
    mTemplateUrl = url;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public void setJSGroupThreadName(String lynxGroupName) {
    mJSGroupThreadName = lynxGroupName;
  }

  public void setLynxUIOwner(LynxUIOwner owner) {
    mLynxUIOwner = new WeakReference<>(owner);
  }

  public UIExposure getExposure() {
    return mExposure;
  }

  public LynxUIOwner getLynxUIOwner() {
    return mLynxUIOwner.get();
  }

  public LynxBaseUI findLynxUIByName(@NonNull String name) {
    LynxBaseUI ui = null;
    LynxUIOwner owner = mLynxUIOwner.get();
    if (owner != null) {
      ui = owner.findLynxUIByName(name);
    }
    return ui;
  }

  public UIBody getUIBody() {
    return mUIBody;
  }

  public void setUIBody(UIBody mUIBody) {
    this.mUIBody = mUIBody;
  }

  public void putSharedData(String key, Object value) {
    if (mSharedData == null) {
      mSharedData = new HashMap<>();
    }
    mSharedData.put(key, value);
  }

  public Object getSharedData(String key) {
    if (mSharedData == null) {
      return null;
    }
    return mSharedData.get(key);
  }

  public <T> T getSharedData(String key, Class<T> type) {
    if (mSharedData == null) {
      return null;
    }
    Object result = mSharedData.get(key);
    if (type.isInstance(result)) {
      return type.cast(result);
    }
    return null;
  }

  public LynxBaseUI findLynxUIByIdSelector(@NonNull String idSelector, LynxBaseUI container) {
    LynxBaseUI ui = null;
    LynxUIOwner owner = mLynxUIOwner.get();
    if (owner != null) {
      ui = owner.findLynxUIByIdSelector(idSelector, container);
    }
    return ui;
  }

  public LynxBaseUI findLynxUIBySign(int sign) {
    LynxBaseUI ui = null;
    LynxUIOwner owner = mLynxUIOwner.get();
    if (owner != null) {
      ui = owner.getNode(sign);
    }
    return ui;
  }

  public LynxBaseUI findLynxUIByComponentId(String componentId) {
    LynxUIOwner owner = mLynxUIOwner.get();
    return owner != null ? owner.findLynxUIByComponentId(componentId) : null;
  }

  public void invokeUIMethod(
      String sign, ReadableArray nodes, String method, ReadableMap params, Callback callback) {
    LynxUIOwner owner = mLynxUIOwner.get();
    if (owner != null) {
      owner.invokeUIMethod(sign, nodes, method, params, callback);
    }
  }

  // issue: #1510
  public void reportModuleCustomError(String message) {
    mLynxViewClient.onReceivedError(
        new LynxError(message, LynxSubErrorCode.E_NATIVE_MODULES_CUSTOM_ERROR));
  }
  public void setShadowNodeOwner(ShadowNodeOwner shadowNodeOwner) {
    mShadowNodeOwnerRef = new WeakReference<>(shadowNodeOwner);
  }

  public ShadowNode findShadowNodeBySign(int sign) {
    ShadowNodeOwner owner = mShadowNodeOwnerRef.get();
    if (owner != null) {
      return owner.getShadowNode(sign);
    }
    return null;
  }

  /**
   * @brief Find shadow node by sign and run task on layout thread
   * @detail Find shadownode by sign, and run task on layout thread,
   * if shadownode is not found, task will not be run.
   * @param sign shadow node id
   * @param task  task to run on layout thread
   */
  public void findShadowNodeAndRunTask(int sign, LynxConsumer<ShadowNode> task) {
    runOnLayoutThread(() -> {
      ShadowNodeOwner owner = mShadowNodeOwnerRef.get();
      if (owner != null) {
        ShadowNode node = owner.getShadowNode(sign);
        task.accept(node);
      }
    });
  }

  public LynxBaseInspectorOwner getBaseInspectorOwner() {
    LynxView lynxView = getLynxView();
    return lynxView != null ? lynxView.getBaseInspectorOwner() : null;
  }

  public void setProviderRegistry(LynxProviderRegistry providerRegistry) {
    this.providerRegistry = providerRegistry;
  }

  public LynxProviderRegistry getProviderRegistry() {
    return providerRegistry;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setGenericResourceFetcher(LynxGenericResourceFetcher genericResourceFetcher) {
    this.genericResourceFetcher = genericResourceFetcher;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public @Nullable LynxGenericResourceFetcher getGenericResourceFetcher() {
    return this.genericResourceFetcher;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setMediaResourceFetcher(LynxMediaResourceFetcher mediaResourceFetcher) {
    this.mediaResourceFetcher = mediaResourceFetcher;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public @Nullable LynxMediaResourceFetcher getMediaResourceFetcher() {
    return this.mediaResourceFetcher;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setTemplateResourceFetcher(LynxTemplateResourceFetcher templateResourceFetcher) {
    this.templateResourceFetcher = templateResourceFetcher;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public @Nullable LynxTemplateResourceFetcher getTemplateResourceFetcher() {
    return this.templateResourceFetcher;
  }

  public void setFontLoader(LynxFontFaceLoader.Loader fontLoader) {
    this.fontLoader = fontLoader;
  }

  @Nullable
  public LynxFontFaceLoader.Loader getFontLoader() {
    return fontLoader;
  }

  public Object getFrescoCallerContext() {
    return mFrescoCallerContext;
  }

  /**
   * inject business custom Info for fresco monitor
   * @param frescoCallerContext business custom Info
   */
  public void setFrescoCallerContext(Object frescoCallerContext) {
    this.mFrescoCallerContext = frescoCallerContext;
  }

  public void runOnTasmThread(Runnable runnable) {
    UIBodyView bodyView = getUIBodyView();
    if (bodyView != null) {
      bodyView.runOnTasmThread(runnable);
    }
  }

  @MainThread
  void registerPatchFinishListener(@NonNull PatchFinishListener listener) {
    if (mPatchFinishListeners == null) {
      mPatchFinishListeners = new ArrayList<>();
    }
    mPatchFinishListeners.add(listener);
  }

  @MainThread
  public void unregisterPatchFinishListener(@NonNull PatchFinishListener listener) {
    if (mPatchFinishListeners != null) {
      mPatchFinishListeners.remove(listener);
    }
  }

  @MainThread
  List<PatchFinishListener> getPatchFinishListeners() {
    return mPatchFinishListeners;
  }

  public boolean isTextRefactorEnabled() {
    if (null != mPageConfig) {
      return mPageConfig.isTextRefactorEnabled();
    }
    return false;
  }

  public boolean isNewClipModeEnabled() {
    if (null != mPageConfig) {
      return mPageConfig.isNewClipModeEnabled();
    }
    return false;
  }

  public boolean isTextOverflowEnabled() {
    if (null != mPageConfig) {
      return mPageConfig.isTextOverflowEnabled();
    }
    return false;
  }

  public boolean isTextBoringLayoutEnabled() {
    if (null != mPageConfig) {
      return mPageConfig.isTextBoringLayoutEnabled();
    }
    return false;
  }

  public boolean isTextLayoutCacheEnabled() {
    if (null != mPageConfig) {
      return mPageConfig.isTextLayoutCacheEnabled();
    }
    return true;
  }

  public boolean useRelativeKeyboardHeightApi() {
    if (null != mPageConfig) {
      return mPageConfig.useRelativeKeyboardHeightApi();
    }
    return false;
  }

  public boolean getDefaultTextIncludePadding() {
    if (null != mPageConfig) {
      return mPageConfig.getDefaultTextIncludePadding();
    }
    return false;
  }

  public boolean getEnableEventRefactor() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableEventRefactor();
    }
    return true;
  }

  public boolean getEnableDisexposureWhenLynxHidden() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableDisexposureWhenLynxHidden();
    }
    return true;
  }

  public boolean getEnableExposureWhenLayout() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableExposureWhenLayout();
    }
    return false;
  }

  public void reset() {
    synchronized (mCSSFontFaces) {
      mCSSFontFaces.clear();
    }

    if (mTouchEventDispatcher != null) {
      mTouchEventDispatcher.reset();
    }

    if (mExposure != null) {
      mExposure.clear();
    }

    if (getEnableNewIntersectionObserver() && mIntersectionObserverManager != null) {
      LynxIntersectionObserverManager intersectionObserverManager =
          mIntersectionObserverManager.get();
      if (intersectionObserverManager != null) {
        intersectionObserverManager.clear();
      }
    }
  }

  public void destory() {
    if (mExposure != null) {
      mExposure.clear();
    }

    if (getEnableNewIntersectionObserver() && mIntersectionObserverManager != null) {
      LynxIntersectionObserverManager intersectionObserverManager =
          mIntersectionObserverManager.get();
      if (intersectionObserverManager != null) {
        intersectionObserverManager.clear();
      }
    }
  }

  public void clearExposure() {
    if (mExposure != null) {
      mExposure.clear();
    }
  }

  public void addUIToExposedMap(LynxBaseUI ui) {
    addUIToExposedMap(ui, null, null, null);
  }

  public void addUIToExposedMap(LynxBaseUI ui, @Nullable String uniqueID,
      @Nullable JavaOnlyMap data, @Nullable JavaOnlyMap options) {
    // Since the exposure node using the custom event method can also set the exposure-related
    // properties, we choose to check whether the node is bound to the relevant event here to avoid
    // modifying it in multiple places.
    if (uniqueID == null && ui.getEvents() != null
        && (ui.getEvents().containsKey(UIAPPEAREVENT)
            || ui.getEvents().containsKey(UIDISAPPEAREVENT))) {
      if (mExposure == null) {
        initUIExposure();
      }
      JavaOnlyMap customOption = new JavaOnlyMap();
      customOption.put("sendCustom", true);
      mExposure.addUIToExposedMap(ui, String.valueOf(ui.getSign()), null, customOption);
    }

    // Native components can call this interface to reuse the exposure capability.
    if (uniqueID != null || ui.getExposureID() != null) {
      if (mExposure == null) {
        initUIExposure();
      }
      mExposure.addUIToExposedMap(ui, uniqueID, data, options);
    }
  }

  public void removeUIFromExposedMap(LynxBaseUI ui) {
    removeUIFromExposedMap(ui, null);
  }

  public void removeUIFromExposedMap(LynxBaseUI ui, @Nullable String uniqueID) {
    // Since the exposure node using the custom event method can also set the exposure-related
    // properties, we choose to check whether the node is bound to the relevant event here to avoid
    // modifying it in multiple places.
    if (uniqueID == null && ui.getEvents() != null
        && (ui.getEvents().containsKey(UIAPPEAREVENT)
            || ui.getEvents().containsKey(UIDISAPPEAREVENT))) {
      if (mExposure == null) {
        initUIExposure();
      }
      mExposure.removeUIFromExposedMap(ui, String.valueOf(ui.getSign()));
    }

    // Native components can call this interface to reuse the exposure capability.
    if (uniqueID != null || ui.getExposureID() != null) {
      if (mExposure == null) {
        initUIExposure();
      }
      mExposure.removeUIFromExposedMap(ui, uniqueID);
    }
  }

  public void onRootViewDraw(Canvas canvas) {
    if (mExposure != null) {
      mExposure.onRootViewDraw(canvas);
    }
    if (getEnableNewIntersectionObserver() && mIntersectionObserverManager != null) {
      LynxIntersectionObserverManager intersectionObserverManager =
          mIntersectionObserverManager.get();
      if (intersectionObserverManager != null) {
        intersectionObserverManager.onRootViewDraw(canvas);
      }
    }
  }

  public void onAttachedToWindow() {
    if (mExposure != null) {
      mExposure.onAttachedToWindow();
    }
    // TODO(jiyishen): call newIntersectionObserver onAttachedToWindow.
  }

  public boolean getEnableFlattenTranslateZ() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableFlattenTranslateZ();
    }
    return false;
  }

  public boolean enableEventThrough() {
    if (null != mPageConfig) {
      return mPageConfig.enableEventThrough();
    }
    return false;
  }

  public boolean getEnableNewIntersectionObserver() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableNewIntersectionObserver();
    }
    return false;
  }

  /**
   * @brief switch for enable CreateViewAsync
   */
  public boolean getEnableCreateViewAsync() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableCreateViewAsync();
    }
    return false;
  }

  /**
   * @brief switch for is using new swiper
   */
  public boolean isUseNewSwiper() {
    if (null != mPageConfig) {
      return mPageConfig.isUseNewSwiper();
    }
    return false;
  }

  public int getObserverFrameRate() {
    if (null != mPageConfig) {
      return mPageConfig.getObserverFrameRate();
    }
    return 20;
  }

  public boolean getEnableExposureUIMargin() {
    if (null != mPageConfig) {
      return mPageConfig.getEnableExposureUIMargin();
    }
    return false;
  }

  public int getLongPressDuration() {
    if (null != mPageConfig) {
      return mPageConfig.getLongPressDuration();
    }
    return -1;
  }

  public int getMapContainerType() {
    if (null != mPageConfig) {
      return mPageConfig.getMapContainerType();
    }
    return 0;
  }

  public LynxAccessibilityWrapper getLynxAccessibilityWrapper() {
    if (mUIBody != null) {
      return mUIBody.getLynxAccessibilityWrapper();
    }
    return null;
  }

  public void setContextData(HashMap<String, Object> contextData) {
    this.mContextData = contextData;
  }

  public Map getContextData() {
    return mContextData;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public int getInstanceId() {
    return mInstanceId;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setInstanceId(int instanceId) {
    this.mInstanceId = instanceId;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setInPreLoad(boolean preload) {
    mInPreLoad = preload;
    if (mUIBody != null && mUIBody.getBodyView() != null) {
      mUIBody.getBodyView().SetShouldInterceptRequestLayout(preload);
    }
    if (mEventEmitter != null) {
      mEventEmitter.setInPreLoad(preload);
    }
  }

  public void setEnableImageResourceHint(boolean enable) {
    this.mEnableImageResourceHint = enable;
  }

  public boolean getEnableImageResourceHint() {
    return this.mEnableImageResourceHint;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setImageCustomParam(Map<String, String> imageCustomParam) {
    mImageCustomParams = imageCustomParam;
  }

  public Map<String, String> getImageCustomParam() {
    return mImageCustomParams;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setImageFetcher(LynxImageFetcher fetcher) {
    mImageFetcher = fetcher;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public LynxImageFetcher getImageFetcher() {
    return mImageFetcher;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void setLynxExtraData(Object extraData) {
    mLynxExtraData = extraData;
  }
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public Object getLynxExtraData() {
    return mLynxExtraData;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public boolean isInPreLoad() {
    return mInPreLoad;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void runOnJSThread(Runnable runnable) {
    if (runnable == null) {
      return;
    }
    JSProxy proxy = mJSProxy.get();
    if (proxy == null) {
      return;
    }
    proxy.runOnJSThread(runnable);
  }

  private void runOnLayoutThread(Runnable runnable) {
    if (runnable == null) {
      return;
    }
    LynxLayoutProxy layoutProxy = mLayoutProxy.get();
    if (layoutProxy == null) {
      return;
    }
    layoutProxy.runOnLayoutThread(runnable);
  }

  public void setForceDarkAllowed(boolean allowed) {
    mForceDarkAllowed = allowed;
    if (allowed && SDK_INT >= Build.VERSION_CODES.Q && sSupportUsageHint
        && LynxFlattenUI.sSetUsageHint == null) {
      try {
        LynxFlattenUI.sSetUsageHint = RenderNode.class.getMethod("setUsageHint", int.class);
      } catch (NoSuchMethodException e) {
        sSupportUsageHint = false;
        LLog.e(TAG, "NoSuchMethodException: setUsageHint");
      }
    }
  }

  public boolean getForceDarkAllowed() {
    return mForceDarkAllowed;
  }

  public void setEnableAutoConcurrency(boolean enable) {
    mEnableAutoConcurrency = enable;
  }

  public boolean getEnableAutoConcurrency() {
    return mEnableAutoConcurrency;
  }

  /**
   * @brief init UIExposure.
   * @detail create UIExposure instance and set rootUI & callback for UIExposure. Call only when
   * necessary to implement lazy loading.
   */
  public void initUIExposure() {
    mExposure = new UIExposure();
    LynxUIOwner owner = mLynxUIOwner.get();
    if (owner != null) {
      mExposure.setRootUI(owner.getRootUI());
    }
    final WeakReference<LynxContext> weakContext = new WeakReference<>(this);
    mExposure.setCallback(new UIExposure.ExposureCallback(weakContext));
    mExposure.updateWindowSize(this);
  }

  /**
   * @brief init TouchEventDispatcher when it's null.
   * @detail create TouchEventDispatcher instance and init some config. Call when it is uncertain
   * whether the TouchEventDispatcher has been created.
   */
  public void EnsureEventDispatcher() {
    if (mTouchEventDispatcher != null) {
      return;
    }
    UIBodyView bodyView = getUIBodyView();
    if (bodyView == null) {
      return;
    }
    ILynxUIRenderer lynxUIRenderer = bodyView.getLynxUIRendererInternal();
    if (lynxUIRenderer != null && lynxUIRenderer instanceof LynxUIRenderer) {
      ((LynxUIRenderer) lynxUIRenderer).EnsureEventDispatcher();
    }
  }

  // This is a experimental API, it is unstable and may break at any time.
  @RestrictTo({RestrictTo.Scope.LIBRARY_GROUP})
  public void setExtensionModuleForKey(LynxExtensionModule module, String key) {
    if (module == null) {
      return;
    }
    mExtensionModules.put(key, module);
  }

  // This is a experimental API, it is unstable and may break at any time.
  @RestrictTo({RestrictTo.Scope.LIBRARY_GROUP})
  public LynxExtensionModule getExtensionModuleByKey(String key) {
    return mExtensionModules.get(key);
  }

  // This is a experimental API, it is unstable and may break at any time.
  @RestrictTo({RestrictTo.Scope.LIBRARY})
  public Map<String, LynxExtensionModule> getExtensionModules() {
    return mExtensionModules;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public void setEnableVSyncAligned(boolean enable) {
    mEnableVSyncAligned = enable;
  }
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public boolean getEnableVSyncAligned() {
    return mEnableVSyncAligned;
  }
}
