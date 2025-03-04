// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.herotransition;

import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import com.lynx.tasm.animation.AnimationInfo;
import com.lynx.tasm.animation.keyframe.LynxKeyframeAnimator;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.HashMap;

/**
 * Hold a LynxUI and apply anim on it.
 */
public class HeroAnimOwner {
  private static final String TAG = "HeroAnimOwner";

  private final LynxUI mLynxUI;
  private int mOriginIndex = 0;
  private ViewGroup mOriginParent = null;
  private UIGroup mOriginParentUI = null;

  private AnimationInfo mEnterAnimName;
  private AnimationInfo mExitAnimName;
  private AnimationInfo mPauseAnimName;
  private AnimationInfo mResumeAnimName;

  private String mSharedElementName;
  private volatile boolean mEnterAnimating = false;
  private volatile boolean mExitAnimating = false;

  private HashMap<String, LynxKeyframeAnimator.LynxAnimationListener> animListeners;

  public HeroAnimOwner(LynxUI lynxUI) {
    mLynxUI = lynxUI;
    animListeners = new HashMap<>();
  }

  private <T extends View> void applyFakeSharedElementEnter(String shareTag) {
    final View nativeView = HeroTransitionManager.inst().getSharedElementByTag(shareTag, mLynxUI);
    final UIBody.UIBodyView lynxView = mLynxUI.getLynxContext().getUIBody().getBodyView();
    if (lynxView != null) {
      final View createdView = mLynxUI.getView();
      if (nativeView != null) {
        createdView.setVisibility(nativeView.getVisibility());
        createdView.setAlpha(nativeView.getAlpha());
        createdView.setTranslationX(nativeView.getTranslationX());
        createdView.setTranslationY(nativeView.getTranslationY());
        createdView.setRotation(nativeView.getRotation());
        createdView.setRotationX(nativeView.getRotationX());
        createdView.setRotationY(nativeView.getRotationY());
        createdView.setScaleX(nativeView.getScaleX());
        createdView.setScaleY(nativeView.getScaleY());
        final int w = nativeView.getWidth();
        final int h = nativeView.getHeight();
        if (lynxView.getRootView() instanceof ViewGroup) {
          ViewGroup decorView = (ViewGroup) lynxView.getRootView();
          if (createdView.getParent() != null) {
            mOriginParent = (ViewGroup) createdView.getParent();
            int count = mOriginParent.getChildCount();
            for (int i = 0; i < count; i++) {
              if (createdView == mOriginParent.getChildAt(i)) {
                mOriginIndex = i;
                break;
              }
            }
            mOriginParent.removeView(createdView);
            mOriginParentUI = (UIGroup) mLynxUI.getParent();
          }
          ViewGroup.MarginLayoutParams mlp = new ViewGroup.MarginLayoutParams(w, h);
          final int[] winLoc = new int[2];
          nativeView.getLocationInWindow(winLoc);
          mlp.setMargins(winLoc[0], winLoc[1], 0, 0);
          decorView.addView(createdView, mlp);
          UIThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
              mLynxUI.setAnimation(mEnterAnimName);
              mLynxUI.requestLayout();
            }
          });
        }
      } else {
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            mLynxUI.setAnimation(mEnterAnimName);
            if (mLynxUI.getKeyframeManager() != null) {
              mLynxUI.getKeyframeManager().notifyAnimationUpdated();
            }
          }
        });
      }
    }
  }

  public void onAnimationEnd(String name) {
    LynxKeyframeAnimator.LynxAnimationListener listener = animListeners.get(name);
    if (listener != null) {
      listener.onAnimationEnd(name);
      animListeners.remove(name);
    }
  }

  public void setEnterAnim(AnimationInfo enterName) {
    mEnterAnimName = enterName;
  }

  public void setExitAnim(AnimationInfo exitName) {
    mExitAnimName = exitName;
  }

  public void setPauseAnim(AnimationInfo pauseAnim) {
    mPauseAnimName = pauseAnim;
  }

  public void setResumeAnim(AnimationInfo resumeName) {
    mResumeAnimName = resumeName;
  }

  public void executePauseAnim() {
    if (!HeroTransitionManager.inst().isSharedTransitionEnable() || isAnimating()) {
      return;
    }
    if (mPauseAnimName != null) {
      mLynxUI.setAnimation(mPauseAnimName);
      if (mLynxUI.getKeyframeManager() != null) {
        mLynxUI.getKeyframeManager().notifyAnimationUpdated();
      }
    }
  }

  public void executeResumeAnim() {
    if (!HeroTransitionManager.inst().isSharedTransitionEnable() || isAnimating()) {
      return;
    }
    if (mResumeAnimName != null) {
      mLynxUI.setAnimation(mResumeAnimName);
      if (mLynxUI.getKeyframeManager() != null) {
        mLynxUI.getKeyframeManager().notifyAnimationUpdated();
      }
    }
  }

  private void resetToLynxView() {
    if (mOriginParent != null) {
      View v = mLynxUI.getView();
      if (v == null) {
        return;
      }
      ViewGroup vg = (ViewGroup) v.getParent();
      ViewGroup.LayoutParams lp = v.getLayoutParams();
      if (vg != null) {
        vg.removeView(v);
      }
      int[] parentLoc = new int[2];
      mOriginParent.getLocationOnScreen(parentLoc);
      mOriginParentUI.removeChild(mLynxUI);
      mOriginParentUI.insertChild(mLynxUI, mOriginIndex);
      int targetLeft = v.getLeft() + parentLoc[0];
      int targetTop = v.getTop() - parentLoc[1];
      mLynxUI.updateLayout(targetLeft, targetTop, lp.width, lp.height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          0, 0, mLynxUI.getBound());
    }
  }

  private void applyFakeSharedElementExit() {
    final int[] winLoc = new int[2];
    final ViewGroup lynxView = mLynxUI.getLynxContext().getUIBody().getBodyView();
    ViewGroup decorView = (ViewGroup) lynxView.getRootView();
    final View createdView = mLynxUI.getView();
    createdView.getLocationInWindow(winLoc);
    ViewParent vp = createdView.getParent();
    if (vp != null) {
      ((ViewGroup) vp).removeView(createdView);
    }
    decorView.addView(createdView);
    mLynxUI.setAnimation(mExitAnimName);
    if (mLynxUI.getKeyframeManager() != null) {
      mLynxUI.getKeyframeManager().notifyAnimationUpdated();
    }
  }

  public void executeExitAnim(final HeroTransitionManager.LynxViewExitFinishListener listener) {
    if (!HeroTransitionManager.inst().isSharedTransitionEnable() || isAnimating()) {
      return;
    }
    if (mExitAnimName != null) {
      final String animName = mExitAnimName.getName();
      animListeners.put(animName, new LynxKeyframeAnimator.LynxAnimationListener() {
        @Override
        public void onAnimationEnd(String value) {
          animListeners.remove(value);
          final View createdView = mLynxUI.getView();
          if (createdView == null) {
            return;
          }
          ViewParent vp = createdView.getParent();
          if (vp != null) {
            ((ViewGroup) vp).removeView(createdView);
          }
          if (listener != null) {
            listener.onLynxViewExited();
          }
          mExitAnimating = false;
        }
      });
      if (mSharedElementName != null) {
        mExitAnimating = true;
        applyFakeSharedElementExit();
      } else {
        mLynxUI.setAnimation(mExitAnimName);
        if (mLynxUI.getKeyframeManager() != null) {
          mLynxUI.getKeyframeManager().notifyAnimationUpdated();
        }
      }
    }
  }

  public void executeEnterAnim(final HeroTransitionManager.LynxViewEnterFinishListener listener) {
    if (!HeroTransitionManager.inst().isSharedTransitionEnable() || isAnimating()) {
      return;
    }
    if (mEnterAnimName != null) {
      if (mSharedElementName != null) {
        mEnterAnimating = true;
        applyFakeSharedElementEnter(mSharedElementName);
        final String animName = mEnterAnimName.getName();
        animListeners.put(animName, new LynxKeyframeAnimator.LynxAnimationListener() {
          @Override
          public void onAnimationEnd(String value) {
            animListeners.remove(value);
            resetToLynxView();
            if (listener != null) {
              listener.onLynxViewEntered();
            }
            mEnterAnimating = false;
          }
        });
      } else {
        mLynxUI.setAnimation(mEnterAnimName);
        if (mLynxUI.getKeyframeManager() != null) {
          mLynxUI.getKeyframeManager().notifyAnimationUpdated();
        }
      }
    }
  }

  private boolean isAnimating() {
    return mEnterAnimating || mExitAnimating;
  }

  public void setSharedElementName(String name) {
    mSharedElementName = name;
    HeroTransitionManager.inst().registerHasSharedElementLynxUI(mLynxUI, name);
  }
}
