// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.app.Activity;
import android.graphics.Rect;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.View;
import android.view.ViewParent;
import android.view.ViewTreeObserver;
import android.view.WindowInsets;
import android.view.WindowInsetsAnimation;
import android.view.animation.PathInterpolator;
import androidx.appcompat.widget.LinearLayoutCompat;
import androidx.recyclerview.widget.OrientationHelper;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.event.LynxKeyboardEvent;
import com.lynx.tasm.utils.ContextUtils;
import com.lynx.tasm.utils.LynxConstants;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.WeakHashMap;

/**
 * Each LynxContext or LynxView would own a KeyboardEvent once it enable that.
 */
public class KeyboardEvent {
  public interface KeyboardEventObserver {
    void keyboardWillShow(int keyboardHeight);
    void keyboardWillHide();
  }
  private LynxContext mLynxContext;
  private ViewTreeObserver.OnGlobalLayoutListener mListener = null;
  private float mDpi;
  private boolean isStartedInUIThread = false;
  private KeyboardMonitor mKeyboardMonitor;
  private int mViewHeight = 0;
  @Deprecated private int mOldViewHeight = 0;
  private Rect mDisplayFrame;
  private static final double KEYBOARD_LOWER_THRESHOLD = 0.4;
  private static final double KEYBOARD_HIGHER_THRESHOLD = 0.9;
  private int keyboardHeightForLast = 0;
  private int keyboardTopFromLynxView = 0;
  private WeakHashMap<Object, ViewTreeObserver.OnGlobalLayoutListener> mOnGlobalLayoutListenerList =
      new WeakHashMap<>();

  private WeakHashMap<Object, KeyboardEventObserver> mObservers = new WeakHashMap<>();
  private static final WeakHashMap<View, KeyboardInsetsAnimationDispatcher>
      sInsetsAnimationDispatchers = new WeakHashMap<>();
  private final KeyboardAvoidingContext mKeyboardAvoidingContext = new KeyboardAvoidingContext();

  public KeyboardEvent(LynxContext lynxContext) {
    LLog.d(LynxConstants.TAG, "KeyboardEvent initialized.");

    mLynxContext = lynxContext;
    mDpi = lynxContext.getScreenMetrics().density;
    mDisplayFrame = new Rect();
  }

  public synchronized void start() {
    if (isStartedInUIThread) {
      LLog.d(LynxConstants.TAG, "KeyboardEvent already started");
      return;
    }

    /*
     ** Need to post to UI thread to avoid multi-thread issue
     * since Template Renderer may run in async mode and setSoftInputMode
     * should be run in main thread
     */
    if (!UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          startInMain();
        }
      });
    } else {
      startInMain();
    }
  }

  public boolean isStart() {
    return isStartedInUIThread;
  }

  /*
   * @return {null} by default
   */
  public KeyboardMonitor getKeyboardMonitor() {
    return mKeyboardMonitor;
  }

  public void addOnGlobalLayoutListener(
      Object key, ViewTreeObserver.OnGlobalLayoutListener listener) {
    mOnGlobalLayoutListenerList.put(key, listener);
    start();
  }

  public void removeOnGlobalLayoutListener(
      Object key, ViewTreeObserver.OnGlobalLayoutListener listener) {
    try {
      if (listener != null && mKeyboardMonitor != null) {
        mOnGlobalLayoutListenerList.remove(key);
      }
    } catch (Exception e) {
    }
  }

  public ViewTreeObserver.OnGlobalLayoutListener getListener(Object key) {
    return mOnGlobalLayoutListenerList.get(key);
  }

  public void detectKeyboardChangeAndSendEvent() {
    try {
      Activity activity = ContextUtils.getActivity(mLynxContext);
      if (activity == null) {
        LLog.e(LynxConstants.TAG, "KeyboardEvent's context must be Activity.");
        return;
      }
      LynxContext lynxContext = mLynxContext;
      final View decorView = activity.getWindow().getDecorView();
      final boolean useRelativeKeyboardHeightApi = lynxContext.useRelativeKeyboardHeightApi();
      if (lynxContext.getUIBody() == null) {
        return;
      }
      final WeakReference<UIBody.UIBodyView> bodyView =
          new WeakReference<>(lynxContext.getUIBody().getBodyView());

      mKeyboardMonitor.getDecorView().getWindowVisibleDisplayFrame(mDisplayFrame);
      // get height of visible screen part
      int keyboardTop = mKeyboardMonitor.getDecorView().getHeight() + mDisplayFrame.top;
      int displayHeight = mKeyboardMonitor.getDecorView().getHeight();
      // get total screen height
      if (mOldViewHeight == 0) {
        // this old implementation is incorrect and takes status bar height into account,
        // so we have to keep backward compatible...
        mOldViewHeight = decorView.getHeight();
      }
      mViewHeight = mKeyboardMonitor.getDefaultMonitorBottom() - mDisplayFrame.top;
      int rotation = activity.getWindow().getWindowManager().getDefaultDisplay().getRotation();
      int height = mOldViewHeight;
      int viewHeight = mViewHeight;
      // get the height of keyboard
      double visibleRatio = (double) displayHeight / viewHeight;
      if ((rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180)
          && visibleRatio < KEYBOARD_LOWER_THRESHOLD) {
        // cover bad cases in Android 11
        // the mKeyboardMonitor.decorView may own a very small height value,
        // we should ignore this LayoutRequest.
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            mKeyboardMonitor.getDecorView().requestLayout();
          }
        });
        return;
      }
      // TODO(zhangkaijie.9): use getRootWindowInsets().isVisible(WindowInsetsCompat.Type.ime())
      boolean visible = visibleRatio < KEYBOARD_HIGHER_THRESHOLD;
      int keyboardHeight = 0;
      int keyboardHeightCompat = 0;
      UIBody.UIBodyView bodyInst = bodyView.get();
      if (visible) {
        keyboardHeight = (int) ((height - displayHeight) / mDpi);
      }

      if (useRelativeKeyboardHeightApi && bodyInst != null) {
        int[] bodyLocation = new int[2];
        bodyInst.getLocationOnScreen(bodyLocation);
        int bodyY = bodyLocation[1];
        int bodyHeight = bodyInst.getHeight();
        keyboardHeightCompat = (int) ((bodyY + bodyHeight - keyboardTop) / mDpi);
      } else {
        if (visible) {
          keyboardHeightCompat = (int) ((viewHeight - displayHeight) / mDpi);
        }
      }

      LLog.d(LynxConstants.TAG,
          "KeyboardEvent visible = " + visible + ", height = " + keyboardHeight
              + ", compatHeight = " + keyboardHeightCompat);

      if ((keyboardHeight != keyboardHeightForLast)
          || (useRelativeKeyboardHeightApi && (keyboardHeightCompat != keyboardTopFromLynxView))) {
        sendKeyboardEvent(visible, keyboardHeight, keyboardHeightCompat);
        keyboardHeightForLast = keyboardHeight;
        keyboardTopFromLynxView = keyboardHeightCompat;
      }

      dispatchOnGlobalLayout();
    } catch (Exception e) {
      LLog.e(LynxConstants.TAG, e.getMessage());
    }
  }

  private void startInMain() {
    LLog.d(LynxConstants.TAG, "KeyboardEvent starting");
    Activity activity = ContextUtils.getActivity(mLynxContext);
    if (activity == null) {
      LLog.e(LynxConstants.TAG, "KeyboardEvent's context must be Activity");
      return;
    }
    if (mKeyboardMonitor == null) {
      mKeyboardMonitor = new KeyboardMonitor(activity);
    }
    mKeyboardAvoidingContext.start(activity.getWindow().getDecorView());

    mListener = new ViewTreeObserver.OnGlobalLayoutListener() {
      @Override
      public void onGlobalLayout() {
        LLog.d(LynxConstants.TAG, "onGlobalLayout invoked.");
        detectKeyboardChangeAndSendEvent();
      }
    };

    mKeyboardMonitor.addOnGlobalLayoutListener(mListener);
    mKeyboardMonitor.start();
    isStartedInUIThread = true;
  }

  public synchronized void stop() {
    if (!isStartedInUIThread) {
      return;
    }
    /*
     ** Need to post to UI thread to avoid multi-thread issue
     * since Template Renderer may run in async mode and setSoftInputMode
     * should be run in main thread
     */
    if (!UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          stopInMain();
        }
      });
    } else {
      stopInMain();
    }
  }

  private void stopInMain() {
    LLog.d(LynxConstants.TAG, "KeyboardEvent stopping");
    try {
      if (mListener != null && mKeyboardMonitor != null) {
        mKeyboardMonitor.removeOnGlobalLayoutListener(mListener);
        mKeyboardMonitor.stop();
      }
      mKeyboardAvoidingContext.stop();
    } catch (Exception e) {
      LLog.w(LynxConstants.TAG, "stop KeyboardEvent failed for " + e.toString());
    }
    isStartedInUIThread = false;
  }

  private void sendKeyboardEvent(boolean isVisible, int height, int heightCompat) {
    // for keeping backward compatible, we send 2 kinds of height here.
    // height is the older implementation, base view height below the status bar
    // heightCompat is the newer one, base view height below the status bar and above the virtual
    // buttons
    if (mLynxContext.getEventEmitter() != null) {
      JavaOnlyArray args = new JavaOnlyArray();
      args.pushString(isVisible ? "on" : "off");
      args.pushInt(heightCompat);
      args.pushInt(heightCompat);
      mLynxContext.sendGlobalEvent(LynxKeyboardEvent.KEYBOARD_STATUS_CHANGED, args);
    }
    for (Map.Entry<Object, KeyboardEventObserver> entry : mObservers.entrySet()) {
      KeyboardEventObserver observer = entry.getValue();
      if (isVisible) {
        observer.keyboardWillShow((int) (heightCompat * mDpi));
      } else {
        observer.keyboardWillHide();
      }
    }
  }

  public Rect getDisplayFrame() {
    return mDisplayFrame;
  }

  public int getEventViewHeight() {
    return mViewHeight;
  }

  private void dispatchOnGlobalLayout() {
    for (Map.Entry<Object, ViewTreeObserver.OnGlobalLayoutListener> entry :
        mOnGlobalLayoutListenerList.entrySet()) {
      Object key = entry.getKey();
      if (key != null) {
        ViewTreeObserver.OnGlobalLayoutListener listener = entry.getValue();
        listener.onGlobalLayout();
      }
    }
  }

  public void addKeyboardEventObserver(KeyboardEventObserver observer) {
    if (observer != null) {
      mObservers.put(observer, observer);
    }
  }

  public void avoidKeyboardPropsDidChangeForOwner(
      Object owner, View inputView, View lynxView, boolean avoidKeyboard, float spacing) {
    mKeyboardAvoidingContext.avoidKeyboardPropsDidChangeForOwner(
        owner, inputView, lynxView, avoidKeyboard, spacing);
  }

  public void inputDidBeginEditingForOwner(
      Object owner, View inputView, View lynxView, boolean avoidKeyboard, float spacing) {
    mKeyboardAvoidingContext.inputDidBeginEditing(
        owner, inputView, lynxView, avoidKeyboard, spacing);
  }

  public void inputDidEndEditingForOwner(Object owner) {
    mKeyboardAvoidingContext.inputDidEndEditing(owner);
  }

  public void inputDidLayoutForOwner(
      Object owner, View inputView, View lynxView, boolean avoidKeyboard, float spacing) {
    mKeyboardAvoidingContext.inputDidLayout(owner, inputView, lynxView, avoidKeyboard, spacing);
  }

  public void keyboardWillShowForOwner(Object owner, View inputView, View lynxView,
      boolean avoidKeyboard, float spacing, int keyboardHeight) {
    mKeyboardAvoidingContext.keyboardWillShow(
        owner, inputView, lynxView, avoidKeyboard, spacing, keyboardHeight);
  }

  public void keyboardWillHideForOwner(Object owner, Runnable finalHideBlock) {
    mKeyboardAvoidingContext.keyboardWillHide(owner, finalHideBlock);
  }

  private class KeyboardAvoidingContext {
    private static final long KEYBOARD_ANIMATION_DURATION_MS = 285L;
    private static final long INSETS_ANIMATION_START_FALLBACK_DELAY_MS = 64L;
    private static final long INSETS_ANIMATION_END_FALLBACK_PADDING_MS = 80L;
    private static final int KEYBOARD_TRANSITION_NONE = 0;
    private static final int KEYBOARD_TRANSITION_SHOW = 1;
    private static final int KEYBOARD_TRANSITION_HIDE = 2;

    private final WeakHashMap<View, KeyboardAvoidTarget> mTargets = new WeakHashMap<>();
    private WeakReference<KeyboardAvoidTarget> mActiveTarget =
        new WeakReference<KeyboardAvoidTarget>(null);
    private WeakReference<Object> mLastEventOwner = new WeakReference<Object>(null);
    private WeakReference<View> mLastLynxView = new WeakReference<View>(null);
    private View mFocusRootView;
    private View mInsetsAnimationHostView;
    private KeyboardInsetsAnimationDispatcher mInsetsAnimationDispatcher;
    private ViewTreeObserver.OnGlobalFocusChangeListener mFocusChangeListener;
    private int mKeyboardHeight = 0;
    private float mCurrentAvoidDistance = 0f;
    private boolean mIsImeAnimationRunning = false;
    private int mKeyboardHeightBeforeImeAnimation = 0;
    private float mAvoidDistanceBeforeImeAnimation = 0f;
    private int mPendingKeyboardTransition = KEYBOARD_TRANSITION_NONE;
    private int mPendingKeyboardHeight = 0;
    private WeakReference<KeyboardAvoidTarget> mPendingKeyboardTarget =
        new WeakReference<KeyboardAvoidTarget>(null);
    private Runnable mPendingKeyboardFallbackRunnable;
    private View mPendingKeyboardFallbackView;

    void start(View focusRootView) {
      if (focusRootView == null) {
        return;
      }
      if (mFocusRootView == focusRootView && mFocusChangeListener != null) {
        return;
      }
      stopFocusTracking();
      mFocusRootView = focusRootView;
      mFocusChangeListener = new ViewTreeObserver.OnGlobalFocusChangeListener() {
        @Override
        public void onGlobalFocusChanged(View oldFocus, View newFocus) {
          handleGlobalFocusChanged(oldFocus, newFocus);
        }
      };
      focusRootView.getViewTreeObserver().addOnGlobalFocusChangeListener(mFocusChangeListener);
    }

    void stop() {
      cancelPendingKeyboardTransitionFallback();
      clearPendingKeyboardTransition();
      resetAvoidDistance();
      stopFocusTracking();
      stopInsetsAnimationTracking();
      mTargets.clear();
      mActiveTarget = new WeakReference<KeyboardAvoidTarget>(null);
      mLastEventOwner = new WeakReference<Object>(null);
      mLastLynxView = new WeakReference<View>(null);
      mKeyboardHeight = 0;
    }

    void avoidKeyboardPropsDidChangeForOwner(
        Object owner, View inputView, View lynxView, boolean avoidKeyboard, float spacing) {
      KeyboardAvoidTarget target = saveTarget(owner, inputView, lynxView, avoidKeyboard, spacing);
      if (target == null) {
        return;
      }
      KeyboardAvoidTarget activeTarget = getActiveTarget();
      if (activeTarget != null && activeTarget.matchesOwner(owner)) {
        setActiveTarget(target);
        updateAvoidDistance();
      } else if (inputView.isFocused()) {
        activateTarget(target);
      }
    }

    void inputDidBeginEditing(
        Object owner, View inputView, View lynxView, boolean avoidKeyboard, float spacing) {
      KeyboardAvoidTarget target = saveTarget(owner, inputView, lynxView, avoidKeyboard, spacing);
      if (target != null) {
        activateTarget(target);
      }
    }

    void inputDidEndEditing(Object owner) {
      KeyboardAvoidTarget activeTarget = getActiveTarget();
      if (activeTarget != null && activeTarget.matchesOwner(owner)) {
        setActiveTarget(null);
        setLastEventOwner(owner);
        if (mKeyboardHeight <= 0) {
          resetAvoidDistance();
        }
      } else {
        setLastEventOwner(owner);
      }
    }

    void inputDidLayout(
        Object owner, View inputView, View lynxView, boolean avoidKeyboard, float spacing) {
      KeyboardAvoidTarget target = saveTarget(owner, inputView, lynxView, avoidKeyboard, spacing);
      KeyboardAvoidTarget activeTarget = getActiveTarget();
      if (target != null && activeTarget != null && activeTarget.matchesOwner(owner)) {
        setActiveTarget(target);
        updateAvoidDistance();
      }
    }

    void keyboardWillShow(Object owner, View inputView, View lynxView, boolean avoidKeyboard,
        float spacing, int keyboardHeight) {
      KeyboardAvoidTarget target = saveTarget(owner, inputView, lynxView, avoidKeyboard, spacing);
      if (target == null) {
        return;
      }
      KeyboardAvoidTarget activeTarget = getActiveTarget();
      if (inputView.isFocused() || (activeTarget != null && activeTarget.matchesOwner(owner))) {
        if (shouldUseInsetsAnimationForKeyboardTransition()) {
          setActiveTarget(target);
          setLastEventOwner(target.getOwner());
          prepareInsetsKeyboardTransition(KEYBOARD_TRANSITION_SHOW, keyboardHeight, target);
        } else {
          mKeyboardHeight = keyboardHeight;
          activateTarget(target);
        }
      }
    }

    void keyboardWillHide(Object owner, Runnable finalHideBlock) {
      if (!isActiveOwner(owner) && !isLastEventOwner(owner)) {
        return;
      }
      KeyboardAvoidTarget focusedTarget = findCurrentFocusedTarget();
      if (shouldUseInsetsAnimationForKeyboardTransition()) {
        setActiveTarget(focusedTarget);
        setLastEventOwner(owner);
        prepareInsetsKeyboardTransition(KEYBOARD_TRANSITION_HIDE, 0, focusedTarget);
        if (finalHideBlock != null) {
          finalHideBlock.run();
        }
        return;
      }
      mKeyboardHeight = 0;
      if (focusedTarget != null) {
        activateTarget(focusedTarget);
      } else {
        setActiveTarget(null);
        resetAvoidDistance();
      }
      setLastEventOwner(owner);
      if (finalHideBlock != null) {
        finalHideBlock.run();
      }
    }

    private void handleGlobalFocusChanged(View oldFocus, View newFocus) {
      KeyboardAvoidTarget oldTarget = findTarget(oldFocus);
      KeyboardAvoidTarget newTarget = findTarget(newFocus);
      if (oldTarget != null) {
        setLastEventOwner(oldTarget.getOwner());
      }
      if (newTarget != null) {
        activateTarget(newTarget);
      } else if (oldTarget != null) {
        inputDidEndEditing(oldTarget.getOwner());
      }
    }

    private KeyboardAvoidTarget saveTarget(
        Object owner, View inputView, View lynxView, boolean avoidKeyboard, float spacing) {
      if (owner == null || inputView == null || lynxView == null) {
        return null;
      }
      KeyboardAvoidTarget target =
          new KeyboardAvoidTarget(owner, inputView, lynxView, avoidKeyboard, spacing);
      mTargets.put(inputView, target);
      startInsetsAnimationTracking();
      return target;
    }

    private void activateTarget(KeyboardAvoidTarget target) {
      if (target == null || !target.isValid()) {
        return;
      }
      setActiveTarget(target);
      setLastEventOwner(target.getOwner());
      if (mPendingKeyboardTransition == KEYBOARD_TRANSITION_SHOW) {
        mPendingKeyboardTarget = new WeakReference<KeyboardAvoidTarget>(target);
      }
      updateAvoidDistance();
    }

    private void updateAvoidDistance() {
      updateAvoidDistance(shouldAnimateAvoidDistance());
    }

    private void updateAvoidDistance(boolean animated) {
      KeyboardAvoidTarget target = getActiveTarget();
      if (target == null || !target.isValid()) {
        setActiveTarget(null);
        resetAvoidDistance(animated);
        return;
      }
      View lynxView = target.getLynxView();
      float avoidDistance = calculateAvoidDistance(target, mKeyboardHeight);
      applyAvoidDistance(lynxView, avoidDistance, animated);
    }

    private float calculateAvoidDistance(KeyboardAvoidTarget target, int keyboardHeight) {
      return calculateAvoidDistance(target, keyboardHeight, null);
    }

    private float calculateAvoidDistance(
        KeyboardAvoidTarget target, int keyboardHeight, WindowInsets insets) {
      if (!target.shouldAvoidKeyboard()) {
        return 0f;
      }
      View inputView = target.getInputView();
      View lynxView = target.getLynxView();
      Activity activity = ContextUtils.getActivity(mLynxContext);
      if (inputView == null || lynxView == null || activity == null) {
        return 0f;
      }
      View decorView = activity.getWindow().getDecorView();
      if (decorView == null || decorView.getHeight() <= 0) {
        return 0f;
      }
      boolean preferVisibleFrame =
          Build.VERSION.SDK_INT >= Build.VERSION_CODES.N && activity.isInMultiWindowMode();
      float hostAvoidDistance = calculateAvoidDistanceInHost(
          target, keyboardHeight, decorView, insets, preferVisibleFrame);
      if (hostAvoidDistance >= 0f) {
        return hostAvoidDistance;
      }
      if (keyboardHeight <= 0) {
        return 0f;
      }
      int screenBottom = getKeyboardAvoidingScreenBottom(decorView);

      int[] inputLocation = new int[2];
      inputView.getLocationOnScreen(inputLocation);
      float inputBottomBeforeAvoid =
          inputLocation[1] + inputView.getHeight() - lynxView.getTranslationY();
      float inputBottomToScreen = screenBottom - inputBottomBeforeAvoid;
      return Math.max(0f, keyboardHeight - inputBottomToScreen + target.getSpacing());
    }

    private float calculateAvoidDistanceInHost(KeyboardAvoidTarget target, int keyboardHeight,
        View hostView, WindowInsets insets, boolean preferVisibleFrame) {
      View inputView = target.getInputView();
      View lynxView = target.getLynxView();
      if (inputView == null || lynxView == null || hostView == null || hostView.getHeight() <= 0) {
        return -1f;
      }
      int availableBottomInHost =
          getKeyboardAvailableBottomInHost(hostView, keyboardHeight, insets, preferVisibleFrame);
      if (availableBottomInHost < 0) {
        return -1f;
      }
      int[] inputLocation = new int[2];
      int[] hostLocation = new int[2];
      inputView.getLocationOnScreen(inputLocation);
      hostView.getLocationOnScreen(hostLocation);
      float inputBottomInHost =
          inputLocation[1] - hostLocation[1] + inputView.getHeight() - lynxView.getTranslationY();
      return Math.max(0f, inputBottomInHost - availableBottomInHost + target.getSpacing());
    }

    private int getKeyboardAvailableBottomInHost(
        View hostView, int keyboardHeight, WindowInsets insets, boolean preferVisibleFrame) {
      if (hostView == null || hostView.getHeight() <= 0) {
        return -1;
      }
      int hostHeight = hostView.getHeight();
      int[] hostLocation = new int[2];
      hostView.getLocationOnScreen(hostLocation);
      boolean imeVisible = false;
      int imeAvailableBottomInHost = -1;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        WindowInsets currentInsets = insets;
        if (currentInsets == null) {
          currentInsets = hostView.getRootWindowInsets();
        }
        if (currentInsets != null) {
          int imeBottom = currentInsets.getInsets(WindowInsets.Type.ime()).bottom;
          imeVisible = imeBottom > 0 || currentInsets.isVisible(WindowInsets.Type.ime());
          if (imeVisible) {
            imeAvailableBottomInHost = Math.max(0, Math.min(hostHeight - imeBottom, hostHeight));
          }
        }
      }
      Rect visibleFrame = new Rect();
      hostView.getWindowVisibleDisplayFrame(visibleFrame);
      int visibleFrameAvailableBottomInHost = -1;
      Rect hostFrame = new Rect(hostLocation[0], hostLocation[1],
          hostLocation[0] + hostView.getWidth(), hostLocation[1] + hostHeight);
      if (visibleFrame.intersect(hostFrame)) {
        int visibleBottomInHost = visibleFrame.bottom - hostLocation[1];
        int visibleOcclusion = hostHeight - visibleBottomInHost;
        if (visibleOcclusion > 0
            && (imeVisible || (keyboardHeight > 0 && visibleOcclusion >= keyboardHeight * 0.5f))) {
          visibleFrameAvailableBottomInHost = visibleBottomInHost;
        }
      }
      if (preferVisibleFrame && visibleFrameAvailableBottomInHost >= 0) {
        return visibleFrameAvailableBottomInHost;
      }
      if (imeAvailableBottomInHost >= 0) {
        return imeAvailableBottomInHost;
      }
      if (visibleFrameAvailableBottomInHost >= 0) {
        return visibleFrameAvailableBottomInHost;
      }
      if (keyboardHeight <= 0) {
        return -1;
      }
      return -1;
    }

    private int getKeyboardAvoidingScreenBottom(View decorView) {
      if (mKeyboardMonitor != null) {
        return mKeyboardMonitor.getDefaultMonitorBottom();
      }
      int[] decorLocation = new int[2];
      decorView.getLocationOnScreen(decorLocation);
      return decorLocation[1] + decorView.getHeight();
    }

    private void applyAvoidDistance(View lynxView, float avoidDistance) {
      applyAvoidDistance(lynxView, avoidDistance, shouldAnimateAvoidDistance());
    }

    private void applyAvoidDistance(View lynxView, float avoidDistance, boolean animated) {
      float targetAvoidDistance = Math.max(0f, avoidDistance);
      if (lynxView == null) {
        mCurrentAvoidDistance = targetAvoidDistance;
        return;
      }
      mLastLynxView = new WeakReference<View>(lynxView);
      mCurrentAvoidDistance = targetAvoidDistance;
      lynxView.animate().cancel();
      if (animated) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
          lynxView.animate()
              .translationY(-targetAvoidDistance)
              .setDuration(KEYBOARD_ANIMATION_DURATION_MS)
              .setInterpolator(new PathInterpolator(0.2f, 0f, 0f, 1f))
              .start();
        } else {
          lynxView.animate()
              .translationY(-targetAvoidDistance)
              .setDuration(KEYBOARD_ANIMATION_DURATION_MS)
              .start();
        }
      } else {
        lynxView.setTranslationY(-targetAvoidDistance);
      }
    }

    private void resetAvoidDistance() {
      resetAvoidDistance(shouldAnimateAvoidDistance());
    }

    private void resetAvoidDistance(boolean animated) {
      View lynxView = null;
      KeyboardAvoidTarget activeTarget = getActiveTarget();
      if (activeTarget != null) {
        lynxView = activeTarget.getLynxView();
      }
      if (lynxView == null) {
        lynxView = mLastLynxView.get();
      }
      if (lynxView != null || mCurrentAvoidDistance != 0f) {
        applyAvoidDistance(lynxView, 0f, animated);
      }
    }

    private KeyboardAvoidTarget getActiveTarget() {
      KeyboardAvoidTarget target = mActiveTarget.get();
      return target != null && target.isValid() ? target : null;
    }

    private void setActiveTarget(KeyboardAvoidTarget target) {
      mActiveTarget = new WeakReference<KeyboardAvoidTarget>(target);
    }

    private void setLastEventOwner(Object owner) {
      if (owner != null) {
        mLastEventOwner = new WeakReference<Object>(owner);
      }
    }

    private boolean isActiveOwner(Object owner) {
      KeyboardAvoidTarget activeTarget = getActiveTarget();
      return activeTarget != null && activeTarget.matchesOwner(owner);
    }

    private boolean isLastEventOwner(Object owner) {
      Object lastOwner = mLastEventOwner.get();
      return owner != null && lastOwner == owner;
    }

    private KeyboardAvoidTarget findCurrentFocusedTarget() {
      if (mFocusRootView == null) {
        return null;
      }
      return findTarget(mFocusRootView.findFocus());
    }

    private KeyboardAvoidTarget findTarget(View view) {
      View current = view;
      while (current != null) {
        KeyboardAvoidTarget target = mTargets.get(current);
        if (target != null && target.isValid()) {
          return target;
        }
        ViewParent parent = current.getParent();
        if (parent instanceof View) {
          current = (View) parent;
        } else {
          current = null;
        }
      }
      return null;
    }

    private void stopFocusTracking() {
      if (mFocusRootView != null && mFocusChangeListener != null) {
        try {
          ViewTreeObserver observer = mFocusRootView.getViewTreeObserver();
          if (observer != null && observer.isAlive()) {
            observer.removeOnGlobalFocusChangeListener(mFocusChangeListener);
          }
        } catch (Exception e) {
          LLog.w(LynxConstants.TAG, "stop KeyboardAvoidingContext failed for " + e.toString());
        }
      }
      mFocusRootView = null;
      mFocusChangeListener = null;
    }

    private void startInsetsAnimationTracking() {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
        return;
      }
      View hostView = resolveInsetsAnimationHostView();
      if (hostView == null) {
        return;
      }
      if (mInsetsAnimationHostView == hostView && mInsetsAnimationDispatcher != null) {
        return;
      }
      stopInsetsAnimationTracking();
      mInsetsAnimationHostView = hostView;
      mInsetsAnimationDispatcher = KeyboardInsetsAnimationDispatcher.register(hostView, this);
    }

    private void stopInsetsAnimationTracking() {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && mInsetsAnimationDispatcher != null) {
        mInsetsAnimationDispatcher.unregister(this);
      }
      mInsetsAnimationHostView = null;
      mInsetsAnimationDispatcher = null;
      endImeAnimation();
    }

    private boolean shouldAnimateAvoidDistance() {
      return !mIsImeAnimationRunning && !hasPendingInsetsKeyboardTransition();
    }

    private boolean shouldUseInsetsAnimationForKeyboardTransition() {
      return Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && mInsetsAnimationDispatcher != null;
    }

    private boolean hasPendingInsetsKeyboardTransition() {
      return shouldUseInsetsAnimationForKeyboardTransition()
          && mPendingKeyboardTransition != KEYBOARD_TRANSITION_NONE;
    }

    private View resolveInsetsAnimationHostView() {
      if (mFocusRootView != null) {
        return mFocusRootView;
      }
      Activity activity = ContextUtils.getActivity(mLynxContext);
      if (activity == null || activity.getWindow() == null) {
        return null;
      }
      return activity.getWindow().getDecorView();
    }

    private boolean applyImeInsetsAvoidDistance(WindowInsets insets) {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || insets == null) {
        return false;
      }
      int keyboardHeight = getKeyboardHeightFromInsets(insets);
      KeyboardAvoidTarget target = getActiveTarget();
      if (target == null || !target.isValid()) {
        applyImeInsetsAvoidDistanceWithoutActiveTarget(keyboardHeight);
        return true;
      }
      mKeyboardHeight = keyboardHeight;
      float avoidDistance = calculateAvoidDistance(target, keyboardHeight, insets);
      applyAvoidDistance(target.getLynxView(), avoidDistance, false);
      return true;
    }

    private int getKeyboardHeightFromInsets(WindowInsets insets) {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || insets == null) {
        return 0;
      }
      int imeBottom = insets.getInsets(WindowInsets.Type.ime()).bottom;
      int navigationBarBottom = insets.getInsets(WindowInsets.Type.navigationBars()).bottom;
      return Math.max(0, imeBottom - navigationBarBottom);
    }

    private void applyImeInsetsAvoidDistanceWithoutActiveTarget(int keyboardHeight) {
      View lynxView = mLastLynxView.get();
      if (lynxView == null) {
        mKeyboardHeight = keyboardHeight;
        return;
      }
      float avoidDistance = 0f;
      if (mKeyboardHeightBeforeImeAnimation > 0 && keyboardHeight > 0) {
        avoidDistance =
            mAvoidDistanceBeforeImeAnimation * keyboardHeight / mKeyboardHeightBeforeImeAnimation;
      }
      mKeyboardHeight = keyboardHeight;
      applyAvoidDistance(lynxView, avoidDistance, false);
    }

    private void prepareInsetsKeyboardTransition(
        int transition, int keyboardHeight, KeyboardAvoidTarget target) {
      mPendingKeyboardTransition = transition;
      mPendingKeyboardHeight = keyboardHeight;
      mPendingKeyboardTarget = new WeakReference<KeyboardAvoidTarget>(target);
      long delay = INSETS_ANIMATION_START_FALLBACK_DELAY_MS;
      if (mIsImeAnimationRunning) {
        delay = getInsetsAnimationFallbackDelay(null);
      }
      schedulePendingKeyboardTransitionFallback(delay);
    }

    private void schedulePendingKeyboardTransitionFallback(long delayMs) {
      cancelPendingKeyboardTransitionFallback();
      if (mPendingKeyboardTransition == KEYBOARD_TRANSITION_NONE && !mIsImeAnimationRunning) {
        return;
      }
      View hostView = mInsetsAnimationHostView;
      if (hostView == null) {
        hostView = resolveInsetsAnimationHostView();
      }
      if (hostView == null) {
        runPendingKeyboardTransitionFallback();
        return;
      }
      mPendingKeyboardFallbackView = hostView;
      mPendingKeyboardFallbackRunnable = new Runnable() {
        @Override
        public void run() {
          mPendingKeyboardFallbackRunnable = null;
          mPendingKeyboardFallbackView = null;
          runPendingKeyboardTransitionFallback();
        }
      };
      hostView.postDelayed(mPendingKeyboardFallbackRunnable, delayMs);
    }

    private void cancelPendingKeyboardTransitionFallback() {
      if (mPendingKeyboardFallbackRunnable != null && mPendingKeyboardFallbackView != null) {
        mPendingKeyboardFallbackView.removeCallbacks(mPendingKeyboardFallbackRunnable);
      }
      mPendingKeyboardFallbackRunnable = null;
      mPendingKeyboardFallbackView = null;
    }

    private void runPendingKeyboardTransitionFallback() {
      if (mPendingKeyboardTransition == KEYBOARD_TRANSITION_NONE) {
        if (mIsImeAnimationRunning) {
          applyCurrentImeInsetsAvoidDistance();
          endImeAnimation();
        }
        return;
      }
      if (mIsImeAnimationRunning) {
        if (!applyCurrentImeInsetsAvoidDistance()) {
          applyPendingKeyboardTransition(false);
        }
        endImeAnimation();
        return;
      }
      applyPendingKeyboardTransition(false);
      clearPendingKeyboardTransition();
    }

    private void applyPendingKeyboardTransition(boolean animated) {
      KeyboardAvoidTarget target = mPendingKeyboardTarget.get();
      if (target != null && !target.isValid()) {
        target = null;
      }
      if (mPendingKeyboardTransition == KEYBOARD_TRANSITION_SHOW) {
        if (target != null) {
          setActiveTarget(target);
        }
        mKeyboardHeight = mPendingKeyboardHeight;
        updateAvoidDistance(animated);
      } else if (mPendingKeyboardTransition == KEYBOARD_TRANSITION_HIDE) {
        mKeyboardHeight = 0;
        if (target != null) {
          setActiveTarget(target);
          updateAvoidDistance(animated);
        } else {
          setActiveTarget(null);
          resetAvoidDistance(animated);
        }
      }
    }

    private boolean applyCurrentImeInsetsAvoidDistance() {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || mInsetsAnimationHostView == null) {
        return false;
      }
      WindowInsets insets = mInsetsAnimationHostView.getRootWindowInsets();
      if (insets == null) {
        return false;
      }
      applyImeInsetsAvoidDistance(insets);
      return true;
    }

    private void clearPendingKeyboardTransition() {
      mPendingKeyboardTransition = KEYBOARD_TRANSITION_NONE;
      mPendingKeyboardHeight = 0;
      mPendingKeyboardTarget = new WeakReference<KeyboardAvoidTarget>(null);
    }

    private long getInsetsAnimationFallbackDelay(WindowInsetsAnimation animation) {
      long duration = KEYBOARD_ANIMATION_DURATION_MS;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && animation != null
          && animation.getDurationMillis() >= 0) {
        duration = animation.getDurationMillis();
      }
      return duration + INSETS_ANIMATION_END_FALLBACK_PADDING_MS;
    }

    private void beginImeAnimation(WindowInsetsAnimation animation) {
      if (!mIsImeAnimationRunning) {
        mKeyboardHeightBeforeImeAnimation = mKeyboardHeight;
        mAvoidDistanceBeforeImeAnimation = mCurrentAvoidDistance;
      }
      mIsImeAnimationRunning = true;
      schedulePendingKeyboardTransitionFallback(getInsetsAnimationFallbackDelay(animation));
    }

    private void endImeAnimation() {
      cancelPendingKeyboardTransitionFallback();
      clearPendingKeyboardTransition();
      mIsImeAnimationRunning = false;
      mKeyboardHeightBeforeImeAnimation = 0;
      mAvoidDistanceBeforeImeAnimation = 0f;
    }

    private void finishImeAnimation(WindowInsets insets) {
      if (!applyImeInsetsAvoidDistance(insets)
          && mPendingKeyboardTransition != KEYBOARD_TRANSITION_NONE) {
        applyPendingKeyboardTransition(false);
      }
      endImeAnimation();
    }
  }

  private static class KeyboardInsetsAnimationDispatcher {
    private final View mHostView;
    private final WeakHashMap<KeyboardAvoidingContext, Boolean> mContexts = new WeakHashMap<>();

    private final WindowInsetsAnimation.Callback mCallback = new WindowInsetsAnimation.Callback(
        WindowInsetsAnimation.Callback.DISPATCH_MODE_CONTINUE_ON_SUBTREE) {
      @Override
      public void onPrepare(WindowInsetsAnimation animation) {
        if (isImeAnimation(animation)) {
          for (KeyboardAvoidingContext context : snapshotContexts()) {
            context.beginImeAnimation(animation);
          }
        }
      }

      @Override
      public WindowInsetsAnimation.Bounds onStart(
          WindowInsetsAnimation animation, WindowInsetsAnimation.Bounds bounds) {
        if (isImeAnimation(animation)) {
          for (KeyboardAvoidingContext context : snapshotContexts()) {
            context.beginImeAnimation(animation);
          }
        }
        return bounds;
      }

      @Override
      public WindowInsets onProgress(
          WindowInsets insets, List<WindowInsetsAnimation> runningAnimations) {
        if (hasRunningImeAnimation(runningAnimations)) {
          for (KeyboardAvoidingContext context : snapshotContexts()) {
            context.applyImeInsetsAvoidDistance(insets);
          }
        }
        return insets;
      }

      @Override
      public void onEnd(WindowInsetsAnimation animation) {
        if (isImeAnimation(animation)) {
          WindowInsets insets = mHostView.getRootWindowInsets();
          for (KeyboardAvoidingContext context : snapshotContexts()) {
            context.finishImeAnimation(insets);
          }
        }
      }
    };

    static KeyboardInsetsAnimationDispatcher register(
        View hostView, KeyboardAvoidingContext context) {
      KeyboardInsetsAnimationDispatcher dispatcher = sInsetsAnimationDispatchers.get(hostView);
      if (dispatcher == null) {
        dispatcher = new KeyboardInsetsAnimationDispatcher(hostView);
        sInsetsAnimationDispatchers.put(hostView, dispatcher);
      }
      dispatcher.add(context);
      return dispatcher;
    }

    private KeyboardInsetsAnimationDispatcher(View hostView) {
      mHostView = hostView;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        mHostView.setWindowInsetsAnimationCallback(mCallback);
      }
    }

    private void add(KeyboardAvoidingContext context) {
      if (context != null) {
        mContexts.put(context, Boolean.TRUE);
      }
    }

    private void unregister(KeyboardAvoidingContext context) {
      mContexts.remove(context);
      if (!mContexts.isEmpty()) {
        return;
      }
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        mHostView.setWindowInsetsAnimationCallback(null);
      }
      sInsetsAnimationDispatchers.remove(mHostView);
    }

    private ArrayList<KeyboardAvoidingContext> snapshotContexts() {
      return new ArrayList<>(mContexts.keySet());
    }

    private static boolean isImeAnimation(WindowInsetsAnimation animation) {
      return Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && animation != null
          && (animation.getTypeMask() & WindowInsets.Type.ime()) != 0;
    }

    private static boolean hasRunningImeAnimation(List<WindowInsetsAnimation> runningAnimations) {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || runningAnimations == null) {
        return false;
      }
      for (WindowInsetsAnimation animation : runningAnimations) {
        if (isImeAnimation(animation)) {
          return true;
        }
      }
      return false;
    }
  }

  private static class KeyboardAvoidTarget {
    private final WeakReference<Object> mOwner;
    private final WeakReference<View> mInputView;
    private final WeakReference<View> mLynxView;
    private final boolean mAvoidKeyboard;
    private final float mSpacing;

    KeyboardAvoidTarget(
        Object owner, View inputView, View lynxView, boolean avoidKeyboard, float spacing) {
      mOwner = new WeakReference<Object>(owner);
      mInputView = new WeakReference<View>(inputView);
      mLynxView = new WeakReference<View>(lynxView);
      mAvoidKeyboard = avoidKeyboard;
      mSpacing = spacing;
    }

    boolean isValid() {
      return mOwner.get() != null && mInputView.get() != null && mLynxView.get() != null;
    }

    boolean matchesOwner(Object owner) {
      return owner != null && mOwner.get() == owner;
    }

    Object getOwner() {
      return mOwner.get();
    }

    View getInputView() {
      return mInputView.get();
    }

    View getLynxView() {
      return mLynxView.get();
    }

    boolean shouldAvoidKeyboard() {
      return mAvoidKeyboard;
    }

    float getSpacing() {
      return mSpacing;
    }
  }
}
