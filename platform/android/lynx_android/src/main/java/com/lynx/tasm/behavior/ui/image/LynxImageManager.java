// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Looper;
import android.text.TextUtils;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.image.AutoSizeImage;
import com.lynx.tasm.image.ImageErrorCodeUtils;
import com.lynx.tasm.image.ImageUtils;
import com.lynx.tasm.image.LynxImageMediaFetcherProxy;
import com.lynx.tasm.image.LynxScaleTypeDrawable;
import com.lynx.tasm.image.ScalingUtils;
import com.lynx.tasm.image.model.AnimationListener;
import com.lynx.tasm.image.model.BitmapPostProcessor;
import com.lynx.tasm.image.model.DiskCacheChoice;
import com.lynx.tasm.image.model.ImageBlurPostProcessor;
import com.lynx.tasm.image.model.ImageInfo;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.image.model.ImageRequestInfoBuilder;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.media.OptionalBool;
import com.lynx.tasm.utils.ColorUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import com.lynx.tasm.utils.UnitUtils;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxImageManager implements Drawable.Callback {
  public static final String TAG = "LynxImageManager";

  private long dirtyFlags = 0;

  private static final long SRC_CHANGED = 1 << 1;

  private static final long PLACEHOLDER_CHANGED = (1 << 2);

  private static final long CAP_INSETS_CHANGED = (1 << 3);

  private static final long MODE_CHANGED = (1 << 4);

  private static final long BLUR_RADIUS_CHANGED = (1 << 5);

  private static final long AUTO_SIZE_CHANGED = (1 << 6);

  private static final long TINT_COLOR_CHANGED = (1 << 7);

  private static final long ASYNC_REQUEST_CHANGED = (1 << 8);

  private static final long LAYOUT_CHANGED = (1 << 9);

  private static final long BORDER_CHANGED = (1 << 10);

  private static final long DOWN_SAMPLING_SCALE_CHANGED = (1 << 11);

  private static final long BORDER_RADIUS_CHANGED = (1 << 12);

  private final LynxContext mContext;

  private final LynxImageLoader mImageLoader;

  private String mSrc;

  private String mPlaceholder;

  private boolean mSkipRedirection;

  private boolean mDeferInvalidation;

  private boolean mDisableDefaultPlaceholder;

  private boolean mEnableResourceHint;

  private boolean mEnableCustomGifDecoder;

  private LynxScaleTypeDrawable mImageDrawable;

  private LynxScaleTypeDrawable mPlaceholderDrawable;

  private String mCapInsets;
  private String mCapInsetsScale;

  private ScalingUtils.ScaleType mMode = ScalingUtils.ScaleType.FIT_XY;

  private String mBlurRadius;

  private boolean mAutoSize;

  private int mLoopCount = 0;

  private Bitmap.Config mBitmapConfig;

  private boolean mAutoPlay = true;

  private String mTintColor;

  private boolean mEnableExtraLoadInfo;

  private boolean mEnableAsyncRequest;

  private int mPaddingLeft;

  private int mPaddingTop;

  private int mPaddingRight;

  private int mPaddingBottom;

  private int mViewWidth;

  private int mViewHeight;

  private int mPreFetchWidth;

  private int mPreFetchHeight;

  private boolean mAwaitLocalCache;

  private boolean mUseLocalCache;

  private boolean mDisableDefaultResize;

  private ImageRequestInfo mCurImageRequest;

  private ImageRequestInfo mCurPlaceholderRequest;

  private float mBorderLeftWidth;

  private float mBorderRightWidth;

  private float mBorderTopWidth;

  private float mBorderBottomWidth;

  private float[] mBorderRadius;

  private RectF mBorderWidthRect = new RectF(0, 0, 0, 0);

  private int mImageWidth;

  private int mImageHeight;

  private ShadowNode mAutoSizeShadowNode = null;

  private boolean mNeedRetryAutoSize;

  private LynxBaseUI mUI;

  private final boolean mAsyncRedirect;

  private final LynxMediaResourceFetcher mMediaResourceFetcher;

  private boolean mEnableCheckLocalImage = false;

  private boolean mEnableStartPlayEvent;
  private boolean mEnableCurrentLoopEvent;
  private boolean mEnableAllLoopEvent;
  private boolean mEnableOnLoad;

  private boolean mEnableOnError;

  private OptionalBool mSrcRedirectCheckResult = OptionalBool.UNDEFINED;
  private OptionalBool mPlaceHolderRedirectCheckResult = OptionalBool.UNDEFINED;

  private final String EVENT_START_PLAY = "startplay";

  private final String EVENT_CURRENT_LOOP_COMPLETE = "currentloopcomplete";

  private final String EVENT_ALL_LOOP_COMPLETE = "finalloopcomplete";

  public static final String EVENT_LOAD = "load";

  public static final String EVENT_ERROR = "error";
  private ColorFilter mColorFilter = null;

  private ImageRequestInfo mPreImageRequestInfo = null;

  private @Nullable BackgroundDrawable.RoundRectPath mInnerClipPathForBorderRadius;

  private boolean mIsPixelated = false;

  private final ImageLoadListener mSrcLoadListener = new ImageLoadListener() {
    @Override
    public void onRequestSubmit(ImageRequestInfo imageRequestInfo) {}

    @Override
    public void onSuccess(
        @Nullable Drawable drawable, ImageRequestInfo requestInfo, ImageInfo imageInfo) {
      if (!TextUtils.equals(requestInfo.getUrl(), mCurImageRequest.getUrl())) {
        return;
      }
      if (drawable != null) {
        if (mIsPixelated) {
          drawable.setFilterBitmap(false);
        }
        if (imageInfo.isAnim()) {
          drawable.setCallback(LynxImageManager.this);
        }
      }
      if (mDeferInvalidation) {
        releaseImage(mPreImageRequestInfo);
        releaseDrawable(mImageDrawable);
        mPreImageRequestInfo = null;
        mImageDrawable = null;
      }
      mImageDrawable = new LynxScaleTypeDrawable(drawable, mMode);
      if (!TextUtils.isEmpty(mCapInsets)) {
        mImageDrawable.setCapInsets(mCapInsets, mCapInsetsScale);
      }
      mImageWidth = imageInfo.getWidth();
      mImageHeight = imageInfo.getHeight();
      justSizeIfNeeded();
      if (mColorFilter != null) {
        mImageDrawable.setColorFilter(mColorFilter);
      }
      configureBounds(mImageDrawable);
      onImageLoadSuccess(mImageWidth, mImageHeight);
      invalidate();
    }

    @Override
    public void onFailure(int errorCode, Throwable throwable) {
      LLog.e(TAG, "onFailed src:" + mSrc + ",with reason:" + throwable.getMessage());
      final int categoryCode = ImageErrorCodeUtils.checkImageExceptionCategory(errorCode);
      String error = throwable.getMessage();
      if (TextUtils.isEmpty(error)) {
        error = Log.getStackTraceString(throwable);
        if (error != null && error.length() > 200) {
          error = error.substring(0, 200);
        }
      }
      LynxError lynxError = new LynxError(
          categoryCode, "Android LynxImageManager loading image failed", "", LynxError.LEVEL_ERROR);
      lynxError.setRootCause(error);
      onImageLoadError(lynxError, categoryCode, errorCode);
    }

    @Override
    public void onImageMonitorInfo(JSONObject monitorInfo) {
      if (mEnableExtraLoadInfo) {
        sendExtraLoadEvent(monitorInfo);
      }
    }
  };

  private final ImageLoadListener mPlaceHolderListener = new ImageLoadListener() {
    @Override
    public void onRequestSubmit(ImageRequestInfo imageRequestInfo) {}

    @Override
    public void onSuccess(
        @Nullable Drawable drawable, ImageRequestInfo requestInfo, ImageInfo imageInfo) {
      if (!TextUtils.equals(requestInfo.getUrl(), mCurPlaceholderRequest.getUrl())) {
        return;
      }
      mPlaceholderDrawable = new LynxScaleTypeDrawable(drawable, mMode);
      configureBounds(mPlaceholderDrawable);
      invalidate();
    }

    @Override
    public void onFailure(int errorCode, Throwable throwable) {}

    @Override
    public void onImageMonitorInfo(JSONObject monitorInfo) {}
  };

  private final AnimationListener mAnimationListener = new AnimationListener() {
    @Override
    public void onAnimationStart(Drawable drawable) {
      if (mEnableStartPlayEvent) {
        sendCustomEvent(EVENT_START_PLAY);
      }
    }

    @Override
    public void onAnimationFinalLoop(Drawable drawable) {
      if (mEnableAllLoopEvent) {
        // It needs to send EVENT_CURRENT_LOOP_COMPLETE to ensure that both ends are consistent
        sendCustomEvent(EVENT_ALL_LOOP_COMPLETE);
      }
    }

    @Override
    public void onAnimationCurrentLoop(Drawable drawable) {
      if (mEnableCurrentLoopEvent) {
        sendCustomEvent(EVENT_CURRENT_LOOP_COMPLETE);
      }
    }
  };
  private Rect mDrawableBounds;

  public LynxImageManager(LynxContext context) {
    mContext = context;
    mImageLoader = new LynxImageLoader(mContext.getImageFetcher());
    mMediaResourceFetcher = new LynxImageMediaFetcherProxy(mContext);
    mAsyncRedirect =
        (mContext.getMediaResourceFetcher() != null || mContext.getAsyncImageInterceptor() != null);
    mEnableCheckLocalImage = mContext.isEnableCheckLocalImage();
  }

  // region setProps
  public void setSkipRedirection(boolean skipRedirection) {
    mSkipRedirection = skipRedirection;
  }

  public void setDeferInvalidation(boolean defer) {
    mDeferInvalidation = defer;
  }

  public void setDisableDefaultPlaceholder(boolean disable) {
    mDisableDefaultPlaceholder = disable;
  }

  public void setEnableResourceHint(boolean enable) {
    mEnableResourceHint = enable;
  }

  public void setEnableCustomGifDecoder(boolean enable) {
    mEnableCustomGifDecoder = enable;
  }

  public void setSrc(String src) {
    if (!TextUtils.equals(src, mSrc)) {
      mSrc = src;
      dirtyFlags |= SRC_CHANGED;
    }
  }

  public void setPlaceholder(String placeholder) {
    if (!TextUtils.equals(placeholder, mPlaceholder)) {
      mPlaceholder = placeholder;
      dirtyFlags |= PLACEHOLDER_CHANGED;
    }
  }

  public void setCapInsets(String capInsets) {
    if (!TextUtils.equals(capInsets, mCapInsets)) {
      mCapInsets = capInsets;
      dirtyFlags |= CAP_INSETS_CHANGED;
    }
  }

  public void setCapInsetsBackUp(String capInsetsBackUp) {
    if (!TextUtils.equals(capInsetsBackUp, mCapInsets)) {
      mCapInsets = capInsetsBackUp;
      dirtyFlags |= CAP_INSETS_CHANGED;
    }
  }

  public void setMode(@Nullable String mode) {
    ScalingUtils.ScaleType scaleType = getMode(mode);
    if (!Objects.equals(scaleType, mMode)) {
      mMode = scaleType;
      dirtyFlags |= MODE_CHANGED;
    }
  }

  public void setBlurRadius(String radius) {
    if (!TextUtils.equals(radius, mBlurRadius)) {
      mBlurRadius = radius;
      dirtyFlags |= BLUR_RADIUS_CHANGED;
    }
  }

  public void setAutoSize(boolean autoSize) {
    if (autoSize != mAutoSize) {
      mAutoSize = autoSize;
      dirtyFlags |= AUTO_SIZE_CHANGED;
    }
  }

  public void setLoopCount(int count) {
    if (count <= 0) {
      count = 0;
    }
    mLoopCount = count;
  }

  public void setImageConfig(Bitmap.Config config) {
    mBitmapConfig = config;
  }

  public void setAutoPlay(boolean autoPlay) {
    mAutoPlay = autoPlay;
  }

  public void setTintColor(String color) {
    if (!TextUtils.equals(color, mTintColor)) {
      mTintColor = color;
      if (ColorUtils.isValid(mTintColor)) {
        mColorFilter =
            new PorterDuffColorFilter(ColorUtils.parse(mTintColor), PorterDuff.Mode.SRC_IN);
      }
      dirtyFlags |= TINT_COLOR_CHANGED;
    }
  }

  public void setExtraLoadInfo(boolean enable) {
    mEnableExtraLoadInfo = enable;
  }

  public void setAsyncRequest(boolean enable) {
    if (mEnableAsyncRequest != enable) {
      mEnableAsyncRequest = enable;
      dirtyFlags |= ASYNC_REQUEST_CHANGED;
    }
  }

  public void setPreFetchWidth(String width) {
    mPreFetchWidth =
        (int) UnitUtils.toPxWithDisplayMetrics(width, 0, 0, 0, 0, -1, mContext.getScreenMetrics());
  }

  public void setCapInsetsScale(String capInsetsScale) {
    if (!TextUtils.equals(capInsetsScale, mCapInsetsScale)) {
      mCapInsetsScale = capInsetsScale;
      dirtyFlags |= CAP_INSETS_CHANGED;
    }
  }

  public void setPreFetchHeight(String height) {
    mPreFetchHeight =
        (int) UnitUtils.toPxWithDisplayMetrics(height, 0, 0, 0, 0, -1, mContext.getScreenMetrics());
  }

  public void setLocalCache(@Nullable Dynamic localCache) {
    ImageUtils.LocalCacheState state = ImageUtils.parseLocalCache(localCache);
    mUseLocalCache = state.mUseLocalCache;
    mAwaitLocalCache = state.mAwaitLocalCache;
  }
  public void setDisableDefaultResize(boolean disable) {
    mDisableDefaultResize = disable;
  }

  public void setLynxBaseUI(LynxBaseUI ui) {
    mUI = ui;
  }

  void setIsPixelated(boolean isPixelated) {
    this.mIsPixelated = isPixelated;
  }

  // endregion

  // region UIMethod
  public void updatePropertiesInterval(StylesDiffMap props) {
    String traceEvent = "updatePropertiesInterval";
    beginTrace(traceEvent);
    ReadableMapKeySetIterator iterator = props.mBackingMap.keySetIterator();
    while (iterator.hasNextKey()) {
      String name = iterator.nextKey();
      switch (name) {
        case PropsConstants.ENABLE_IMAGE_ASYNC_REQUEST:
          setAsyncRequest(props.getBoolean(name, false));
          break;
        case PropsConstants.AUTO_SIZE:
          setAutoSize(props.getBoolean(name, false));
          break;
        case PropsConstants.AUTO_PLAY:
          setAutoPlay(props.getBoolean(name, true));
          break;
        case PropsConstants.BLUR_RADIUS:
          setBlurRadius(props.getString(name));
          break;
        case PropsConstants.CAP_INSETS_BACKUP:
          setCapInsetsBackUp(props.getString(name));
          break;
        case PropsConstants.CAP_INSETS_SCALE:
          setCapInsetsScale(props.getString(name));
          break;
        case PropsConstants.CAP_INSETS:
          setCapInsets(props.getString(name));
          break;
        case PropsConstants.DEFER_SRC_INVALIDATION:
          setDeferInvalidation(props.getBoolean(name, false));
          break;
        case PropsConstants.DISABLE_DEFAULT_PLACEHOLDER:
          setDisableDefaultPlaceholder(props.getBoolean(name, false));
          break;
        case PropsConstants.DISABLE_DEFAULT_RESIZE:
          setDisableDefaultResize(props.getBoolean(name, false));
          break;
        case PropsConstants.ENABLE_CUSTOM_GIF_DECODER:
          setEnableCustomGifDecoder(props.getBoolean(name, false));
          break;
        case PropsConstants.ENABLE_RESOURCE_HINT:
          setEnableResourceHint(props.getBoolean(name, false));
          break;
        case PropsConstants.EXTRA_LOAD_INFO:
          setExtraLoadInfo(props.getBoolean(name, false));
          break;
        case PropsConstants.IAMGE_CONFIG:
          setImageConfig(props.getString(name));
          break;
        case PropsConstants.LOCAL_CACHE:
          setLocalCache(props.getDynamic(name));
          break;
        case PropsConstants.LOOP_COUNT:
          setLoopCount(props.getInt(name, 0));
          break;
        case PropsConstants.MODE:
          setMode(props.getString(name));
          break;
        case PropsConstants.PLACEHOLDER:
          setPlaceholder(props.getString(name));
          break;
        case PropsConstants.PRE_FETCH_HEIGHT:
          setPreFetchHeight(props.getString(name));
          break;
        case PropsConstants.PRE_FETCH_WIDTH:
          setPreFetchWidth(props.getString(name));
          break;
        case PropsConstants.SKIP_REDIRECTION:
          setSkipRedirection(props.getBoolean(name, false));
          break;
        case PropsConstants.SRC:
          setSrc(props.getString(name));
          break;
        case PropsConstants.TINT_COLOR:
          setTintColor(props.getString(name));
          break;
      }
    }
    updateRedirectCheckResult();
    endTrace(traceEvent);
  }

  public void pauseAnimation(ReadableMap params, Callback callback) {
    if (mImageDrawable != null && mImageLoader.pauseAnimation(mImageDrawable.getCurrent())) {
      callback.invoke(LynxUIMethodConstants.SUCCESS, "Animation paused.");
    } else {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "Not support pause yet");
    }
  }

  public void resumeAnimation(ReadableMap params, Callback callback) {
    if (mImageDrawable != null && mImageLoader.resumeAnimation(mImageDrawable.getCurrent())) {
      callback.invoke(LynxUIMethodConstants.SUCCESS, "Animation resumed.");
    } else {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "Not support resume yet");
    }
  }

  public void stopAnimation(ReadableMap params, Callback callback) {
    if (mImageDrawable != null && mImageLoader.stopAnimation(mImageDrawable.getCurrent())) {
      callback.invoke(LynxUIMethodConstants.SUCCESS, "Animation stopped.");
    } else {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "Not support stop yet");
    }
  }

  public void startAnimate(ReadableMap params, Callback callback) {
    if (mImageDrawable != null && mImageLoader.startAnimation(mImageDrawable.getCurrent())) {
      callback.invoke(LynxUIMethodConstants.SUCCESS, "Animation started.");
    } else {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "Not support start yet");
    }
  }

  // endregion

  void updateRedirectCheckResult() {
    if (mSkipRedirection) {
      return;
    }
    if (!mEnableCheckLocalImage) {
      return;
    }
    boolean needUpdateSrc = isDirty(SRC_CHANGED);
    boolean needUpdatePlaceholder = isDirty(PLACEHOLDER_CHANGED);
    LynxResourceRequest srcRequest = null;
    LynxResourceRequest placeholderRequest = null;
    if (needUpdateSrc) {
      mSrcRedirectCheckResult = mMediaResourceFetcher.isLocalResource(mSrc);
      if (mSrcRedirectCheckResult != OptionalBool.FALSE) {
        srcRequest = new LynxResourceRequest(
            mSrc, LynxResourceRequest.LynxResourceType.LynxResourceTypeImage);
      }
    }
    if (needUpdatePlaceholder) {
      mPlaceHolderRedirectCheckResult = mMediaResourceFetcher.isLocalResource(mPlaceholder);
      if (mPlaceHolderRedirectCheckResult != OptionalBool.FALSE) {
        placeholderRequest = new LynxResourceRequest(
            mPlaceholder, LynxResourceRequest.LynxResourceType.LynxResourceTypeImage);
      }
    }

    if (Thread.currentThread() != Looper.getMainLooper().getThread() || !mAsyncRedirect) {
      if (srcRequest != null) {
        mSrc = mMediaResourceFetcher.shouldRedirectUrl(srcRequest);
      }
      if (placeholderRequest != null) {
        mPlaceholder = mMediaResourceFetcher.shouldRedirectUrl(placeholderRequest);
      }
    } else if (srcRequest != null || placeholderRequest != null) {
      LynxResourceRequest finalSrcRequest = srcRequest;
      LynxResourceRequest finalPlaceholderRequest = placeholderRequest;
      if (srcRequest != null) {
        dirtyFlags &= ~SRC_CHANGED;
      }
      if (placeholderRequest != null) {
        dirtyFlags &= ~PLACEHOLDER_CHANGED;
      }
      LynxThreadPool.getBriefIOExecutor().execute(() -> {
        boolean srcUpdated = false;
        boolean placeHolderUpdated = false;
        if (finalPlaceholderRequest != null) {
          mPlaceholder = mMediaResourceFetcher.shouldRedirectUrl(finalPlaceholderRequest);
          placeHolderUpdated = true;
        }
        if (finalSrcRequest != null) {
          mSrc = mMediaResourceFetcher.shouldRedirectUrl(finalSrcRequest);
          srcUpdated = true;
        }

        boolean finalSrcUpdated = srcUpdated;
        boolean finalPlaceHolderUpdated = placeHolderUpdated;
        UIThreadUtils.runOnUiThread(() -> {
          if (finalSrcUpdated) {
            dirtyFlags |= SRC_CHANGED;
          }
          if (finalPlaceHolderUpdated) {
            dirtyFlags |= PLACEHOLDER_CHANGED;
          }
          onNodeReady();
        });
      });
    }
  }

  public void onPropsUpdated() {}

  private void updateImageSource() {
    String traceEvent = "updateImageSource";
    beginTrace(traceEvent);
    int width = 0;
    int height = 0;
    boolean needRequest = true;
    if (mViewWidth > 0 && mViewHeight > 0) {
      width = mViewWidth;
      height = mViewHeight;
    } else if (mPreFetchWidth > 0 && mPreFetchHeight > 0) {
      width = mPreFetchWidth;
      height = mPreFetchHeight;
    } else if (!mAutoSize) {
      needRequest = false;
    }
    if (needRequest) {
      tryFetchImageFromService(width, height);
    }
    endTrace(traceEvent);
  }

  private void updatePlaceholderSource() {
    String traceEvent = "updatePlaceholderSource";
    beginTrace(traceEvent);
    int width = 0;
    int height = 0;
    boolean needRequest = true;
    if (mViewWidth > 0 && mViewHeight > 0) {
      width = mViewWidth;
      height = mViewHeight;
    } else if (mPreFetchWidth > 0 && mPreFetchHeight > 0) {
      width = mPreFetchWidth;
      height = mPreFetchHeight;
    } else {
      needRequest = false;
    }
    if (needRequest) {
      tryFetchPlaceholderFromService(width, height);
    }
    endTrace(traceEvent);
  }

  private void beginTrace(String methodName) {
    if (TraceEvent.enableTrace()) {
      Map<String, String> params = new HashMap<>();
      params.put("src", mSrc);
      TraceEvent.beginSection(TAG + "." + methodName, params);
    }
  }

  private void endTrace(String methodName) {
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(TAG + "." + methodName);
    }
  }

  public void onNodeReady() {
    // set one more time, need opt
    updateNodeProps();
    invalidate();
  }

  public void updateNodeProps() {
    if (isDirty(DOWN_SAMPLING_SCALE_CHANGED) && (mDisableDefaultResize || mAutoSize)) {
      dirtyFlags &= ~DOWN_SAMPLING_SCALE_CHANGED;
    }
    if (isDirty(MODE_CHANGED) && mImageDrawable != null) {
      mImageDrawable = new LynxScaleTypeDrawable(mImageDrawable.getCurrent(), mMode);
    }
    if (isDirty(CAP_INSETS_CHANGED) && mImageDrawable != null) {
      mImageDrawable.setCapInsets(mCapInsets, mCapInsetsScale);
    }
    if (isDirty(TINT_COLOR_CHANGED) && mImageDrawable != null) {
      mImageDrawable.setColorFilter(mColorFilter);
    }

    if (isDirty(PLACEHOLDER_CHANGED) || isDirty(DOWN_SAMPLING_SCALE_CHANGED)) {
      releaseImage(mCurPlaceholderRequest);
      releaseDrawable(mPlaceholderDrawable);
      mCurPlaceholderRequest = null;
      mPlaceholderDrawable = null;
      updatePlaceholderSource();
    }

    if (isDirty(SRC_CHANGED) || isDirty(BLUR_RADIUS_CHANGED)
        || isDirty(DOWN_SAMPLING_SCALE_CHANGED)) {
      if (!mDeferInvalidation) {
        releaseImage(mCurImageRequest);
        releaseDrawable(mImageDrawable);
        mImageDrawable = null;
        mCurImageRequest = null;
        mImageWidth = 0;
        mImageHeight = 0;
      } else {
        mPreImageRequestInfo = mCurImageRequest;
      }
      updateImageSource();
    }
    if (isDirty(LAYOUT_CHANGED) || isDirty(BORDER_CHANGED) || isDirty(BORDER_RADIUS_CHANGED)) {
      configureBounds(mImageDrawable);
      configureBounds(mPlaceholderDrawable);
      if (mBorderRadius != null && mBorderRadius.length > 0) {
        if (mInnerClipPathForBorderRadius == null) {
          mInnerClipPathForBorderRadius = new BackgroundDrawable.RoundRectPath();
        }
        mInnerClipPathForBorderRadius.updateValue(
            new Rect(mPaddingLeft, mPaddingTop, mViewWidth - mPaddingRight,
                mViewHeight - mPaddingBottom),
            mBorderRadius, mBorderWidthRect, 1.0f, false);
      }
    }

    if (mNeedRetryAutoSize) {
      mNeedRetryAutoSize = false;
      if (mAutoSize) {
        justSizeIfNeeded();
      }
    }
    dirtyFlags = 0;
  }

  private void releaseImage(ImageRequestInfo requestInfo) {
    if (requestInfo != null) {
      mImageLoader.releaseImage(requestInfo);
    }
  }

  private void releaseDrawable(Drawable drawable) {
    if (drawable != null) {
      mImageLoader.releaseAnimDrawable(drawable.getCurrent());
    }
  }

  private ImageRequestInfo createImageRequest(int width, int height, String url) {
    if (TextUtils.isEmpty(url)) {
      return null;
    }
    ImageRequestInfoBuilder builder = new ImageRequestInfoBuilder();
    builder.setUrl(url)
        .setResizeWidth(width)
        .setResizeHeight(height)
        .setLoopCount(mLoopCount)
        .setCallerContext(mContext.getFrescoCallerContext())
        .setEnableAnimationAutoPlay(mAutoPlay)
        .setEnableDownSampling(!mDisableDefaultResize && !mAutoSize)
        .setEnableAsyncRequest(mEnableAsyncRequest)
        .setEnableGifLiteDecoder(mEnableCustomGifDecoder)
        .setEnableResourceHint(mEnableResourceHint);
    if (mBitmapConfig != null) {
      builder.setBitmapConfig(mBitmapConfig);
    }
    if (mContext.getEnableImageSmallDiskCache()) {
      builder.setDiskCacheChoice(DiskCacheChoice.SMALL_DISK);
    }
    if (mContext.getImageCustomParam() != null) {
      builder.setCustomParam(mContext.getImageCustomParam());
    }
    if (!TextUtils.isEmpty(mBlurRadius)) {
      float radius = UnitUtils.toPxWithDisplayMetrics(
          mBlurRadius, 0, 0, 0, 0, -1, mContext.getScreenMetrics());
      if (radius > 0) {
        ImageBlurPostProcessor postProcessor = new ImageBlurPostProcessor((int) radius);
        List<BitmapPostProcessor> postProcessors = new ArrayList<>();
        postProcessors.add(postProcessor);
        builder.setBitmapPostProcessor(postProcessors);
      }
    }
    return builder.build();
  }

  public void tryFetchImageFromService(int width, int height) {
    AnimationListener animationListener =
        (mEnableAllLoopEvent || mEnableStartPlayEvent || mEnableCurrentLoopEvent)
        ? mAnimationListener
        : null;
    ImageRequestInfo requestInfo = createImageRequest(width, height, mSrc);
    if (requestInfo != null) {
      mCurImageRequest = requestInfo;
      mImageLoader.fetchImage(requestInfo, mSrcLoadListener, animationListener, mContext);
    }
  }

  public void tryFetchPlaceholderFromService(int width, int height) {
    if (mImageDrawable != null) {
      return;
    }
    ImageRequestInfo requestInfo = createImageRequest(width, height, mPlaceholder);
    if (requestInfo != null) {
      mCurPlaceholderRequest = requestInfo;
      mImageLoader.fetchImage(requestInfo, mPlaceHolderListener, null, mContext);
    }
  }

  public void destroy() {
    releaseAllImage();
  }

  private void releaseAllImage() {
    releaseImage(mCurImageRequest);
    releaseImage(mCurPlaceholderRequest);
    releaseDrawable(mImageDrawable);
    releaseDrawable(mPlaceholderDrawable);
    mCurImageRequest = null;
    mCurPlaceholderRequest = null;
    mImageDrawable = null;
    mPlaceholderDrawable = null;
  }

  public void onDraw(Canvas canvas) {
    canvas.save();
    if (mInnerClipPathForBorderRadius != null && mInnerClipPathForBorderRadius.path != null) {
      canvas.clipPath(mInnerClipPathForBorderRadius.path);
    }
    if (mPlaceholderDrawable != null) {
      mPlaceholderDrawable.draw(canvas);
    }
    if (mImageDrawable != null) {
      mImageDrawable.draw(canvas);
    }
    canvas.restore();
  }

  private void configureBounds(Drawable drawable) {
    mDrawableBounds = new Rect(0, 0, mViewWidth, mViewHeight);
    mDrawableBounds.left = (int) (mDrawableBounds.left + mBorderLeftWidth + mPaddingLeft);
    mDrawableBounds.top = (int) (mDrawableBounds.top + mBorderTopWidth + mPaddingTop);
    mDrawableBounds.right = (int) (mDrawableBounds.right - mBorderRightWidth - mPaddingRight);
    mDrawableBounds.bottom = (int) (mDrawableBounds.bottom - mBorderBottomWidth - mPaddingBottom);
    if (drawable != null) {
      drawable.setBounds(mDrawableBounds);
    }
  }

  public void invalidate() {
    if (mUI != null) {
      mUI.invalidate();
    }
  }

  public void setEvents(Map<String, EventsListener> events) {
    if (events == null) {
      return;
    }
    mEnableStartPlayEvent = false;
    mEnableCurrentLoopEvent = false;
    mEnableAllLoopEvent = false;
    mEnableOnLoad = false;
    mEnableOnError = false;
    if (events.containsKey(EVENT_START_PLAY)) {
      mEnableStartPlayEvent = true;
    }
    if (events.containsKey(EVENT_CURRENT_LOOP_COMPLETE)) {
      mEnableCurrentLoopEvent = true;
    }
    if (events.containsKey(EVENT_ALL_LOOP_COMPLETE)) {
      mEnableAllLoopEvent = true;
    }
    if (events.containsKey(EVENT_LOAD)) {
      mEnableOnLoad = true;
    }
    if (events.containsKey(EVENT_ERROR)) {
      mEnableOnError = true;
    }
  }

  public void onLayoutUpdated(
      int width, int height, int paddingLeft, int paddingRight, int paddingTop, int paddingBottom) {
    if (width != mViewWidth || height != mViewHeight) {
      if (width > mViewWidth || height > mViewHeight) {
        dirtyFlags |= DOWN_SAMPLING_SCALE_CHANGED;
      }
      mViewWidth = width;
      mViewHeight = height;
      dirtyFlags |= LAYOUT_CHANGED;
    }
    if (mPaddingTop != paddingTop || mPaddingRight != paddingRight
        || mPaddingBottom != paddingBottom || mPaddingLeft != paddingLeft) {
      mPaddingTop = paddingTop;
      mPaddingRight = paddingRight;
      mPaddingLeft = paddingLeft;
      mPaddingBottom = paddingBottom;
      dirtyFlags |= LAYOUT_CHANGED;
    }
  }

  ScalingUtils.ScaleType getMode(String prop) {
    ScalingUtils.ScaleType mode;
    if ("aspectFit".equals(prop)) {
      mode = ScalingUtils.ScaleType.FIT_CENTER;
    } else if ("aspectFill".equals(prop)) {
      mode = ScalingUtils.ScaleType.CENTER_CROP;
    } else if ("center".equals(prop)) {
      mode = ScalingUtils.ScaleType.CENTER;
    } else {
      mode = ScalingUtils.ScaleType.FIT_XY;
    }
    return mode;
  }

  private boolean isDirty(long flag) {
    return (dirtyFlags & flag) != 0;
  }

  public void setImageConfig(String config) {
    if (config == null || config.equalsIgnoreCase("")) {
      mBitmapConfig = null;
      return;
    }
    if (config.equalsIgnoreCase("RGB_565")) {
      mBitmapConfig = Bitmap.Config.RGB_565;
    } else if (config.equalsIgnoreCase("ARGB_8888")) {
      mBitmapConfig = Bitmap.Config.ARGB_8888;
    }
  }

  @Override
  public void invalidateDrawable(@NonNull Drawable who) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        invalidate();
      }
    });
  }

  @Override
  public void scheduleDrawable(@NonNull Drawable who, @NonNull Runnable what, long when) {
    UIThreadUtils.runOnUiThreadAtTime(what, who, when);
  }

  @Override
  public void unscheduleDrawable(@NonNull Drawable who, @NonNull Runnable what) {
    UIThreadUtils.removeCallbacks(what, who);
  }

  public void setBorderWidth(RectF borderRect) {
    mBorderWidthRect = borderRect;
    float borderTop = mBorderWidthRect.top;
    float borderBottom = mBorderWidthRect.bottom;
    float borderLeft = mBorderWidthRect.left;
    float borderRight = mBorderWidthRect.right;
    if (mBorderTopWidth != borderTop) {
      mBorderTopWidth = borderTop;
      dirtyFlags |= BORDER_CHANGED;
    }
    if (mBorderLeftWidth != borderLeft) {
      mBorderLeftWidth = borderLeft;
      dirtyFlags |= BORDER_CHANGED;
    }

    if (mBorderRightWidth != borderRight) {
      mBorderRightWidth = borderRight;
      dirtyFlags |= BORDER_CHANGED;
    }

    if (mBorderBottomWidth != borderBottom) {
      mBorderBottomWidth = borderBottom;
      dirtyFlags |= BORDER_CHANGED;
    }
  }

  public void justSizeIfNeeded() {
    if (mImageWidth == 0 || mImageHeight == 0 || !mAutoSize || mUI == null) {
      return;
    }
    if (mAutoSizeShadowNode == null) {
      mAutoSizeShadowNode = mUI.getLynxContext().findShadowNodeBySign(mUI.getSign());
    }
    if (!(mAutoSizeShadowNode instanceof AutoSizeImage)) {
      mNeedRetryAutoSize = true;
      return;
    }
    ((AutoSizeImage) mAutoSizeShadowNode)
        .justSizeIfNeeded(mAutoSize, mImageWidth, mImageHeight, mUI.getWidth(), mUI.getHeight());
  }

  private void sendCustomEvent(String eventName) {
    if (mContext != null && mUI != null) {
      LynxDetailEvent event = new LynxDetailEvent(mUI.getSign(), eventName);
      mContext.getEventEmitter().sendCustomEvent(event);
    }
  }

  protected void onImageLoadSuccess(int width, int height) {
    if (mEnableOnLoad) {
      sendLoadEvent(width, height);
    }
  }

  protected void onImageLoadError(LynxError error, int categorizedCode, int imageErrorCode) {
    handlerFailure(error, categorizedCode, imageErrorCode);
  }

  private void sendLoadEvent(int width, int height) {
    if (mContext != null && mUI != null && !mEnableExtraLoadInfo) {
      LynxDetailEvent event = new LynxDetailEvent(mUI.getSign(), EVENT_LOAD);
      event.addDetail("height", height);
      event.addDetail("width", width);
      mContext.getEventEmitter().sendCustomEvent(event);
    }
  }

  private void sendExtraLoadEvent(JSONObject monitorInfo) {
    if (mContext != null && mUI != null && mEnableExtraLoadInfo && monitorInfo != null) {
      LynxDetailEvent event = new LynxDetailEvent(mUI.getSign(), EVENT_LOAD);
      Iterator<String> keys = monitorInfo.keys();
      while (keys.hasNext()) {
        String key = keys.next();
        Object value = null;
        try {
          value = monitorInfo.get(key);
        } catch (JSONException e) {
          LLog.e(TAG, e.getMessage());
        }
        event.addDetail(key, value);
      }
    }
  }

  private void handlerFailure(LynxError lynxError, int categorizedCode, int imageErrorCode) {
    if (mContext != null && mUI != null) {
      lynxError.addCustomInfo("node_index", Integer.toString(mUI.getNodeIndex()));
      if (mEnableOnError) {
        LynxDetailEvent event = new LynxDetailEvent(mUI.getSign(), EVENT_ERROR);
        event.addDetail("errMsg", lynxError.getSummaryMessage());
        event.addDetail(ImageErrorCodeUtils.LYNX_IMAGE_CATEGORIZED_CODE_KEY, categorizedCode);
        event.addDetail(ImageErrorCodeUtils.LYNX_IMAGE_ERROR_CODE_KEY, imageErrorCode);
        mContext.getEventEmitter().sendCustomEvent(event);
      }
      lynxError.addCustomInfo(
          LynxError.LYNX_ERROR_KEY_IMAGE_CATEGORIZED_CODE, String.valueOf(categorizedCode));
      mContext.reportResourceError(mSrc, "image", lynxError);
    }
  }

  public void setBorderRadius(float[] borderRadius) {
    mBorderRadius = borderRadius;
    dirtyFlags |= BORDER_RADIUS_CHANGED;
  }

  public Drawable getSrcImageDrawable() {
    return mImageDrawable;
  }
}
