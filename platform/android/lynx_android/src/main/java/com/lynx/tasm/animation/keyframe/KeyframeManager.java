// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation.keyframe;

import android.text.TextUtils;
import android.view.View;
import androidx.annotation.Nullable;
import com.lynx.tasm.animation.AnimationInfo;
import com.lynx.tasm.behavior.PropertyIDConstants;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.LynxUI;
import java.lang.ref.WeakReference;
import java.util.HashMap;

// Some acronyms are used as the letters are so long.
// kf is keyframe, opa is opacity, tran is translation or transform
// rot is rotation, sca is scale, bColor is backgroundColor

public class KeyframeManager {
  private WeakReference<LynxUI> mUI;
  private AnimationInfo[] mInfos;
  private HashMap<String, LynxKeyframeAnimator> mAnimators;

  public static boolean hasKeyframeAnimation(StylesDiffMap map) {
    return map.hasKey(PropsConstants.ANIMATION) || map.hasKey(PropertyIDConstants.Animation);
  }

  public KeyframeManager(LynxUI ui) {
    mUI = new WeakReference<>(ui);
    mAnimators = new HashMap<>();
  }

  @Nullable
  LynxUI getUI() {
    return mUI.get();
  }

  @Nullable
  View getView() {
    return getUI().getView();
  }

  public void setAnimations(AnimationInfo[] infos) {
    mInfos = infos;
  }

  public void setAnimation(AnimationInfo info) {
    mInfos = new AnimationInfo[] {info};
  }

  public void notifyAnimationUpdated() {
    if (mInfos == null || (getUI().getHeight() == 0 && getUI().getWidth() == 0)) {
      return;
    }
    HashMap<String, LynxKeyframeAnimator> animators = new HashMap<>();
    for (AnimationInfo info : mInfos) {
      if (info == null || TextUtils.isEmpty(info.getName())) {
        continue;
      }
      LynxKeyframeAnimator animator = (mAnimators != null ? mAnimators.get(info.getName()) : null);
      if (animator == null) {
        animator = new LynxKeyframeAnimator(getView(), getUI());
      } else {
        mAnimators.remove(info.getName());
      }
      animators.put(info.getName(), animator);
    }

    // Should destroy all animators that be removed by user firstly, then apply info to other
    // animators.
    if (mAnimators != null) {
      for (LynxKeyframeAnimator animator : mAnimators.values()) {
        animator.destroy();
      }
    }

    // Should ensure that animators apply info in order
    for (AnimationInfo info : mInfos) {
      if (info == null || TextUtils.isEmpty(info.getName())) {
        continue;
      }
      animators.get(info.getName()).apply(info);
    }

    mAnimators = animators;
  }

  public void endAllAnimation() {
    if (mAnimators == null) {
      return;
    }
    for (LynxKeyframeAnimator animator : mAnimators.values()) {
      animator.destroy();
    }
    mAnimators = null;
    mInfos = null;
  }

  public void notifyPropertyUpdated(String name, Object value) {
    if (mAnimators == null) {
      return;
    }
    for (LynxKeyframeAnimator animator : mAnimators.values()) {
      animator.notifyPropertyUpdated(name, value);
    }
  }

  public boolean hasAnimationRunning() {
    if (mAnimators != null) {
      for (LynxKeyframeAnimator animator : mAnimators.values()) {
        if (animator.isRunning()) {
          return true;
        }
      }
    }
    return false;
  }

  public void onAttach() {
    if (mAnimators == null) {
      return;
    }
    for (LynxKeyframeAnimator animator : mAnimators.values()) {
      animator.onAttach();
    }
  }

  public void detachFromUI() {
    mUI = null;
    if (mAnimators == null) {
      return;
    }
    for (LynxKeyframeAnimator animator : mAnimators.values()) {
      animator.detachFromUI();
    }
  }

  public void attachToUI(LynxUI ui) {
    mUI = new WeakReference<>(ui);
    if (mAnimators == null) {
      return;
    }
    for (LynxKeyframeAnimator animator : mAnimators.values()) {
      animator.attachToUI(ui);
    }
  }

  public void onDetach() {
    if (mAnimators == null) {
      return;
    }
    for (LynxKeyframeAnimator animator : mAnimators.values()) {
      animator.onDetach();
    }
  }

  public void onDestroy() {
    endAllAnimation();
  }
}
