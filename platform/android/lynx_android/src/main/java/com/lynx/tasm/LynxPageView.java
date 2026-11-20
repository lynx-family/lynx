// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import android.graphics.Canvas;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.annotation.UiThread;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.group.LynxHost;
import com.lynx.tasm.utils.CallStackUtil;
import com.lynx.tasm.utils.UIThreadUtils;

/**
 * A page rendered by its own LynxTemplateRender using a host's shared environment.
 * Pages retain independent page runtimes and follow the host group's JS context sharing policy.
 * Obtain a page through {@link LynxHost#createPageView(Context, LynxPageOption)}.
 * Create, load, update and destroy it on the UI thread.
 */
@UiThread
public final class LynxPageView extends UIBodyView {
  private final LynxTemplateRender mTemplateRender;
  private final LynxHostEnvironment mHostEnvironment;
  private boolean mDestroyed;
  private boolean mCanDispatchTouchEvent;
  private boolean mConsumeSlideEvent;

  /** Internal constructor; callers should use the host's createPageView method. */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public LynxPageView(Context context, @NonNull LynxHost host, @NonNull LynxPageOption option) {
    super(context);
    UIThreadUtils.assertOnUiThread();
    if (host == null || host.getHostEnvironment() == null) {
      throw new IllegalArgumentException("A surface requires a shared environment");
    }
    if (option == null) {
      throw new IllegalArgumentException("A surface requires page options");
    }
    mHostEnvironment = host.getHostEnvironment();
    if (!LynxEnv.inst().isNativeLibraryLoaded()) {
      throw new IllegalStateException("Lynx must be initialized before creating a page");
    }
    mLynxUIRender = option.getUIRendererCreator().createLynxUIRender();
    mLynxUIRender.onInitBodyView(
        this, context, mHostEnvironment.getRuntimeOptions().getLynxGroup());
    mTemplateRender =
        new LynxTemplateRender(context, this, option, mHostEnvironment, host.getEmbeddedMode());
  }

  LynxHostEnvironment getHostEnvironment() {
    return mHostEnvironment;
  }

  LynxTemplateRender getTemplateRender() {
    return mTemplateRender;
  }

  public LynxContext getLynxContext() {
    return mTemplateRender.getLynxContext();
  }

  public ThreadStrategyForRendering getThreadStrategyForRendering() {
    return mTemplateRender.getThreadStrategyForRendering();
  }

  public void loadTemplate(TemplateBundle bundle, TemplateData data, String url) {
    loadTemplate(bundle, data, url, null);
  }

  /**
   * Loads a bundle with page-specific global properties. These properties are merged into this
   * page without changing other surfaces.
   */
  public void loadTemplate(
      TemplateBundle bundle, TemplateData data, String url, @Nullable TemplateData globalProps) {
    UIThreadUtils.assertOnUiThread();
    if (mDestroyed) {
      return;
    }
    if (bundle == null || !bundle.isValid()) {
      mTemplateRender.onErrorOccurred(new LynxError(
          LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_BUNDLE, "A valid template bundle is required"));
      return;
    }
    LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
    builder.setTemplateBundle(bundle);
    builder.setInitialData(data);
    builder.setUrl(url);
    builder.setGlobalProps(globalProps);
    mTemplateRender.loadTemplate(builder.build());
  }

  public void updateData(TemplateData data) {
    UIThreadUtils.assertOnUiThread();
    if (!mDestroyed) {
      mTemplateRender.updateData(data, false);
    }
  }

  public void destroy() {
    UIThreadUtils.assertOnUiThread();
    if (mDestroyed) {
      return;
    }
    mDestroyed = true;
    mTemplateRender.destroy();
  }

  @Override
  protected void onMeasure(int widthSpec, int heightSpec) {
    if (mDestroyed) {
      setMeasuredDimension(0, 0);
      return;
    }
    mTemplateRender.onMeasure(widthSpec, heightSpec);
    if (mLynxUIRender.shouldInvokeNativeViewMethod()
        || getLynxContext().isFragmentLayerRenderOn()) {
      super.onMeasure(widthSpec, heightSpec);
    }
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    if (mDestroyed) {
      return;
    }
    if (mLynxUIRender.shouldInvokeNativeViewMethod()
        || getLynxContext().isFragmentLayerRenderOn()) {
      super.onLayout(changed, left, top, right, bottom);
    }
    mTemplateRender.onLayout(changed, left, top, right, bottom);
  }

  @Override
  public boolean dispatchTouchEvent(MotionEvent event) {
    if (mDestroyed) {
      return false;
    }
    try {
      if (mLynxUIRender.shouldInvokeNativeViewMethod() || isChildLynxPageUI()) {
        return super.dispatchTouchEvent(event);
      }
      int action = event.getActionMasked();
      if (action == MotionEvent.ACTION_DOWN) {
        mCanDispatchTouchEvent = true;
        mConsumeSlideEvent = false;
      }
      boolean consumed = false;
      if (mCanDispatchTouchEvent) {
        // Dispatch to Lynx before an Android child can consume the event.
        float x = event.getX();
        float y = event.getY();
        try {
          consumed = mTemplateRender.dispatchTouchEvent(event);
        } finally {
          event.setLocation(x, y);
        }
        if (consumed && mTemplateRender.blockNativeEvent(event) && getParent() != null) {
          getParent().requestDisallowInterceptTouchEvent(true);
        }
      }
      if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
        mCanDispatchTouchEvent = false;
      }
      if (consumed) {
        if (!mConsumeSlideEvent) {
          mConsumeSlideEvent = mTemplateRender.consumeSlideEvent(event);
        }
        return mConsumeSlideEvent || super.dispatchTouchEvent(event);
      }
    } catch (Throwable error) {
      reportTouchError("dispatchTouchEvent", error);
    }
    return false;
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent event) {
    if (mDestroyed) {
      return false;
    }
    try {
      if (isChildLynxPageUI()) {
        return super.onInterceptTouchEvent(event);
      }
      if (mCanDispatchTouchEvent && mTemplateRender.onInterceptTouchEvent(event)) {
        if (getParent() != null) {
          getParent().requestDisallowInterceptTouchEvent(true);
        }
        return true;
      }
      return super.onInterceptTouchEvent(event);
    } catch (Throwable error) {
      reportTouchError("onInterceptTouchEvent", error);
    }
    return false;
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (mDestroyed) {
      return false;
    }
    try {
      if (isChildLynxPageUI()) {
        return super.onTouchEvent(event);
      }
      if (mCanDispatchTouchEvent) {
        mTemplateRender.onTouchEvent(event);
      }
      if (!mTemplateRender.blockNativeEvent(event) || getParent() == null) {
        super.onTouchEvent(event);
      }
      // Keep receiving the gesture when no Android child consumes it.
      return true;
    } catch (Throwable error) {
      reportTouchError("onTouchEvent", error);
    }
    return false;
  }

  private void reportTouchError(String method, Throwable cause) {
    LynxError error = new LynxError(LynxSubErrorCode.E_EVENT_EXCEPTION,
        "An exception occurred during " + method + "(): " + cause.getMessage(),
        "This error is caught by native, please ask Lynx for help", LynxError.LEVEL_ERROR);
    error.setCallStack(CallStackUtil.getStackTraceStringTrimmed(cause));
    mTemplateRender.onErrorOccurred(error);
  }

  @Override
  protected void dispatchDraw(Canvas canvas) {
    super.dispatchDraw(canvas);
    if (!mDestroyed) {
      mTemplateRender.onRootViewDraw(canvas);
    }
  }

  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();
    if (!mDestroyed) {
      mTemplateRender.onAttachedToWindow();
    }
  }

  @Override
  protected void onDetachedFromWindow() {
    if (!mDestroyed) {
      mTemplateRender.onDetachedFromWindow();
    }
    super.onDetachedFromWindow();
  }
}
