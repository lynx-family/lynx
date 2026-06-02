// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown;

import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Looper;
import android.view.Choreographer;
import androidx.annotation.Nullable;
import com.lynx.markdown.Constants;
import com.lynx.markdown.Markdown;
import com.lynx.markdown.MarkdownValuePack;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxShadowNode;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.AlignParam;
import com.lynx.tasm.behavior.shadow.CustomMeasureFunc;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.MeasureResult;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.utils.UIThreadUtils;
import com.lynx.xelement.markdown.adaptor.LynxMarkdownBundle;
import com.lynx.xelement.markdown.adaptor.LynxServalViewWrapper;
import com.lynx.xelement.markdown.adaptor.MarkdownEventContext;
import com.lynx.xelement.markdown.adaptor.MarkdownEventListener;
import com.lynx.xelement.markdown.adaptor.MarkdownResourceContext;
import com.lynx.xelement.markdown.adaptor.MarkdownResourceLoader;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;

public class LynxUIMarkdownShadowNode
    extends ShadowNode implements CustomMeasureFunc, MarkdownResourceContext, MarkdownEventContext {
  private LynxServalViewWrapper mMarkdown;
  private final LynxMarkdownBundle mBundle = new LynxMarkdownBundle();
  private final MarkdownResourceLoader mResourceLoader = new MarkdownResourceLoader(this);
  private final MarkdownEventListener mEventListener = new MarkdownEventListener(this);
  private MeasureContext mMeasureContext;
  private AlignContext mAlignContext;
  private String mContent = "";
  private String mContentID = "";
  private final Looper mLayoutLooper;
  private Handler mLayoutHandler;
  private Choreographer.FrameCallback mFrameCallback;
  public LynxUIMarkdownShadowNode() {
    Markdown.ensureInitialized();
    setCustomMeasureFunc(this);
    mLayoutLooper = Looper.myLooper();
    if (mLayoutLooper != null) {
      mLayoutHandler = new Handler(mLayoutLooper);
    }
  }
  @Override
  public void setContext(LynxContext context) {
    super.setContext(context);
    ensureMarkdownView();
  }
  private @Nullable LynxServalViewWrapper ensureMarkdownView() {
    if (mMarkdown == null && mContext != null) {
      mMarkdown = new LynxServalViewWrapper(mContext, this);
      mMarkdown.setResourceLoader(mResourceLoader);
      mMarkdown.setEventListener(mEventListener);
      mMarkdown.disableInternalVSync(true);
      if (mLayoutLooper != null) {
        mFrameCallback = this::onVSync;
        Choreographer.getInstance().postFrameCallback(mFrameCallback);
      }
      mBundle.mMarkdownView = mMarkdown;
    }
    return mMarkdown;
  }
  private boolean isChildDirty() {
    boolean isChildDirty = false;
    for (int i = 0; i < getChildCount(); i++) {
      ShadowNode child = getChildAt(i);
      if (!(child instanceof NativeLayoutNodeRef)) {
        continue;
      }
      if (child.isDirty()) {
        isChildDirty = true;
        child.resetIsDirty();
      }
    }
    return isChildDirty;
  }
  @Override
  public MeasureResult measure(MeasureParam param, MeasureContext context) {
    mMeasureContext = context;
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown == null) {
      return new MeasureResult(0, 0, 0);
    }
    if (isChildDirty()) {
      markdown.markDirty();
    }
    long result = markdown.measure((int) param.mWidth, param.mWidthMode.intValue(),
        (int) param.mHeight, param.mHeightMode.intValue());
    int width = MarkdownValuePack.unpackMeasureResultWidth(result);
    int height = MarkdownValuePack.unpackMeasureResultHeight(result);
    int baseline = MarkdownValuePack.unpackMeasureResultBaseline(result);
    return new MeasureResult(width, height, baseline);
  }
  @Override
  public void align(AlignParam param, AlignContext context) {
    mAlignContext = context;
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.align((int) param.getLeftOffset(), (int) param.getTopOffset());
    }
  }
  @Nullable
  @Override
  public Object getExtraBundle() {
    mBundle.mMarkdownView = mMarkdown;
    return mBundle;
  }
  @Override
  protected void onDestroy() {
    super.onDestroy();
    mResourceLoader.release();
    mMeasureContext = null;
    mAlignContext = null;
    if (mMarkdown != null) {
      mMarkdown.destroy();
      mMarkdown = null;
    }
    mBundle.mMarkdownView = null;
  }
  @LynxProp(name = "text-selection", defaultBoolean = false)
  public void setEnableTextSelection(boolean enable) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setBooleanProp(Constants.MARKDOWN_PROPS_ENABLE_TEXT_SELECTION, enable);
    }
  }
  @LynxProp(name = "selection-background-color", defaultInt = 0)
  public void setSelectionBackgroundColor(int color) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setColorProp(Constants.MARKDOWN_PROPS_SELECTION_HIGHLIGHT_COLOR, color);
    }
  }
  @LynxProp(name = "selection-handle-color", defaultInt = 0)
  public void setSelectionHandleColor(int color) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setColorProp(Constants.MARKDOWN_PROPS_SELECTION_HANDLE_COLOR, color);
    }
  }
  @LynxProp(name = "selection-handle-size", defaultInt = 0)
  public void setSelectionHandleSize(int size) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setNumberProp(Constants.MARKDOWN_PROPS_SELECTION_HANDLE_SIZE, size);
    }
  }
  @LynxProp(name = "markdown-effect")
  public void setMarkdownEffect(ReadableMap map) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      HashMap<String, Object> value = map == null ? null : map.asHashMap();
      markdown.setObjectProp(Constants.MARKDOWN_PROPS_MARKDOWN_EFFECT, value);
    }
  }

  @LynxProp(name = "text-mark-attachments")
  public void setTextMarkAttachments(ReadableArray array) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      ArrayList<Object> value = array == null ? null : array.asArrayList();
      markdown.setArrayProp(Constants.MARKDOWN_PROPS_TEXT_MARK_ATTACHMENTS, value);
    }
  }

  @LynxProp(name = "enable-region-view")
  public void setEnableRegionView(boolean value) {}

  @LynxProp(name = "image-downsampling")
  public void setImageDownSampling(boolean value) {
    mResourceLoader.setEnableImageDownSampling(value);
  }

  @LynxProp(name = "image-sync-load")
  public void setImageSyncLoad(boolean value) {
    mResourceLoader.setImageSyncLoad(value);
  }

  @LynxProp(name = "content")
  public void setContent(String content) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    mContent = content == null ? "" : content;
    if (markdown != null) {
      markdown.setContent(mContent);
    }
  }

  @LynxProp(name = "content-id")
  public void setContentID(String contentID) {
    mContentID = contentID == null ? "" : contentID;
  }

  @LynxProp(name = "animation-type")
  public void setAnimationType(String type) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      int animationType = "typewriter".equals(type) ? Constants.ANIMATION_TYPE_TYPEWRITER
                                                    : Constants.ANIMATION_TYPE_NONE;
      markdown.setAnimationType(animationType);
    }
    markDirty();
  }

  @LynxProp(name = "animation-velocity")
  public void setAnimationVelocity(float velocity) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setAnimationVelocity(velocity >= 0 ? velocity : 1f);
    }
    markDirty();
  }

  @LynxProp(name = "markdown-style")
  public void setStyleSheet(ReadableMap style) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      HashMap<String, Object> value = style == null ? null : style.asHashMap();
      markdown.setStyle(value);
    }
    markDirty();
  }

  @LynxProp(name = "text-maxline")
  public void setTextMaxLine(int maxLine) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setNumberProp(Constants.MARKDOWN_PROPS_TEXT_MAXLINE, maxLine);
    }
    markDirty();
  }

  @LynxProp(name = "content-complete")
  public void setContentComplete(boolean complete) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setBooleanProp(Constants.MARKDOWN_PROPS_CONTENT_COMPLETE, complete);
    }
    markDirty();
  }

  @LynxProp(name = "typewriter-dynamic-height")
  public void setTypewriterAutoHeight(boolean dynamicHeight) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setBooleanProp(Constants.MARKDOWN_PROPS_TYPEWRITER_DYNAMIC_HEIGHT, dynamicHeight);
    }
    markDirty();
  }

  @LynxProp(name = "initial-animation-step")
  public void setInitialAnimationStep(int step) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setInitialAnimationStep(step);
    }
    markDirty();
  }

  @LynxProp(name = "markdown-max-height")
  public void setMarkdownMaxHeight(float height) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setNumberProp(Constants.MARKDOWN_PROPS_MARKDOWN_MAX_HEIGHT, height);
    }
    markDirty();
  }

  @LynxProp(name = "content-range")
  public void setMarkdownContentRange(ReadableArray array) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown == null || array == null) {
      return;
    }
    if (array.size() > 0) {
      markdown.setNumberProp(Constants.MARKDOWN_PROPS_CONTENT_RANGE_START, array.getInt(0));
    }
    if (array.size() > 1) {
      markdown.setNumberProp(Constants.MARKDOWN_PROPS_CONTENT_RANGE_END, array.getInt(1));
    }
    markDirty();
  }

  @LynxProp(name = "exposure-tags")
  public void setExposureTags(ReadableArray array) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      ArrayList<Object> value = array == null ? null : array.asArrayList();
      markdown.setArrayProp(Constants.MARKDOWN_PROPS_EXPOSURE_TAGS, value);
    }
  }

  @LynxProp(name = "animation-frame-rate")
  public void setAnimationFrameRate(float frameRate) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setNumberProp(Constants.MARKDOWN_PROPS_ANIMATION_FRAME_RATE, frameRate);
    }
  }

  @LynxProp(name = "typewriter-height-transition-duration")
  public void setTypewriterHeightTransitionDuration(float duration) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setNumberProp(
          Constants.MARKDOWN_PROPS_TYPEWRITER_HEIGHT_TRANSITION_DURATION, duration);
    }
  }

  @LynxProp(name = "typewriter-height-transition-prefetch")
  public void setTypewriterHeightTransitionPrefetch(boolean prefetch) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setBooleanProp(
          Constants.MARKDOWN_PROPS_TYPEWRITER_HEIGHT_TRANSITION_PREFETCH, prefetch);
    }
  }

  @LynxProp(name = "allow-break-around-punctuation")
  public void setAllowBreakAroundPunctuation(boolean allow) {
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.setBooleanProp(Constants.MARKDOWN_PROPS_ALLOW_BREAK_AROUND_PUNCTUATION, allow);
    }
  }

  @Override
  public ShadowNode getShadowNode() {
    return this;
  }

  @Override
  public String getParseEndContentID() {
    return mMarkdown != null ? mMarkdown.getContentID() : mContentID;
  }

  @Override
  public int getContentLength() {
    return mContent == null ? 0 : mContent.length();
  }

  @Override
  public @Nullable LynxContext getLynxContext() {
    return mContext;
  }

  @Override
  public @Nullable Drawable.Callback getDrawableCallback() {
    return ensureMarkdownView();
  }

  @Override
  public @Nullable MeasureContext getMeasureContext() {
    return mMeasureContext;
  }

  @Override
  public @Nullable AlignContext getAlignContext() {
    return mAlignContext;
  }

  @Override
  public void onFontLoaded(String family, int weight, int style) {
    if (isDestroyed()) {
      return;
    }
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.onFontLoaded(family, weight, style);
    }
  }

  @Override
  public void onImageLoaded(String url) {
    if (isDestroyed()) {
      return;
    }
    LynxServalViewWrapper markdown = ensureMarkdownView();
    if (markdown != null) {
      markdown.onImageLoaded(url);
    }
  }

  @Override
  public void onImageLoadError(String source, @Nullable Throwable throwable) {
    if (isDestroyed() || mContext == null) {
      return;
    }
    String message = throwable == null ? "failed to load image" : throwable.getMessage();
    mContext.reportResourceError(source, "image", message);
  }

  public void runOnLayoutThread(Runnable runnable) {
    if (mLayoutLooper != null) {
      mLayoutHandler.post(runnable);
    } else {
      UIThreadUtils.runOnUiThreadImmediately(runnable);
    }
  }

  @Override
  public void markDirty() {
    if (mLayoutLooper == null || Looper.myLooper() == mLayoutLooper) {
      super.markDirty();
    } else {
      runOnLayoutThread(super::markDirty);
    }
  }

  private void onVSync(long time) {
    if (mFrameCallback == null) {
      return;
    }
    LynxServalViewWrapper view = ensureMarkdownView();
    if (view != null) {
      view.onLayoutFrame(time);
    }
    Choreographer.getInstance().postFrameCallback(mFrameCallback);
  }
}
