// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.NativeFacade;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.base.LynxPageLoadListener;
import com.lynx.tasm.behavior.shadow.LayoutTick;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.UIGroup;

public interface ILynxUIRenderer {
  void onInitBodyView(UIBodyView bodyView, Context context, LynxGroup group);

  void onInitLynxTemplateRender(LynxContext context, BehaviorRegistry behaviorRegistry,
      @Nullable UIBodyView body, @Nullable LynxBooleanOption longTaskMonitorEnabled);

  void onCreateTemplateRenderer(LynxContext context, LynxPageLoadListener pageLoadListener,
      ThreadStrategyForRendering threadStrategy, BehaviorRegistry behaviorRegistry,
      LayoutTick layoutTick);

  void attachBodyView(UIBodyView bodyView, LynxContext lynxContext, Context context);

  void attachNativeFacade(NativeFacade nativeFacade);

  void setLynxEngineForPlatformRendererContext(long ptr);

  void onReloadAndInitUIThreadPart();

  void onReloadAndInitAnyThreadPart();

  long getUIDelegatePtr();

  DisplayMetrics getScreenMetrics();

  void onPageConfigDecoded(PageConfig config);

  void onEnterForeground();

  void onEnterBackground();

  void onDestroyTemplateRenderer();

  void onDestroy();

  LynxUIOwner lynxUIOwner();

  UIGroup<UIBody.UIBodyView> getLynxRootUI();

  void pauseRootLayoutAnimation();

  void resumeRootLayoutAnimation();

  void onAttach();

  void onDetach();

  void onEnterForegroundInternal();

  void onEnterBackgroundInternal();

  void setContextFree(boolean isContextFree);

  void setFirstLayout();

  LynxBaseUI findLynxUIByName(@NonNull String name);

  LynxBaseUI findLynxUIByIdSelector(@NonNull String id);

  LynxBaseUI findLynxUIByIndex(@NonNull int index);

  boolean onTouchEvent(MotionEvent ev, UIGroup rootUi);

  boolean onInterceptTouchEvent(MotionEvent ev);

  boolean consumeSlideEvent(MotionEvent ev);

  boolean blockNativeEvent(MotionEvent ev);

  ThreadStrategyForRendering getSupportedThreadStrategy(ThreadStrategyForRendering threadStrategy);

  void performInnerMeasure(int widthMeasureSpec, int heightMeasureSpec);

  void onLayout(boolean changed, int left, int top, int right, int bottom);

  boolean useInvokeUIMethod();

  boolean isAccessibilityDisabled();

  boolean enableTimingCollector();

  boolean shouldInvokeNativeViewMethod();

  boolean disableBindDrawChildHook();

  boolean needHandleDispatchKeyEvent();
  boolean dispatchKeyEvent(KeyEvent event);

  void scrollIntoViewFromUI(int nodeId);

  String getActualScreenshotMode();

  void takeScreenshot(ScreenshotBitmapHandler handler, String screenShotMode);

  Bitmap getBitmapOfView();

  void drawViewToBitmap(View view, Bitmap bitmap, Canvas canvas);

  int getNodeForLocation(float x, float y, String mode);

  float[] getTransformValue(int sign, float[] padBorderMarginLayout);
}
