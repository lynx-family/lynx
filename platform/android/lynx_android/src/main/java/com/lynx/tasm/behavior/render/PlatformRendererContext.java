// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.Matrix;
import android.graphics.PointF;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;
import com.lynx.react.bridge.mapbuffer.ReadableMapBuffer;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.shadow.ShadowNodeType;
import com.lynx.tasm.behavior.shadow.TextLayout;
import com.lynx.tasm.behavior.shadow.TextMeasurerProvider;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.PropBundle;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.utils.LynxUIMethodsExecutor;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.performance.PerformanceController;
import com.lynx.tasm.service.ILynxTextService.Page;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class PlatformRendererContext implements TextMeasurerProvider, LayerRenderContext {
  // Must match NodeInfoBits::kSupportFragmentLayerChildrenMask in Element.
  static final int SUPPORT_FRAGMENT_LAYER_CHILDREN = 1 << 18;
  final private static String TAG = "PlatformRendererContext";
  final private static String TENDS_TO_FLATTEN_INIT_DATA_KEY = "__lynx_tends_to_flatten";

  public static final class PlatformRendererType {
    public static final int kUnknown = 0;
    public static final int kView = 1;
    public static final int kPage = 2;
    public static final int kScroll = 3;
    public static final int kText = 4;
    public static final int kImage = 5;
    public static final int kList = 6;
    public static final int kListItem = 7;
    public static final int kExtended = 8;
  }

  private static final int IMAGE_PAINT_INFO_MODE = 1;
  private static final int IMAGE_PAINT_INFO_BLUR_RADIUS = 2;
  private static final int IMAGE_PAINT_INFO_AUTO_SIZE = 3;
  private static final int IMAGE_PAINT_INFO_PLACEHOLDER = 4;
  private static final int IMAGE_PAINT_INFO_TINT_COLOR = 5;
  private static final int IMAGE_PAINT_INFO_CAP_INSETS = 6;
  private static final int IMAGE_PAINT_INFO_CAP_INSETS_SCALE = 7;
  private static final int IMAGE_PAINT_INFO_SKIP_REDIRECTION = 8;
  private static final int IMAGE_PAINT_INFO_AUTOPLAY = 9;
  private static final int IMAGE_PAINT_INFO_LOOP_COUNT = 10;

  WeakReference<UIBody.UIBodyView> mRootView = null;

  @Nullable private PlatformFocusTarget mFocusedTarget = null;

  private LynxContext mContext;
  private BehaviorRegistry mBehaviorRegistry;
  private long mNativePtr = 0;
  private TextLayout mTextLayout;
  private boolean mDestroyed = false;

  private ConcurrentHashMap<Integer, Object> mExtraDatas = new ConcurrentHashMap<>();

  // TextMeasurer instance for text measurement functionality
  private TextMeasurer mTextMeasurer = null;

  public TextMeasurer getTextMeasurer() {
    return mTextMeasurer;
  }

  public PlatformRendererContext(@Nullable UIBody.UIBodyView rootView, LynxContext context,
      BehaviorRegistry behaviorRegistry) {
    if (rootView != null) {
      this.mRootView = new WeakReference<>(rootView);
    }
    this.mContext = context;
    this.mBehaviorRegistry = behaviorRegistry;
    this.mNativePtr = nativeCreateEmbeddedViewContext(this);

    // Initialize TextMeasurer if layout mode is enabled
    if (context != null && context.isLayoutInElementModeOn()) {
      this.mTextMeasurer = new TextMeasurer(context);
      mTextLayout = new TextLayout(this);
    }
  }

  public void setRootView(@NonNull UIBody.UIBodyView rootView) {
    this.mRootView = new WeakReference<>(rootView);
  }

  public LynxContext getLynxContext() {
    return mContext;
  }

  public long getNativePtr() {
    return mNativePtr;
  }

  @Nullable
  LynxBaseUI getPlatformRendererUI(int sign) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    return owner != null ? owner.getFragmentLayer(sign) : null;
  }

  @Override
  @Nullable
  public LynxBaseUI getFragmentLayer(int sign) {
    return getPlatformRendererUI(sign);
  }

  @CalledByNative
  public float[] getRootViewLocationOnScreen() {
    float[] res = new float[] {0, 0};
    UIBody.UIBodyView view = mRootView != null ? mRootView.get() : null;
    if (view != null) {
      int[] location = new int[2];
      view.getLocationOnScreen(location);
      res[0] = location[0];
      res[1] = location[1];
    }
    return res;
  }

  @CalledByNative
  public float[] getScreenSize() {
    float[] res = new float[] {0, 0};
    if (mContext != null) {
      DisplayMetrics metrics = DisplayMetricsHolder.getRealScreenDisplayMetrics(mContext);
      res[0] = metrics.widthPixels;
      res[1] = metrics.heightPixels;
    }
    return res;
  }

  @CalledByNative
  float[] getRendererHostScrollOffset(int sign) {
    float[] res = new float[] {0, 0};
    LynxBaseUI ui = getPlatformRendererUI(sign);
    View view = ui != null ? ui.getFragmentLayerView() : null;
    if (view instanceof AndroidScrollView) {
      AndroidScrollView scrollView = (AndroidScrollView) view;
      res[0] = scrollView.getRealScrollX();
      res[1] = scrollView.getRealScrollY();
    } else if (ui != null) {
      res[0] = ui.getFragmentLayerScrollX();
      res[1] = ui.getFragmentLayerScrollY();
    }
    return res;
  }

  @CalledByNative
  boolean isRendererHostScrollable(int sign) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    return ui != null && ui.isScrollable();
  }

  @CalledByNative
  private void invokeUIMethod(
      int sign, String method, ReadableMap params, long nativePtr, int callbackId) {
    Callback callback = createInvokeUIMethodCallback(nativePtr, callbackId);
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (mDestroyed || mNativePtr == 0) {
          return;
        }
        LynxBaseUI ui = findUIMethodTarget(sign);
        if (ui != null) {
          LynxUIMethodsExecutor.invokeMethod(ui, method, params,
              (Object... args)
                  -> UIThreadUtils.runOnUiThreadImmediately(() -> callback.invoke(args)));
        } else {
          callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE, "node does not have a LynxUI");
        }
      }
    });
  }

  private Callback createInvokeUIMethodCallback(long nativePtr, int callbackId) {
    return (Object... args) -> {
      if (mDestroyed || mNativePtr == 0 || nativePtr == 0 || callbackId < 0) {
        return;
      }
      if (args == null || args.length == 0) {
        nativeInvokeUIMethodCallback(
            nativePtr, callbackId, LynxUIMethodConstants.SUCCESS, new JavaOnlyArray());
        return;
      }
      if (args[0] instanceof Number) {
        int code = ((Number) args[0]).intValue();
        Object[] data = new Object[args.length - 1];
        if (args.length > 1) {
          System.arraycopy(args, 1, data, 0, args.length - 1);
        }
        nativeInvokeUIMethodCallback(nativePtr, callbackId, code, JavaOnlyArray.of(data));
        return;
      }
      nativeInvokeUIMethodCallback(
          nativePtr, callbackId, LynxUIMethodConstants.SUCCESS, JavaOnlyArray.of(args));
    };
  }

  private LynxBaseUI findUIMethodTarget(int sign) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    return owner != null ? owner.getNode(sign) : null;
  }

  PointF convertPointInViewToScreen(int sign, PointF point) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    if (ui == null) {
      LLog.e(TAG, "convertPointInViewToScreen failed since can not find target UI.");
      return point;
    }
    return ui.convertPointInFragmentLayerToScreen(point);
  }

  public int getTargetWidth(int sign) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    if (ui == null) {
      LLog.e(TAG, "getTargetWidth failed since can not find target UI.");
      return 0;
    }

    return ui.getFragmentLayerWidth();
  }

  public int getTargetHeight(int sign) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    if (ui == null) {
      LLog.e(TAG, "getTargetHeight failed since can not find target UI.");
      return 0;
    }

    return ui.getFragmentLayerHeight();
  }

  public int getMeaningfulPaintingAreaVisibleStatus(int sign) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    View view = ui != null ? ui.getFragmentLayerView() : null;
    if (view == null) {
      return View.VISIBLE;
    }
    return view.getVisibility();
  }

  public float getMeaningfulPaintingAreaAlpha(int sign) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    View view = ui != null ? ui.getFragmentLayerView() : null;
    if (view == null) {
      return 1.f;
    }
    return view.getAlpha();
  }

  public float getMeaningfulPaintingAreaScaleX(int sign) {
    return getMeaningfulPaintingAreaScale(sign, true);
  }

  public float getMeaningfulPaintingAreaScaleY(int sign) {
    return getMeaningfulPaintingAreaScale(sign, false);
  }

  private float getMeaningfulPaintingAreaScale(int sign, boolean scaleX) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    View view = ui != null ? ui.getFragmentLayerView() : null;
    if (view == null) {
      return 1.f;
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      Matrix animationMatrix = view.getAnimationMatrix();
      if (animationMatrix != null && !animationMatrix.isIdentity()) {
        float[] values = new float[9];
        // cspell:ignore MSCALE MSKEW
        animationMatrix.getValues(values);
        if (scaleX) {
          return (float) Math.hypot(values[Matrix.MSCALE_X], values[Matrix.MSKEW_Y]);
        }
        return (float) Math.hypot(values[Matrix.MSKEW_X], values[Matrix.MSCALE_Y]);
      }
    }
    return scaleX ? view.getScaleX() : view.getScaleY();
  }

  @CalledByNative
  public void createPlatformRenderer(int sign, int type) {
    switch (type) {
      case PlatformRendererType.kView:
      case PlatformRendererType.kText:
      case PlatformRendererType.kImage:
      case PlatformRendererType.kList:
      case PlatformRendererType.kListItem:
      case PlatformRendererType.kScroll: {
        LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
        if (owner == null) {
          LLog.e(TAG, "createPlatformRenderer failed since LynxUIOwner is null.");
          return;
        }
        owner.createFragmentLayer(sign, this);
        break;
      }
      case PlatformRendererType.kPage: {
        LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
        if (owner == null || owner.attachRootFragmentLayer(sign, this) == null) {
          LLog.e(TAG, "create page fragment layer failed since root UI is unavailable.");
        }
        break;
      }
      default:
        // TODO: support customized PlatformRendererHostView.
        break;
    }
  }

  @CalledByNative
  public int getTagInfo(String tagName) {
    int info = 0;
    Behavior behavior;
    try {
      behavior = mBehaviorRegistry.get(tagName);
    } catch (RuntimeException ignored) {
      // When BehaviorRegistry cannot find Behavior by tagName, it will throw RuntimeException.
      // However, a tag without corresponding Behavior is NOT virtual by default.
      // Here is no exception in logic, so just ignore the RuntimeException.
      return info;
    }
    ShadowNode node = null;
    if (behavior != null) {
      if (behavior.supportFragmentLayerChildren()) {
        info |= SUPPORT_FRAGMENT_LAYER_CHILDREN;
      }
      node = behavior.createShadowNode();
    }
    if (node != null) {
      info |= ShadowNodeType.CUSTOM;
      if (node.isVirtual()) {
        info |= ShadowNodeType.VIRTUAL;
      }
    } else {
      info |= ShadowNodeType.COMMON;
    }
    return info;
  }

  @CalledByNative
  public void createPlatformExtendedRenderer(int sign, String tagName, PropBundle initData) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner == null) {
      LLog.e(TAG, "createPlatformExtendedRenderer failed since LynxUIOwner is null.");
      return;
    }

    Behavior behavior = mBehaviorRegistry != null ? mBehaviorRegistry.get(tagName) : null;
    ReadableMap initialProps = initData != null ? initData.getProps() : null;
    ReadableArray eventListeners = initData != null ? initData.getEventHandlers() : null;
    ReadableArray gestureDetectors = initData != null ? initData.getGestures() : null;
    boolean isFlatten = shouldCreateFallbackUIAsFlatten(behavior, initialProps);
    owner.createView(
        sign, tagName, initialProps, null, eventListeners, isFlatten, sign, gestureDetectors);
    LynxBaseUI layerUI = owner.attachFragmentLayer(sign, this);
    if (layerUI == null) {
      owner.cleanupCreatedView(sign, tagName, initialProps);
    }
  }

  private boolean shouldCreateFallbackUIAsFlatten(
      @Nullable Behavior behavior, @Nullable ReadableMap props) {
    boolean tendsToFlatten =
        props != null && props.getBoolean(TENDS_TO_FLATTEN_INIT_DATA_KEY, false);
    return shouldCreateFallbackUIAsFlatten(behavior, tendsToFlatten);
  }

  private boolean shouldCreateFallbackUIAsFlatten(
      @Nullable Behavior behavior, boolean tendsToFlatten) {
    return tendsToFlatten && behavior != null && behavior.supportUIFlatten();
  }

  void updatePlatformFocus(int targetSign, int rendererHostSign) {
    PlatformFocusTarget nextTarget = resolvePlatformFocusTarget(targetSign, rendererHostSign);
    PlatformFocusTarget previousTarget = mFocusedTarget;
    if (previousTarget != null && previousTarget.isSameObject(nextTarget)) {
      mFocusedTarget = nextTarget;
      return;
    }

    boolean nextFocusable = nextTarget.isFocusable();
    boolean previousFocusable = previousTarget != null && previousTarget.isFocusable();
    // Keep the same ordering as the existing focus transition: focus the new target before
    // notifying the previous target that it lost focus.
    mFocusedTarget = nextTarget;
    if (nextFocusable) {
      nextTarget.onFocusChanged(true, previousFocusable);
    }
    if (previousFocusable) {
      previousTarget.onFocusChanged(false, nextFocusable);
    }
  }

  private PlatformFocusTarget resolvePlatformFocusTarget(int targetSign, int rendererHostSign) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    LynxBaseUI ui = owner != null ? owner.getNode(targetSign) : null;

    if (ui == null) {
      ui = getFragmentLayer(rendererHostSign);
    }
    return new PlatformFocusTarget(targetSign, rendererHostSign, ui);
  }

  private static final class PlatformFocusTarget {
    private final int mTargetSign;
    private final int mRendererHostSign;
    @Nullable private final LynxBaseUI mUI;

    PlatformFocusTarget(int targetSign, int rendererHostSign, @Nullable LynxBaseUI ui) {
      mTargetSign = targetSign;
      mRendererHostSign = rendererHostSign;
      mUI = ui;
    }

    boolean isSameObject(PlatformFocusTarget target) {
      Object object = getFocusObject();
      Object targetObject = target.getFocusObject();
      if (object != null || targetObject != null) {
        return object == targetObject;
      }
      return mTargetSign == target.mTargetSign && mRendererHostSign == target.mRendererHostSign;
    }

    boolean containsSign(int sign) {
      return mTargetSign == sign || mRendererHostSign == sign;
    }

    boolean isFocusable() {
      return mUI != null && mUI.isFocusable();
    }

    void onFocusChanged(boolean hasFocus, boolean isFocusTransition) {
      if (mUI != null) {
        mUI.onFocusChanged(hasFocus, isFocusTransition);
      }
    }

    @Nullable
    private Object getFocusObject() {
      return mUI;
    }
  }

  @CalledByNative
  public void destroyPlatformRenderer(int sign) {
    if (mFocusedTarget != null && mFocusedTarget.containsSign(sign)) {
      mFocusedTarget = null;
    }
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner != null) {
      owner.destroyFragmentLayer(sign);
    }
  }

  @CalledByNative
  void insertPlatformRenderer(int parent, int child, int index, boolean shouldUpdateUIOwner) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    LynxBaseUI parentUI = owner != null ? owner.getNode(parent) : null;
    LynxBaseUI childUI = owner != null ? owner.getNode(child) : null;
    // Overlay Views are mounted by their component, not by the Layer parent.
    if (parentUI == null || childUI == null || (!shouldUpdateUIOwner && childUI.isOverlay())) {
      return;
    }
    LynxBaseUI oldParent = childUI.getParentBaseUI();
    int oldIndex = oldParent != null ? oldParent.getChildren().indexOf(childUI) : -1;
    int targetIndex = index < 0 ? parentUI.getChildren().size() : index;
    if (oldParent == parentUI) {
      targetIndex = Math.min(targetIndex, parentUI.getChildren().size() - 1);
      if (oldIndex == targetIndex) {
        return;
      }
    }
    if (oldParent != null) {
      owner.remove(oldParent.getSign(), child);
    }
    owner.insert(parent, child, Math.min(targetIndex, parentUI.getChildren().size()));
  }

  @CalledByNative
  public void invalidatePlatformRenderer(int sign) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    if (ui != null) {
      ui.invalidateFragmentLayer(0);
    }
  }

  @CalledByNative
  public void updatePlatformRendererFrame(int sign, boolean needClip, int left, int top, int width,
      int height, int dx, int dy, int paddingLeft, int paddingTop, int paddingRight,
      int paddingBottom, int marginLeft, int marginTop, int marginRight, int marginBottom,
      int borderLeftWidth, int borderTopWidth, int borderRightWidth, int borderBottomWidth) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    if (ui == null) {
      LLog.d(TAG, "fragment layer UI not found for sign: " + sign);
      return;
    }
    ui.setFragmentLayerFrame(needClip, left, top, left + width, top + height, dx, dy);

    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner != null && owner.getNode(sign) != null) {
      int layoutLeft = left + dx;
      int layoutTop = top + dy;
      owner.updateLayout(sign, layoutLeft, layoutTop, width, height, paddingLeft, paddingTop,
          paddingRight, paddingBottom, marginLeft, marginTop, marginRight, marginBottom,
          borderLeftWidth, borderTopWidth, borderRightWidth, borderBottomWidth, null, null, 0,
          sign);
    }

    ui.requestLayout();
    ui.invalidateFragmentLayer(LynxBaseUI.FRAGMENT_LAYER_INVALIDATE_PARENT
        | LynxBaseUI.FRAGMENT_LAYER_INVALIDATE_DISPLAY_LIST);
  }

  @CalledByNative
  void updatePlatformRendererAttributes(int sign, PropBundle propBundle, boolean tendsToFlatten) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    LynxBaseUI ui = owner != null ? owner.getNode(sign) : null;
    if (ui == null) {
      LLog.d(TAG, "fragment layer UI not found for sign: " + sign);
      return;
    }

    LynxBaseUI oldLayerUI = owner.getFragmentLayer(sign);
    ReadableMap props = propBundle != null ? propBundle.getProps() : null;
    Map<String, EventsListener> listeners = EventsListener.convertEventListeners(
        propBundle != null ? propBundle.getEventHandlers() : null);
    Map<Integer, GestureDetector> detectors = GestureDetector.convertGestureDetectors(
        propBundle != null ? propBundle.getGestures() : null);
    Behavior behavior = mBehaviorRegistry != null ? mBehaviorRegistry.get(ui.getTagName()) : null;
    boolean isFlatten = shouldCreateFallbackUIAsFlatten(behavior, tendsToFlatten);
    owner.updateProperties(
        sign, isFlatten, props != null ? new StylesDiffMap(props) : null, listeners, detectors);

    owner.updateFragmentLayer(sign, oldLayerUI, this);
  }

  @CalledByNative
  public void updatePlatformRendererSubtreeProperties(int sign, ByteBuffer buffer, int count) {
    LynxBaseUI ui = getPlatformRendererUI(sign);
    if (ui == null) {
      LLog.d(TAG, "fragment layer UI not found for sign: " + sign);
      return;
    }

    buffer.order(java.nio.ByteOrder.nativeOrder());
    ui.applyFragmentLayerSubtreeProperties(buffer, count);
  }

  @CalledByNative
  public void updatePlatformExtraData(int sign, Object extraData) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner != null && owner.getNode(sign) != null) {
      owner.updateViewExtraData(sign, extraData);
    }
  }

  public LynxImageManager getImage(int imageKey) {
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      return rootView.peekImageAccordingToNodeIndex(imageKey);
    }
    return null;
  }

  @CalledByNative
  void createImage(int sign, String src, ReadableMapBuffer paintInfo, int width, int height,
      int eventMask, int imageKey, boolean disableDefaultResize) {
    // Create Image managed by LynxImageManager and register to UIBodyView
    LynxImageManager imageManager = new LynxImageManager(mContext);
    imageManager.setFallbackSign(sign);
    imageManager.setEventMask(eventMask);
    imageManager.setSrc(src);
    imageManager.setDisableDefaultResize(disableDefaultResize);
    if (paintInfo != null) {
      imageManager.setMode(paintInfo.getInt(IMAGE_PAINT_INFO_MODE));
      imageManager.setBlurRadius(paintInfo.getString(IMAGE_PAINT_INFO_BLUR_RADIUS, null));
      imageManager.setAutoSize(paintInfo.getBoolean(IMAGE_PAINT_INFO_AUTO_SIZE, false));
      imageManager.setPlaceholder(paintInfo.getString(IMAGE_PAINT_INFO_PLACEHOLDER, null));
      imageManager.setTintColor(paintInfo.getString(IMAGE_PAINT_INFO_TINT_COLOR, null));
      imageManager.setCapInsets(paintInfo.getString(IMAGE_PAINT_INFO_CAP_INSETS, null));
      imageManager.setCapInsetsScale(
          Double.toString(paintInfo.getDouble(IMAGE_PAINT_INFO_CAP_INSETS_SCALE, 1.0)));
      imageManager.setSkipRedirection(
          paintInfo.getBoolean(IMAGE_PAINT_INFO_SKIP_REDIRECTION, false));
      imageManager.setAutoPlay(paintInfo.getBoolean(IMAGE_PAINT_INFO_AUTOPLAY, true));
      imageManager.setLoopCount(paintInfo.getInt(IMAGE_PAINT_INFO_LOOP_COUNT, 0));
    }
    imageManager.onLayoutUpdated(width, height, 0, 0, 0, 0);
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      rootView.registerImageAccordingToNodeIndex(imageKey, imageManager);
    }
    imageManager.onNodeReady();
  }

  @CalledByNative
  public void destroyImage(int imageKey) {
    // Remove and release the image source from UIBodyView
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      rootView.obtainImageAccordingToNodeIndex(imageKey);
    }
  }

  public Page getTextBundle(int sign) {
    return (Page) mExtraDatas.get(sign);
  }

  @CalledByNative
  public void updateTextBundle(int sign, long textBundle) {
    // Update the text layout bundle for the specified sign
    Page page = mContext.getTextService().createPage(textBundle);
    if (page != null) {
      mExtraDatas.put(sign, page);
    }
  }

  @CalledByNative
  public void destroyTextBundle(final int sign) {
    // Destroy the text layout bundle for the specified sign
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        Page page = (Page) mExtraDatas.remove(sign);
        if (page != null) {
          page.destroy();
        }
      }
    });
  }

  @CalledByNative
  public void finishLayoutOperation(int componentId, long operationId, boolean isFirstScreen) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner == null) {
      return;
    }
    owner.onLayoutFinish(componentId, operationId);
  }

  @CalledByNative
  void setNeedMarkPaintEndTiming(String pipelineId) {
    PerformanceController perfController = mContext != null ? mContext.getPerfController() : null;
    if (perfController != null) {
      perfController.setNeedMarkPaintEndTiming(pipelineId);
    }
  }

  @CalledByNative
  public void onNodeReadyBatch(int[] signs) {
    if (signs == null) {
      return;
    }
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner == null) {
      return;
    }
    for (int sign : signs) {
      if (owner.getNode(sign) == null) {
        continue;
      }
      owner.onNodeReady(sign);
    }
  }

  @CalledByNative
  private void finishTasmOperation(long operationId) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner == null) {
      return;
    }
    owner.onTasmFinish(operationId);
  }

  @CalledByNative
  void removePlatformRendererFromParent(int parent, int sign, boolean shouldUpdateUIOwner) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    LynxBaseUI parentUI = owner != null ? owner.getNode(parent) : null;
    LynxBaseUI childUI = owner != null ? owner.getNode(sign) : null;
    if (parentUI != null && childUI != null && childUI.getParentBaseUI() == parentUI
        && (shouldUpdateUIOwner || !childUI.isOverlay())) {
      owner.remove(parent, sign);
    }
  }

  @Override
  public ByteBuffer getDisplayListItemsBuffer(int id) {
    if (mDestroyed || mNativePtr == 0) {
      return null;
    }
    return makeReadOnlyDisplayListBuffer(
        nativeGetDisplayListItemsBuffer(mNativePtr, id, DisplayListApplier.DISPLAY_LIST_ITEM_SIZE));
  }

  @Override
  public ByteBuffer getDisplayListDataBuffer(int id) {
    if (mDestroyed || mNativePtr == 0) {
      return null;
    }
    return makeReadOnlyDisplayListBuffer(nativeGetDisplayListDataBuffer(mNativePtr, id));
  }

  static ByteBuffer makeReadOnlyDisplayListBuffer(ByteBuffer buffer) {
    return buffer == null ? null : buffer.asReadOnlyBuffer();
  }

  public TextLayout getTextLayout() {
    return mTextLayout;
  }

  /**
   * Implements TextMeasurerProvider.measureText to delegate to the TextMeasurer instance.
   * This allows PlatformRendererContext to provide text measurement functionality directly.
   */
  @Override
  public float[] measureText(int sign, float width, int widthMode, float height, int heightMode,
      float[] inlineViewLayoutResult) {
    if (mTextMeasurer != null) {
      return mTextMeasurer.measureText(
          sign, width, widthMode, height, heightMode, inlineViewLayoutResult);
    }
    // Return default measurement if TextMeasurer is not available
    return new float[] {0.0f, 0.0f};
  }

  /**
   * Implements TextMeasurerProvider.dispatchLayoutBefore to delegate to the TextMeasurer instance.
   * This allows PlatformRendererContext to handle layout dispatch functionality directly.
   */
  @Override
  public void dispatchLayoutBefore(int sign, ReadableCompactArrayBuffer buffer) {
    if (mTextMeasurer != null) {
      mTextMeasurer.dispatchLayoutBefore(sign, buffer);
    }
  }

  @Override
  public float[] align(int sign) {
    if (mTextMeasurer != null) {
      return mTextMeasurer.align(sign);
    }
    return new float[0];
  }

  native long nativeCreateEmbeddedViewContext(PlatformRendererContext jThis);

  private native ByteBuffer nativeGetDisplayListItemsBuffer(
      long nativePtr, int id, int expectedItemSize);

  private native ByteBuffer nativeGetDisplayListDataBuffer(long nativePtr, int id);

  private native void nativeDestroy(long nativePtr);

  private native void nativeInvokeUIMethodCallback(
      long nativePtr, int callbackId, int code, Object params);

  public void destroy() {
    if (mDestroyed) {
      return;
    }
    mDestroyed = true;

    if (mNativePtr != 0) {
      nativeDestroy(mNativePtr);
    }
    mNativePtr = 0;
    mFocusedTarget = null;

    for (Object value : mExtraDatas.values()) {
      if (value instanceof Page) {
        ((Page) value).destroy();
      }
    }
    mExtraDatas.clear();

    mTextMeasurer = null;
    mTextLayout = null;
    UIBody.UIBodyView root = mRootView != null ? mRootView.get() : null;
    if (root != null) {
      root.clearNodeIndexImageMap();
    }
  }
}
