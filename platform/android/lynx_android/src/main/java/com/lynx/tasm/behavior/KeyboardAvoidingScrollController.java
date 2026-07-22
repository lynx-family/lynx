// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.app.Activity;
import android.graphics.Rect;
import android.os.Build;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowInsets;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.ui.scroll.LynxUIScrollViewInternal;
import com.lynx.tasm.behavior.ui.scroll.UIScrollView;
import com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollView;
import com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller;
import java.lang.ref.WeakReference;

class KeyboardAvoidingScrollController {
  interface Delegate {
    Iterable<KeyboardEvent.KeyboardAvoidingTarget> getTargets();

    KeyboardEvent.KeyboardAvoidingTarget getActiveTarget();

    Activity getActivity();

    int getKeyboardAvoidingScreenBottom(View decorView);
  }

  private final Delegate mDelegate;
  private WeakReference<LynxBaseUI> mScrollUI = new WeakReference<LynxBaseUI>(null);
  private boolean mHasContentState = false;
  private int mBaseContentHeight = 0;
  private int mContentHeightExtra = 0;
  private boolean mHasOriginalOffset = false;
  private int mOriginalOffsetX = 0;
  private int mOriginalOffsetY = 0;
  private WeakReference<LynxBaseUI> mFocusedInputScrollUI = new WeakReference<LynxBaseUI>(null);
  private WeakReference<View> mFocusedInputView = new WeakReference<View>(null);
  private boolean mHasFocusedInputBottom = false;
  private float mFocusedInputBottomInScrollViewContent = 0f;
  private WeakReference<LynxBaseUI> mSmoothScrollUI = new WeakReference<LynxBaseUI>(null);
  private Runnable mPendingClearRunnable;
  private View mPendingClearView;
  private int mClearGeneration = 0;
  private ViewTreeObserver.OnPreDrawListener mPendingPostLayoutReapplyListener;
  private View mPendingPostLayoutReapplyView;
  private int mPostLayoutReapplyGeneration = 0;

  KeyboardAvoidingScrollController(Delegate delegate) {
    mDelegate = delegate;
  }

  boolean canHandleTarget(KeyboardEvent.KeyboardAvoidingTarget target) {
    return scrollUIForTarget(target) != null;
  }

  boolean applyAvoidDistance(KeyboardEvent.KeyboardAvoidingTarget target, int keyboardHeight,
      WindowInsets insets, boolean animated, boolean usePendingKeyboardHeight,
      int pendingKeyboardHeight) {
    return applyAvoidDistance(target, keyboardHeight, insets, animated, usePendingKeyboardHeight,
        pendingKeyboardHeight, true);
  }

  private boolean applyAvoidDistance(KeyboardEvent.KeyboardAvoidingTarget target,
      int keyboardHeight, WindowInsets insets, boolean animated, boolean usePendingKeyboardHeight,
      int pendingKeyboardHeight, boolean schedulePostLayoutReapply) {
    LynxBaseUI scrollUI = scrollUIForTarget(target);
    if (scrollUI == null) {
      return false;
    }
    LynxBaseUI currentScrollUI = mScrollUI.get();
    if (currentScrollUI != null && currentScrollUI != scrollUI) {
      clear(false);
    }
    mScrollUI = new WeakReference<LynxBaseUI>(scrollUI);
    int effectiveKeyboardHeight = keyboardHeight;
    if (usePendingKeyboardHeight) {
      effectiveKeyboardHeight = Math.max(effectiveKeyboardHeight, pendingKeyboardHeight);
    }

    View inputView = target.getInputView();
    View scrollView = scrollViewForScrollUI(scrollUI);
    if (inputView == null || scrollView == null || scrollView.getHeight() <= 0) {
      return true;
    }
    captureOriginalOffsetIfNeeded(scrollUI);
    preserveFocusedInputBottomIfNeeded(scrollUI, inputView, scrollView,
        effectiveKeyboardHeight > 0
            && (contentHeightExtraForScrollUI(scrollUI) > 0 || mContentHeightExtra > 0));
    if (effectiveKeyboardHeight <= 0) {
      return true;
    }

    int baseContentHeight = baseContentHeightForScrollUI(scrollUI);
    boolean hadContentHeightExtra =
        contentHeightExtraForScrollUI(scrollUI) > 0 || mContentHeightExtra > 0;
    int minOffsetY = 0;
    int currentOffsetY = scrollOffsetYForScrollUI(scrollUI);
    int viewportHeight = viewportHeightForScrollView(scrollView);
    int rawMaxOffsetYWithoutExtra = baseContentHeight - viewportHeight;
    int maxOffsetYWithoutExtra = Math.max(minOffsetY, rawMaxOffsetYWithoutExtra);
    float visibleHeight = scrollView.getHeight();
    WindowInsets effectiveInsets = usePendingKeyboardHeight ? null : insets;
    int availableBottomOnScreen =
        keyboardAvailableBottomOnScreen(effectiveKeyboardHeight, effectiveInsets);
    if (effectiveKeyboardHeight > 0 && availableBottomOnScreen >= 0) {
      int[] scrollLocation = new int[2];
      scrollView.getLocationOnScreen(scrollLocation);
      visibleHeight = Math.min(visibleHeight, availableBottomOnScreen - scrollLocation[1]);
    }
    visibleHeight = Math.max(0f, visibleHeight);

    float inputBottomY = inputBottomInScrollViewContent(inputView, scrollView, currentOffsetY);
    float desiredOffsetY = inputBottomY + target.getSpacing() - visibleHeight;
    int coveredHeight = coveredHeightForScrollView(scrollView, visibleHeight);
    KeyboardEvent.KeyboardAvoidingTarget lowestTarget = lowestTargetInScrollUI(scrollUI);
    if (lowestTarget == null) {
      lowestTarget = target;
    }
    float lowestInputBottomY =
        inputBottomInScrollViewContent(lowestTarget.getInputView(), scrollView, currentOffsetY);
    float lowestDesiredOffsetY = lowestInputBottomY + lowestTarget.getSpacing() - visibleHeight;
    int requiredExtra = coveredHeight;
    if (lowestDesiredOffsetY > maxOffsetYWithoutExtra) {
      requiredExtra = Math.max(requiredExtra,
          Math.max(0, (int) Math.ceil(lowestDesiredOffsetY - rawMaxOffsetYWithoutExtra)));
    }
    setContentHeightExtraForScrollUI(scrollUI, requiredExtra);

    int maxOffsetY = Math.max(minOffsetY, rawMaxOffsetYWithoutExtra + requiredExtra);
    int targetOffsetY = currentOffsetY;
    if (desiredOffsetY > currentOffsetY) {
      targetOffsetY = Math.min(Math.max((int) Math.ceil(desiredOffsetY), minOffsetY), maxOffsetY);
    }
    if (Math.abs(targetOffsetY - currentOffsetY) < 1) {
      schedulePostLayoutReapplyIfNeeded(scrollUI, target, keyboardHeight, insets, animated,
          usePendingKeyboardHeight, pendingKeyboardHeight,
          schedulePostLayoutReapply && hadContentHeightExtra && requiredExtra > 0);
      return true;
    }
    boolean needsScrollAfterContentLayout =
        requiredExtra > 0 && targetOffsetY > maxOffsetYWithoutExtra;
    if (!animated && shouldKeepSmoothScroll(scrollUI, usePendingKeyboardHeight)) {
      return true;
    }
    if (animated && needsScrollAfterContentLayout) {
      scrollToAfterContentLayout(scrollUI, target, targetOffsetY, true);
      return true;
    }
    scrollTo(scrollUI, targetOffsetY, animated);
    if (needsScrollAfterContentLayout) {
      scrollToAfterContentLayout(scrollUI, target, targetOffsetY, animated);
    }
    return true;
  }

  void clear(boolean animated) {
    LynxBaseUI scrollUI = mScrollUI.get();
    mScrollUI = new WeakReference<LynxBaseUI>(null);
    clearSmoothScroll();
    clearPendingPostLayoutReapply();
    if (scrollUI == null) {
      resetContentState();
      return;
    }
    final int baseContentHeight = Math.max(0, mBaseContentHeight);
    final int clearGeneration = ++mClearGeneration;
    final Runnable clearContentHeightExtra = new Runnable() {
      @Override
      public void run() {
        clearPendingClearRunnable(this);
        if (clearGeneration != mClearGeneration) {
          return;
        }
        if (mScrollUI.get() == scrollUI) {
          return;
        }
        clearContentHeightForScrollUI(scrollUI, baseContentHeight);
      }
    };
    boolean isRestoring =
        restoreOriginalOffsetForScrollUI(scrollUI, animated, clearContentHeightExtra);
    if (!isRestoring || !animated) {
      clearContentHeightExtra.run();
    }
    resetContentState();
  }

  private LynxBaseUI scrollUIForTarget(KeyboardEvent.KeyboardAvoidingTarget target) {
    return target == null ? null : scrollUIForOwner(target.getOwner());
  }

  private LynxBaseUI scrollUIForOwner(Object owner) {
    if (!(owner instanceof LynxBaseUI)) {
      return null;
    }
    LynxBaseUI ui = ((LynxBaseUI) owner).getParentBaseUI();
    while (ui != null) {
      if (isVerticalScrollUI(ui)) {
        return ui;
      }
      ui = ui.getParentBaseUI();
    }
    return null;
  }

  private boolean isVerticalScrollUI(LynxBaseUI ui) {
    if (ui instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) ui).getView();
      return scrollView != null && !scrollView.isHorizontal();
    }
    if (ui instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) ui).getView();
      return scrollView != null && scrollView.isVertical();
    }
    return false;
  }

  private View scrollViewForScrollUI(LynxBaseUI scrollUI) {
    if (scrollUI instanceof LynxUI) {
      return ((LynxUI<?>) scrollUI).getView();
    }
    return null;
  }

  private KeyboardEvent.KeyboardAvoidingTarget lowestTargetInScrollUI(LynxBaseUI scrollUI) {
    KeyboardEvent.KeyboardAvoidingTarget lowestTarget = null;
    float lowestInputBottom = -Float.MAX_VALUE;
    int currentOffsetY = scrollOffsetYForScrollUI(scrollUI);
    View scrollView = scrollViewForScrollUI(scrollUI);
    if (scrollUI == null || scrollView == null) {
      return null;
    }
    for (KeyboardEvent.KeyboardAvoidingTarget target : mDelegate.getTargets()) {
      if (target == null || !target.isValid() || !target.shouldAvoidKeyboard()
          || !isTargetInScrollUI(target, scrollUI)) {
        continue;
      }
      float inputBottom =
          inputBottomInScrollViewContent(target.getInputView(), scrollView, currentOffsetY);
      if (inputBottom > lowestInputBottom) {
        lowestInputBottom = inputBottom;
        lowestTarget = target;
      }
    }
    return lowestTarget;
  }

  private boolean isTargetInScrollUI(
      KeyboardEvent.KeyboardAvoidingTarget target, LynxBaseUI scrollUI) {
    Object owner = target.getOwner();
    if (!(owner instanceof LynxBaseUI) || scrollUI == null) {
      return false;
    }
    LynxBaseUI ui = ((LynxBaseUI) owner).getParentBaseUI();
    while (ui != null) {
      if (ui == scrollUI) {
        return true;
      }
      if (isVerticalScrollUI(ui)) {
        return false;
      }
      ui = ui.getParentBaseUI();
    }
    return false;
  }

  private float inputBottomInScrollViewContent(View inputView, View scrollView, int scrollOffsetY) {
    int[] inputLocation = new int[2];
    int[] scrollLocation = new int[2];
    inputView.getLocationOnScreen(inputLocation);
    scrollView.getLocationOnScreen(scrollLocation);
    return inputLocation[1] - scrollLocation[1] + inputView.getHeight() + scrollOffsetY;
  }

  private int coveredHeightForScrollView(View scrollView, float visibleHeight) {
    if (scrollView == null) {
      return 0;
    }
    return Math.max(0, (int) Math.ceil(scrollView.getHeight() - Math.max(0f, visibleHeight)));
  }

  private int keyboardAvailableBottomOnScreen(int keyboardHeight, WindowInsets insets) {
    Activity activity = mDelegate.getActivity();
    if (activity == null || activity.getWindow() == null) {
      return -1;
    }
    View decorView = activity.getWindow().getDecorView();
    if (decorView == null || decorView.getHeight() <= 0) {
      return -1;
    }
    boolean preferVisibleFrame =
        Build.VERSION.SDK_INT >= Build.VERSION_CODES.N && activity.isInMultiWindowMode();
    int availableBottomInHost =
        getKeyboardAvailableBottomInHost(decorView, keyboardHeight, insets, preferVisibleFrame);
    if (availableBottomInHost >= 0) {
      int[] decorLocation = new int[2];
      decorView.getLocationOnScreen(decorLocation);
      return decorLocation[1] + availableBottomInHost;
    }
    if (keyboardHeight <= 0) {
      return -1;
    }
    return mDelegate.getKeyboardAvoidingScreenBottom(decorView) - keyboardHeight;
  }

  private int baseContentHeightForScrollUI(LynxBaseUI scrollUI) {
    int currentContentHeight = Math.max(0, contentHeightForScrollUI(scrollUI));
    int currentContentHeightExtra = Math.max(0, contentHeightExtraForScrollUI(scrollUI));
    if (currentContentHeightExtra > 0) {
      int baseContentHeight = Math.max(0, currentContentHeight - currentContentHeightExtra);
      mBaseContentHeight = baseContentHeight;
      mContentHeightExtra = currentContentHeightExtra;
      mHasContentState = true;
      return baseContentHeight;
    }
    if (mHasContentState) {
      return Math.max(0, mBaseContentHeight);
    }
    mBaseContentHeight = currentContentHeight;
    mHasContentState = true;
    return currentContentHeight;
  }

  private void setContentHeightExtraForScrollUI(LynxBaseUI scrollUI, int extra) {
    int baseContentHeight = baseContentHeightForScrollUI(scrollUI);
    int contentHeightExtra = Math.max(0, extra);
    int appliedContentHeight = baseContentHeight + contentHeightExtra;
    mContentHeightExtra = contentHeightExtra;
    if (contentHeightExtra > 0) {
      setContentHeightForScrollUI(scrollUI, appliedContentHeight);
    } else {
      clearContentHeightForScrollUI(scrollUI);
    }
    mBaseContentHeight = baseContentHeight;
    mHasContentState = true;
  }

  private void resetContentState() {
    mHasContentState = false;
    mBaseContentHeight = 0;
    mContentHeightExtra = 0;
    mHasOriginalOffset = false;
    mOriginalOffsetX = 0;
    mOriginalOffsetY = 0;
    resetFocusedInputBottomState();
  }

  private void resetFocusedInputBottomState() {
    mFocusedInputScrollUI = new WeakReference<LynxBaseUI>(null);
    mFocusedInputView = new WeakReference<View>(null);
    mHasFocusedInputBottom = false;
    mFocusedInputBottomInScrollViewContent = 0f;
  }

  private void clearSmoothScroll() {
    mSmoothScrollUI = new WeakReference<LynxBaseUI>(null);
  }

  private void schedulePostLayoutReapplyIfNeeded(final LynxBaseUI scrollUI,
      final KeyboardEvent.KeyboardAvoidingTarget target, final int keyboardHeight,
      final WindowInsets insets, final boolean animated, final boolean usePendingKeyboardHeight,
      final int pendingKeyboardHeight, boolean shouldSchedule) {
    if (!shouldSchedule || scrollUI == null || target == null) {
      return;
    }
    final View scrollView = scrollViewForScrollUI(scrollUI);
    if (scrollView == null || scrollView.getHeight() <= 0) {
      return;
    }
    clearPendingPostLayoutReapply();
    final int generation = ++mPostLayoutReapplyGeneration;
    ViewTreeObserver observer = scrollView.getViewTreeObserver();
    if (observer == null || !observer.isAlive()) {
      return;
    }
    mPendingPostLayoutReapplyView = scrollView;
    mPendingPostLayoutReapplyListener = new ViewTreeObserver.OnPreDrawListener() {
      @Override
      public boolean onPreDraw() {
        clearPendingPostLayoutReapply(this);
        if (generation != mPostLayoutReapplyGeneration
            || !isScrollRequestCurrent(scrollUI, target)) {
          return true;
        }
        applyAvoidDistance(target, keyboardHeight, insets, animated, usePendingKeyboardHeight,
            pendingKeyboardHeight, false);
        return true;
      }
    };
    observer.addOnPreDrawListener(mPendingPostLayoutReapplyListener);
    scrollView.invalidate();
  }

  private void clearPendingPostLayoutReapply() {
    clearPendingPostLayoutReapply(null);
  }

  private void clearPendingPostLayoutReapply(ViewTreeObserver.OnPreDrawListener listener) {
    if (listener != null && mPendingPostLayoutReapplyListener != listener) {
      return;
    }
    if (mPendingPostLayoutReapplyListener != null && mPendingPostLayoutReapplyView != null) {
      ViewTreeObserver observer = mPendingPostLayoutReapplyView.getViewTreeObserver();
      if (observer != null && observer.isAlive()) {
        observer.removeOnPreDrawListener(mPendingPostLayoutReapplyListener);
      }
    }
    mPendingPostLayoutReapplyListener = null;
    mPendingPostLayoutReapplyView = null;
  }

  private void markSmoothScroll(LynxBaseUI scrollUI) {
    mSmoothScrollUI = new WeakReference<LynxBaseUI>(scrollUI);
  }

  private boolean shouldKeepSmoothScroll(LynxBaseUI scrollUI, boolean usePendingKeyboardHeight) {
    return usePendingKeyboardHeight && mSmoothScrollUI.get() == scrollUI;
  }

  private int contentHeightForScrollUI(LynxBaseUI scrollUI) {
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      return scrollView == null ? 0 : Math.max(0, scrollView.getContentHeight());
    }
    if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      return scrollView == null ? 0 : Math.max(0, scrollView.getScrollContentSizeVertically());
    }
    return 0;
  }

  private int contentHeightExtraForScrollUI(LynxBaseUI scrollUI) {
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      return scrollView == null ? 0
                                : Math.max(0, scrollView.getKeyboardAvoidingContentHeightExtra());
    }
    if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      return scrollView == null ? 0
                                : Math.max(0, scrollView.getKeyboardAvoidingContentHeightExtra());
    }
    return 0;
  }

  private void setContentHeightForScrollUI(LynxBaseUI scrollUI, int contentHeight) {
    int targetContentHeight = Math.max(0, contentHeight);
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      if (scrollView == null) {
        return;
      }
      int scrollX = scrollView.getRealScrollX();
      int scrollY = scrollView.getRealScrollY();
      scrollView.setKeyboardAvoidingContentHeightExtra(mContentHeightExtra);
      if (scrollView.getContentHeight() != targetContentHeight) {
        scrollView.setMeasuredSize(scrollView.getContentWidth(), targetContentHeight);
      }
      restoreScrollOffset(scrollView, scrollX, scrollY);
    } else if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      if (scrollView == null) {
        return;
      }
      int offsetX = scrollView.getScrollOffsetHorizontally();
      int offsetY = scrollView.getScrollOffsetVertically();
      scrollView.setKeyboardAvoidingContentHeightExtra(mContentHeightExtra);
      if (scrollView.getScrollContentSizeVertically() != targetContentHeight) {
        scrollView.setScrollContentSizeVertically(targetContentHeight);
      }
      restoreScrollOffset(scrollView, offsetX, offsetY);
    }
  }

  private void clearContentHeightForScrollUI(LynxBaseUI scrollUI) {
    if (!mHasContentState) {
      return;
    }
    clearContentHeightForScrollUI(scrollUI, Math.max(0, mBaseContentHeight));
  }

  private void clearContentHeightForScrollUI(LynxBaseUI scrollUI, int baseContentHeight) {
    int targetContentHeight = Math.max(0, baseContentHeight);
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      if (scrollView == null) {
        return;
      }
      int scrollX = scrollView.getRealScrollX();
      int scrollY = scrollView.getRealScrollY();
      scrollView.setKeyboardAvoidingContentHeightExtra(0);
      if (scrollView.getContentHeight() != targetContentHeight) {
        scrollView.setMeasuredSize(scrollView.getContentWidth(), targetContentHeight);
      }
      restoreScrollOffset(scrollView, scrollX, scrollY);
    } else if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      if (scrollView == null) {
        return;
      }
      int offsetX = scrollView.getScrollOffsetHorizontally();
      int offsetY = scrollView.getScrollOffsetVertically();
      scrollView.setKeyboardAvoidingContentHeightExtra(0);
      if (scrollView.getScrollContentSizeVertically() != targetContentHeight) {
        scrollView.setScrollContentSizeVertically(targetContentHeight);
      }
      restoreScrollOffset(scrollView, offsetX, offsetY);
    }
  }

  private void reapplyContentHeightForScrollUI(LynxBaseUI scrollUI) {
    if (!mHasContentState || mContentHeightExtra <= 0) {
      return;
    }
    setContentHeightForScrollUI(scrollUI, mBaseContentHeight + mContentHeightExtra);
  }

  private int viewportHeightForScrollView(View scrollView) {
    return scrollView == null ? 0 : Math.max(0, scrollView.getHeight());
  }

  private void preserveFocusedInputBottomIfNeeded(
      LynxBaseUI scrollUI, View inputView, View scrollView, boolean shouldPreserve) {
    if (scrollUI == null || inputView == null || scrollView == null) {
      resetFocusedInputBottomState();
      return;
    }
    int currentOffsetY = scrollOffsetYForScrollUI(scrollUI);
    float currentInputBottom =
        inputBottomInScrollViewContent(inputView, scrollView, currentOffsetY);
    LynxBaseUI lastScrollUI = mFocusedInputScrollUI.get();
    View lastInputView = mFocusedInputView.get();
    boolean canPreserve = shouldPreserve && mHasFocusedInputBottom && lastScrollUI == scrollUI
        && lastInputView == inputView;
    if (canPreserve) {
      int delta = Math.round(currentInputBottom - mFocusedInputBottomInScrollViewContent);
      if (Math.abs(delta) >= 1) {
        int targetOffsetY = clampScrollOffsetYForScrollUI(scrollUI, currentOffsetY + delta);
        if (Math.abs(targetOffsetY - currentOffsetY) >= 1) {
          setScrollOffsetYForScrollUI(scrollUI, targetOffsetY);
        }
      }
    }
    mFocusedInputScrollUI = new WeakReference<LynxBaseUI>(scrollUI);
    mFocusedInputView = new WeakReference<View>(inputView);
    mHasFocusedInputBottom = true;
    mFocusedInputBottomInScrollViewContent = currentInputBottom;
  }

  private int clampScrollOffsetYForScrollUI(LynxBaseUI scrollUI, int offsetY) {
    int minOffsetY = 0;
    int maxOffsetY = 0;
    View scrollView = scrollViewForScrollUI(scrollUI);
    if (scrollView != null) {
      int viewportHeight = viewportHeightForScrollView(scrollView);
      maxOffsetY = Math.max(0, contentHeightForScrollUI(scrollUI) - viewportHeight);
    }
    return Math.min(Math.max(offsetY, minOffsetY), maxOffsetY);
  }

  private void setScrollOffsetYForScrollUI(LynxBaseUI scrollUI, int offsetY) {
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      if (scrollView != null) {
        setAndroidScrollOffset(scrollView, scrollView.getRealScrollX(), offsetY);
      }
    } else if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      if (scrollView != null) {
        restoreScrollOffset(scrollView, scrollView.getScrollOffsetHorizontally(), offsetY);
      }
    }
  }

  private int scrollOffsetYForScrollUI(LynxBaseUI scrollUI) {
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      return scrollView == null ? 0 : scrollView.getRealScrollY();
    }
    if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      return scrollView == null ? 0 : scrollView.getScrollOffsetVertically();
    }
    return 0;
  }

  private void captureOriginalOffsetIfNeeded(LynxBaseUI scrollUI) {
    if (mHasOriginalOffset || scrollUI == null) {
      return;
    }
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      if (scrollView == null) {
        return;
      }
      mOriginalOffsetX = scrollView.getRealScrollX();
      mOriginalOffsetY = scrollView.getRealScrollY();
    } else if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      if (scrollView == null) {
        return;
      }
      mOriginalOffsetX = scrollView.getScrollOffsetHorizontally();
      mOriginalOffsetY = scrollView.getScrollOffsetVertically();
    } else {
      return;
    }
    mHasOriginalOffset = true;
  }

  private boolean restoreOriginalOffsetForScrollUI(
      LynxBaseUI scrollUI, boolean animated, Runnable completion) {
    if (!mHasOriginalOffset || scrollUI == null) {
      return false;
    }
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      return restoreScrollOffset(
          scrollView, mOriginalOffsetX, mOriginalOffsetY, animated, completion);
    } else if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      return restoreScrollOffset(
          scrollView, mOriginalOffsetX, mOriginalOffsetY, animated, completion);
    }
    return false;
  }

  private void scrollTo(LynxBaseUI scrollUI, int offsetY, boolean animated) {
    if (animated) {
      markSmoothScroll(scrollUI);
    } else if (!shouldKeepSmoothScroll(scrollUI, false)) {
      clearSmoothScroll();
    }
    reapplyContentHeightForScrollUI(scrollUI);
    if (scrollUI instanceof UIScrollView) {
      AndroidScrollView scrollView = ((UIScrollView) scrollUI).getView();
      if (scrollView != null) {
        scrollAndroidScrollViewTo(scrollView, offsetY, animated);
      }
    } else if (scrollUI instanceof LynxUIScrollViewInternal) {
      LynxBaseScrollView scrollView = ((LynxUIScrollViewInternal) scrollUI).getView();
      if (scrollView != null) {
        int[] target = {scrollView.getScrollOffsetHorizontally(), offsetY};
        if (animated) {
          scrollView.animatedScrollTo(target, null);
        } else {
          scrollView.scrollTo(target);
        }
      }
    }
  }

  private void scrollAndroidScrollViewTo(
      AndroidScrollView scrollView, int offsetY, boolean animated) {
    if (scrollView == null) {
      return;
    }
    int scrollX = scrollView.getRealScrollX();
    int startY = scrollView.getRealScrollY();
    if (Math.abs(offsetY - startY) < 1) {
      return;
    }
    scrollView.setScrollTo(scrollX, offsetY, animated);
  }

  private void setAndroidScrollOffset(AndroidScrollView scrollView, int scrollX, int scrollY) {
    if (scrollView == null) {
      return;
    }
    scrollView.setScrollTo(scrollX, scrollY, false);
  }

  private void restoreScrollOffset(AndroidScrollView scrollView, int scrollX, int scrollY) {
    restoreScrollOffset(scrollView, scrollX, scrollY, false, null);
  }

  private boolean restoreScrollOffset(AndroidScrollView scrollView, int scrollX, int scrollY,
      boolean animated, Runnable completion) {
    if (scrollView == null) {
      return false;
    }
    if (scrollView.getRealScrollX() != scrollX || scrollView.getRealScrollY() != scrollY) {
      if (animated) {
        scrollView.setScrollTo(scrollX, scrollY, true);
        schedulePendingClear(scrollView, completion);
      } else {
        setAndroidScrollOffset(scrollView, scrollX, scrollY);
      }
      return true;
    }
    return false;
  }

  private void restoreScrollOffset(LynxBaseScrollView scrollView, int offsetX, int offsetY) {
    restoreScrollOffset(scrollView, offsetX, offsetY, false, null);
  }

  private boolean restoreScrollOffset(LynxBaseScrollView scrollView, int offsetX, int offsetY,
      boolean animated, final Runnable completion) {
    if (scrollView == null) {
      return false;
    }
    if (scrollView.getScrollOffsetHorizontally() != offsetX
        || scrollView.getScrollOffsetVertically() != offsetY) {
      int[] target = new int[] {offsetX, offsetY};
      if (animated) {
        scrollView.animatedScrollTo(
            target, new LynxBaseScrollViewScroller.ScrollFinishedCallback() {
              @Override
              public void finished(boolean completed) {
                if (completion != null) {
                  completion.run();
                }
              }
            });
      } else {
        scrollView.scrollToUnlimited(target);
      }
      return true;
    }
    return false;
  }

  private void schedulePendingClear(View view, Runnable runnable) {
    clearPendingClearRunnable(null);
    if (view == null || runnable == null) {
      return;
    }
    mPendingClearView = view;
    mPendingClearRunnable = runnable;
    view.postDelayed(runnable, KeyboardEvent.KEYBOARD_ANIMATION_DURATION_MS);
  }

  private void clearPendingClearRunnable(Runnable runnable) {
    if (runnable != null && mPendingClearRunnable != runnable) {
      return;
    }
    if (mPendingClearRunnable != null && mPendingClearView != null) {
      mPendingClearView.removeCallbacks(mPendingClearRunnable);
    }
    mPendingClearRunnable = null;
    mPendingClearView = null;
  }

  private void scrollToAfterContentLayout(final LynxBaseUI scrollUI,
      final KeyboardEvent.KeyboardAvoidingTarget target, final int offsetY,
      final boolean animated) {
    final View scrollView = scrollViewForScrollUI(scrollUI);
    if (scrollView == null) {
      return;
    }
    ViewTreeObserver observer = scrollView.getViewTreeObserver();
    if (observer == null || !observer.isAlive()) {
      scrollTo(scrollUI, offsetY, animated);
      return;
    }
    if (animated) {
      markSmoothScroll(scrollUI);
    }
    observer.addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
      @Override
      public boolean onPreDraw() {
        ViewTreeObserver currentObserver = scrollView.getViewTreeObserver();
        if (currentObserver.isAlive()) {
          currentObserver.removeOnPreDrawListener(this);
        }
        if (!isScrollRequestCurrent(scrollUI, target)) {
          return true;
        }
        reapplyContentHeightForScrollUI(scrollUI);
        int currentOffsetY = scrollOffsetYForScrollUI(scrollUI);
        if (Math.abs(offsetY - currentOffsetY) >= 1) {
          scrollTo(scrollUI, offsetY, animated);
        }
        return true;
      }
    });
    scrollView.invalidate();
  }

  private boolean isScrollRequestCurrent(
      LynxBaseUI scrollUI, KeyboardEvent.KeyboardAvoidingTarget target) {
    if (scrollUI == null || target == null || mScrollUI.get() != scrollUI) {
      return false;
    }
    KeyboardEvent.KeyboardAvoidingTarget activeTarget = mDelegate.getActiveTarget();
    return activeTarget != null && activeTarget.matchesOwner(target.getOwner());
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
}
