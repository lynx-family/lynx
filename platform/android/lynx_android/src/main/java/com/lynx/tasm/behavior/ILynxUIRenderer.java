// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.NativeFacade;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.base.LynxPageLoadListener;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.shadow.LayoutTick;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.performance.TimingCollector;

public interface ILynxUIRenderer {
  public void onInitLynxView(LynxView lynxView, Context context, LynxGroup group);

  public void onInitLynxTemplateRender(
      LynxContext context, BehaviorRegistry behaviorRegistry, @Nullable UIBodyView body);

  public void onCreateTemplateRenderer(LynxContext context, TimingCollector timingCollector,
      LynxPageLoadListener pageLoadListener, ThreadStrategyForRendering threadStrategy,
      BehaviorRegistry behaviorRegistry, LayoutTick layoutTick);

  public void attachLynxView(LynxView lynxView, LynxContext lynxContext, Context context);

  public void attachNativeFacade(NativeFacade nativeFacade);

  public void onReloadAndInitUIThreadPart();

  public void onReloadAndInitAnyThreadPart();

  // Native pointer
  long getNativeTimingCollectorPtr();
  long getUIDelegatePtr();

  DisplayMetrics getScreenMetrics();

  public void onPageConfigDecoded(PageConfig config);

  public void onEnterForeground();

  public void onEnterBackground();

  public void onDestroyTemplateRenderer();

  public void onDestroy();

  public LynxUIOwner lynxUIOwner();

  public UIGroup<UIBody.UIBodyView> getLynxRootUI();

  public void pauseRootLayoutAnimation();

  public void resumeRootLayoutAnimation();

  public void onAttach();

  public void onDetach();

  public void onEnterForegroundInternal();

  public void onEnterBackgroundInternal();

  public void setContextFree(boolean isContextFree);

  public void setFirstLayout();

  public LynxBaseUI findLynxUIByName(@NonNull String name);

  public LynxBaseUI findLynxUIByIdSelector(@NonNull String id);

  public LynxBaseUI findLynxUIByIndex(@NonNull int index);

  public boolean onTouchEvent(MotionEvent ev, UIGroup rootUi);

  public boolean consumeSlideEvent(MotionEvent ev);

  public boolean blockNativeEvent(MotionEvent ev);

  public ThreadStrategyForRendering getSupportedThreadStrategy(
      ThreadStrategyForRendering threadStrategy);

  public void performInnerMeasure(int widthMeasureSpec, int heightMeasureSpec);

  public void onLayout(boolean changed, int left, int top, int right, int bottom);

  public boolean useInvokeUIMethod();

  public boolean isAccessibilityDisabled();

  public boolean enableTimingCollector();

  public boolean shouldInvokeNativeViewMethod();

  public boolean disableBindDrawChildHook();

  public boolean needHandleDispatchKeyEvent();
  public boolean dispatchKeyEvent(KeyEvent event);

  public void scrollIntoViewFromUI(int nodeId);

  public String getActualScreenshotMode();

  public void takeScreenshot(ScreenshotBitmapHandler handler, String screenShotMode);

  public Bitmap getBitmapOfView();

  public int getNodeForLocation(float x, float y, String mode);

  public float[] getTransformValue(int sign, float[] padBorderMarginLayout);
}
