// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.Matrix;
import android.graphics.PointF;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;
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
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.PropBundle;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.list.container.UIListContainer;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.behavior.utils.LynxUIMethodsExecutor;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.service.ILynxTextService.Page;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class PlatformRendererContext implements TextMeasurerProvider {
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

  public static final class PlatformEventHandlerState {
    public static final int kNone = 0;
  }

  WeakReference<UIBody.UIBodyView> mRootView = null;

  HashMap<Integer, IRendererHost> mViewHolder = new HashMap<>();

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
    IRendererHost host = mViewHolder.get(sign);
    if (host instanceof AndroidScrollView) {
      AndroidScrollView scrollView = (AndroidScrollView) host;
      res[0] = scrollView.getRealScrollX();
      res[1] = scrollView.getRealScrollY();
    } else if (host != null) {
      res[0] = host.getRendererHostScrollX();
      res[1] = host.getRendererHostScrollY();
    }
    return res;
  }

  @CalledByNative
  boolean isRendererHostScrollable(int sign) {
    IRendererHost host = mViewHolder.get(sign);
    Renderer renderer = host != null ? host.getRenderer() : null;
    LynxBaseUI uiHost = renderer != null ? renderer.getUIHost() : null;
    return uiHost != null && uiHost.isScrollable();
  }

  @CalledByNative
  private void invokeUIMethod(
      int sign, String method, ReadableMap params, long nativePtr, int callbackId) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      private void cb(Object... args) {
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
      }

      @Override
      public void run() {
        if (mDestroyed || mNativePtr == 0) {
          return;
        }
        LynxBaseUI ui = findUIMethodTarget(sign);
        if (ui != null) {
          LynxUIMethodsExecutor.invokeMethod(ui, method, params,
              (Object... args) -> UIThreadUtils.runOnUiThreadImmediately(() -> cb(args)));
        } else {
          cb(LynxUIMethodConstants.NO_UI_FOR_NODE, "node does not have a LynxUI");
        }
      }
    });
  }

  private LynxBaseUI findUIMethodTarget(int sign) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    LynxBaseUI ui = owner != null ? owner.getNode(sign) : null;
    if (ui != null) {
      return ui;
    }
    IRendererHost host = mViewHolder.get(sign);
    Renderer renderer = host != null ? host.getRenderer() : null;
    return renderer != null ? renderer.getUIHost() : null;
  }

  PointF convertPointInViewToScreen(int sign, PointF point) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null) {
      LLog.e(TAG, "convertPointInViewToScreen failed since can not find target host.");
      return point;
    }
    return host.convertPointInRendererHostToScreen(point);
  }

  public int getTargetWidth(int sign) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null) {
      LLog.e(TAG, "getTargetWidth failed since can not find target view.");
      return 0;
    }

    return host.getRendererHostWidth();
  }

  public int getTargetHeight(int sign) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null) {
      LLog.e(TAG, "getTargetHeight failed since can not find target view.");
      return 0;
    }

    return host.getRendererHostHeight();
  }

  public int getMeaningfulPaintingAreaVisibleStatus(int sign) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null || host.getView() == null) {
      return View.VISIBLE;
    }
    return host.getView().getVisibility();
  }

  public float getMeaningfulPaintingAreaAlpha(int sign) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null || host.getView() == null) {
      return 1.f;
    }
    return host.getView().getAlpha();
  }

  public float getMeaningfulPaintingAreaScaleX(int sign) {
    return getMeaningfulPaintingAreaScale(sign, true);
  }

  public float getMeaningfulPaintingAreaScaleY(int sign) {
    return getMeaningfulPaintingAreaScale(sign, false);
  }

  private float getMeaningfulPaintingAreaScale(int sign, boolean scaleX) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null || host.getView() == null) {
      return 1.f;
    }
    View view = host.getView();
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
      // TODO(songshourui.null): Support <list>, <list-item> and <scroll-view>'s platform view
      // later, use ContainerRenderer for now
      case PlatformRendererType.kList:
      case PlatformRendererType.kListItem:
      case PlatformRendererType.kScroll: {
        ContainerRenderer view = new ContainerRenderer(mContext);
        Renderer renderer = view.createRenderer(this, sign);
        renderer.setRenderHost(view);
        view.setRenderer(renderer);
        mViewHolder.put(sign, view);
        view.invalidate();
        break;
      }
      case PlatformRendererType.kPage: {
        LynxUIOwner owner = mContext.getLynxUIOwner();
        if (owner != null) {
          owner.setRootSign(sign);
          owner.setNode(sign, mContext.getUIBody());
        }
        UIBody.UIBodyView view = mRootView.get();
        if (view != null) {
          view.setWillNotDraw(false);
          view.invalidate();
          Renderer renderer = view.createRenderer(this, sign);
          renderer.setUIHost((LynxUI) mContext.getUIBody());
          renderer.setRenderHost(view);
          view.setRenderer(renderer);
          mViewHolder.put(sign, view);
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
    if (mBehaviorRegistry != null) {
      Behavior behavior = mBehaviorRegistry.get(tagName);
      if (behavior != null && behavior.supportFragmentLayerRenderer()) {
        IRendererHost host = behavior.createPlatformRendererHost(mContext);
        if (host != null) {
          Renderer renderer = host.createRenderer(this, sign);
          renderer.setRenderHost(host);
          host.setRenderer(renderer);
          mViewHolder.put(sign, host);
          host.getView().invalidate();
          renderer.updateAttributes(initData);
          return;
        }
      }
    }

    LynxUIOwner owner = mContext.getLynxUIOwner();
    if (owner != null) {
      ReadableMap initialProps = initData != null ? initData.getProps() : null;
      ReadableArray eventListeners = initData != null ? initData.getEventHandlers() : null;
      ReadableArray gestureDetectors = initData != null ? initData.getGestures() : null;
      boolean isFlatten =
          initialProps != null && initialProps.getBoolean(TENDS_TO_FLATTEN_INIT_DATA_KEY, false);
      owner.createView(
          sign, tagName, initialProps, null, eventListeners, isFlatten, sign, gestureDetectors);
      LynxBaseUI createdUI = owner.getNode(sign);
      IRendererHost host = null;
      if (createdUI instanceof IRendererHost) {
        host = (IRendererHost) createdUI;
      } else if (createdUI instanceof LynxUI
          && ((LynxUI) createdUI).getView() instanceof IRendererHost) {
        host = (IRendererHost) ((LynxUI) createdUI).getView();
      }
      if (host != null) {
        Renderer renderer = host.createRenderer(this, sign);
        renderer.setUIHost(createdUI);
        renderer.setRenderHost(host);
        host.setRenderer(renderer);
        mViewHolder.put(sign, host);
        host.setWillNotDrawForRenderer(false);
        host.setClipChildrenForRenderer(false);
        host.invalidateForRenderer();
        renderer.updateAttributes(initData);
        return;
      }
      owner.cleanupCreatedView(sign, tagName, initialProps);
    }

    // For extended platform renderers, we need to create a custom view based on the tag name
    // Currently, we'll create a ContainerRenderer as a fallback, but in the future
    // we should look up the actual class based on the tag name
    ContainerRenderer view = new ContainerRenderer(mContext);
    Renderer renderer = view.createRenderer(this, sign);
    renderer.setRenderHost(view);
    view.setRenderer(renderer);
    mViewHolder.put(sign, view);
    view.invalidate();
  }

  @CalledByNative
  public void destroyPlatformRenderer(int sign) {
    LynxUIOwner owner = mContext.getLynxUIOwner();
    boolean shouldRemoveFromNativeParent = false;
    if (owner != null && owner.getNode(sign) != null) {
      LynxBaseUI child = owner.getNode(sign);
      shouldRemoveFromNativeParent = !(child.getParent() instanceof LynxBaseUI);
      owner.destroy(-1, child.getSign());
    }

    IRendererHost host = mViewHolder.get(sign);
    try {
      if (host != null) {
        View hostView = host.getView();
        if (shouldRemoveFromNativeParent && hostView != null) {
          View parent = (View) hostView.getParent();
          if (parent instanceof ViewGroup) {
            ((ViewGroup) parent).removeView(hostView);
          }
        }
        Renderer renderer = host.getRenderer();
        if (renderer != null) {
          renderer.onDestroy();
        }
      }
    } finally {
      mViewHolder.remove(sign);
    }
  }

  @CalledByNative
  public void insertPlatformRenderer(int parent, int child, int index) {
    // LynxUIOwner owner = mContext.getLynxUIOwner();
    // if (owner != null && owner.getNode(parent) != null && owner.getNode(child) != null) {
    //   owner.insert(parent, child, index);
    //   return;
    // }

    IRendererHost hParent = mViewHolder.get(parent);
    IRendererHost hChild = mViewHolder.get(child);
    if (hParent == null || hChild == null) {
      return;
    }
    if (!(hParent.getView() instanceof ViewGroup)) {
      return;
    }
    ViewGroup parentView = (ViewGroup) hParent.getView();
    View childView = hChild.getView();
    if (childView == null) {
      return;
    }
    int count = parentView.getChildCount();
    if (index == -1 || index >= count) {
      parentView.addView(childView);
    } else {
      parentView.addView(childView, index);
    }
  }

  @CalledByNative
  public void invalidatePlatformRenderer(int sign) {
    IRendererHost host = mViewHolder.get(sign);
    if (host != null) {
      host.invalidateForRenderer();
    }
  }

  @CalledByNative
  public void updatePlatformRendererFrame(
      int sign, boolean needClip, int left, int top, int width, int height, int dx, int dy) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null) {
      LLog.d(TAG, "host renderer not found for sign: " + sign);
      return;
    }
    host.getRenderer().setLynxFrame(needClip, left, top, left + width, top + height, dx, dy);

    LynxUIOwner owner = mContext.getLynxUIOwner();
    if (owner != null && owner.getNode(sign) != null) {
      com.lynx.tasm.behavior.ui.LynxBaseUI node = owner.getNode(sign);
      owner.updateLayout(sign, left, top, width, height, node.getPaddingLeft(),
          node.getPaddingTop(), node.getPaddingRight(), node.getPaddingBottom(),
          node.getMarginLeft(), node.getMarginTop(), node.getMarginRight(), node.getMarginBottom(),
          node.getBorderLeftWidth(), node.getBorderTopWidth(), node.getBorderRightWidth(),
          node.getBorderBottomWidth(), null, null, 0, sign);
    }

    host.requestLayoutForRenderer();
    host.getRenderer().invalidate(Renderer.INVALIDATE_PARENT | Renderer.INVALIDATE_DISPLAY_LIST);
  }

  @CalledByNative
  public void updatePlatformRendererAttributes(int sign, PropBundle propBundle) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null) {
      LLog.d(TAG, "host renderer not found for sign: " + sign);
      return;
    }

    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner != null && owner.getNode(sign) != null) {
      ReadableMap props = propBundle != null ? propBundle.getProps() : null;
      Map<String, EventsListener> listeners = EventsListener.convertEventListeners(
          propBundle != null ? propBundle.getEventHandlers() : null);
      Map<Integer, GestureDetector> detectors = GestureDetector.convertGestureDetectors(
          propBundle != null ? propBundle.getGestures() : null);
      owner.updateProperties(
          sign, false, props != null ? new StylesDiffMap(props) : null, listeners, detectors);
    }

    // Get the renderer
    Renderer renderer = host.getRenderer();
    if (renderer != null) {
      renderer.updateAttributes(propBundle);
    }
  }

  @CalledByNative
  public void updatePlatformRendererSubtreeProperties(int sign, ByteBuffer buffer, int count) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null) {
      LLog.d(TAG, "host renderer not found for sign: " + sign);
      return;
    }

    // Get the renderer
    Renderer renderer = host.getRenderer();
    if (renderer != null) {
      buffer.order(java.nio.ByteOrder.nativeOrder());
      renderer.applySubtreeProperties(buffer, count);
    }
  }

  @CalledByNative
  public void updatePlatformExtraData(int sign, Object extraData) {
    IRendererHost host = mViewHolder.get(sign);
    if (host == null) {
      LLog.d(TAG, "host renderer not found for sign: " + sign);
      return;
    }

    // Get the renderer
    Renderer renderer = host.getRenderer();
    if (renderer != null) {
      renderer.updateExtraData(extraData);
    }
  }

  public LynxImageManager getImage(int sign) {
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      return rootView.peekImageAccordingToNodeIndex(sign);
    }
    return null;
  }

  @CalledByNative
  public void createImage(int sign, String src, int width, int height, int eventMask) {
    // Create Image managed by LynxImageManager and register to UIBodyView
    LynxImageManager imageManager = new LynxImageManager(mContext);
    imageManager.setFallbackSign(sign);
    imageManager.setEventMask(eventMask);
    imageManager.setSrc(src);
    imageManager.onLayoutUpdated(width, height, 0, 0, 0, 0);
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      rootView.registerImageAccordingToNodeIndex(sign, imageManager);
    }
    imageManager.onNodeReady();
  }

  @CalledByNative
  public void destroyImage(int sign) {
    // Remove and release the image source from UIBodyView
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      rootView.obtainImageAccordingToNodeIndex(sign);
    }
  }

  Page getTextBundle(int sign) {
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
  public void insertListItemPaintingNode(int listSign, int childSign) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner == null) {
      return;
    }
    LynxBaseUI parent = owner.getNode(listSign);
    LynxBaseUI child = owner.getNode(childSign);
    if (parent instanceof UIListContainer && child instanceof UIComponent) {
      ((UIListContainer) parent).insertListItemNode(child);
    }
  }

  @CalledByNative
  public void removeListItemPaintingNode(int listSign, int childSign) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner == null) {
      return;
    }
    LynxBaseUI parent = owner.getNode(listSign);
    LynxBaseUI child = owner.getNode(childSign);
    if (parent instanceof UIListContainer && child instanceof UIComponent) {
      ((UIListContainer) parent).removeView(child);
    }
  }

  @CalledByNative
  public void updateContentOffsetForListContainer(int listSign, float contentSize, float deltaX,
      float deltaY, boolean isInitScrollOffset, boolean fromLayout) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner == null) {
      return;
    }
    LynxBaseUI parent = owner.getNode(listSign);
    if (parent instanceof UIListContainer) {
      ((UIListContainer) parent).updateContentSizeAndOffset(contentSize, deltaX, deltaY);
    }
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
  private void finishTasmOperation(long operationId) {
    LynxUIOwner owner = mContext != null ? mContext.getLynxUIOwner() : null;
    if (owner == null) {
      return;
    }
    owner.onTasmFinish(operationId);
  }

  @CalledByNative
  public void removePlatformRendererFromParent(int sign) {
    // LynxUIOwner owner = mContext.getLynxUIOwner();
    // if (owner != null && owner.getNode(sign) != null) {
    //   LynxBaseUI child = owner.getNode(sign);
    //   if (child.getParent() instanceof LynxBaseUI) {
    //     LynxBaseUI parent = (LynxBaseUI) child.getParent();
    //     if (parent == null) {
    //       return;
    //     }
    //     owner.remove(parent.getSign(), child.getSign());
    //     return;
    //   }
    // }

    IRendererHost host = mViewHolder.get(sign);
    if (host != null) {
      View hostView = host.getView();
      if (hostView != null && hostView.getParent() instanceof ViewGroup) {
        ((ViewGroup) hostView.getParent()).removeView(hostView);
      }
    }
  }

  public void getDisplayList(int id, DisplayList displayList) {
    if (displayList == null || mDestroyed || mNativePtr == 0) {
      return;
    }

    // Get the display list lengths first
    int[] lengths = nativeGetDisplayListLengths(mNativePtr, id);
    if (lengths == null || lengths.length != 3) {
      return;
    }

    int opsLength = lengths[0];
    int iArgvLength = lengths[1];
    int fArgvLength = lengths[2];

    // Allocate arrays
    displayList.ops = new int[opsLength];
    displayList.iArgv = new int[iArgvLength];
    displayList.fArgv = new float[fArgvLength];

    // Fill the arrays with actual data
    nativeGetDisplayListData(mNativePtr, id, displayList.ops, displayList.iArgv, displayList.fArgv);
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

  native int[] nativeGetDisplayListLengths(long nativePtr, int id);

  /**
   * Fills the provided arrays with display list data.
   * The arrays must be pre-allocated with lengths obtained from nativeGetDisplayListLengths().
   */
  native void nativeGetDisplayListData(
      long nativePtr, int id, int[] ops, int[] iArgv, float[] fArgv);

  native void nativeDestroy(long nativePtr);

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
    mViewHolder.clear();

    for (Object value : mExtraDatas.values()) {
      if (value instanceof Page) {
        ((Page) value).destroy();
      }
    }
    mExtraDatas.clear();

    mTextMeasurer = null;
    mTextLayout = null;
    UIBody.UIBodyView root = mRootView.get();
    if (root != null) {
      root.clearNodeIndexImageMap();
    }
  }
}
