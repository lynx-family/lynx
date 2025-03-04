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
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.ListNodeInfoFetcher;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIExposure;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.core.JSProxy;
import com.lynx.tasm.fluency.FluencyTraceHelper;
import com.lynx.tasm.fontface.FontFace;
import com.lynx.tasm.image.model.LynxImageFetcher;
import com.lynx.tasm.loader.LynxFontFaceLoader;
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
  private ListNodeInfoFetcher mListNodeInfoFetcher;
  private WeakReference<JSProxy> mJSProxy;
  private UIBody mUIBody;
  private Map<String, FontFace> mParsedFontFace;
  private WeakReference<LynxUIOwner> mLynxUIOwner;
  private WeakReference<LynxIntersectionObserverManager> mIntersectionObserverManager;
  private String mTemplateUrl = null;
  private String mJSGroupThreadName = null;
  private Map<String, Object> mSharedData;
  private LynxViewClient mLynxViewClient = null;
  private WeakReference<LynxView> mLynxView = null;
  private WeakReference<ShadowNodeOwner> mShadowNodeOwnerRef;
  private DisplayMetrics mVirtualScreenMetrics;
  private Boolean mEnableAsyncLoadImage;
  private boolean mDefaultOverflowVisible = false;
  private boolean mAsyncInitTTVideoEngine = false;
  private boolean mAsyncRedirect = false;
  private LynxProviderRegistry providerRegistry;
  private LynxResourceFetcher mGenericResourceFetcher;
  private LynxFontFaceLoader.Loader fontLoader;
  private List<PatchFinishListener> mPatchFinishListeners;
  private boolean mEnableTextRefactor = false;
  private boolean mEnableTextOverflow = false;
  private boolean mEnableTextBoringLayout = false;
  private boolean mEnableNewClipMode = false;
  private UIExposure mExposure = null;
  private boolean mUseRelativeKeyboardHeightApi = false;
  // If the targetSdkVersion >= 2.4, the default value should be true
  private boolean mDefaultTextIncludePadding = false;
  // If the targetSdkVersion >= 2.5, the default value should be true
  private boolean mEnableEventRefactor = true;
  // The switch controlling whether to enable send disexposure events when lynxview is hidden.
  private boolean mEnableDisexposureWhenLynxHidden = true;
  // The switch controlling whether to enable exposure check when lynxview is isLayoutRequested.
  // If mEnableExposureWhenLayout == true, execute exposure check when lynxview is
  // isLayoutRequested. Otherwise, do not execute exposure check.
  private boolean mEnableExposureWhenLayout = false;
  // for fresco monitor
  private Object mFrescoCallerContext = null;
  // for fresco disk cache choice
  private boolean mEnableImageSmallDiskCache = false;
  // If the targetSdkVersion >= 2.5, the default value should be true
  private boolean mEnableFlattenTranslateZ = false;
  // The default value is false
  private boolean mEnableEventThrough = false;
  // The default value is false
  private boolean mEnableNewIntersectionObserver = false;
  // defalut value is 20fps
  private int mObserverFrameRate = 20;
  // The default value is false
  private boolean mEnableExposureUIMargin = false;
  // default value is true
  private boolean mSyncImageAttach = true;

  private boolean mPrefetchImageOnCreate = false;
  // default value is true
  private boolean mEnableCheckLocalImage = true;
  // default value is false
  private boolean mEnableAsyncRequestImage = false;
  // default value is false
  private boolean mEnableAsyncImageCallback = false;
  // default value is false
  private boolean mUseImagePostProcessor = false;
  // default value is false
  private boolean mEnableNewGesture = false;

  // default value is true
  private boolean mEnableLoadImageFromService = true;

  private boolean mEnableImageResourceHint = false;

  // when mLongPressDuration < 0, we think long-press-duration is not set
  private int mLongPressDuration = -1;

  private int mMapContainerType = 0;

  private boolean mEnableFiberArc = false;
  private boolean mCssAlignWithLegacyW3c = false;

  private HashMap<String, Object> mContextData;

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

  public LynxContext(Context base, DisplayMetrics screenMetrics) {
    super(base);
    mVirtualScreenMetrics = new DisplayMetrics();
    mVirtualScreenMetrics.setTo(screenMetrics);

    initUIExposure();
  }

  public void setEnableAsyncLoadImage(boolean b) {
    this.mEnableAsyncLoadImage = b;
  }

  public boolean isEnableAsyncLoadImage() {
    return mEnableAsyncLoadImage != null && mEnableAsyncLoadImage;
  }

  public void onPageConfigDecoded(PageConfig config) {
    mDefaultOverflowVisible = config.getDefaultOverflowVisible();
    mEnableTextRefactor = config.isTextRefactorEnabled();
    mEnableTextOverflow = config.isTextOverflowEnabled();
    mEnableTextBoringLayout = config.isTextBoringLayoutEnabled();
    mEnableNewClipMode = config.isNewClipModeEnabled();
    mUseRelativeKeyboardHeightApi = config.useRelativeKeyboardHeightApi();
    mAsyncRedirect = config.isAsyncRedirect();
    mSyncImageAttach = config.isSyncImageAttach();
    mEnableCheckLocalImage = config.isEnableCheckLocalImage();
    mEnableAsyncRequestImage = config.isEnableAsyncRequestImage();
    mUseImagePostProcessor = config.isUseImagePostProcessor();
    mEnableLoadImageFromService = config.isEnableLoadImageFromService();
    mDefaultTextIncludePadding = config.getDefaultTextIncludePadding();
    mEnableEventRefactor = config.getEnableEventRefactor();
    mEnableDisexposureWhenLynxHidden = config.getEnableDisexposureWhenLynxHidden();
    mEnableExposureWhenLayout = config.getEnableExposureWhenLayout();
    mEnableFlattenTranslateZ = config.getEnableFlattenTranslateZ();
    mEnableEventThrough = config.enableEventThrough();
    mEnableNewIntersectionObserver = config.getEnableNewIntersectionObserver();
    mEnableNewGesture = config.isEnableNewGesture();
    mObserverFrameRate = config.getObserverFrameRate();
    mEnableExposureUIMargin = config.getEnableExposureUIMargin();
    mAsyncInitTTVideoEngine = config.isAsyncInitTTVideoEngine();
    mLongPressDuration = config.getLongPressDuration();
    mMapContainerType = config.getMapContainerType();
    mEnableFiberArc = config.getEnableFiberArc();
    mCssAlignWithLegacyW3c = config.isCssAlignWithLegacyW3c();
  }

  public boolean isAsyncRedirect() {
    return mAsyncRedirect;
  }

  public boolean isSyncImageAttach() {
    return mSyncImageAttach;
  }

  public boolean isPrefetchImageOnCreate() {
    return mPrefetchImageOnCreate;
  }

  public boolean isEnableCheckLocalImage() {
    return mEnableCheckLocalImage;
  }

  public boolean isEnableAsyncRequestImage() {
    return mEnableAsyncRequestImage;
  }

  public boolean isEnableAsyncImageCallback() {
    return mEnableAsyncImageCallback;
  }

  public boolean isForceImageAsyncRequest() {
    return mForceImageAsyncRequest;
  }

  public boolean isUseImagePostProcessor() {
    return mUseImagePostProcessor;
  }

  public boolean getEnableLoadImageFromService() {
    return mEnableLoadImageFromService;
  }

  public boolean getDefaultOverflowVisible() {
    return mDefaultOverflowVisible;
  }

  public boolean isAsyncInitTTVideoEngine() {
    return mAsyncInitTTVideoEngine;
  }

  public boolean getEnableFiberArch() {
    return mEnableFiberArc;
  }

  public boolean getCssAlignWithLegacyW3c() {
    return mCssAlignWithLegacyW3c;
  }

  public boolean isEnableNewGesture() {
    return mEnableNewGesture;
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

  private void updateLynxSessionID(LynxView lynxView) {
    TraceEvent.beginSection("LynxContext.updateLynxSessionID");
    String currentTimestamp = String.valueOf(System.currentTimeMillis());
    String lynxViewIdentify = String.valueOf(System.identityHashCode(lynxView));
    // sessionID would be like "$currentTimestamp-$lynxViewIdentify"
    // Use StringBuilder to replace String format for String format is significantly slower than
    // StringBuilder in this case.
    StringBuilder builder = new StringBuilder();
    builder.append(currentTimestamp);
    builder.append("-");
    builder.append(lynxViewIdentify);
    mLynxSessionId = builder.toString();
    TraceEvent.endSection("LynxContext.updateLynxSessionID");
  }

  public void setLynxView(LynxView lynxview) {
    setHasLynxViewAttached(lynxview != null);
    this.mLynxView = new WeakReference<>(lynxview);
    // Prevent generate lynxSessionID multiple times
    if (TextUtils.isEmpty(mLynxSessionId)) {
      updateLynxSessionID(lynxview);
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
    if (mLynxView == null) {
      return null;
    }
    return mLynxView.get();
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

  public ListNodeInfoFetcher getListNodeInfoFetcher() {
    return mListNodeInfoFetcher;
  }

  public void setListNodeInfoFetcher(ListNodeInfoFetcher listNodeInfoFetcher) {
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
    mExposure.setRootUI(owner.getRootUI());
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

  public LynxBaseInspectorOwner getBaseInspectorOwner() {
    return mLynxView.get() != null ? mLynxView.get().getBaseInspectorOwner() : null;
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
    LynxView lynxView = mLynxView.get();

    if (lynxView != null) {
      lynxView.runOnTasmThread(runnable);
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
    return mEnableTextRefactor;
  }

  public boolean isNewClipModeEnabled() {
    return mEnableNewClipMode;
  }

  public boolean isTextOverflowEnabled() {
    return mEnableTextOverflow;
  }

  public boolean isTextBoringLayoutEnabled() {
    return mEnableTextBoringLayout;
  }

  public boolean useRelativeKeyboardHeightApi() {
    return mUseRelativeKeyboardHeightApi;
  }

  public boolean getDefaultTextIncludePadding() {
    return mDefaultTextIncludePadding;
  }

  public boolean getEnableEventRefactor() {
    return mEnableEventRefactor;
  }

  public boolean getEnableDisexposureWhenLynxHidden() {
    return mEnableDisexposureWhenLynxHidden;
  }

  public boolean getEnableExposureWhenLayout() {
    return mEnableExposureWhenLayout;
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

    if (mEnableNewIntersectionObserver && mIntersectionObserverManager != null) {
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

    if (mEnableNewIntersectionObserver && mIntersectionObserverManager != null) {
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
    if (mExposure == null) {
      LLog.e(TAG, "addUIToExposedMap failed, since mExposure is null");
      return;
    }
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
      JavaOnlyMap customOption = new JavaOnlyMap();
      customOption.put("sendCustom", true);
      mExposure.addUIToExposedMap(ui, String.valueOf(ui.getSign()), null, customOption);
    }

    // Native components can call this interface to reuse the exposure capability.
    mExposure.addUIToExposedMap(ui, uniqueID, data, options);
  }

  public void removeUIFromExposedMap(LynxBaseUI ui) {
    if (mExposure == null) {
      LLog.e(TAG, "removeUIFromExposedMap failed, since mExposure is null");
      return;
    }
    removeUIFromExposedMap(ui, null);
  }

  public void removeUIFromExposedMap(LynxBaseUI ui, @Nullable String uniqueID) {
    // Since the exposure node using the custom event method can also set the exposure-related
    // properties, we choose to check whether the node is bound to the relevant event here to avoid
    // modifying it in multiple places.
    if (uniqueID == null && ui.getEvents() != null
        && (ui.getEvents().containsKey(UIAPPEAREVENT)
            || ui.getEvents().containsKey(UIDISAPPEAREVENT))) {
      mExposure.removeUIFromExposedMap(ui, String.valueOf(ui.getSign()));
    }

    // Native components can call this interface to reuse the exposure capability.
    mExposure.removeUIFromExposedMap(ui, uniqueID);
  }

  public void onRootViewDraw(Canvas canvas) {
    if (mExposure != null) {
      mExposure.onRootViewDraw(canvas);
    }
    if (mEnableNewIntersectionObserver && mIntersectionObserverManager != null) {
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
    return mEnableFlattenTranslateZ;
  }

  public boolean enableEventThrough() {
    return mEnableEventThrough;
  }

  public boolean getEnableNewIntersectionObserver() {
    return mEnableNewIntersectionObserver;
  }

  public int getObserverFrameRate() {
    return mObserverFrameRate;
  }

  public boolean getEnableExposureUIMargin() {
    return mEnableExposureUIMargin;
  }

  public int getLongPressDuration() {
    return mLongPressDuration;
  }

  public int getMapContainerType() {
    return mMapContainerType;
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

  public HashMap getContextData() {
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

  private void initUIExposure() {
    mExposure = new UIExposure();

    final WeakReference<LynxContext> weakContext = new WeakReference<>(this);
    mExposure.setCallback(new UIExposure.ICallBack() {
      final WeakReference<LynxContext> mWeakContext = weakContext;

      @Override
      public boolean canSendGlobalEvent() {
        LynxContext context = mWeakContext.get();
        if (context == null) {
          LLog.e(TAG, "canSendGlobalEvent check failed since can not get LynxContext.");
          return true;
        }
        LynxView view = context.getLynxView();
        if (view == null) {
          LLog.e(TAG, "canSendGlobalEvent check failed since can not get LynxView.");
          return true;
        }
        return view.enableJSRuntime() || view.enableAirStrictMode();
      }

      @Override
      public void sendGlobalEvent(String name, JavaOnlyArray params) {
        LynxContext context = mWeakContext.get();
        if (context == null) {
          LLog.e(TAG, "sendGlobalEvent failed since can not get LynxContext.");
          return;
        }
        LynxView view = context.getLynxView();
        if (view == null) {
          LLog.e(TAG, "sendGlobalEvent failed since can not get LynxView.");
          return;
        }
        view.sendGlobalEvent(name, params);
      }

      @Override
      public LynxBaseUI findNode(int sign) {
        LynxContext context = mWeakContext.get();
        if (context == null) {
          LLog.e(TAG, "findNode failed since can not get LynxContext.");
          return null;
        }
        LynxUIOwner owner = context.getLynxUIOwner();
        if (owner == null) {
          LLog.e(TAG, "findNode failed since can not get LynxUIOwner.");
          return null;
        }
        return context.getLynxUIOwner().getNode(sign);
      }
    });
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
}
