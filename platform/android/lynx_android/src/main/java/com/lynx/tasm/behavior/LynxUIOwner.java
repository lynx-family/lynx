// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static com.lynx.tasm.animation.transition.TransitionAnimationManager.hasTransitionAnimation;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityMutationHelper.MUTATION_ACTION_DETACH;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityMutationHelper.MUTATION_ACTION_INSERT;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityMutationHelper.MUTATION_ACTION_REMOVE;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityMutationHelper.MUTATION_ACTION_UPDATE;

import android.graphics.Rect;
import android.os.Build;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.annotation.UiThread;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.mapbuffer.ReadableMapBuffer;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.NativeFacade;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.animation.keyframe.KeyframeManager;
import com.lynx.tasm.animation.transition.TransitionAnimationManager;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.shadow.ShadowNodeType;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.behavior.ui.list.UIList;
import com.lynx.tasm.behavior.ui.list.container.UIListContainer;
import com.lynx.tasm.behavior.ui.swiper.XSwiperUI;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.behavior.utils.LynxUIMethodsExecutor;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.gesture.LynxNewGestureDelegate;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.utils.LynxConstants;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Future;
import java.util.concurrent.FutureTask;

@UiThread
public class LynxUIOwner {
  private int mRootSign;
  private UIBody mUIBody;
  private LynxContext mContext;
  // Record used components in LynxView.
  private final Set<String> mComponentSet;
  private final HashSet<LynxBaseUI> mTranslateZParentHolder;
  private final List<ForegroundListener> mForegroundListeners;
  private final HashMap<Integer, LynxBaseUI> mUIHolder;
  private final HashMap<Integer, LynxBaseUI> mTextChildUIHolder;

  // Hold the UI that exec the boundingClientRect method in the layout process. Call the UI's
  // uiOwnerDidPerformLayout method after exec performLayout.
  private final HashSet<LynxBaseUI> mCachedBoundingClientRectUI;
  private static final String LYNXSDK_COMPONENT_STATISTIC_EVENT = "lynxsdk_component_statistic";

  private static final String LYNXSDK_ASYNC_CREATE_CONFIG = "lynxsdk_async_create_config";
  private static final String LYNXSDK_ASYNC_CREATE_SUCCESS_EVENT =
      "lynxsdk_async_create_success_event";
  /**
   * mComponentIdToUiIdHolder is used to map radon component id/fiber component id to element id.
   * Because unlike virtual component id, radon/fiber component id is not equal to element id.
   * In method invokeUIMethod, we need to use this map and radon/fiber(js) component id to find
   * related UI.
   */
  private final HashMap<String, Integer> mComponentIdToUiIdHolder;
  private final BehaviorRegistry mBehaviorRegistry;
  private boolean mIsFirstLayout;
  private boolean mIsRootLayoutAnimationRunning;
  private boolean mIsContextFree = false;
  private static final String TAG = "LynxUIOwner";
  private WeakReference<NativeFacade> mNativeFacade;
  private Boolean mSettingsEnableNewImage = null;
  // Manages the gesture arenas for handling touch events
  private GestureArenaManager mGestureArenaManager;

  private boolean mEnableReportCreateAsync;

  // Optimize ContextFree to trigger CreateNodeAsyncTask on attachContext.
  private final ConcurrentLinkedQueue<FutureTask<Runnable>> mCreateNodeAsyncTasks =
      new ConcurrentLinkedQueue<>();

  private HashMap<String, Boolean> mCreateNodeConfigHasReportedMark;

  public LynxUIOwner(
      LynxContext context, BehaviorRegistry behaviorRegistry, @Nullable UIBodyView body) {
    TraceEvent.beginSection("LynxUIOwner initialized");
    mContext = context;
    mBehaviorRegistry = behaviorRegistry;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
      // In the higher version of Android, we need to modify componentset and report, so it must be
      // thread safty.
      mComponentSet = ConcurrentHashMap.newKeySet();
    } else {
      // In the lower version of Android, we don't modify componentset and report.
      mComponentSet = new HashSet<>();
    }
    mTranslateZParentHolder = new HashSet<>();
    mForegroundListeners = new ArrayList<>();
    mUIHolder = new HashMap<>();
    mTextChildUIHolder = new HashMap<>();
    mComponentIdToUiIdHolder = new HashMap<>();
    mRootSign = -1;
    mUIBody = new UIBody(mContext, body);
    mContext.setUIBody(mUIBody);
    mIsFirstLayout = true;
    mIsRootLayoutAnimationRunning = true;
    mCachedBoundingClientRectUI = new HashSet<>();
    mEnableReportCreateAsync =
        LynxEnv.getBooleanFromExternalEnv(LynxEnvKey.ENABLE_REPORT_CREATE_ASYNC_TAG, false);
    mCreateNodeConfigHasReportedMark = new HashMap<String, Boolean>();
    attachUIBodyView(body);
    TraceEvent.endSection("LynxUIOwner initialized");
  }

  /**
   * when async render, we should attach LynxView
   *
   * @param view
   */
  public void attachUIBodyView(@Nullable UIBodyView view) {
    if (mUIBody.getBodyView() == null) {
      mUIBody.attachUIBodyView(view);
    }
    if (isContextFree()) {
      while (!mCreateNodeAsyncTasks.isEmpty()) {
        FutureTask<Runnable> task = mCreateNodeAsyncTasks.poll();
        if (task != null) {
          LynxThreadPool.postUIOperationTask(new Runnable() {
            @Override
            public void run() {
              task.run();
            }
          });
        }
      }
    }
  }

  public void attachNativeFacade(NativeFacade nativeFacade) {
    mNativeFacade = new WeakReference<>(nativeFacade);
  }

  public void updateProperties(int tag, boolean tendToFlatten, StylesDiffMap props,
      @Nullable Map<String, EventsListener> eventsListenerMap,
      @Nullable Map<Integer, GestureDetector> gestureDetectors) {
    UIThreadUtils.assertOnUiThread();
    LynxBaseUI ui = mUIHolder.get(tag);
    if (ui == null) {
      return;
    }
    updateComponentIdToUiIdMapIfNeeded(tag, ui.getTagName(), props);
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = "UIOwner.updateProps." + ui.getTagName();
      TraceEvent.beginSection(traceEvent);
    }

    if (eventsListenerMap != null) {
      ui.setEvents(eventsListenerMap);
    }

    // update gesture detectors
    if (gestureDetectors != null) {
      ui.setGestureDetectors(gestureDetectors);
    }

    if (props != null && !props.isEmpty()) {
      boolean needUpdateFlatten = !tendToFlatten && ui.isFlatten();

      // Transit UI between LynxUI and LynxFlattenUI according to props.
      // tendToFlatten is determined by native.
      if (needUpdateFlatten) {
        // update flatten status with old props first & get the new created UI.
        updateFlatten(tag, tendToFlatten);
        ui = mUIHolder.get(tag);
      }

      // flush new props.
      // When updating UI, the transition and animation properties need to be consumed first.
      if (hasTransitionAnimation(props)) {
        if (ui instanceof UIShadowProxy) {
          // transitionAnimator should only be set to shadowProxy's child
          UIShadowProxy shadow = (UIShadowProxy) ui;
          shadow.getChild().initTransitionAnimator(props.mBackingMap);
        } else {
          ui.initTransitionAnimator(props.mBackingMap);
        }
      }

      if (KeyframeManager.hasKeyframeAnimation(props)) {
        if (ui instanceof UIShadowProxy) {
          // keyframe animation's props should not be set to shadowProxy.
          UIShadowProxy shadow = (UIShadowProxy) ui;
          shadow.getChild().setAnimation(props.getArray(PropsConstants.ANIMATION));
        } else {
          ui.setAnimation(props.getArray(PropsConstants.ANIMATION));
        }
      }

      checkShadowOrOutline(props, ui);

      // TODO(renzhongyue): now will setprops twice, one set old props to new UI in updateFlatten,
      // other set new props to new UI here. Can check props, if don't have initial state related
      // props, only update the new props to new UI directly.

      ui.updateProperties(props);
      checkTranslateZ(ui);
    }
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  public void initGestureArenaManager(LynxContext context) {
    if (mGestureArenaManager == null) {
      mGestureArenaManager = new GestureArenaManager();
    }
    mGestureArenaManager.init(true, context);
  }

  private void addShadowProxy(LynxBaseUI ui) {
    LynxBaseUI parent = ui.getParentBaseUI();
    if (parent == null) {
      LLog.e(TAG, "addShadowProxy failed, parent is null.");
      return;
    }

    int index = parent.getIndex(ui);
    remove(parent.getSign(), ui.getSign());
    mContext.removeUIFromExposedMap(ui);
    UIShadowProxy shadowProxy = new UIShadowProxy(mContext, ui);
    mUIHolder.put(ui.getSign(), shadowProxy);
    insert(parent.getSign(), shadowProxy.getSign(), index);
  }

  private void handleShadowOrOutlineStyle(LynxBaseUI ui) {
    if (ui instanceof UIShadowProxy || ui.getParent() == null
        || ui.getParent() instanceof UIShadowProxy) {
      return;
    }
    addShadowProxy(ui);
  }

  private void checkShadowOrOutline(StylesDiffMap props, LynxBaseUI ui) {
    if (!hasShadowOrOutline(props)) {
      return;
    }

    if (props.getArray(PropsConstants.BOX_SHADOW) == null
        && (props.getInt(PropsConstants.OUTLINE_STYLE, -1) == -1)) {
      return;
    }

    if (ui instanceof UIShadowProxy || ui.getParent() == null
        || ui.getParent() instanceof UIShadowProxy) {
      return;
    }
    addShadowProxy(ui);
  }

  public void updateViewExtraData(int tag, Object extraData) {
    UIThreadUtils.assertOnUiThread();

    LynxBaseUI ui = mUIHolder.get(tag);
    if (ui != null) {
      String traceEvent = null;
      if (TraceEvent.enableTrace()) {
        traceEvent = "UIOwner.updateViewExtraData." + ui.getTagName();
        TraceEvent.beginSection(traceEvent);
      }
      ui.updateExtraData(extraData);
      if (TraceEvent.enableTrace()) {
        TraceEvent.endSection(traceEvent);
      }
    }
  }

  public void pauseRootLayoutAnimation() {
    mIsRootLayoutAnimationRunning = false;
  }

  public void resumeRootLayoutAnimation() {
    mIsRootLayoutAnimationRunning = true;
  }

  public void updateLayout(int tag, int x, int y, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, final Rect bound, float[] sticky,
      float maxHeight, int nodeIndex) {
    LynxBaseUI ui = mUIHolder.get(tag);
    if (ui == null) {
      LynxError error =
          new LynxError(LynxSubErrorCode.E_LAYOUT_UPDATE_UI_NOT_FOUND, "Can't find ui tag");
      error.addCustomInfo("node_index", Integer.toString(nodeIndex));
      mContext.handleLynxError(error);
      return;
    }
    // Update size first. if ShadowProxy, use it's child instead
    (ui instanceof UIShadowProxy ? ((UIShadowProxy) ui).getChild() : ui)
        .updateLayoutSize(width, height);

    if (ui.getLayoutAnimator() != null) {
      // To prevent layout animation manager from using outdated layout information to update UI, we
      // need to ensure that the layout information in the layout animation manager is always
      // up-to-date. Therefore, even if the layout update does not trigger a layout animation, we
      // still need to update the latest layout information to the layout animation manager.
      ui.getLayoutAnimator().updateLatestLayoutInfo(x, y, width, height, paddingLeft, paddingTop,
          paddingRight, paddingBottom, marginLeft, marginTop, marginRight, marginBottom,
          borderLeftWidth, borderTopWidth, borderRightWidth, borderBottomWidth, bound);
    }

    TransitionAnimationManager transitionAnimator = ui.getTransitionAnimator();
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = "UIOwner.updateLayout." + ui.getTagName();
      TraceEvent.beginSection(traceEvent);
    }
    if (transitionAnimator != null && transitionAnimator.containLayoutTransition()
        && !mIsFirstLayout && !ui.isFirstAnimatedReady()) {
      transitionAnimator.applyLayoutTransition(
          (ui instanceof UIShadowProxy ? (LynxUI) (((UIShadowProxy) ui).getChild()) : (LynxUI) ui),
          x, y, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom, marginLeft,
          marginTop, marginRight, marginBottom, borderLeftWidth, borderTopWidth, borderRightWidth,
          borderBottomWidth, bound);
      mUIBody.invalidate();
    } else if (ui.enableLayoutAnimation() && !mIsFirstLayout
        && (tag != mRootSign || (tag == mRootSign && mIsRootLayoutAnimationRunning))) {
      ui.getLayoutAnimator().applyLayoutUpdate(
          (ui instanceof UIShadowProxy ? (LynxUI) (((UIShadowProxy) ui).getChild()) : (LynxUI) ui),
          x, y, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom, marginLeft,
          marginTop, marginRight, marginBottom, borderLeftWidth, borderTopWidth, borderRightWidth,
          borderBottomWidth, bound);
      mUIBody.invalidate();
    } else {
      ui.updateLayout(x, y, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom,
          marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth, borderTopWidth,
          borderRightWidth, borderBottomWidth, bound);
    }

    ui.updateSticky(sticky);
    ui.updateMaxHeight(maxHeight);
    insertA11yMutationEvent(MUTATION_ACTION_UPDATE, ui);
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  // checkTranslateZ checks ui has updated translateZ prop and mark its parent need sort children by
  // translateZ, updating parent to LynxUI if parent is LynxFlattenUI.
  private void checkTranslateZ(LynxBaseUI ui) {
    if (ui.getParentBaseUI() != null) {
      // UI not be added to UI tree or UIBody.
      checkTranslateZ(ui.getSign(), ui.getParentBaseUI().getSign());
    }
  }

  private void checkTranslateZ(int childTag, int parentTag) {
    if (!mContext.getEnableFlattenTranslateZ()) {
      return;
    }
    LynxBaseUI child = mUIHolder.get(childTag);
    if (child == null) {
      return;
    }
    if (child.getTranslationZ() != child.getLastTranslateZ()) {
      LynxBaseUI parent = mUIHolder.get(parentTag);
      if (parent != null && parent.isFlatten()) {
        newUpdateFlatten(parentTag, false);
      }
      parent = mUIHolder.get(parentTag);
      mTranslateZParentHolder.add(parent);
      parent.setNeedSortChildren(true);
      child.setLastTranslateZ(child.getTranslationZ());
    }
  }

  public void setFirstLayout() {
    mIsFirstLayout = false;
  }

  public void onNodeReady(int tag) {
    LynxBaseUI ui = mUIHolder.get(tag);
    if (ui != null) {
      (ui instanceof UIShadowProxy ? ((UIShadowProxy) ui).getChild() : ui).onNodeReady();
    }
  }

  public void onNodeReload(int tag) {
    LynxBaseUI ui = mUIHolder.get(tag);
    if (ui != null) {
      (ui instanceof UIShadowProxy ? ((UIShadowProxy) ui).getChild() : ui).onNodeReload();
    }
  }

  public void onNodeRemoved(int tag) {
    LynxBaseUI ui = mUIHolder.get(tag);
    if (ui != null) {
      onNodeRemovedRecursively(ui);
    }
  }

  private void onNodeRemovedRecursively(LynxBaseUI ui) {
    (ui instanceof UIShadowProxy ? ((UIShadowProxy) ui).getChild() : ui).onNodeRemoved();
    if (!(ui instanceof UIShadowProxy)) {
      for (LynxBaseUI child : ui.getChildren()) {
        onNodeRemovedRecursively(child);
      }
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public synchronized void createViewInternal(int sign, String tagName,
      @Nullable StylesDiffMap initialProps, @Nullable Map<String, EventsListener> eventsListenerMap,
      boolean flatten, int nodeIndex, @Nullable Map<Integer, GestureDetector> gestureDetectors) {
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = "UIOwner.createView." + tagName;
      TraceEvent.beginSection(traceEvent);
    }
    UIThreadUtils.assertOnUiThread();
    LynxBaseUI ui = null;
    try {
      ui = createViewInterval(
          sign, tagName, eventsListenerMap, flatten, nodeIndex, gestureDetectors);
      ui = consumeInitialProps(ui, initialProps);
    } catch (Throwable e) {
      RuntimeException exception = new RuntimeException(
          "createUI catch error while createUI for tag: " + tagName + ", " + e.getMessage(), e);
      exception.setStackTrace(e.getStackTrace());
      // take care: LynxError.LYNX_ERROR_MAIN_FLOW will be treated as FatalError(like code<=200) in
      // Bullet, which will call LynxView#destroy in onError process
      mUIBody.getLynxContext().handleException(exception);
    } finally {
      // if ui is not null, make sure the ui is saved to mUIHolder
      if (ui != null) {
        // Report the usage of the component.
        reportStatistic(tagName);
        updateComponentIdToUiIdMapIfNeeded(sign, tagName, initialProps);
        mUIHolder.put(sign, ui);
      } else {
        LLog.e(TAG, "createUI got null ui for tag:" + tagName);
      }
    }
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void createView(int sign, String tagName, ReadableMap initialProps,
      ReadableMapBuffer initialStyles, ReadableArray eventListeners, boolean isFlatten,
      int nodeIndex, ReadableArray gestureDetectors) {
    StylesDiffMap styles = null;
    if (initialProps != null) {
      styles = new StylesDiffMap(initialProps, initialStyles);
    }
    Map<String, EventsListener> listeners = EventsListener.convertEventListeners(eventListeners);
    Map<Integer, GestureDetector> detectors =
        GestureDetector.convertGestureDetectors(gestureDetectors);

    createViewInternal(sign, tagName, styles, listeners, isFlatten, nodeIndex, detectors);
  }

  // TODO(ZHOUZHITAO): REFACTOR CODE TO REUSE SHARED NODE SNIPPET
  public Runnable createViewAsyncRunnable(final int sign, final String tagName,
      final ReadableMap initialProps, final ReadableMapBuffer initialStyles,
      final ReadableArray eventListeners, final boolean isFlatten, int nodeIndex,
      final ReadableArray gestureDetectors) {
    reportCreateViewConfig(sign, tagName, true);
    StylesDiffMap styles = null;
    if (initialProps != null) {
      styles = new StylesDiffMap(initialProps, initialStyles);
    }
    Map<String, EventsListener> listeners = EventsListener.convertEventListeners(eventListeners);
    Map<Integer, GestureDetector> detectors =
        GestureDetector.convertGestureDetectors(gestureDetectors);
    try {
      String traceEvent = null;
      if (TraceEvent.enableTrace()) {
        traceEvent = "UIOwner.createAsyncViewRunnable." + tagName;
        TraceEvent.beginSection(traceEvent);
      }
      final LynxBaseUI[] ui = new LynxBaseUI[1];
      ui[0] = createViewInterval(sign, tagName, listeners, isFlatten, nodeIndex, detectors);
      final UIShadowProxy proxy = consumeInitialPropsInterval(ui[0], styles);
      if (TraceEvent.enableTrace()) {
        TraceEvent.endSection(traceEvent);
      }
      reportCreateAsyncSuccessEvent(sign, tagName, true, CreateViewAsyncStatus.FUTURE_DONE);
      final StylesDiffMap styleMap = styles;
      return new Runnable() {
        @Override
        public void run() {
          String traceEvent = null;
          if (TraceEvent.enableTrace()) {
            traceEvent = "UIOwner.AfterCreateAsyncViewRunnable." + tagName;
            TraceEvent.beginSection(traceEvent);
          }
          ui[0] = afterConsumeInitialProps(ui[0], proxy, styleMap);
          // Report the usage of the component.
          reportStatistic(tagName);
          updateComponentIdToUiIdMapIfNeeded(sign, tagName, styleMap);
          mUIHolder.put(sign, ui[0]);
          if (TraceEvent.enableTrace()) {
            TraceEvent.endSection(traceEvent);
          }
        }
      };
    } catch (Throwable e) {
      String errorMessage = "createViewAsync failed, tagName:" + tagName + ", error:" + e;
      LLog.e(TAG, errorMessage);

      final Exception exception = new Exception(errorMessage);
      // need replace for original stacktrace,
      // because our handleException will drop the cause...
      exception.setStackTrace(e.getStackTrace());
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          mContext.handleException(exception);
        }
      });
      Runnable runnable = new Runnable() {
        @Override
        public void run() {
          createView(sign, tagName, initialProps, initialStyles, eventListeners, isFlatten,
              nodeIndex, gestureDetectors);
        }
      };
      reportCreateAsyncSuccessEvent(
          sign, tagName, false, CreateViewAsyncStatus.FUTURE_DONE_EXCEPTION);
      return runnable;
    }
  }

  // TODO(zhouzhitao): Control group logic, will be removed once the experiment yields results
  public Future<Runnable> createViewAsync(final int sign, final String tagName,
      @Nullable final StylesDiffMap initialProps,
      @Nullable final Map<String, EventsListener> eventsListenerMap, final boolean flatten,
      int nodeIndex, @Nullable final Map<Integer, GestureDetector> gestureDetectors) {
    Callable<Runnable> createViewAsyncTask = () -> {
      try {
        String traceEvent = null;
        if (TraceEvent.enableTrace()) {
          traceEvent = "UIOwner.createViewAsync." + tagName;
          TraceEvent.beginSection(traceEvent);
        }
        final LynxBaseUI[] ui = new LynxBaseUI[1];
        ui[0] = createViewInterval(
            sign, tagName, eventsListenerMap, flatten, nodeIndex, gestureDetectors);
        final UIShadowProxy proxy = consumeInitialPropsInterval(ui[0], initialProps);
        if (TraceEvent.enableTrace()) {
          TraceEvent.endSection(traceEvent);
        }
        return new Runnable() {
          @Override
          public void run() {
            String traceEvent = null;
            if (TraceEvent.enableTrace()) {
              traceEvent = "UIOwner.AfterCreateViewAsync." + tagName;
              TraceEvent.beginSection(traceEvent);
            }
            ui[0] = afterConsumeInitialProps(ui[0], proxy, initialProps);
            // Report the usage of the component.
            reportStatistic(tagName);
            updateComponentIdToUiIdMapIfNeeded(sign, tagName, initialProps);
            mUIHolder.put(sign, ui[0]);
            if (TraceEvent.enableTrace()) {
              TraceEvent.endSection(traceEvent);
            }
          }
        };
      } catch (Throwable e) {
        String errorMessage = "createViewAsync failed, tagName:" + tagName + ", error:" + e;
        LLog.e(TAG, errorMessage);

        final Exception exception = new Exception(errorMessage);
        // need replace for original stacktrace,
        // because our handleException will drop the cause...
        exception.setStackTrace(e.getStackTrace());
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            mContext.handleException(exception);
          }
        });
        return null;
      }
    };
    if (isContextFree()) {
      FutureTask<Runnable> futureTask = new FutureTask<>(createViewAsyncTask);
      mCreateNodeAsyncTasks.add(futureTask);
      return futureTask;
    }
    return LynxThreadPool.postUIOperationTask(createViewAsyncTask);
  }

  private LynxBaseUI consumeInitialProps(LynxBaseUI ui, @Nullable StylesDiffMap initialProps) {
    UIShadowProxy proxy = consumeInitialPropsInterval(ui, initialProps);
    return afterConsumeInitialProps(ui, proxy, initialProps);
  }

  private UIShadowProxy consumeInitialPropsInterval(
      LynxBaseUI ui, @Nullable StylesDiffMap initialProps) {
    UIShadowProxy proxy = null;
    if (initialProps != null) {
      if (hasShadowOrOutline(initialProps)) {
        proxy = new UIShadowProxy(mContext, ui);
      }
      ui.updatePropertiesInterval(initialProps);
    }
    return proxy;
  }

  private LynxBaseUI afterConsumeInitialProps(
      LynxBaseUI ui, UIShadowProxy proxy, StylesDiffMap initialProps) {
    if (ui instanceof PatchFinishListener) {
      mContext.registerPatchFinishListener((PatchFinishListener) ui);
    }
    if (ui instanceof ForegroundListener) {
      registerForegroundListener((ForegroundListener) ui);
    }
    if (initialProps != null) {
      ui.afterPropsUpdated(initialProps);
      // When updating UI, the transition and animation properties need to be consumed last.
      if (hasTransitionAnimation(initialProps)) {
        ui.initTransitionAnimator(initialProps.mBackingMap);
      }
      if (KeyframeManager.hasKeyframeAnimation(initialProps)) {
        ui.setAnimation(initialProps.getArray(PropsConstants.ANIMATION));
      }
    }
    if (proxy != null) {
      ui = proxy;
    }
    return ui;
  }

  private LynxBaseUI createViewInterval(int sign, String tagName,
      @Nullable Map<String, EventsListener> eventsListenerMap, boolean flatten, int nodeIndex,
      @Nullable Map<Integer, GestureDetector> gestureDetectors) {
    LynxBaseUI ui = null;

    // Root ui do not need to create from behavior as ui has been created through
    // createRootUI()
    if (mRootSign < 0 && tagName.equals(LynxConstants.ROOT_TAG_NAME)) {
      ui = mUIBody;
      mRootSign = sign;
    } else {
      ui = createUI(tagName, flatten);
      ui.setEvents(eventsListenerMap);
    }
    ui.setSign(sign, tagName);
    ui.setNodeIndex(nodeIndex);
    ui.setGestureDetectors(gestureDetectors);
    return ui;
  }

  private boolean hasShadowOrOutline(StylesDiffMap map) {
    return map.hasKey(PropsConstants.BOX_SHADOW) || map.hasKey(PropsConstants.OUTLINE_COLOR)
        || map.hasKey(PropsConstants.OUTLINE_STYLE) || map.hasKey(PropsConstants.OUTLINE_WIDTH);
  }

  public void updateFlatten(int tag, boolean flatten) {
    newUpdateFlatten(tag, flatten);
  }

  private void newUpdateFlatten(int tag, boolean flatten) {
    LynxBaseUI oldUI = mUIHolder.get(tag);
    if (oldUI == null) {
      return;
    }
    LynxBaseUI parent = oldUI.getParentBaseUI();
    StylesDiffMap props = new StylesDiffMap(oldUI.getProps());
    // get all children of the oldUI.
    List<LynxBaseUI> tempChildren = new ArrayList<>(oldUI.getChildren());
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = "UIOwner.updateFlatten." + oldUI.getTagName();
      TraceEvent.beginSection(traceEvent);
    }
    int index = 0;

    // remove from parent
    if (parent != null) {
      // remove old ui from parent
      index = parent.getIndex(oldUI);
      // If old UI flatten, it will remove all UIs which are flatten to its parent.
      removeFromDrawList(oldUI);
      parent.removeChild(oldUI);
    }

    // LynxFlattenUI will do this part when remove itself from drawList.
    if (!oldUI.isFlatten()) {
      for (LynxBaseUI child : tempChildren) {
        removeFromDrawList(child);
      }
    }

    // remove children in the UI tree after removing them from the drawList.
    for (int i = oldUI.getChildren().size() - 1; i >= 0; --i) {
      LynxBaseUI mChild = oldUI.getChildAt(i);
      oldUI.removeChild(mChild);
    }
    LynxBaseUI newUI = createUI(oldUI.getTagName(), flatten);

    // apply before setSign
    oldUI.applyUIPaintStylesToTarget(newUI);

    newUI.setSign(oldUI.getSign(), oldUI.getTagName());

    // Restore the Style
    consumeInitialProps(newUI, props);
    mUIHolder.put(oldUI.getSign(), newUI);

    if (mTranslateZParentHolder.contains(oldUI)) {
      mTranslateZParentHolder.remove(oldUI);
      mTranslateZParentHolder.add(newUI);
    }

    // Restore the tree structure
    if (parent != null) {
      parent.insertChild(newUI, index);
      insertIntoDrawList(parent, newUI, index);
    }
    int childIndex = 0;
    for (LynxBaseUI child : tempChildren) {
      // Insert all children in UI tree.
      // If newUI is flatten, child drawingLayoutInfo will be re-computed during layout. If newUI is
      // LyxnUI, will use the left, top ,bound directly during layout. Need to reset the
      // drawingLayoutInfo.
      resetUIDrawingLayoutInfo(child);
      newUI.insertChild(child, childIndex++);
    }
    // Re-build the drawList.
    insertChildIntoDrawListRecursive(newUI);

    // Restore layout.
    newUI.updateLayoutInfo(oldUI);
    newUI.copyPropFromOldUiInUpdateFlatten(oldUI);
    newUI.measure();
    ((LynxUI) newUI).handleLayout();
    if (newUI instanceof UIGroup) {
      ((UIGroup) newUI).layoutChildren();
    }
    newUI.invalidate();
    oldUI.destroy();
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  private void resetUIDrawingLayoutInfo(LynxBaseUI ui) {
    ui.setBound(null);
    ui.setLeft(ui.getOriginLeft());
    ui.setTop(ui.getOriginTop());
  }

  /**
   * Recursively insert children of LynxFlattenUI into corresponding drawList.
   * @param parent LynxFlattenUI
   */
  private void insertChildIntoDrawListRecursive(LynxBaseUI parent) {
    int index = 0;
    for (LynxBaseUI child : parent.getChildren()) {
      insertIntoDrawList(parent, child, index++);
      if (child.isFlatten()) {
        insertChildIntoDrawListRecursive(child);
      }
    }
  }

  public void insert(int parentTag, int childTag, int index) {
    newInsert(parentTag, childTag, index);
  }

  private void newInsert(int parentTag, int childTag, int index) {
    if (mUIHolder.size() > 0) {
      LynxBaseUI parent = mUIHolder.get(parentTag);
      if (parent == null) {
        throw new RuntimeException(
            "Insertion (new) failed due to unknown parent signature: " + parentTag);
      }
      LynxBaseUI child = mUIHolder.get(childTag);
      if (child == null) {
        throw new RuntimeException(
            "Insertion (new) failed due to unknown child signature: " + childTag);
      }
      checkTranslateZ(childTag, parentTag);
      parent = mUIHolder.get(parentTag);

      if (!parent.canHaveFlattenChild() && child.isFlatten()) {
        newUpdateFlatten(childTag, false);
        child = mUIHolder.get(childTag);
      }
      if (index == -1) { // If the index is equal to -1 should add to the last
        index = parent.getChildren().size();
      }
      parent.insertChild(child, index);
      insertIntoDrawList(parent, child, index);
      if (child.isFlatten()) {
        parent.flattenChildrenCountIncrement();
      }
      // when moveNode should recursively insert children of flattenUI into drawingList
      if (child.isFlatten()) {
        insertChildIntoDrawListRecursive(child);
        child.requestLayout();
        child.invalidate();
      }
      insertA11yMutationEvent(MUTATION_ACTION_INSERT, child);
    }
  }

  private void insertIntoDrawList(LynxBaseUI parent, LynxBaseUI child, int index) {
    // Only LynxUI can have a draw list.
    LynxBaseUI realParent = parent.isFlatten() ? parent.getDrawParent() : parent;
    if (realParent == null) {
      return;
    }
    if (index == 0) {
      // index is 0, child is the head or child's precursor is its flatten parent.
      ((LynxUI) realParent).insertDrawList(parent.isFlatten() ? parent : null, child);
    } else {
      // find precursor in the drawList. Should be the first non-flatten right most UI in brother
      // node's sub UI tree.
      LynxBaseUI pre = parent.getChildAt(index - 1);
      while (pre.isFlatten() && !pre.getChildren().isEmpty()) {
        pre = pre.getChildAt(pre.getChildren().size() - 1);
      }
      ((LynxUI) realParent).insertDrawList(pre, child);
    }

    if ((!child.isFlatten()) && ((UIGroup) realParent).isInsertViewCalled()) {
      // Some customized view will handle the insertion of view itself
      // when insertChild for UI tree(e.g x-swiper).
      ((UIGroup) realParent).insertView((LynxUI) child);
    }
  }

  public void remove(int parentTag, int childTag) {
    newRemove(parentTag, childTag);
  }

  private void newRemove(int parentTag, int childTag) {
    if (mUIHolder.size() > 0) {
      LynxBaseUI child = mUIHolder.get(childTag);
      if (child == null) {
        throw new RuntimeException("Trying to remove unknown ui signature: " + childTag);
      }
      final LynxBaseUI parent =
          parentTag == -1 ? child.getParentBaseUI() : mUIHolder.get(parentTag);
      if (parent == null) {
        throw new RuntimeException("Trying to remove unknown ui signature: " + childTag);
      }
      String traceEvent = null;
      if (TraceEvent.enableTrace()) {
        traceEvent = "UIOwner.remove." + parent.getTagName() + "." + child.getTagName();
        TraceEvent.beginSection(traceEvent);
      }
      removeFromDrawList(child);
      parent.removeChild(child);
      resetUIDrawingLayoutInfo(child);
      if (child.isFlatten()) {
        parent.flattenChildrenCountDecrement();
      }
      child.removeChildrenExposureUI();
      insertA11yMutationEvent(MUTATION_ACTION_REMOVE, child);
      if (TraceEvent.enableTrace()) {
        TraceEvent.endSection(traceEvent);
      }
    }
  }

  private void removeFromDrawList(LynxBaseUI child) {
    UIGroup drawParent = (UIGroup) child.getDrawParent();
    LynxBaseUI parent = child.getParentBaseUI();

    if (drawParent == null || parent == null) {
      return;
    }

    // Child is not a flatten view. Only need to remove itself
    if (!child.isFlatten()) {
      LynxBaseUI pre = child.getPreviousDrawUI();
      LynxBaseUI next = child.getNextDrawUI();
      if (pre != null) {
        // child is not head of list
        pre.setNextDrawUI(next);
        if (next != null) {
          next.setPreviousDrawUI(pre);
        }
      } else {
        // child is head
        drawParent.setDrawHead(next);
        if (next != null) {
          next.setPreviousDrawUI(null);
        }
      }
      if (parent.isFlatten()) {
        drawParent.removeView(child);
      }
      child.setNextDrawUI(null);
      child.setPreviousDrawUI(null);
      child.setDrawParent(null);
    } else {
      // Child is flatten view, should remove the entire subtree from drawList.
      LynxBaseUI last = child;

      // Find the last ui in its subtree that flatten to the parent.
      while (last.isFlatten() && !last.getChildren().isEmpty()) {
        last = last.getChildAt(last.getChildren().size() - 1);
      }

      LynxBaseUI pre = child.getPreviousDrawUI();

      if (pre != null) {
        // Child is not the head of drawList.
        pre.setNextDrawUI(last.getNextDrawUI());
        if (last.getNextDrawUI() != null) {
          last.getNextDrawUI().setPreviousDrawUI(pre);
        }
      } else {
        drawParent.setDrawHead(last.getNextDrawUI());
        if (last.getNextDrawUI() != null) {
          last.getNextDrawUI().setPreviousDrawUI(null);
        }
      }
      // Break the list and remove the view
      child.setPreviousDrawUI(null);
      for (LynxBaseUI ui = child.getNextDrawUI(); ui != last.getNextDrawUI();
           ui = ui.getNextDrawUI()) {
        // Node in subtree is not flatten, remove it from drawParent's view tree
        ui.getPreviousDrawUI().setNextDrawUI(null);
        ui.setPreviousDrawUI(null);
        drawParent.removeView(ui);
        ui.setDrawParent(null);
      }
      last.setNextDrawUI(null);

      child.setDrawParent(null);
      parent.invalidate();
    }
  }

  public void destroy(int parentTag, int childTag) {
    String traceEvent = "UIOwner.destroy";
    TraceEvent.beginSection(traceEvent);
    if (mUIHolder.size() > 0) {
      LynxBaseUI child = mUIHolder.get(childTag);
      if (child == null) {
        TraceEvent.endSection(traceEvent);
        return;
      }
      mTranslateZParentHolder.remove(child);
      // if child has no parent still need to be removed
      removeFromDrawList(child);
      mUIHolder.remove(childTag);
      mContext.removeUIFromExposedMap(child);
      child.destroy();
      insertA11yMutationEvent(MUTATION_ACTION_DETACH, child);
      // FiberElement may be referenced by JS engine. Just clear the parent-child relationship.
      PageConfig pageConfig = getPageConfig();
      if (pageConfig != null && !pageConfig.getEnableFiberArc()) {
        destroyChildrenRecursively(child);
      }
      final LynxBaseUI parent =
          parentTag == -1 ? child.getParentBaseUI() : mUIHolder.get(parentTag);
      if (parent == null) {
        TraceEvent.endSection(traceEvent);
        return;
      }
      parent.removeChild(child);
    }
    mCreateNodeAsyncTasks.clear();
    TraceEvent.endSection(traceEvent);
  }

  public void reuseListNode(int tag, String itemKey) {
    LynxBaseUI baseUI = mUIHolder.get(tag);
    if (baseUI != null
        && (baseUI.getParentBaseUI() instanceof UIList
            || baseUI.getParentBaseUI() instanceof UIListContainer)) {
      baseUI.onListCellPrepareForReuse(itemKey, baseUI.getParentBaseUI());
    }
  }

  public void listCellDisappear(int tag, Boolean isExist, String itemKey) {
    LynxBaseUI baseUI = mUIHolder.get(tag);
    if (baseUI != null && baseUI.getParentBaseUI() instanceof UIListContainer) {
      baseUI.onListCellDisAppear(itemKey, baseUI.getParentBaseUI(), isExist);
    }
  }

  public void listCellAppear(int tag, String itemKey) {
    LynxBaseUI baseUI = mUIHolder.get(tag);
    if (baseUI != null && baseUI.getParentBaseUI() instanceof UIListContainer) {
      baseUI.onListCellAppear(itemKey, baseUI.getParentBaseUI());
    }
  }

  @Nullable
  public GestureArenaManager getGestureArenaManager() {
    return mGestureArenaManager;
  }

  public void destroy() {
    for (Map.Entry<Integer, LynxBaseUI> e : mUIHolder.entrySet()) {
      if (!(e.getValue() instanceof LynxBaseUI)) {
        // In some unknown case, e.getValue() is instance of java.lang.Double.
        // Here, we filter these cases to avoid ClassCastException.
        continue;
      }
      LynxBaseUI baseUI = e.getValue();
      if (baseUI != null) {
        baseUI.destroy();
      }
    }
    if (mContext != null) {
      mContext.destory();
    }
    if (mGestureArenaManager != null) {
      mGestureArenaManager.onDestroy();
    }
  }

  public void onTasmFinish(long operationId) {
    List<PatchFinishListener> listeners = mContext.getPatchFinishListeners();
    if (listeners != null) {
      for (PatchFinishListener listener : listeners) {
        listener.onPatchFinish();
      }
    }

    if (mUIBody.getBodyView() != null && mUIBody.getBodyView().HasPendingRequestLayout()) {
      LLog.i(TAG, "onTasmFinish do force RequestLayout after UpdateData in PreLoad Mode!");
      // FIXME(linxs): we need to find a better timing to do the request
      // step 1. preload, root's layoutRequested flag is flase
      // step 2. update, root's layoutRequested flag is flase
      //  1) update has layout changed, no problem
      //  2) update do not has layout changed, we need to find a timing to make root do
      //  requestLayout
      mUIBody.getBodyView().requestLayout();
    }
  }

  public void onLayoutFinish(int componentID, long operationId) {
    handleTranslateZUI();
    if (mUIBody.getLynxAccessibilityWrapper() != null) {
      mUIBody.getLynxAccessibilityWrapper().onLayoutFinish();
    }
    if (operationId == 0) {
      return;
    }
    LynxBaseUI ui = mUIHolder.get(getSignFromOperationId(operationId));
    if (ui != null) {
      String traceEvent = null;
      if (TraceEvent.enableTrace()) {
        traceEvent = "UIOwner.layoutFinish." + ui.getTagName();
        TraceEvent.beginSection(traceEvent);
      }
      ui.onLayoutFinish(operationId, mUIHolder.get(componentID));
      if (TraceEvent.enableTrace()) {
        TraceEvent.endSection(traceEvent);
      }
    }
  }

  private int getSignFromOperationId(long operationId) {
    return (int) (operationId >>> 32);
  }

  private void destroyChildrenRecursively(LynxBaseUI node) {
    for (int i = 0; i < node.getChildren().size(); i++) {
      LynxBaseUI child = node.getChildAt(i);
      child.destroy();
      mUIHolder.remove(child.getSign());
      mTranslateZParentHolder.remove(child);
      mContext.removeUIFromExposedMap(child);
      destroyChildrenRecursively(child);
    }
  }

  public void performMeasure() {
    mUIBody.measureChildren();
  }

  public void performLayout() {
    mUIBody.layoutChildren();

    if (mUIBody.getLynxContext().getEventEmitter() != null) {
      mUIBody.getLynxContext().getEventEmitter().sendLayoutEvent();
    }

    didPerformLayout();
  }

  private void didPerformLayout() {
    if (mCachedBoundingClientRectUI.isEmpty()) {
      return;
    }
    HashSet<LynxBaseUI> copyBoundingClientRectUI = new HashSet<>(mCachedBoundingClientRectUI);
    mCachedBoundingClientRectUI.clear();
    for (LynxBaseUI ui : copyBoundingClientRectUI) {
      ui.uiOwnerDidPerformLayout();
    }
    // call ui.uiOwnerDidPerformLayout() maybe add new ui to mCachedBoundingClientRectUI.
    didPerformLayout();
  }

  public int getRootWidth() {
    return mUIBody.getWidth();
  }

  public int getRootHeight() {
    return mUIBody.getHeight();
  }

  public UIBody getRootUI() {
    return mUIBody;
  }

  public LynxBaseUI getNode(int sign) {
    if (mUIHolder != null) {
      return mUIHolder.get(sign);
    }
    return null;
  }

  /**
   * Finds the component by its component id.
   *
   * If the component to find is root, whose component id is -1, then getRootUI()
   * is returned.
   * If the component to find is a VirtualComponent, whose component
   * id equals to its sign of ui, then findUIBySign is called to find it directly.
   * If the component to find is a RadonComponent or FiberComponent, whose component id doesn't
   * equal to its sign of ui, then mComponentIdToUiIdHolder is used to find its
   * sign before calling findUIBySign.
   *
   * @param componentId the id of the component to find.
   * @return the component to find.
   */
  public LynxBaseUI findLynxUIByComponentId(String componentId) {
    if (componentId.isEmpty() || LynxConstants.LYNX_DEFAULT_COMPONENT_ID.equals(componentId)) {
      return getRootUI();
    }

    // Initialize sign as 0 to prevent applying ROOT_UI_SIGN by default
    int sign = 0;
    if (mComponentIdToUiIdHolder.containsKey(componentId)) {
      sign = mComponentIdToUiIdHolder.get(componentId);
    } else {
      try {
        sign = Integer.parseInt(componentId);
      } catch (NumberFormatException numberFormatException) {
        sign = LynxConstants.LYNX_ROOT_UI_SIGN;
      }
    }

    if (sign == LynxConstants.LYNX_ROOT_UI_SIGN) {
      return getRootUI();
    }

    return getNode(sign);
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public LynxBaseUI findLynxUIBySign(int sign) {
    if (mUIHolder.get(sign) instanceof UIShadowProxy) {
      return ((UIShadowProxy) mUIHolder.get(sign)).getChild();
    }
    return mUIHolder.get(sign);
  }

  public LynxBaseUI findLynxUIById(String id, LynxBaseUI ui) {
    if (ui != null && ui.getIdSelector() != null && ui.getIdSelector().equals(id)) {
      return ui;
    }

    if (!(ui instanceof UIGroup)) {
      return null;
    }

    UIGroup target = (UIGroup) ui;
    for (int i = 0; i < target.getChildCount(); i++) {
      LynxBaseUI child = target.getChildAt(i);
      if (child.getIdSelector() != null && child.getIdSelector().equals(id)) {
        return child;
      }
      if (!child.getTagName().equals("component") && child instanceof UIGroup) {
        child = findLynxUIById(id, child);
        if (child != null) {
          return child;
        }
      }
    }
    return null;
  }

  private LynxBaseUI findLynxUIByIdWithGroup(String id, UIGroup target) {
    for (int i = 0; i < target.getChildCount(); i++) {
      LynxBaseUI child = target.getChildAt(i);
      if (child.getIdSelector() != null && child.getIdSelector().equals(id)) {
        return child;
      }
      if (!child.getTagName().equals("component") && child instanceof UIGroup) {
        UIGroup ui = (UIGroup) child;
        child = findLynxUIByIdWithGroup(id, ui);
        if (child != null) {
          return child;
        }
      }
    }
    return null;
  }

  public LynxContext getContext() {
    return mContext;
  }

  public void reset() {
    mIsFirstLayout = true;
    mRootSign = -1;
    if (mUIHolder != null) {
      for (LynxBaseUI value : mUIHolder.values()) {
        value.destroy();
      }
      mUIHolder.clear();
      mTranslateZParentHolder.clear();
    }
    if (mUIBody != null) {
      mUIBody.removeAll();
    }
    if (mComponentIdToUiIdHolder != null) {
      mComponentIdToUiIdHolder.clear();
    }
    NativeFacade nativeFacade = (mNativeFacade != null) ? mNativeFacade.get() : null;
    if (nativeFacade != null) {
      nativeFacade.clearAllNativeTimingInfo();
    }
  }

  public LynxBaseUI findLynxUIByName(@NonNull String name) {
    for (LynxBaseUI ui : mUIHolder.values()) {
      if (ui != null && name.equals(ui.getName())) {
        return ui;
      }
    }
    return null;
  }

  public LynxBaseUI findLynxUIByIdSelector(@NonNull String id) {
    for (LynxBaseUI ui : mUIHolder.values()) {
      if (ui != null && id.equals(ui.getIdSelector())) {
        return ui;
      }
    }
    return null;
  }

  public LynxBaseUI findLynxUIByA11yId(@NonNull String a11yIdStr) {
    if (a11yIdStr.isEmpty()) {
      return null;
    }
    LynxBaseUI ui;
    for (Integer index : mUIHolder.keySet()) {
      ui = mUIHolder.get(index);
      if (ui != null && a11yIdStr.equals(ui.getAccessibilityId())) {
        return ui;
      }
    }
    return null;
  }

  /**
   * @param componentId the sign of LynBaseUI
   * @param nodes       nodes compose of ancestorSelectorNames and selectorName
   * @param method      name of method
   * @param params      parameters used for executing method. Find LynxUI by RefId
   *                    when the variable named "_isCallByRefId" is true,
   *                    otherwise find LynxUI by id selector
   * @param callback    callback function after method execute
   */
  public void invokeUIMethod(String componentId, ReadableArray nodes, String method,
      ReadableMap params, Callback callback) {
    LynxBaseUI ui = findLynxUIByComponentId(componentId);
    String errorMsg = "component not found, nodes: " + nodes.toString() + ", method: " + method;

    if (ui != null) {
      for (int i = 0; i < nodes.size(); i++) {
        String name = nodes.getString(i);
        boolean isCalledByRef = params != null && params.size() > 0
            && params.hasKey("_isCallByRefId") && params.getBoolean("_isCallByRefId");
        if (!name.startsWith("#") && !isCalledByRef) {
          if (callback != null) {
            callback.invoke(LynxUIMethodConstants.SELECTOR_NOT_SUPPORTED,
                name + " not support, only support id selector currently");
          }
          return;
        }
        // It means that user invoke UI method with "ref" in ReactLynx when "_isCallByRefId" is
        // true
        String id = name.substring(1);
        ui = isCalledByRef ? findLynxUIByRefId(name, ui) : findLynxUIByIdSelector(id, ui);
        if (ui == null) {
          errorMsg = "not found " + name;
          break;
        }
        if (ui.getIdSelector() != null && ui.getIdSelector().equals(id)) {
          continue;
        }
      }
    }

    if (ui != null) {
      LynxUIMethodsExecutor.invokeMethod(ui, method, params, callback);
    } else if (callback != null) {
      callback.invoke(LynxUIMethodConstants.NODE_NOT_FOUND, errorMsg);
    }
  }

  /**
   * @param sign     the sign of LynxUI
   * @param method   name of method
   * @param params   parameters used for executing method
   * @param callback callback function after method execute
   */
  public void invokeUIMethodForSelectorQuery(
      int sign, String method, ReadableMap params, Callback callback) {
    LynxBaseUI ui = getNode(sign);
    if (ui != null) {
      String traceEvent = null;
      if (TraceEvent.enableTrace()) {
        traceEvent = ui.getTagName() + ".invokeUIMethodForSelectorQuery." + method;
        TraceEvent.beginSection(traceEvent);
      }
      LynxUIMethodsExecutor.invokeMethod(ui, method, params, callback);
      if (TraceEvent.enableTrace()) {
        TraceEvent.endSection(traceEvent);
      }
    } else if (callback != null) {
      callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE, "node does not have a LynxUI");
    }
  }

  public LynxBaseUI findLynxUIByIndex(@NonNull int index) {
    return mUIHolder.get(index);
  }

  public LynxBaseUI findLynxUIByIdSelectorSearchUp(String idSelector, LynxBaseUI ui) {
    if (ui == null) {
      LLog.e(TAG, "findLynxUIByIdSelectorSearchUp failed, the ui is null for " + idSelector + ".");
      return null;
    }

    if (ui != null && ui.getIdSelector() != null && ui.getIdSelector().equals(idSelector)) {
      return ui;
    }
    return findLynxUIByIdSelectorSearchUp(idSelector, ui.getParentBaseUI());
  }

  public LynxBaseUI findLynxUIByIdSelector(String idSelector, LynxBaseUI ui) {
    if (ui != null && ui.getIdSelector() != null && ui.getIdSelector().equals(idSelector)) {
      return ui;
    }
    if (ui == null) {
      return null;
    }
    for (LynxBaseUI child : ui.getChildren()) {
      if (child.getIdSelector() != null && child.getIdSelector().equals(idSelector)) {
        return child;
      }
      if (!(child instanceof UIComponent)) {
        child = findLynxUIByIdSelector(idSelector, child);
        if (child != null) {
          return child;
        }
      }
    }
    return null;
  }

  /**
   * Sets the state of a gesture detector for a specific gesture on a UI node.
   *
   * @param sign The unique identifier of the UI node.
   * @param gestureId The identifier of the specific gesture.
   * @param state The desired state to set for the gesture detector.
   */
  public void setGestureDetectorState(int sign, int gestureId, int state) {
    // Retrieve the UI node associated with the given identifier.
    LynxBaseUI ui = getNode(sign);

    // Check if the UI node exists.
    if (ui == null) {
      LLog.e(TAG, "Attempted to set gesture detector state for a non-existing node");
      return;
    }

    // Check if the UI node implements the new gesture delegate interface.
    if (ui instanceof LynxNewGestureDelegate) {
      // Cast the UI node to the new gesture delegate interface and set the gesture detector state.
      ((LynxNewGestureDelegate) ui).setGestureDetectorState(gestureId, state);
    }
  }

  /**
   * Handle whether internal lynxUI of the current gesture node consume the gesture and whether
   * native view outside the current node (outside of lynxView) consume the gesture.
   *
   * @param sign The unique identifier of the UI node.
   * @param gestureId The identifier of the specific native gesture.
   * @param params {internal: boolean, isConsume: boolean, ...}
   */
  public void consumeGesture(int sign, int gestureId, ReadableMap params) {
    // Retrieve the UI node associated with the given identifier.
    LynxBaseUI ui = getNode(sign);

    // Check if the UI node exists.
    if (ui == null) {
      LLog.e(TAG, "Attempted to set gesture detector state for a non-existing node");
      return;
    }

    if (ui instanceof LynxNewGestureDelegate) {
      ((LynxNewGestureDelegate) ui).consumeGesture(gestureId, params);
    }
  }

  public void validate(int tag) {
    LynxBaseUI ui = getNode(tag);
    if (ui == null) {
      LLog.e(TAG, "try to validate a not-existing node");
      return;
    }
    ui.renderIfNeeded();
  }

  @Deprecated
  // Due to historical reason, componentSet may be used in another thread. To promise thread safty,
  // we return an empty set in this function. And this function will be removed later.
  public Set<String> getComponentSet() {
    return new HashSet<>();
  }

  public void reportStatistic(String componentName) {
    // Avoid modifying and reporting componentset in the lower version of Android.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
      return;
    }
    // According to the client configuration "enable_component_statistic_report" whether to enable
    // reporting.
    if (LynxEnv.inst().enableComponentStatisticReport()) {
      componentStatistic(componentName);
    }
  }

  /**
   * Only reported when the component is first created.
   *
   * @param componentName the used component in LynxView
   */
  public void componentStatistic(String componentName) {
    if (!mComponentSet.contains(componentName)) {
      mComponentSet.add(componentName);
      // Report asynchronously to avoid affecting the main thread.
      int instanceId =
          mContext == null ? LynxEventReporter.INSTANCE_ID_UNKNOWN : mContext.getInstanceId();
      LynxEventReporter.onEvent(LYNXSDK_COMPONENT_STATISTIC_EVENT, instanceId, () -> {
        Map<String, Object> data = new HashMap<>();
        data.put("component_name", componentName);
        return data;
      });
    }
  }

  public LynxBaseUI createUI(String tag, boolean flatten) {
    LynxBaseUI ui = null;
    if (isUseNewSwiper()) {
      ui = createSwiperIfNeeded(tag, ui);
    }
    if (ui == null) {
      Behavior behavior = mBehaviorRegistry.get(tag);
      // Not create flatten UI if system accessibility is enabled.
      LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
      if (wrapper != null && wrapper.shouldCreateNoFlattenUI()) {
        flatten = false;
      }
      if (flatten && behavior.supportUIFlatten()) {
        ui = behavior.createFlattenUI(mContext);
      } else {
        ui = behavior.createUI(mContext);
      }
      if (ui == null) {
        ui = behavior.createUI(mContext);
      }
    }
    return ui;
  }

  void dump(StringBuilder output, LynxBaseUI element, int level) {
    for (int i = 0; i < level; i++) {
      output.append(" ");
    }
    output.append("id: ")
        .append(element.getSign())
        .append(", tag: ")
        .append(element.getTagName())
        .append(", rect: [")
        .append(element.getLeft())
        .append(", ")
        .append(element.getTop())
        .append(", ")
        .append(element.getWidth())
        .append(", ")
        .append(element.getHeight())
        .append("]");

    if (element.getLynxBackground() != null) {
      output.append(", bg: 0x")
          .append(Integer.toHexString(element.getLynxBackground().getBackgroundColor()));
    }
    output.append("\n");
  }

  void dumpTree(StringBuilder output, LynxBaseUI element, int level) {
    dump(output, element, level);
    for (int index = 0; index < element.getChildren().size(); index++) {
      dumpTree(output, element.getChildren().get(index), level + 1);
    }
  }

  void dumpDrawList(StringBuilder output, UIGroup owner, int level) {
    dump(output, owner, level);
    for (LynxBaseUI ui = owner.getDrawHead(); ui != null; ui = ui.getNextDrawUI()) {
      dump(output, ui, level);
    }
  }

  /**
   * @param refId ref ID defined in ReactLynx
   * @param ui    parent LynxBaseUI
   * @return target LynxBaseUI
   */
  public LynxBaseUI findLynxUIByRefId(String refId, LynxBaseUI ui) {
    if (ui != null && ui.getRefIdSelector() != null && ui.getRefIdSelector().equals(refId)) {
      return ui;
    }
    if (ui == null) {
      return null;
    }
    for (LynxBaseUI child : ui.getChildren()) {
      if (child.getRefIdSelector() != null && child.getRefIdSelector().equals(refId)) {
        return child;
      }
      if (!child.getTagName().equals("component")) {
        child = findLynxUIByRefId(refId, child);
        if (child != null) {
          return child;
        }
      }
    }
    return null;
  }

  public boolean getEnableCreateViewAsync() {
    PageConfig pageConfig = getPageConfig();
    if (pageConfig == null) {
      return false;
    }
    return pageConfig.getEnableCreateViewAsync();
  }

  public void setContextFree(boolean isContextFree) {
    mIsContextFree = isContextFree;
  }

  public boolean isContextFree() {
    return mIsContextFree;
  }

  private boolean isUseNewSwiper() {
    PageConfig pageConfig = getPageConfig();
    if (pageConfig == null) {
      return false;
    }
    return pageConfig.isUseNewSwiper();
  }

  private LynxBaseUI createSwiperIfNeeded(String tagName, LynxBaseUI origin) {
    if ("swiper".equals(tagName) || "x-swiper".equals(tagName)) {
      return new XSwiperUI(mContext);
    }
    return origin;
  }

  private void updateComponentIdToUiIdMapIfNeeded(
      int sign, String tagName, @Nullable StylesDiffMap initialProps) {
    if (tagName.equals("component") && initialProps.hasKey("ComponentID")) {
      String componentIDValue = initialProps.getString("ComponentID");
      if (componentIDValue == null) {
        componentIDValue = LynxConstants.LYNX_DEFAULT_COMPONENT_ID;
      }
      mComponentIdToUiIdHolder.put(componentIDValue, sign);
    }
  }

  final private static Comparator<LynxBaseUI> translationZComparator =
      new Comparator<LynxBaseUI>() {
        @Override
        public int compare(LynxBaseUI o1, LynxBaseUI o2) {
          if (o1.getTranslationZ() > o2.getTranslationZ()) {
            return 1;
          } else if (o1.getTranslationZ() == o2.getTranslationZ()) {
            return 0;
          }
          return -1;
        }
      };

  // Sort the UI order in UI tree and change the drawing order by removing children and re-insert
  // into the drawList and View tree after sorting.
  private void sortTranslateZChildren(LynxBaseUI parent) {
    for (LynxBaseUI child : parent.getChildren()) {
      removeFromDrawList(child);
    }
    try {
      Collections.sort(parent.getChildren(), translationZComparator);
    } catch (Exception e) {
      // FIXME(renzhongyue): Android bellow 6.0, have exception cannot cast String to LynxBaseUI.
      LLog.i(
          TAG, "Something went wrong during sort children by translation Z " + e.getStackTrace());
    }
    insertChildIntoDrawListRecursive(parent);
  }

  private void handleTranslateZUI() {
    if (!mContext.getEnableFlattenTranslateZ()) {
      return;
    }
    for (LynxBaseUI parent : mTranslateZParentHolder) {
      if (parent.flattenChildrenCount() > 0 && parent.getNeedSortChildren()) {
        sortTranslateZChildren(parent);
        parent.setNeedSortChildren(false);
      }
    }
  }

  public boolean behaviorSupportCreateAsync(String tagName) {
    if (tagName.equals(LynxConstants.ROOT_TAG_NAME)) {
      return false;
    }
    Behavior behavior = mBehaviorRegistry.get(tagName);
    if (behavior != null) {
      return behavior.supportCreateAsync();
    } else {
      return false;
    }
  }

  private void insertA11yMutationEvent(final int action, final LynxBaseUI ui) {
    LynxAccessibilityWrapper wrapper = mUIBody.getLynxAccessibilityWrapper();
    if (wrapper != null) {
      wrapper.insertA11yMutationEvent(action, ui);
    }
  }

  public void registerBoundingClientRectUI(LynxBaseUI ui) {
    mCachedBoundingClientRectUI.add(ui);
  }

  /**
   * when lynxview is enter foreground
   */
  public void onEnterForeground() {
    if (mForegroundListeners == null) {
      return;
    }
    for (ForegroundListener listener : mForegroundListeners) {
      listener.onLynxViewEnterForeground();
    }
  }

  /**
   * when lynxview is enter background
   */
  public void onEnterBackground() {
    if (mForegroundListeners == null) {
      return;
    }
    for (ForegroundListener listener : mForegroundListeners) {
      listener.onLynxViewEnterBackground();
    }
  }

  /**
   * register listener for LynxUI which implement `ForegroundListener` interface
   * @param listener triggered when lynxview enter/exit foreground
   */
  void registerForegroundListener(@NonNull ForegroundListener listener) {
    if (mForegroundListeners != null) {
      mForegroundListeners.add(listener);
    }
  }

  /**
   * unregister listener for LynxUI which implement `ForegroundListener` interface
   * @param listener triggered when lynxview enter/exit foreground
   */
  public void unregisterForegroundListener(@NonNull ForegroundListener listener) {
    if (mForegroundListeners != null) {
      mForegroundListeners.remove(listener);
    }
  }

  // TODO(linxiaosong): only return common, custom and virtual info now. More info may be needed in
  // the future.
  // TODO(chennengshi): After the Decouple Layout is completed, unify this function and the similar
  // functions in ShadowNodeOwner.
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

  public int getRootSign() {
    return mRootSign;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void reportCreateAsyncSuccessEvent(
      int sign, String tagName, boolean isSuccess, int status) {
    if (mEnableReportCreateAsync) {
      LynxBaseUI ui = getNode(sign);
      String uiName = null;
      if (ui != null) {
        uiName = ui.getClass().getSimpleName();
      }
      String finalUiName = uiName;
      LynxEventReporter.onEvent(
          LYNXSDK_ASYNC_CREATE_SUCCESS_EVENT, getContext().getInstanceId(), () -> {
            Map<String, Object> props = new HashMap<>();
            props.put("tag_name", tagName);
            props.put("class_name", finalUiName);
            props.put("success", isSuccess);
            props.put("status", status);
            return props;
          });
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void reportCreateViewConfig(int sign, String tagName, boolean createAsync) {
    if (TextUtils.equals(tagName, LynxConstants.ROOT_TAG_NAME)) {
      // page node will not be created asynchronously, just block it.
      return;
    }
    // every ui type will only be reported once.
    if (mEnableReportCreateAsync && !mCreateNodeConfigHasReportedMark.containsKey(tagName)) {
      mCreateNodeConfigHasReportedMark.put(tagName, createAsync);
      LynxBaseUI ui = getNode(sign);
      String uiName = null;
      if (ui != null) {
        uiName = ui.getClass().getSimpleName();
      }
      String finalUiName = uiName;
      LynxEventReporter.onEvent(LYNXSDK_ASYNC_CREATE_CONFIG, getContext().getInstanceId(), () -> {
        Map<String, Object> props = new HashMap<>();
        props.put("tag_name", tagName);
        props.put("class_name", finalUiName);
        props.put("enable_async", createAsync);
        return props;
      });
    }
  }

  private PageConfig getPageConfig() {
    NativeFacade nativeFacade = (mNativeFacade != null) ? mNativeFacade.get() : null;
    return (nativeFacade != null) ? nativeFacade.getPageConfig() : null;
  }
}
