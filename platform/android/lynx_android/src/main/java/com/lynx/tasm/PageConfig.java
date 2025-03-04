// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static androidx.annotation.RestrictTo.Scope.LIBRARY;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;

/**
 * class hold part of config in c++ PageConfig
 * this class is mainly for usage inside sdk
 */
@RestrictTo(LIBRARY)
public class PageConfig {
  private static final String TAG = "PageConfig";
  private static final String KEY_AUTO_EXPOSE = "autoExpose";
  private static final String KEY_PAGE_VERSION = "pageVersion";
  private static final String KEY_EVENT_THROUGH = "enableEventThrough";
  private static final String KEY_DEFAULT_OVERFLOW_VISIBLE = "defaultOverflowVisible";
  private static final String KEY_ASYNC_REDIRECT = "asyncRedirect";
  private static final String KEY_SYNC_IMAGE_ATTACH = "syncImageAttach";
  private static final String KEY_ENABLE_CHECK_LOCAL_IMAGE = "enableCheckLocalImage";
  private static final String KEY_ENABLE_ASYNC_REQUEST_IMAGE = "enableAsyncRequestImage";
  private static final String KEY_USE_IMAGE_POST_PROCESSOR = "useImagePostProcessor";
  private static final String KEY_ENABLE_NEW_IMAGE = "enableNewImage";
  private static final String KEY_PAGE_TYPE = "pageType";
  private static final String KEY_CLI_VERSION = "cliVersion";
  private static final String KEY_CUSTOM_DATA = "customData";
  private static final String KEY_USE_NEW_SWIPER = "useNewSwiper";
  private static final String KEY_TARGET_SDK_VERSION = "targetSdkVersion";
  private static final String KEY_LEPUS_VERSION = "lepusVersion";
  private static final String KEY_ENABLE_LEPUS_NG = "enableLepusNG";
  private static final String KEY_RADON_MODE = "radonMode";
  private static final String KEY_TAP_SLOP = "tapSlop";
  private static final String KEY_ENABLE_CREATE_VIEW_ASYNC = "enableCreateViewAsync";
  private static final String KEY_ENABLE_VSYNC_ALIGNED_FLUSH = "enableVsyncAlignedFlush";
  private static final String KEY_CSS_ALIGN_WITH_LEGACY_W3C = "cssAlignWithLegacyW3c";
  private static final String KEY_ENABLE_ACCESSIBILITY_ELEMENT = "enableAccessibilityElement";
  private static final String KEY_ENABLE_OVERLAP_ACCESSIBILITY_ELEMENT =
      "enableOverlapForAccessibilityElement";
  private static final String KEY_ENABLE_NEW_ACCESSIBILITY = "enableNewAccessibility";
  private static final String KEY_ENABLE_A11Y_ID_MUTATION_OBSERVER = "enableA11yIDMutationObserver";
  private static final String KEY_ENABLE_A11Y = "enableA11y";
  private static final String KEY_REACT_VERSION = "reactVersion";
  private static final String KEY_ENABLE_TEXT_REFACTOR = "enableTextRefactor";
  private static final String KEY_ENABLE_TEXT_OVERFLOW = "enableTextOverflow";
  private static final String KEY_ENABLE_TEXT_BORING_LAYOUT = "enableTextBoringLayout";
  private static final String KEY_ENABLE_NEW_CLIP_MODE = "enableNewClipMode";
  private static final String KEY_KEYBOARD_CALLBACK_PASS_RELATIVE_HEIGHT =
      "keyboardCallbackPassRelativeHeight";
  private static final String KEY_ENABLE_CSS_PARSER = "enableCSSParser";
  private static final String KEY_ENABLE_EVENT_REFACTOR = "enableEventRefactor";
  private static final String KEY_ENABLE_DISEXPOSURE_WHEN_LYNX_HIDDEN =
      "enableDisexposureWhenLynxHidden";
  private static final String KEY_ENABLE_EXPOSURE_WHEN_LAYOUT = "enableExposureWhenLayout";
  private static final String KEY_INCLUDE_FONT_PADDING = "includeFontPadding";
  private static final String KEY_ENABLE_NEW_INTERSECTION_OBSERVER =
      "enableNewIntersectionObserver";
  private static final String KEY_OBSERVER_FRAME_RATE = "observerFrameRate";
  private static final String KEY_ENABLE_EXPOSURE_UI_MARGIN = "enableExposureUIMargin";
  private static final String KEY_LONG_PRESS_DURATION = "longPressDuration";
  private static final String KEY_INIT_ASYNC_TT_VIDEO_ENGINE = "enableAsyncInitVideoEngine";
  private static final String KEY_PAGE_FLATTEN = "pageFlatten";
  private static final String KEY_ENABLE_NEW_GESTURE = "enableNewGesture";
  private static final String KEY_USER = "user";
  private static final String KEY_GIT = "git";
  private static final String KEY_FILE_PATH = "filePath";
  private static final String KEY_ENABLE_FIBER = "enableFiber";
  private static final String KEY_ENABLE_MULTITOUCH = "enableMultiTouch";
  private static final String KEY_ENABLE_LYNX_SCROLL_FLUENCY = "enableLynxScrollFluency";
  private static final String KEY_ENABLE_FLATTEN_TRANSLATE_Z = "enableFlattenTranslateZ";
  private static final String KEY_MAP_CONTAINER_TYPE = "mapContainerType";

  private boolean autoExpose;
  private boolean enableEventThrough;
  private boolean defaultOverflowVisible;
  private String pageVersion;
  private boolean asyncRedirect;
  private boolean syncImageAttach;
  private boolean enableCheckLocalImage;
  private boolean enableAsyncRequestImage;
  private boolean useImagePostProcessor;
  private boolean enableLoadImageFromService;
  private String pageType;
  private String cliVersion;
  private String customData;
  private boolean useNewSwiper;
  private boolean mEnableAsyncInitTTVideoEngine;
  private String targetSdkVersion;
  private String lepusVersion;
  private boolean enableLepusNG;
  private String radonMode;
  private String mTapSlop;
  private boolean mEnableCreateViewAsync;
  private boolean mEnableVsyncAlignedFlush;
  private boolean mCssAlignWithLegacyW3c;
  private boolean mEnableAccessibilityElement;
  private boolean mEnableOverlapForAccessibilityElement;
  private boolean mEnableNewAccessibility;
  private boolean mEnableA11yIDMutationObserver;
  private boolean mEnableA11y;
  private String mReactVersion;
  private boolean mEnableTextRefactor;
  private boolean mEnableTextOverflow;
  private boolean mEnableTextBoringLayout;
  private boolean mEnableNewClipMode;
  private boolean mKeyboardCallbackUseRelativeHeight;
  private boolean mEnableCSSParser;
  private boolean mDefaultTextIncludePadding = false;
  private boolean mEnableEventRefactor = true;
  private boolean mEnableDisexposureWhenLynxHidden = true;
  private boolean mEnableExposureWhenLayout = false;
  private boolean mEnableFlattenTranslateZ = false;
  private boolean mEnableNewGesture = false;

  private boolean mEnableNewIntersectionObserver = false;
  private boolean mEnableFiber = false;
  private boolean mEnableMultiTouch = false;
  private int mObserverFrameRate = 20;
  private boolean mEnableExposureUIMargin = false;
  // when mLongPressDuration < 0, we think long-press-duration is not set
  private int mLongPressDuration = -1;
  private int mMapContainerType = 0; // 0-surface mapview 1-texture mapview
  private boolean mPageFlatten = true;
  private String mUser;
  private String mGit;
  private String mFilePath;
  private double mEnableLynxScrollFluency = -1d;

  public PageConfig(ReadableMap map) {
    autoExpose = true;
    enableEventThrough = false;
    pageVersion = "error";

    if (null != map) {
      if (map.hasKey(KEY_AUTO_EXPOSE)) {
        autoExpose = map.getBoolean(KEY_AUTO_EXPOSE);
      }
      if (map.hasKey(KEY_PAGE_VERSION)) {
        pageVersion = map.getString(KEY_PAGE_VERSION);
      }
      if (map.hasKey(KEY_EVENT_THROUGH)) {
        enableEventThrough = map.getBoolean(KEY_EVENT_THROUGH);
      }
      if (map.hasKey(KEY_DEFAULT_OVERFLOW_VISIBLE)) {
        defaultOverflowVisible = map.getBoolean(KEY_DEFAULT_OVERFLOW_VISIBLE);
      }

      if (map.hasKey(KEY_SYNC_IMAGE_ATTACH)) {
        syncImageAttach = map.getBoolean(KEY_SYNC_IMAGE_ATTACH);
      }

      if (map.hasKey(KEY_ENABLE_CHECK_LOCAL_IMAGE)) {
        enableCheckLocalImage = map.getBoolean(KEY_ENABLE_CHECK_LOCAL_IMAGE);
      }

      if (map.hasKey(KEY_ENABLE_ASYNC_REQUEST_IMAGE)) {
        enableAsyncRequestImage = map.getBoolean(KEY_ENABLE_ASYNC_REQUEST_IMAGE);
      }

      if (map.hasKey(KEY_USE_IMAGE_POST_PROCESSOR)) {
        useImagePostProcessor = map.getBoolean(KEY_USE_IMAGE_POST_PROCESSOR);
      }

      if (map.hasKey(KEY_ENABLE_NEW_IMAGE)) {
        enableLoadImageFromService = map.getBoolean(KEY_ENABLE_NEW_IMAGE);
      }

      if (map.hasKey(KEY_ASYNC_REDIRECT)) {
        asyncRedirect = map.getBoolean(KEY_ASYNC_REDIRECT);
      }
      if (map.hasKey(KEY_PAGE_TYPE)) {
        pageType = map.getString(KEY_PAGE_TYPE);
      }
      if (map.hasKey(KEY_CLI_VERSION)) {
        cliVersion = map.getString(KEY_CLI_VERSION);
      }
      if (map.hasKey(KEY_CUSTOM_DATA)) {
        customData = map.getString(KEY_CUSTOM_DATA);
      }
      if (map.hasKey(KEY_USE_NEW_SWIPER)) {
        useNewSwiper = map.getBoolean(KEY_USE_NEW_SWIPER);
      }
      if (map.hasKey(KEY_INIT_ASYNC_TT_VIDEO_ENGINE)) {
        mEnableAsyncInitTTVideoEngine = map.getBoolean(KEY_INIT_ASYNC_TT_VIDEO_ENGINE);
      }
      if (map.hasKey(KEY_TARGET_SDK_VERSION)) {
        targetSdkVersion = map.getString(KEY_TARGET_SDK_VERSION);
      }

      if (map.hasKey(KEY_ENABLE_FLATTEN_TRANSLATE_Z)) {
        mEnableFlattenTranslateZ = map.getBoolean(KEY_ENABLE_FLATTEN_TRANSLATE_Z);
      }

      if (map.hasKey(KEY_ENABLE_NEW_GESTURE)) {
        // page config enable new gesture
        mEnableNewGesture = map.getBoolean(KEY_ENABLE_NEW_GESTURE);
      }

      if (map.hasKey(KEY_INCLUDE_FONT_PADDING)) {
        // page config overwrite targetSdkVersion value
        mDefaultTextIncludePadding = map.getBoolean(KEY_INCLUDE_FONT_PADDING);
      }

      if (map.hasKey(KEY_LEPUS_VERSION)) {
        lepusVersion = map.getString(KEY_LEPUS_VERSION);
      }
      if (map.hasKey(KEY_ENABLE_LEPUS_NG)) {
        enableLepusNG = map.getBoolean(KEY_ENABLE_LEPUS_NG);
      }
      if (map.hasKey(KEY_RADON_MODE)) {
        radonMode = map.getString(KEY_RADON_MODE);
      }
      if (map.hasKey(KEY_TAP_SLOP)) {
        mTapSlop = map.getString(KEY_TAP_SLOP);
      }
      if (map.hasKey(KEY_ENABLE_CREATE_VIEW_ASYNC)) {
        mEnableCreateViewAsync = map.getBoolean(KEY_ENABLE_CREATE_VIEW_ASYNC);
      }
      if (map.hasKey(KEY_ENABLE_VSYNC_ALIGNED_FLUSH)) {
        mEnableVsyncAlignedFlush = map.getBoolean(KEY_ENABLE_VSYNC_ALIGNED_FLUSH);
      }

      if (map.hasKey(KEY_CSS_ALIGN_WITH_LEGACY_W3C)) {
        mCssAlignWithLegacyW3c = map.getBoolean(KEY_CSS_ALIGN_WITH_LEGACY_W3C);
      }

      if (map.hasKey(KEY_ENABLE_ACCESSIBILITY_ELEMENT)) {
        mEnableAccessibilityElement = map.getBoolean(KEY_ENABLE_ACCESSIBILITY_ELEMENT);
      }

      if (map.hasKey(KEY_ENABLE_OVERLAP_ACCESSIBILITY_ELEMENT)) {
        mEnableOverlapForAccessibilityElement =
            map.getBoolean(KEY_ENABLE_OVERLAP_ACCESSIBILITY_ELEMENT, true);
      }

      if (map.hasKey(KEY_ENABLE_NEW_ACCESSIBILITY)) {
        mEnableNewAccessibility = map.getBoolean(KEY_ENABLE_NEW_ACCESSIBILITY, false);
      }

      if (map.hasKey(KEY_ENABLE_A11Y_ID_MUTATION_OBSERVER)) {
        mEnableA11yIDMutationObserver = map.getBoolean(KEY_ENABLE_A11Y_ID_MUTATION_OBSERVER, false);
      }

      if (map.hasKey(KEY_ENABLE_A11Y)) {
        mEnableA11y = map.getBoolean(KEY_ENABLE_A11Y, false);
      }

      if (map.hasKey(KEY_REACT_VERSION)) {
        mReactVersion = map.getString(KEY_REACT_VERSION);
      }

      if (map.hasKey(KEY_ENABLE_TEXT_REFACTOR)) {
        mEnableTextRefactor = map.getBoolean(KEY_ENABLE_TEXT_REFACTOR);
      }

      if (map.hasKey(KEY_ENABLE_TEXT_OVERFLOW)) {
        mEnableTextOverflow = map.getBoolean(KEY_ENABLE_TEXT_OVERFLOW);
      }

      if (map.hasKey(KEY_ENABLE_TEXT_BORING_LAYOUT)) {
        mEnableTextBoringLayout = map.getBoolean(KEY_ENABLE_TEXT_BORING_LAYOUT);
      } else {
        mEnableTextBoringLayout = LynxEnv.inst().enableTextBoringLayout();
      }

      if (map.hasKey(KEY_ENABLE_NEW_CLIP_MODE)) {
        mEnableNewClipMode = map.getBoolean(KEY_ENABLE_NEW_CLIP_MODE);
      }

      if (map.hasKey(KEY_KEYBOARD_CALLBACK_PASS_RELATIVE_HEIGHT)) {
        mKeyboardCallbackUseRelativeHeight =
            map.getBoolean(KEY_KEYBOARD_CALLBACK_PASS_RELATIVE_HEIGHT);
      }

      if (map.hasKey(KEY_ENABLE_CSS_PARSER)) {
        mEnableCSSParser = map.getBoolean(KEY_ENABLE_CSS_PARSER);
      }

      if (map.hasKey(KEY_ENABLE_EVENT_REFACTOR)) {
        mEnableEventRefactor = map.getBoolean(KEY_ENABLE_EVENT_REFACTOR);
      }

      if (map.hasKey(KEY_ENABLE_DISEXPOSURE_WHEN_LYNX_HIDDEN)) {
        mEnableDisexposureWhenLynxHidden = map.getBoolean(KEY_ENABLE_DISEXPOSURE_WHEN_LYNX_HIDDEN);
      }

      if (map.hasKey(KEY_ENABLE_EXPOSURE_WHEN_LAYOUT)) {
        mEnableExposureWhenLayout = map.getBoolean(KEY_ENABLE_EXPOSURE_WHEN_LAYOUT, false);
      }

      if (map.hasKey(KEY_ENABLE_NEW_INTERSECTION_OBSERVER)) {
        mEnableNewIntersectionObserver = map.getBoolean(KEY_ENABLE_NEW_INTERSECTION_OBSERVER);
      }

      if (map.hasKey(KEY_OBSERVER_FRAME_RATE)) {
        mObserverFrameRate = map.getInt(KEY_OBSERVER_FRAME_RATE);
      }

      if (map.hasKey(KEY_ENABLE_EXPOSURE_UI_MARGIN)) {
        mEnableExposureUIMargin = map.getBoolean(KEY_ENABLE_EXPOSURE_UI_MARGIN);
      }

      if (map.hasKey(KEY_LONG_PRESS_DURATION)) {
        mLongPressDuration = map.getInt(KEY_LONG_PRESS_DURATION);
      }

      if (map.hasKey(KEY_MAP_CONTAINER_TYPE)) {
        mMapContainerType = map.getInt(KEY_MAP_CONTAINER_TYPE);
      }

      if (map.hasKey(KEY_PAGE_FLATTEN)) {
        mPageFlatten = map.getBoolean(KEY_PAGE_FLATTEN);
      }

      if (map.hasKey(KEY_ENABLE_NEW_GESTURE)) {
        mEnableNewGesture = map.getBoolean(KEY_ENABLE_NEW_GESTURE);
      }

      if (map.hasKey(KEY_USER)) {
        mUser = map.getString(KEY_USER);
      }

      if (map.hasKey(KEY_GIT)) {
        mGit = map.getString(KEY_GIT);
      }

      if (map.hasKey(KEY_FILE_PATH)) {
        mFilePath = map.getString(KEY_FILE_PATH);
      }

      if (map.hasKey(KEY_ENABLE_FIBER)) {
        mEnableFiber = map.getBoolean(KEY_ENABLE_FIBER);
      }

      if (map.hasKey(KEY_ENABLE_MULTITOUCH)) {
        mEnableMultiTouch = map.getBoolean(KEY_ENABLE_MULTITOUCH);
      }

      if (map.hasKey(KEY_ENABLE_LYNX_SCROLL_FLUENCY)) {
        mEnableLynxScrollFluency = map.getDouble(KEY_ENABLE_LYNX_SCROLL_FLUENCY);
      }
    }
  }

  public boolean isCssAlignWithLegacyW3c() {
    return mCssAlignWithLegacyW3c;
  }

  public void setCssAlignWithLegacyW3c(boolean mCssAlignWithLegacyW3c) {
    this.mCssAlignWithLegacyW3c = mCssAlignWithLegacyW3c;
  }

  public boolean isAutoExpose() {
    return autoExpose;
  }

  public boolean enableEventThrough() {
    return enableEventThrough;
  }

  public String getPageVersion() {
    return pageVersion;
  }

  public boolean getDefaultOverflowVisible() {
    return defaultOverflowVisible;
  }

  public boolean isAsyncRedirect() {
    return asyncRedirect;
  }

  public boolean isSyncImageAttach() {
    return syncImageAttach;
  }

  public boolean isEnableCheckLocalImage() {
    return enableCheckLocalImage;
  }

  public boolean isEnableAsyncRequestImage() {
    return enableAsyncRequestImage;
  }

  public boolean isUseImagePostProcessor() {
    return useImagePostProcessor;
  }

  public boolean isEnableLoadImageFromService() {
    return enableLoadImageFromService;
  }

  public boolean isUseNewSwiper() {
    return useNewSwiper;
  }

  public boolean isAsyncInitTTVideoEngine() {
    return mEnableAsyncInitTTVideoEngine;
  }

  public String getPageType() {
    return pageType;
  }

  public String getCliVersion() {
    return cliVersion;
  }

  public String getCustomData() {
    return customData;
  }

  public String getTargetSdkVersion() {
    return targetSdkVersion;
  }

  public String getLepusVersion() {
    return lepusVersion;
  }

  public boolean isEnableLepusNG() {
    return enableLepusNG;
  }

  public String getRadonMode() {
    return radonMode;
  }

  public String getTapSlop() {
    return mTapSlop;
  }

  public boolean getEnableCreateViewAsync() {
    return mEnableCreateViewAsync;
  }

  public boolean getEnableVsyncAlignedFlush() {
    return mEnableVsyncAlignedFlush;
  }

  public boolean getEnableAccessibilityElement() {
    return mEnableAccessibilityElement;
  }

  public boolean getEnableOverlapForAccessibilityElement() {
    return mEnableOverlapForAccessibilityElement;
  }

  public boolean getEnableNewAccessibility() {
    return mEnableNewAccessibility;
  }

  public boolean getEnableA11yIDMutationObserver() {
    return mEnableA11yIDMutationObserver;
  }

  public boolean getEnableA11y() {
    return mEnableA11y;
  }

  public String getReactVersion() {
    return mReactVersion;
  }

  public boolean isTextRefactorEnabled() {
    return mEnableTextRefactor;
  }

  public boolean isTextOverflowEnabled() {
    return mEnableTextOverflow;
  }

  public boolean isTextBoringLayoutEnabled() {
    return mEnableTextBoringLayout;
  }

  public boolean isNewClipModeEnabled() {
    return mEnableNewClipMode;
  }

  public boolean useRelativeKeyboardHeightApi() {
    return mKeyboardCallbackUseRelativeHeight;
  }

  public boolean isCSSParserEnabled() {
    return mEnableCSSParser;
  }

  public boolean getDefaultTextIncludePadding() {
    return mDefaultTextIncludePadding;
  }

  public boolean getEnableFlattenTranslateZ() {
    return mEnableFlattenTranslateZ;
  }

  public boolean isEnableNewGesture() {
    return mEnableNewGesture;
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

  public boolean isPageFlatten() {
    return mPageFlatten;
  }

  public String getUser() {
    return mUser;
  }

  public String getGit() {
    return mGit;
  }

  public String getFilePath() {
    return mFilePath;
  }

  public boolean getEnableFiberArc() {
    return mEnableFiber;
  }

  public boolean getEnableMultiTouch() {
    return mEnableMultiTouch;
  }

  public double getEnableLynxScrollFluency() {
    return mEnableLynxScrollFluency;
  }

  @Override
  public String toString() {
    return "PageConfig{"
        + "autoExpose=" + autoExpose + ", pageVersion='" + pageVersion + '}';
  }
}
