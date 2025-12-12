// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_TRUE;
import static com.lynx.tasm.performance.timing.TimingConstants.HOST_PLATFORM_DRAW_END;
import static com.lynx.tasm.performance.timing.TimingConstants.HOST_PLATFORM_DRAW_START;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.core.util.Consumer;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.OnceTask;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.render.ContainerRenderer;
import com.lynx.tasm.behavior.render.DisplayList;
import com.lynx.tasm.behavior.render.DisplayListApplier;
import com.lynx.tasm.behavior.render.IRendererHost;
import com.lynx.tasm.behavior.render.PlatformRendererContext;
import com.lynx.tasm.behavior.render.Renderer;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.performance.longtasktiming.LynxLongTaskMonitor;
import com.lynx.tasm.performance.timing.ITimingCollector;
import com.lynx.tasm.utils.SizeValue;
import java.lang.annotation.Native;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;
import org.json.JSONArray;
import org.json.JSONObject;

public class UIBody extends UIGroup<UIBodyView> {
  private final static String TAG = "UIBody";

  @Nullable private UIBodyView mBodyView;
  @Nullable private EventTarget mParentLynxPageUI;
  @Nullable private HashMap<String, EventTarget> mChildrenLynxPageUI;

  private ArrayList<LynxUI> mCreateViewUI;

  private LynxAccessibilityWrapper mA11yWrapper;

  private OnceTask<Void> mDetachTask = null;

  private OnceTask<Void> mAttachTask = null;

  private Consumer<Exception> mExceptionHandler = null;

  public UIBody(LynxContext context, final UIBodyView view) {
    super(context);
    mBodyView = view;
    if (mBodyView != null) {
      if (context != null && context.isFragmentLayerRenderOn()) {
        mBodyView.mLynxContext = context;
        mBodyView.setClipChildren(false);
      }
    }
    initialize();

    mExceptionHandler = new Consumer<Exception>() {
      @Override
      public void accept(Exception exception) {
        if (getLynxContext() != null) {
          getLynxContext().handleException(exception);
        }
      }
    };
  }

  public UIBodyView getBodyView() {
    return mBodyView;
  }

  @Override
  UIBodyView getOrCreateView(Context context, Object params) {
    return mBodyView;
  }

  public List<MeaningfulPaintingArea> getMeaningfulPaintingAreas() {
    tryRunDetachAndAttachTask();
    int width = getWidth();
    int height = getHeight();
    if (width == 0 || height == 0) {
      return new ArrayList<>();
    }
    ArrayList<MeaningfulPaintingArea> areas = new ArrayList<MeaningfulPaintingArea>();
    convertToMeaningfulPaintingAreaRecursive(0, 0, width, height, areas);
    return areas;
  }

  /**
   * when async render, we should attach LynxView
   *
   * @param view
   */
  synchronized public void attachUIBodyView(UIBodyView view, LynxContext context) {
    mAttachTask = new OnceTask<>(new Callable<Void>() {
      @Override
      public Void call() throws Exception {
        if (mContext.isEnginePoolEnabled()) {
          if (mBodyView == view) {
            return null;
          }

          if (mBodyView != null) {
            detachUIBodyView();
          }
        }

        mBodyView = view;
        initialize();

        if (!mContext.isEnginePoolEnabled()) {
          return null;
        }

        if (mBodyView == null) {
          LLog.e(TAG, "attachUIBodyView failed since mBodyView is null.");
          return null;
        }

        mContext = context;
        mBodyView.mLynxContext = context;
        mContext.setUIBodyView(mBodyView);

        TraceEvent.beginSection(TraceEventDef.UI_BODY_ATTACH_UI_BODY_VIEW);

        mContext.setUIBodyView(mBodyView);
        mCreateViewUI = new ArrayList<>();
        attachToView(context);
        mBodyView.markNeedRemoveExistingViews();

        TraceEvent.endSection(TraceEventDef.UI_BODY_ATTACH_UI_BODY_VIEW);
        return null;
      }
    }, mExceptionHandler);

    if (mDetachTask == null) {
      mAttachTask.run();
    }
  }

  synchronized public void detachUIBodyView() {
    if (!mContext.isEnginePoolEnabled()) {
      LLog.w(
          TAG, "UIBody.detachUIBodyView should not be called when isEnginePoolEnabled == false.");
      return;
    }

    if (mBodyView == null) {
      return;
    }

    boolean isLayoutRequested = mView.isLayoutRequested();

    mBodyView.cacheLayoutInfo(getWidth(), getHeight());

    mDetachTask = new OnceTask<>(new Callable<Void>() {
      @Override
      public Void call() throws Exception {
        TraceEvent.beginSection(TraceEventDef.UI_BODY_DETACH_UI_BODY_VIEW);

        markDetachWithViewRecursively(true);

        if (isLayoutRequested) {
          // measure
          performMeasureChildrenUI();

          // layout
          performLayoutChildrenUI();
        }

        // process view info
        processViewInfo();

        // detach
        detachWithViewInfo(mViewInfo);

        markDetachWithViewRecursively(false);

        TraceEvent.endSection(TraceEventDef.UI_BODY_DETACH_UI_BODY_VIEW);

        synchronized (this) {
          if (mAttachTask != null) {
            mAttachTask.run();
          }
        }
        return null;
      }
    }, mExceptionHandler);

    LynxThreadPool.postUIOperationTask(mDetachTask);
  }

  @Override
  protected void registerViewAccordingToNodeIndex() {}

  @Override
  protected void detachWithViewInfo(ViewInfo parentViewInfo) {
    super.detachWithViewInfo(mViewInfo != null ? mViewInfo : parentViewInfo);
    mBodyView = null;
  }

  public void appendUIWithCreateViewAsync(LynxUI ui) {
    if (mCreateViewUI == null) {
      LLog.w(TAG, "UIBody.appendUIWithCreateViewAsync failed since mCreateViewUI is null.");
      return;
    }
    mCreateViewUI.add(ui);
  }

  public void rebuildViewTree() {
    if (!mContext.isEnginePoolEnabled()) {
      return;
    }

    TraceEvent.beginSection(TraceEventDef.UI_BODY_REBUILD_VIEW_TREE);

    synchronized (this) {
      if (mDetachTask != null) {
        mDetachTask.run();
        mDetachTask.get();
        mDetachTask = null;
      }

      if (mAttachTask != null) {
        mAttachTask.run();
        mAttachTask.get();
        mAttachTask = null;
      }
    }

    if (mBodyView != null) {
      mBodyView.removeExistingViews();
    }

    if (mCreateViewUI == null) {
      TraceEvent.endSection(TraceEventDef.UI_BODY_REBUILD_VIEW_TREE);
      return;
    }

    for (LynxUI ui : mCreateViewUI) {
      ui.ensureCreateView();
    }
    mCreateViewUI.clear();

    TraceEvent.endSection(TraceEventDef.UI_BODY_REBUILD_VIEW_TREE);
  }

  @Override
  public void onAttach() {
    if (mContext.isEnginePoolEnabled()) {
      tryRunDetachAndAttachTask();
    }
    super.onAttach();
  }

  synchronized public void tryRunDetachAndAttachTask() {
    if (!mContext.isEnginePoolEnabled()) {
      return;
    }

    if (mDetachTask != null) {
      mDetachTask.run();
      mDetachTask.get();
    }
    if (mAttachTask != null) {
      mAttachTask.run();
      mAttachTask.get();
    }
  }

  @Override
  protected void createViewAsync() {}

  @Override
  protected void ensureCreateView() {}

  @Override
  public void initialize() {
    super.initialize();
    initAccessibility();
  }

  protected void initAccessibility() {
    final UIBodyView view = getBodyView();
    if (view == null || view.isAccessibilityDisabled()) {
      return;
    }
    if (mA11yWrapper == null) {
      mA11yWrapper = new LynxAccessibilityWrapper(this);
    }
    mAccessibilityElementStatus = ACCESSIBILITY_ELEMENT_TRUE;
    view.setLynxAccessibilityWrapper(mA11yWrapper);
  }

  public void onPageConfigDecoded(PageConfig config) {
    if (mA11yWrapper != null) {
      mA11yWrapper.onPageConfigDecoded(config);
    }
  }

  @Override
  protected View getRealParentView() {
    return mBodyView;
  }

  @Override
  protected UIBodyView createView(final Context context, Object params) {
    return mBodyView;
  }

  @Override
  protected UIBodyView createView(final Context context) {
    return mBodyView;
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    if (mBodyView != null) {
      mBodyView.notifyMeaningfulLayout();
    }
  }

  @Override
  public boolean eventThrough(float x, float y) {
    // If <page event-through={true}/>, the res will be true.
    // Otherwise the res will be false, then check PageConfig enableEventThrough. If PageConfig
    // enableEventThrough == true, let the res be true. In other words, when
    // config.enableEventThrough == true or page.event-through == true, rootUI's eventThrough will
    // be true.
    boolean isEventThrough = super.eventThrough(x, y);
    if (!isEventThrough) {
      isEventThrough |= mContext.enableEventThrough();
    }

    if (mEventThroughActiveRegions == null) {
      return isEventThrough;
    }

    boolean isHitEventThroughActiveRegions = false;
    float density = mContext.getResources().getDisplayMetrics().density;
    float deviceX = x * density, deviceY = y * density;
    for (int i = 0; i < this.mEventThroughActiveRegions.size(); ++i) {
      ArrayList<SizeValue> region = this.mEventThroughActiveRegions.get(i);
      if (region != null && region.size() == 4) {
        float left = region.get(0).convertToDevicePx(getWidth());
        float top = region.get(1).convertToDevicePx(getHeight());
        float right = left + region.get(2).convertToDevicePx(getWidth());
        float bottom = top + region.get(3).convertToDevicePx(getHeight());
        isHitEventThroughActiveRegions =
            deviceX >= left && deviceX < right && deviceY >= top && deviceY < bottom;
        if (isHitEventThroughActiveRegions) {
          LLog.i(TAG, "hit the event through active regions!");
          break;
        }
      }
    }
    return isHitEventThroughActiveRegions ? isEventThrough : !isEventThrough;
  }

  @Override
  public void removeAll() {
    super.removeAll();
    // removeAll function means reloading, we should clear some flags.
    if (mBodyView != null) {
      mBodyView.clearMeaningfulFlag();
    }
  }

  public boolean enableNewAccessibility() {
    return mA11yWrapper != null && mA11yWrapper.enableDelegate();
  }

  public LynxAccessibilityWrapper getLynxAccessibilityWrapper() {
    return mA11yWrapper;
  }

  @Override
  public EventTarget getParentLynxPageUI() {
    return mParentLynxPageUI;
  }

  @Override
  public void setParentLynxPageUI(EventTarget parentLynxPageUI) {
    mParentLynxPageUI = parentLynxPageUI;
  }

  @Override
  public HashMap<String, EventTarget> getChildrenLynxPageUI() {
    return mChildrenLynxPageUI;
  }

  @Override
  public void setChildrenLynxPageUI(HashMap<String, EventTarget> childrenLynxPageUI) {
    mChildrenLynxPageUI = childrenLynxPageUI;
  }

  public static class UIBodyView
      extends FrameLayout implements IDrawChildHook.IDrawChildHookBinding, IRendererHost {
    private ConcurrentHashMap<Integer, View> mViewMap = new ConcurrentHashMap<>();
    private ConcurrentHashMap<Integer, LynxImageManager> mImageMap = new ConcurrentHashMap<>();

    private IDrawChildHook mDrawChildHook;
    private long mMeaningfulPaintTiming;
    private boolean mHasMeaningfulLayout;
    private boolean mHasMeaningfulPaint;

    private boolean mInterceptRequestLayout;
    private boolean mHasPendingRequestLayout;
    private LynxContext mLynxContext = null;

    private WeakReference<ITimingCollector> mTimingCollector = new WeakReference<>(null);

    private int mInstanceId = LynxContext.INSTANCE_ID_DEFAULT;

    protected LynxAccessibilityWrapper mA11yWrapper;

    private boolean mIsChildLynxPageUI;

    private boolean mNeedRemoveExistingViews = false;

    private int mCacheWidth;
    private int mCacheHeight;
    boolean mIsMeaningfulPaintingAreaInvalidate = false;

    private Renderer mRenderer;

    @Override
    public void setRenderer(Renderer renderer) {
      mRenderer = renderer;
    }

    @Override
    public Renderer getRenderer() {
      return mRenderer;
    }

    @Override
    public ViewGroup getView() {
      return this;
    }

    @Override
    public Renderer createRenderer(PlatformRendererContext context, int sign) {
      return new Renderer(context, sign);
    }

    // assign with the uiRenderer instance on TemplateRender
    @RestrictTo(RestrictTo.Scope.LIBRARY) protected ILynxUIRenderer mLynxUIRender;

    public interface attachLynxPageUICallback {
      void attachLynxPageUI(@NonNull WeakReference<Object> ui);
    }

    public UIBodyView(Context context) {
      super(context);
    }

    public UIBodyView(Context context, AttributeSet attrs) {
      super(context, attrs);
    }

    public void cacheLayoutInfo(int width, int height) {
      mCacheWidth = width;
      mCacheHeight = height;
    }

    protected void onMeasureWhenDetach(int widthMeasureSpec, int heightMeasureSpec) {
      performMeasure(widthMeasureSpec, mCacheWidth, heightMeasureSpec, mCacheHeight);
      if (mDrawChildHook instanceof ViewInfo) {
        ((ViewInfo) mDrawChildHook).measure();
      }
    }

    protected void onLayoutWhenDetach() {
      if (mDrawChildHook instanceof ViewInfo) {
        ((ViewInfo) mDrawChildHook).layout();
      }
    }

    public void performMeasure(int widthMeasureSpec, int width, int heightMeasureSpec, int height) {
      int finalWidth = 0;
      int finalHeight = 0;
      int widthMode = MeasureSpec.getMode(widthMeasureSpec);
      if (widthMode == MeasureSpec.AT_MOST || widthMode == MeasureSpec.UNSPECIFIED) {
        finalWidth = mCacheWidth;
      } else {
        finalWidth = MeasureSpec.getSize(widthMeasureSpec);
      }
      int heightMode = MeasureSpec.getMode(heightMeasureSpec);
      if (heightMode == MeasureSpec.AT_MOST || heightMode == MeasureSpec.UNSPECIFIED) {
        finalHeight = mCacheHeight;
      } else {
        finalHeight = MeasureSpec.getSize(heightMeasureSpec);
      }
      innerSetMeasuredDimension(finalWidth, finalHeight);
    }

    public boolean containsViewForNodeIndex(int nodeIndex) {
      return mViewMap.containsKey(nodeIndex);
    }

    public View obtainViewAccordingToNodeIndex(int nodeIndex) {
      View result = mViewMap.get(nodeIndex);
      mViewMap.remove(nodeIndex);
      return result;
    }

    public void registerViewAccordingToNodeIndex(int nodeIndex, View view) {
      mViewMap.put(nodeIndex, view);
    }

    public void registerImageAccordingToNodeIndex(int nodeIndex, LynxImageManager image) {
      mImageMap.put(nodeIndex, image);
    }

    public LynxImageManager obtainImageAccordingToNodeIndex(int nodeIndex) {
      LynxImageManager image = mImageMap.get(nodeIndex);
      mImageMap.remove(nodeIndex);
      return image;
    }

    public LynxImageManager peekImageAccordingToNodeIndex(int nodeIndex) {
      return mImageMap.get(nodeIndex);
    }

    public void clearNodeIndexImageMap() {
      mImageMap.clear();
    }

    public void markNeedRemoveExistingViews() {
      mNeedRemoveExistingViews = true;
    }

    public void removeExistingViews() {
      if (!mNeedRemoveExistingViews) {
        return;
      }

      for (HashMap.Entry<Integer, View> entry : mViewMap.entrySet()) {
        View view = entry.getValue();
        if (view.getParent() instanceof ViewGroup) {
          ((ViewGroup) view.getParent()).removeView(view);
        }
      }
      mViewMap.clear();
      mNeedRemoveExistingViews = false;
    }

    @Override
    public void bindDrawChildHook(IDrawChildHook hook) {
      mDrawChildHook = hook;
    }

    public void setLynxAccessibilityWrapper(LynxAccessibilityWrapper wrapper) {
      mA11yWrapper = wrapper;
    }

    @Override
    public void requestLayout() {
      if (mInterceptRequestLayout) {
        mHasPendingRequestLayout = true;
        return;
      }
      mHasPendingRequestLayout = false;
      super.requestLayout();
    }

    private boolean shouldDrawWithDisplayList() {
      return mRenderer != null;
    }

    @Override
    protected void dispatchDraw(final Canvas canvas) {
      if (shouldDrawWithDisplayList()) {
        super.dispatchDraw(canvas);
        mRenderer.afterDispatchDraw(canvas);
        return;
      }

      mIsMeaningfulPaintingAreaInvalidate = false;
      ITimingCollector timingCollector = mTimingCollector.get();
      if (timingCollector != null) {
        timingCollector.markHostPlatformTiming(HOST_PLATFORM_DRAW_START);
      }

      boolean needLongTaskMonitor = LynxLongTaskMonitor.willProcessTask(
          "LynxTemplateRender.Draw", mInstanceId, getLongTaskMonitorEnabled());
      if (mDrawChildHook != null) {
        mDrawChildHook.beforeDispatchDraw(canvas);
      }

      super.dispatchDraw(canvas);

      if (mDrawChildHook != null) {
        mDrawChildHook.afterDispatchDraw(canvas);
      }
      if (mHasMeaningfulLayout && !mHasMeaningfulPaint) {
        TraceEvent.instant(TraceEvent.CATEGORY_VITALS, TraceEventDef.FIRST_MEANINGFUL_PAINT);
        mMeaningfulPaintTiming = System.currentTimeMillis();
        mHasMeaningfulPaint = true;
      }

      if (timingCollector != null) {
        if (!timingCollector.getPendingPaintEndPipelineIds().isEmpty()) {
          TraceUITreeLayout();
        }
        timingCollector.markHostPlatformTiming(HOST_PLATFORM_DRAW_END);
        timingCollector.markPaintEndTimingIfNeeded();
      }
      if (needLongTaskMonitor) {
        LynxLongTaskMonitor.didProcessTask();
      }
    }

    private void TraceUITreeLayout() {
      if (!TraceEvent.isTracingStarted() || mLynxUIRender == null) {
        return;
      }

      UIGroup<UIBodyView> rootUI = mLynxUIRender.getLynxRootUI();
      if (rootUI instanceof UIBody) {
        TraceEvent.beginSection(TraceEvent.CATEGORY_DEFAULT, TraceEventDef.DUMP_UI_TREE_LAYOUT);

        JSONObject uiTree = getUITreeRecursively(rootUI);
        HashMap<String, String> map = new HashMap<String, String>();
        map.put("detail", uiTree.toString());
        TraceEvent.endSection(TraceEvent.CATEGORY_DEFAULT, TraceEventDef.DUMP_UI_TREE_LAYOUT, map);
      }
    }

    private JSONObject getUITreeRecursively(@NonNull LynxBaseUI ui) {
      JSONObject jsonData = new JSONObject();
      try {
        jsonData.put("name", ui.getClass().getName());
        jsonData.put("frame",
            new JSONArray(Arrays.asList(ui.getLeft(), ui.getTop(), ui.getWidth(), ui.getHeight())));
        List<Object> list = new ArrayList<>();
        for (LynxBaseUI child : ui.getChildren()) {
          list.add(getUITreeRecursively(child));
        }
        jsonData.put("children", new JSONArray(list));
      } catch (Exception e) {
        LLog.e(TAG, "getUITreeRecursively error: " + e.getMessage());
      }
      return jsonData;
    }

    void notifyMeaningfulLayout() {
      mHasMeaningfulLayout = true;
    }

    public long getMeaningfulPaintTiming() {
      return mMeaningfulPaintTiming;
    }

    void clearMeaningfulFlag() {
      mHasMeaningfulLayout = false;
      mHasMeaningfulPaint = false;
      mMeaningfulPaintTiming = 0;
    }

    public void setTimingCollector(ITimingCollector timingCollector) {
      mTimingCollector = new WeakReference<>(timingCollector);
    }

    public void setInstanceId(int instanceId) {
      mInstanceId = instanceId;
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
      super.onLayout(changed, l, t, r, b);
      if (shouldDrawWithDisplayList()) {
        mRenderer.onLayout(changed, l, t, r, b);
      }
    }

    @Override
    protected void onDraw(Canvas canvas) {
      super.onDraw(canvas);
      if (shouldDrawWithDisplayList()) {
        mRenderer.onDraw(canvas);
      }
    }

    @Override
    protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
      if (shouldDrawWithDisplayList()) {
        mRenderer.beforeDrawChild(canvas, child);
        boolean ret = super.drawChild(canvas, child, drawingTime);
        mRenderer.afterDrawChild(canvas, child);
        return ret;
      }
      Rect bound = null;
      if (mDrawChildHook != null) {
        bound = mDrawChildHook.beforeDrawChild(canvas, child, drawingTime);
      }
      boolean ret;
      if (bound != null) {
        canvas.save();
        canvas.clipRect(bound);
        ret = super.drawChild(canvas, child, drawingTime);
        canvas.restore();
      } else {
        ret = super.drawChild(canvas, child, drawingTime);
      }

      if (mDrawChildHook != null) {
        mDrawChildHook.afterDrawChild(canvas, child, drawingTime);
      }

      return ret;
    }

    @Override
    public void setChildrenDrawingOrderEnabled(boolean enabled) {
      super.setChildrenDrawingOrderEnabled(enabled);
    }

    @Override
    protected int getChildDrawingOrder(int childCount, int index) {
      if (mDrawChildHook != null) {
        return mDrawChildHook.getChildDrawingOrder(childCount, index);
      }
      return super.getChildDrawingOrder(childCount, index);
    }

    /* intercept Hover event on UIBodyView, do not dispatch if consumed by
     * LynxAccessibilityNodeProvider */
    @Override
    public boolean dispatchHoverEvent(MotionEvent event) {
      if (isAccessibilityDisabled()) {
        return super.dispatchHoverEvent(event);
      }
      if (mA11yWrapper != null) {
        if (mA11yWrapper.enableHelper()) {
          return super.dispatchHoverEvent(event);
        } else if (mA11yWrapper.onHoverEvent(this, event)) {
          return true;
        }
      }
      return onHoverEvent(event);
    }

    /* WINDOW_CONTENT_CHANGED event come from scroll
     *  focused view will move when scroll if it is child of event source
     *  so set event source to UIBodyView
     * */
    @Override
    public boolean requestSendAccessibilityEvent(
        View child, android.view.accessibility.AccessibilityEvent event) {
      if (isAccessibilityDisabled()) {
        return super.requestSendAccessibilityEvent(child, event);
      }
      if (mA11yWrapper != null && !mA11yWrapper.enableHelper()
          && event.getEventType()
              == android.view.accessibility.AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED) {
        event.setSource(this);
      }
      return super.requestSendAccessibilityEvent(child, event);
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public void setShouldInterceptRequestLayout(boolean intercept) {
      mInterceptRequestLayout = intercept;
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public boolean HasPendingRequestLayout() {
      return mHasPendingRequestLayout;
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public LynxBooleanOption getLongTaskMonitorEnabled() {
      LynxBooleanOption longTaskEnabled = LynxBooleanOption.UNSET;
      Context context = getContext();
      if (context instanceof LynxContext) {
        longTaskEnabled = ((LynxContext) context).getLongTaskMonitorEnabled();
      }
      return longTaskEnabled;
    }

    public boolean isAccessibilityDisabled() {
      return false;
    }

    public boolean isChildLynxPageUI() {
      return mIsChildLynxPageUI;
    }

    public void setIsChildLynxPageUI(boolean isChildLynxPageUI) {
      mIsChildLynxPageUI = isChildLynxPageUI;
    }

    public void setAttachLynxPageUICallback(attachLynxPageUICallback callback) {}

    // run task on engine thread
    public void runOnTasmThread(Runnable runnable) {}

    /**
     * @return internal UIRenderer
     * @brief To be combative with different kinds of UIRenders, UIRender has to be built in
     * BodyView ahead of TemplateRender. So we must provide this method for TemplateRender to get
     * UIRenderer.
     */
    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public ILynxUIRenderer getLynxUIRendererInternal() {
      return mLynxUIRender;
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public void setLynxUIRendererInternal(ILynxUIRenderer uiRenderer) {
      mLynxUIRender = uiRenderer;
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public void innerSetMeasuredDimension(int w, int h) {
      if (TraceEvent.isTracingStarted()) {
        HashMap<String, String> map = new HashMap<>();
        map.put("width", String.valueOf(w));
        map.put("height", String.valueOf(h));
        TraceEvent.instant(
            TraceEvent.CATEGORY_DEFAULT, TraceEventDef.UI_BODY_SET_MEASURED_DIMENSION, map);
      }
      setMeasuredDimension(w, h);
    }

    /**
     * @return LynxViewBuilder for FrameView
     * @brief to build frame view
     */
    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public LynxViewBuilder getLynxViewBuilder() {
      return null;
    }

    public boolean isMeaningfulPaintingAreaInvalidate() {
      return mIsMeaningfulPaintingAreaInvalidate;
    }

    public void invalidateMeaningfulPaintingArea() {
      mIsMeaningfulPaintingAreaInvalidate = true;
    }

    public List<MeaningfulPaintingArea> getMeaningfulPaintingAreas() {
      if (!(mDrawChildHook instanceof ViewInfo)) {
        return null;
      }

      ArrayList<MeaningfulPaintingArea> areas = new ArrayList<>();
      ((ViewInfo) mDrawChildHook).generateMeaningfulPaintingArea(0, 0, areas);
      return areas;
    }
  }
}
