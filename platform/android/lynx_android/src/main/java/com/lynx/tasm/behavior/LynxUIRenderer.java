// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static android.view.View.MeasureSpec;

import android.app.Dialog;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.PixelCopy;
import android.view.Surface;
import android.view.View;
import android.view.ViewParent;
import android.view.WindowManager;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.config.LynxLiteConfigs;
import com.lynx.devtoolwrapper.DevToolOverlayDelegate;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.devtoolwrapper.ScreenshotMode;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.NativeFacade;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxPageLoadListener;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.shadow.LayoutTick;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.performance.TimingCollector;
import com.lynx.tasm.performance.longtasktiming.LynxLongTaskMonitor;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.UnitUtils;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.util.ArrayList;

enum BoxModelOffset {
  PAD_LEFT,
  PAD_TOP,
  PAD_RIGHT,
  PAD_BOTTOM,
  BORDER_LEFT,
  BORDER_TOP,
  BORDER_RIGHT,
  BORDER_BOTTOM,
  MARGIN_LEFT,
  MARGIN_TOP,
  MARGIN_RIGHT,
  MARGIN_BOTTOM,
  LAYOUT_LEFT,
  LAYOUT_TOP,
  LAYOUT_RIGHT,
  LAYOUT_BOTTOM
}

public class LynxUIRenderer implements ILynxUIRenderer {
  protected static final String TAG = "LynxUIRenderer";
  private WeakReference<LynxContext> mLynxContext;
  private LynxUIOwner mLynxUIOwner;
  private TouchEventDispatcher mEventDispatcher;

  private ShadowNodeOwner mShadowNodeOwner;
  private PaintingContext mPaintingContext;
  private TimingCollector mTimingCollector;
  private long mNativeUIDelegatePtr = 0;
  private String mScreenshotMode = ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN;

  // static synchronization object to ensure thread-safe operations of screenshot
  private static final Object mSyncObject = new Object();
  private static HandlerThread mPixelCopyHandlerThread = null;

  public static synchronized void startPixelCopyHandlerThreadIfNecessary() {
    if (mPixelCopyHandlerThread == null && LynxEnv.inst().isLynxDebugEnabled()) {
      mPixelCopyHandlerThread = new HandlerThread("PixelCopier");
      mPixelCopyHandlerThread.start();
    }
  }

  @Override
  public void onInitLynxView(LynxView lynxView, Context context, LynxGroup group) {}

  @Override
  public void onInitLynxTemplateRender(
      LynxContext lynxContext, BehaviorRegistry behaviorRegistry, @Nullable UIBodyView body) {
    // Prepare owner to manage ui and shadow node
    mLynxUIOwner = new LynxUIOwner(lynxContext, behaviorRegistry, body);
    lynxContext.setLynxUIOwner(mLynxUIOwner);
    mEventDispatcher = new TouchEventDispatcher(mLynxUIOwner);
    lynxContext.setTouchEventDispatcher(mEventDispatcher);

    mLynxContext = new WeakReference<>(lynxContext);

    // Check if the handler thread is required and start it if it hasn't been started already.
    // This is necessary for the devtool to take screenshot.
    LynxUIRenderer.startPixelCopyHandlerThreadIfNecessary();
  }

  @Override
  public void attachLynxView(LynxView lynxView, LynxContext lynxContext, Context context) {
    if (lynxContext != null) {
      lynxContext.setBaseContext(context);
      lynxContext.setLynxView(lynxView);
      if (mEventDispatcher != null) {
        mEventDispatcher.attachContext(context);
      }
    }
    if (mLynxUIOwner != null) {
      mLynxUIOwner.attachUIBodyView(lynxView);
    }
  }

  @Override
  public void attachNativeFacade(NativeFacade nativeFacade) {
    if (mLynxUIOwner != null) {
      mLynxUIOwner.attachNativeFacade(nativeFacade);
    }
  }

  @Override
  public void onCreateTemplateRenderer(LynxContext context, TimingCollector timingCollector,
      LynxPageLoadListener pageLoadListener, ThreadStrategyForRendering threadStrategy,
      BehaviorRegistry behaviorRegistry, LayoutTick layoutTick) {
    if (mLynxUIOwner == null) {
      return;
    }
    mTimingCollector = timingCollector;
    mPaintingContext = new PaintingContext(mLynxUIOwner, threadStrategy.id());
    mShadowNodeOwner = new ShadowNodeOwner(context, behaviorRegistry, layoutTick);
    context.setShadowNodeOwner(mShadowNodeOwner);
  }

  @Override
  public long getNativeTimingCollectorPtr() {
    if (mTimingCollector != null) {
      return mTimingCollector.getNativeTimingCollectorPtr();
    }
    return 0;
  }

  @Override
  public long getUIDelegatePtr() {
    if (mNativeUIDelegatePtr == 0) {
      long paintingContextPtr =
          (mPaintingContext != null) ? mPaintingContext.getNativePaintingContextPtr() : 0;
      long layoutContextPtr =
          (mShadowNodeOwner != null) ? mShadowNodeOwner.getNativeLayoutContextPtr() : 0;
      if ((paintingContextPtr != 0) && (layoutContextPtr != 0)) {
        mNativeUIDelegatePtr = nativeCreateUIDelegate(paintingContextPtr, layoutContextPtr);
      }
    }
    return mNativeUIDelegatePtr;
  }

  @Override
  public DisplayMetrics getScreenMetrics() {
    if (mShadowNodeOwner != null) {
      return mShadowNodeOwner.getScreenMetrics();
    }
    return null;
  }

  @Override
  public void onReloadAndInitUIThreadPart() {}

  @Override
  public void onReloadAndInitAnyThreadPart() {
    // destroy event dispatcher
    if (mEventDispatcher != null) {
      mEventDispatcher.destroy();
    }
    if (mLynxUIOwner != null) {
      mLynxUIOwner.reset();
    }
  }

  @Override
  public void onPageConfigDecoded(PageConfig config) {
    if (mEventDispatcher != null) {
      LynxContext lynxContext = (mLynxContext != null) ? mLynxContext.get() : null;
      mEventDispatcher.setTapSlop(
          UnitUtils.toPxWithDisplayMetrics(config.getTapSlop(), 0, 0, 0, 0, 0, 0,
              lynxContext != null ? lynxContext.getScreenMetrics()
                                  : DisplayMetricsHolder.getScreenDisplayMetrics()));
      // Enable touch pseudo if it is fiber arch.
      mEventDispatcher.setHasTouchPseudo(config.getEnableFiberArc());
      // Enable support multi-finger events.
      mEventDispatcher.setEnableMultiTouch(config.getEnableMultiTouch());
      // init if Enable new gesture in page config
      if (config.isEnableNewGesture() && LynxLiteConfigs.enableNewGesture()) {
        mLynxUIOwner.initGestureArenaManager(lynxContext);
        mEventDispatcher.setGestureArenaManager(mLynxUIOwner.getGestureArenaManager());
      }
    }
    if ((mLynxUIOwner != null) && (mLynxUIOwner.getRootUI() != null)) {
      mLynxUIOwner.getRootUI().onPageConfigDecoded(config);
    }
  }

  @Override
  public void onEnterForeground() {}

  @Override
  public void onEnterBackground() {}

  @Override
  public void onDestroyTemplateRenderer() {
    if (mTimingCollector != null) {
      mTimingCollector.destroy();
      mTimingCollector = null;
    }
    if (mShadowNodeOwner != null) {
      mShadowNodeOwner.destroy();
      mShadowNodeOwner = null;
    }
    if (mPaintingContext != null) {
      mPaintingContext.destroy();
      mPaintingContext = null;
    }
    if (mNativeUIDelegatePtr != 0) {
      nativeDestroyUIDelegate(mNativeUIDelegatePtr);
      mNativeUIDelegatePtr = 0;
    }
  }

  @Override
  public void onDestroy() {
    if (mLynxUIOwner != null) {
      mLynxUIOwner.destroy();
    }
    if (mEventDispatcher != null) {
      mEventDispatcher.destroy();
    }
  }

  @Override
  public LynxUIOwner lynxUIOwner() {
    return mLynxUIOwner;
  }

  public UIGroup<UIBody.UIBodyView> getLynxRootUI() {
    return (mLynxUIOwner != null) ? mLynxUIOwner.getRootUI() : null;
  }

  @Override
  public void pauseRootLayoutAnimation() {
    if (mLynxUIOwner != null) {
      mLynxUIOwner.pauseRootLayoutAnimation();
    }
  }

  @Override
  public void resumeRootLayoutAnimation() {
    if (mLynxUIOwner != null) {
      mLynxUIOwner.resumeRootLayoutAnimation();
    }
  }

  @Override
  public void onAttach() {
    if (mLynxUIOwner == null) {
      return;
    }
    UIBody body = mLynxUIOwner.getRootUI();
    if (body != null) {
      body.onAttach();
    }
  }

  @Override
  public void onDetach() {
    if (mLynxUIOwner == null) {
      return;
    }
    UIBody body = mLynxUIOwner.getRootUI();
    if (body != null) {
      body.onDetach();
    }
  }

  @Override
  public void onEnterForegroundInternal() {
    if (mLynxUIOwner != null) {
      mLynxUIOwner.onEnterForeground();
    }
  }

  @Override
  public void onEnterBackgroundInternal() {
    if (mLynxUIOwner != null) {
      mLynxUIOwner.onEnterBackground();
    }
  }

  @Override
  public void setContextFree(boolean isContextFree) {
    if (mLynxUIOwner != null) {
      mLynxUIOwner.setContextFree(isContextFree);
    }
  }

  @Override
  public void setFirstLayout() {
    if (mLynxUIOwner != null) {
      mLynxUIOwner.setFirstLayout();
    }
  }

  @Override
  public LynxBaseUI findLynxUIByName(@NonNull String name) {
    return (mLynxUIOwner != null) ? mLynxUIOwner.findLynxUIByName(name) : null;
  }

  @Override
  public LynxBaseUI findLynxUIByIdSelector(@NonNull String id) {
    return (mLynxUIOwner != null) ? mLynxUIOwner.findLynxUIByIdSelector(id) : null;
  }

  @Override
  public LynxBaseUI findLynxUIByIndex(@NonNull int index) {
    return (mLynxUIOwner != null) ? mLynxUIOwner.findLynxUIByIndex(index) : null;
  }

  @Override
  public boolean onTouchEvent(MotionEvent ev, UIGroup rootUi) {
    return (mEventDispatcher != null) ? mEventDispatcher.onTouchEvent(ev, rootUi) : false;
  }

  @Override
  public boolean consumeSlideEvent(MotionEvent ev) {
    return (mEventDispatcher != null) ? mEventDispatcher.consumeSlideEvent(ev) : false;
  }

  @Override
  public boolean blockNativeEvent(MotionEvent ev) {
    return (mEventDispatcher != null) ? mEventDispatcher.blockNativeEvent(ev) : false;
  }

  @Override
  public ThreadStrategyForRendering getSupportedThreadStrategy(
      ThreadStrategyForRendering threadStrategy) {
    return threadStrategy;
  }

  @Override
  public void performInnerMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    LynxContext lynxContext = (mLynxContext != null) ? mLynxContext.get() : null;
    LynxView lynxView = (lynxContext != null) ? lynxContext.getLynxView() : null;
    if ((mLynxUIOwner == null) || (lynxView == null)) {
      LLog.e(TAG,
          "performInnerMeasure failed, mLynxUIOwner:" + mLynxUIOwner + ",lynxView:" + lynxView);
      return;
    }
    mLynxUIOwner.performMeasure();
    int width = 0;
    int height = 0;
    int widthMode = MeasureSpec.getMode(widthMeasureSpec);
    if (widthMode == MeasureSpec.AT_MOST || widthMode == MeasureSpec.UNSPECIFIED) {
      width = mLynxUIOwner.getRootWidth();
    } else {
      width = MeasureSpec.getSize(widthMeasureSpec);
    }
    int heightMode = MeasureSpec.getMode(heightMeasureSpec);
    if (heightMode == MeasureSpec.AT_MOST || heightMode == MeasureSpec.UNSPECIFIED) {
      height = mLynxUIOwner.getRootHeight();
    } else {
      height = MeasureSpec.getSize(heightMeasureSpec);
    }
    lynxView.innerSetMeasuredDimension(width, height);
  }

  @Override
  public void onLayout(boolean changed, int left, int top, int right, int bottom) {
    if (mLynxUIOwner == null) {
      LLog.e(TAG, "onLayout failed, mLynxUIOwner is null");
      return;
    }
    LynxContext lynxContext = (mLynxContext != null) ? mLynxContext.get() : null;
    if (lynxContext != null) {
      String eventName = "LynxTemplateRender.Layout";
      LynxLongTaskMonitor.willProcessTask(eventName, lynxContext.getInstanceId());
      mLynxUIOwner.performLayout();
      LynxLongTaskMonitor.didProcessTask();
    } else {
      mLynxUIOwner.performLayout();
    }
  }

  @Override
  public boolean useInvokeUIMethod() {
    return false;
  }

  @Override
  public boolean isAccessibilityDisabled() {
    return false;
  }

  @Override
  public boolean enableTimingCollector() {
    return true;
  }

  @Override
  public boolean shouldInvokeNativeViewMethod() {
    return false;
  }

  @Override
  public boolean disableBindDrawChildHook() {
    return false;
  }

  @Override
  public boolean needHandleDispatchKeyEvent() {
    return false;
  }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event) {
    return false;
  }

  @Override
  public void scrollIntoViewFromUI(int nodeId) {
    if (mLynxUIOwner == null || mLynxUIOwner.getRootSign() == -1) {
      return;
    }
    LynxBaseUI ui = mLynxUIOwner.getNode(nodeId);
    if (ui != null) {
      ui.scrollIntoView(false, "center", "center", null);
    }
  }

  @Override
  public String getActualScreenshotMode() {
    return mScreenshotMode;
  }

  @Override
  public void takeScreenshot(ScreenshotBitmapHandler handler, String screenShotMode) {
    mScreenshotMode = screenShotMode;
    Bitmap bitmap;
    if (mScreenshotMode != null
        && mScreenshotMode.equals(ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW)) {
      bitmap = getBitmapOfView();
    } else {
      bitmap = getBitmapOfScreen();
    }
    handler.sendBitmap(bitmap);
  }

  private @ColorInt int getParentBackgroundColor(View targetView) {
    if (targetView != null) {
      try {
        ViewParent view;
        ViewParent parentView = targetView.getParent();
        Drawable draw;
        while (parentView instanceof View) {
          view = parentView;
          parentView = view.getParent();
          draw = ((View) view).getBackground();
          if (draw instanceof ColorDrawable) {
            int color = ((ColorDrawable) draw).getColor();
            if (color != Color.TRANSPARENT) {
              return color;
            }
          }
        }
      } catch (Throwable e) {
        e.printStackTrace();
      }
    }
    return Color.WHITE;
  }

  // draw view to given bitmap of the given canvas
  // whether to use pixel copy or view.draw depends on android version and pixelCopy switch, default
  // use view.draw
  private void drawViewToBitmap(View view, Bitmap bitmap, Canvas canvas) {
    try {
      // pixel copy for e2e and some special circumstances, switch off by default
      // Above Android 8.0, use PixelCopy
      if (LynxEnv.inst().isPixelCopyEnabled() && Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        int[] location = new int[2];
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
          view.getLocationInSurface(location);
        } else {
          view.getLocationInWindow(location);
        }

        Class<?> clazz = Class.forName("android.view.ViewRootImpl");
        Field field = clazz.getDeclaredField("mSurface");
        if (view.getRootView().getParent() == null) {
          view.draw(canvas);
          return;
        }
        Surface surface = (Surface) field.get(view.getRootView().getParent());

        synchronized (mSyncObject) {
          PixelCopy.request(surface,
              new Rect(location[0], location[1], location[0] + view.getWidth(),
                  location[1] + view.getHeight()),
              bitmap, new PixelCopy.OnPixelCopyFinishedListener() {
                @Override
                public void onPixelCopyFinished(int copyResult) {
                  synchronized (mSyncObject) {
                    mSyncObject.notify();
                  }
                }
              }, new Handler(mPixelCopyHandlerThread.getLooper()));
          try {
            // causes the current thread to wait up to 10 seconds until the PixelCopy operation
            // is completed and the thread is notified.
            mSyncObject.wait(10000);
          } catch (InterruptedException e) {
            LLog.e("DevTool Screenshot", e.toString());
          }
        }
      } else {
        view.draw(canvas);
      }
    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  private Bitmap createScreenBitmap(View view) {
    if (view == null || view.getContext() == null) {
      return null;
    }
    WindowManager wm = (WindowManager) view.getContext().getSystemService(Context.WINDOW_SERVICE);
    if (wm == null) {
      return null;
    }
    Display display = wm.getDefaultDisplay();
    DisplayMetrics dm = new DisplayMetrics();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      display.getRealMetrics(dm);
    } else {
      display.getMetrics(dm);
    }
    int screenWidth = dm.widthPixels;
    int screenHeight = dm.heightPixels;
    Bitmap screenBitmap = Bitmap.createBitmap(screenWidth, screenHeight, Bitmap.Config.ARGB_8888);
    return screenBitmap;
  }

  private void drawOverlayViewToScreenCanvas(
      Canvas screenCanvas, ArrayList<View> overlayDecoderView) {
    if (screenCanvas == null || overlayDecoderView == null) {
      return;
    }
    for (int i = 0, size = overlayDecoderView.size(); i < size; i++) {
      View overlayView = overlayDecoderView.get(i);
      Bitmap overlayBitmap = Bitmap.createBitmap(
          overlayView.getWidth(), overlayView.getHeight(), Bitmap.Config.ARGB_8888);
      final Canvas overlayCanvas = new Canvas(overlayBitmap);
      drawViewToBitmap(overlayView, overlayBitmap, overlayCanvas);
      int[] loc = new int[2];
      overlayView.getLocationOnScreen(loc);
      Rect overlayRect = new Rect(
          loc[0], loc[1], loc[0] + overlayView.getWidth(), loc[1] + overlayView.getHeight());
      screenCanvas.drawBitmap(overlayBitmap, null, overlayRect, null);
      overlayBitmap.recycle();
    }
  }

  // get bitmap of full screen when take screenshot
  private Bitmap getBitmapOfScreen() {
    View view = mLynxContext.get().getLynxView();
    if (view == null || view.getWidth() <= 0 || view.getHeight() <= 0) {
      return null;
    }

    // ScreenBitmap: createBitmap use ScreenWidth ScreenHeight
    // lynxDecoderView : lynxView's DecoderView
    // overlayDecoderView: array store all overlay's DecoderView
    Bitmap screenBitmap = createScreenBitmap(view);
    if (screenBitmap == null) {
      LLog.i(TAG, "getBitmapOfScreen: get null from createScreenBitmap");
      return null;
    }
    View lynxDecoderView = view.getRootView();
    LLog.e(TAG, "getBitmapOfScreen: lynxDecoderView is null");
    if (lynxDecoderView == null) {
      return null;
    }
    final Canvas screenCanvas = new Canvas(screenBitmap);
    screenCanvas.drawColor(getParentBackgroundColor(lynxDecoderView));
    // get overlay view
    ArrayList<Dialog> array = DevToolOverlayDelegate.getInstance().getGlobalOverlayNGView();
    ArrayList<View> overlayDecoderView = new ArrayList<View>();
    if (array != null) {
      for (int i = 0, size = array.size(); i < size; i++) {
        overlayDecoderView.add(array.get(i).getWindow().getDecorView());
      }
    }
    Bitmap lynxBitmap = Bitmap.createBitmap(
        lynxDecoderView.getWidth(), lynxDecoderView.getHeight(), Bitmap.Config.ARGB_8888);
    final Canvas lynxCanvas = new Canvas(lynxBitmap);

    drawViewToBitmap(lynxDecoderView, lynxBitmap, lynxCanvas);

    // draw lynxDecoderView and overlayDecoderView to ScreenBitmap and return Bitmap
    int[] lynx_loc = new int[2];
    lynxDecoderView.getLocationOnScreen(lynx_loc);
    Rect lynx_rect = new Rect(lynx_loc[0], lynx_loc[1], lynx_loc[0] + lynxDecoderView.getWidth(),
        lynx_loc[1] + lynxDecoderView.getHeight());
    screenCanvas.drawBitmap(lynxBitmap, null, lynx_rect, null);
    lynxBitmap.recycle();
    drawOverlayViewToScreenCanvas(screenCanvas, overlayDecoderView);
    return screenBitmap;
  }

  @Override
  public Bitmap getBitmapOfView() {
    View view = mLynxContext.get().getLynxView();
    if (view == null || view.getWidth() <= 0 || view.getHeight() <= 0) {
      return null;
    }
    Bitmap bitmap = Bitmap.createBitmap(view.getWidth(), view.getHeight(), Bitmap.Config.RGB_565);
    Canvas canvas = new Canvas(bitmap);
    // Draw bg color
    canvas.drawColor(getParentBackgroundColor(view));
    // Draw content
    drawViewToBitmap(view, bitmap, canvas);
    return bitmap;
  }

  /*
   *convert point from screen to given view
   */
  public float[] convertPointFromScreenToGivenUI(float x, float y, @NonNull LynxBaseUI ui) {
    float[] point = {x, y};
    float[] base_point = new float[2];

    base_point = ui.getLocationOnScreen(base_point);

    point[0] -= base_point[0];
    point[1] -= base_point[1];

    return point;
  }

  /*
   *find the minimum ui node which the point falls in
   *
   *Parameter:
   * x,y is coordinate to the screen of the point
   * uiSign is the id of the starting search node, lynxView or overlay view
   * thus,before calling view's hitTest function, We need to first convert the coordinates relative
   *to the screen into coordinates relative to the view
   *
   * Return Value:
   * the id of the found node , return 0 if not found
   */
  public int findNodeIdForLocationFromUI(float x, float y, int uiSign, String mode) {
    if (mLynxUIOwner != null) {
      LynxBaseUI ui;
      if (uiSign == 0) {
        ui = mLynxUIOwner.getRootUI();
      } else {
        // this result is LynxOverlayViewProxyNG
        ui = mLynxUIOwner.getNode(uiSign);
        // the LynxUI we need is LynxOverlayViewNG
        if (ui != null) {
          ui = ((LynxUI) ui).getTransitionUI();
        }
      }
      if (ui != null) {
        float[] point = {x, y};
        if (mode != null && mode.equals(ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN)) {
          point = convertPointFromScreenToGivenUI(x, y, ui);
        }
        EventTarget target = ui.hitTest(point[0], point[1], true);
        if (target != null) {
          return target.getSign();
        }
      }
    }
    return 0;
  }

  @Override
  public int getNodeForLocation(float x, float y, String mode) {
    int node_id = 0;
    if (mode == null) {
      return node_id;
    }
    if (mode.equals(ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN)) {
      ArrayList<Integer> overlays = DevToolOverlayDelegate.getInstance().getAllVisibleOverlaySign();
      if (overlays != null) {
        for (int i = overlays.size() - 1; i >= 0; i--) {
          node_id = findNodeIdForLocationFromUI(x, y, overlays.get(i), mode);
          // overlay node's size is window size and it has one and only
          // one child if id == overlays[i], it means point is not in child so
          // not in overlay Under this circumstances,we need reset id to 0
          if (node_id != 0 && node_id != overlays.get(i)) {
            return node_id;
          } else {
            node_id = 0;
          }
        }
      }
      node_id = node_id != 0 ? node_id : findNodeIdForLocationFromUI(x, y, 0, mode);
    } else { // lynxview
      node_id = findNodeIdForLocationFromUI(x, y, 0, mode);
    }
    return node_id;
  }

  @Override
  public float[] getTransformValue(int sign, float[] padBorderMarginLayout) {
    float[] res = new float[32];
    if (mLynxUIOwner != null) {
      LynxBaseUI ui = mLynxUIOwner.getNode(sign);
      if (ui != null) {
        for (int i = 0; i < 4; i++) {
          LynxBaseUI.TransOffset arr;
          if (i == 0) {
            arr = ui.getTransformValue(padBorderMarginLayout[BoxModelOffset.PAD_LEFT.ordinal()]
                    + padBorderMarginLayout[BoxModelOffset.BORDER_LEFT.ordinal()]
                    + padBorderMarginLayout[BoxModelOffset.LAYOUT_LEFT.ordinal()],
                -padBorderMarginLayout[BoxModelOffset.PAD_RIGHT.ordinal()]
                    - padBorderMarginLayout[BoxModelOffset.BORDER_RIGHT.ordinal()]
                    - padBorderMarginLayout[BoxModelOffset.LAYOUT_RIGHT.ordinal()],
                padBorderMarginLayout[BoxModelOffset.PAD_TOP.ordinal()]
                    + padBorderMarginLayout[BoxModelOffset.BORDER_TOP.ordinal()]
                    + padBorderMarginLayout[BoxModelOffset.LAYOUT_TOP.ordinal()],
                -padBorderMarginLayout[BoxModelOffset.PAD_BOTTOM.ordinal()]
                    - padBorderMarginLayout[BoxModelOffset.BORDER_BOTTOM.ordinal()]
                    - padBorderMarginLayout[BoxModelOffset.LAYOUT_BOTTOM.ordinal()]);
          } else if (i == 1) {
            arr = ui.getTransformValue(padBorderMarginLayout[BoxModelOffset.BORDER_LEFT.ordinal()]
                    + padBorderMarginLayout[BoxModelOffset.LAYOUT_LEFT.ordinal()],
                -padBorderMarginLayout[BoxModelOffset.BORDER_RIGHT.ordinal()]
                    - padBorderMarginLayout[BoxModelOffset.LAYOUT_RIGHT.ordinal()],
                padBorderMarginLayout[BoxModelOffset.BORDER_TOP.ordinal()]
                    + padBorderMarginLayout[BoxModelOffset.LAYOUT_TOP.ordinal()],
                -padBorderMarginLayout[BoxModelOffset.BORDER_BOTTOM.ordinal()]
                    - padBorderMarginLayout[BoxModelOffset.LAYOUT_BOTTOM.ordinal()]);
          } else if (i == 2) {
            arr = ui.getTransformValue(padBorderMarginLayout[BoxModelOffset.LAYOUT_LEFT.ordinal()],
                -padBorderMarginLayout[BoxModelOffset.LAYOUT_RIGHT.ordinal()],
                padBorderMarginLayout[BoxModelOffset.LAYOUT_TOP.ordinal()],
                -padBorderMarginLayout[BoxModelOffset.LAYOUT_BOTTOM.ordinal()]);
          } else {
            arr = ui.getTransformValue(-padBorderMarginLayout[BoxModelOffset.MARGIN_LEFT.ordinal()]
                    + padBorderMarginLayout[BoxModelOffset.LAYOUT_LEFT.ordinal()],
                padBorderMarginLayout[BoxModelOffset.MARGIN_RIGHT.ordinal()]
                    - padBorderMarginLayout[BoxModelOffset.LAYOUT_RIGHT.ordinal()],
                -padBorderMarginLayout[BoxModelOffset.MARGIN_TOP.ordinal()]
                    + padBorderMarginLayout[BoxModelOffset.LAYOUT_TOP.ordinal()],
                padBorderMarginLayout[BoxModelOffset.MARGIN_BOTTOM.ordinal()]
                    - padBorderMarginLayout[BoxModelOffset.LAYOUT_BOTTOM.ordinal()]);
          }
          /**
           * The arr returns the x and y coordinates of four points, a total of 8 numbers,
           * which are stored in the res array in the order of top-left, top-right, bottom-right,
           * bottom-left.
           */

          if (arr != null) {
            res[i * 8] = arr.left_top[0];
            res[i * 8 + 1] = arr.left_top[1];
            res[i * 8 + 2] = arr.right_top[0];
            res[i * 8 + 3] = arr.right_top[1];
            res[i * 8 + 4] = arr.right_bottom[0];
            res[i * 8 + 5] = arr.right_bottom[1];
            res[i * 8 + 6] = arr.left_bottom[0];
            res[i * 8 + 7] = arr.left_bottom[1];
          }
        }
      }
    }
    return res;
  }

  private static native long nativeCreateUIDelegate(long paintingContextPtr, long layoutContextPtr);

  private static native void nativeDestroyUIDelegate(long uiDelegatePtr);
}
