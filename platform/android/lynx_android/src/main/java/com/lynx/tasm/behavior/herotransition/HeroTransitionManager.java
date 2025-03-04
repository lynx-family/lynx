// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.herotransition;

import android.text.TextUtils;
import android.view.View;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.animation.AnimationInfo;
import com.lynx.tasm.behavior.ui.LynxUI;
import java.lang.ref.WeakReference;
import java.util.Iterator;
import java.util.Map;
import java.util.WeakHashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * HeroTransitionManager
 * <p>
 * How to use:
 * (1) register a anim
 * HeroTransitionManager.inst().registerSharedElement
 * (2) play exit anim
 * HeroTransitionManager.inst().executeExitAnim
 * (3) unregister
 * HeroTransitionManager.inst().unRegisterSharedElement
 */
public class HeroTransitionManager {
  private static final String TAG = "HeroTransitionManager";

  private ConcurrentHashMap<String, WeakReference<View>> elementsMap = new ConcurrentHashMap<>();
  private WeakHashMap<LynxUI, AnimationInfo> mExitTransitionMap = new WeakHashMap<>();
  private WeakHashMap<LynxUI, AnimationInfo> mEnterTransitionMap = new WeakHashMap<>();
  private WeakHashMap<LynxUI, AnimationInfo> mResumeTransitionMap = new WeakHashMap<>();
  private WeakHashMap<LynxUI, AnimationInfo> mPauseTransitionMap = new WeakHashMap<>();

  private WeakHashMap<LynxUI, String> mHasSharedElementLynxUIMap = new WeakHashMap<>();
  private volatile boolean mEnableSharedTransition = true;

  public boolean isSharedTransitionEnable() {
    return mEnableSharedTransition;
  }

  public void setSharedTransitionEnable(boolean mEnableSharedTransition) {
    this.mEnableSharedTransition = mEnableSharedTransition;
  }

  private static final class Holder {
    private static HeroTransitionManager sInstance = new HeroTransitionManager();
  }

  public static HeroTransitionManager inst() {
    return HeroTransitionManager.Holder.sInstance;
  }

  public synchronized void registerSharedElement(View v, String tag) {
    if (!TextUtils.isEmpty(tag) && v != null) {
      elementsMap.put(tag, new WeakReference<>(v));
    }
  }

  public synchronized void unRegisterSharedElement(String tag) {
    for (Map.Entry<String, WeakReference<View>> entry : elementsMap.entrySet()) {
      String key = entry.getKey();
      if (key != null && key.equals(tag)) {
        elementsMap.remove(key);
        break;
      }
    }
  }

  public synchronized void unRegisterSharedElement(View v) {
    for (Map.Entry<String, WeakReference<View>> entry : elementsMap.entrySet()) {
      WeakReference<View> ref = entry.getValue();
      if (ref != null && ref.get() == v) {
        elementsMap.remove(entry.getKey());
        break;
      }
    }
  }

  protected synchronized void registerHasSharedElementLynxUI(
      LynxUI lynxUI, String sharedElementName) {
    mHasSharedElementLynxUIMap.put(lynxUI, sharedElementName);
  }

  @Nullable
  private View getSharedElementByTag(String tag) {
    WeakReference<View> v = elementsMap.get(tag);
    if (v != null) {
      return v.get();
    }
    return null;
  }

  public synchronized View getSharedElementByTag(String tag, LynxUI lynxUI) {
    View v = getSharedElementByTag(tag);
    if (v == null) {
      for (Map.Entry<LynxUI, String> entry : mHasSharedElementLynxUIMap.entrySet()) {
        LynxUI tmp = entry.getKey();
        if (lynxUI != tmp && entry.getValue().equals(tag)) {
          return tmp.getView();
        }
      }
    }
    return v;
  }

  /* --------Anim----------- */
  public synchronized void registerEnterAnim(LynxUI lynxUI, AnimationInfo transitionName) {
    mEnterTransitionMap.put(lynxUI, transitionName);
    lynxUI.setEnterAnim(transitionName);
  }

  public synchronized void registerExitAnim(LynxUI lynxUI, AnimationInfo transitionName) {
    mExitTransitionMap.put(lynxUI, transitionName);
    lynxUI.setExitAnim(transitionName);
  }

  public synchronized void registerResumeAnim(LynxUI lynxUI, AnimationInfo transitionName) {
    mResumeTransitionMap.put(lynxUI, transitionName);
    lynxUI.setResumeAnim(transitionName);
  }

  public synchronized void registerPauseAnim(LynxUI lynxUI, AnimationInfo transitionName) {
    mPauseTransitionMap.put(lynxUI, transitionName);
    lynxUI.setPauseAnim(transitionName);
  }

  /**
   * @param lynxView the view to enter
   * @param listener listener triggered when all anim done
   */
  public void executeEnterAnim(LynxView lynxView, final LynxViewEnterFinishListener listener) {
    final AtomicInteger flag = new AtomicInteger(0);
    for (LynxUI lynxUI : mEnterTransitionMap.keySet()) {
      if (lynxUI.getLynxContext().getUIBody().getBodyView() == lynxView) {
        flag.incrementAndGet();
        lynxUI.execEnterAnim(new LynxViewEnterFinishListener() {
          @Override
          public void onLynxViewEntered() {
            if (flag.decrementAndGet() == 0 && listener != null) {
              listener.onLynxViewEntered();
            }
          }
        });
      }
    }
    if (flag.get() == 0 && listener != null) {
      listener.onLynxViewEntered();
    }
  }

  /**
   * @param lynxView the view to exit
   * @param listener listener triggered when all anim done
   */
  public void executeExitAnim(LynxView lynxView, final LynxViewExitFinishListener listener) {
    final AtomicInteger flag = new AtomicInteger(0);
    for (LynxUI lynxUI : mExitTransitionMap.keySet()) {
      if (lynxUI.getLynxContext().getUIBody().getBodyView() == lynxView) {
        flag.incrementAndGet();
        lynxUI.execExitAnim(new LynxViewExitFinishListener() {
          @Override
          public void onLynxViewExited() {
            if (flag.decrementAndGet() == 0 && listener != null) {
              listener.onLynxViewExited();
            }
          }
        });
      }
    }
    if (flag.get() == 0 && listener != null) {
      listener.onLynxViewExited();
    }
  }

  public void executeResumeAnim(LynxView lynxView) {
    for (LynxUI lynxUI : mResumeTransitionMap.keySet()) {
      if (lynxUI.getLynxContext().getUIBody().getBodyView() == lynxView) {
        lynxUI.execResumeAnim();
      }
    }
  }

  public void executePauseAnim(LynxView lynxView) {
    for (LynxUI lynxUI : mPauseTransitionMap.keySet()) {
      if (lynxUI.getLynxContext().getUIBody().getBodyView() == lynxView) {
        lynxUI.execPauseAnim();
      }
    }
  }

  public void onLynxViewDestroy(LynxView lynxView) {
    clearLynxViewRegisters(mHasSharedElementLynxUIMap, lynxView);
    clearLynxViewRegisters(mEnterTransitionMap, lynxView);
    clearLynxViewRegisters(mExitTransitionMap, lynxView);
    clearLynxViewRegisters(mPauseTransitionMap, lynxView);
    clearLynxViewRegisters(mResumeTransitionMap, lynxView);
  }

  private <T> void clearLynxViewRegisters(Map<LynxUI, T> map, LynxView lynxView) {
    for (Iterator<Map.Entry<LynxUI, T>> it = map.entrySet().iterator(); it.hasNext();) {
      Map.Entry<LynxUI, T> item = it.next();
      LynxUI ui = item.getKey();
      if (ui != null && ui.getLynxContext().getUIBody().getBodyView() == lynxView) {
        it.remove();
      }
    }
  }

  public interface LynxViewEnterFinishListener {
    void onLynxViewEntered();
  }

  public interface LynxViewExitFinishListener {
    void onLynxViewExited();
  }
}
