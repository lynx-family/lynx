// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation.keyframe;

import static com.lynx.tasm.behavior.StyleConstants.ANIMATIONPLAYSTATE_PAUSED;
import static com.lynx.tasm.behavior.StyleConstants.ANIMATIONPLAYSTATE_RUNNING;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_ROTATE;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_ROTATE_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_ROTATE_Y;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_ROTATE_Z;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SCALE;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SCALE_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SCALE_Y;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_3d;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_Y;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_Z;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ArgbEvaluator;
import android.animation.Keyframe;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.animation.ValueAnimator;
import android.os.Build;
import android.view.View;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.tasm.animation.AnimationInfo;
import com.lynx.tasm.animation.InterpolatorFactory;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.utils.BackgroundManager;
import com.lynx.tasm.behavior.ui.utils.TransformOrigin;
import com.lynx.tasm.behavior.ui.utils.TransformProps;
import com.lynx.tasm.behavior.ui.utils.TransformRaw;
import com.lynx.tasm.event.LynxCustomEvent;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

// Some acronyms are used as the letters are so long.
// kf is keyframe, opa is opacity, tran is translation or transform
// rot is rotation, sca is scale, bColor is backgroundColor

public class LynxKeyframeAnimator {
  private enum LynxAnimationPropertyType {
    TRANSLATE_X,
    TRANSLATE_Y,
    TRANSLATE_Z,
    ROTATE_X,
    ROTATE_Y,
    ROTATE_Z,
    SCALE_X,
    SCALE_Y,
    OPACITY,
    BG_COLOR;
  }

  public static final String sAlphaStr = "Alpha";
  public static final String sTransformStr = "Transform";
  public static final String sBackgroundColorStr = "BackgroundColor";

  private static final String sTranslationXStr = "TranslationX";
  private static final String sTranslationYStr = "TranslationY";
  private static final String sTranslationZStr = "TranslationZ";
  private static final String sRotationZStr = "Rotation";
  private static final String sRotationXStr = "RotationX";
  private static final String sRotationYStr = "RotationY";
  private static final String sScaleXStr = "ScaleX";
  private static final String sScaleYStr = "ScaleY";
  private static final String sColorStr = "Color";

  public enum LynxKFAnimatorState { IDLE, RUNNING, PAUSED, CANCELED, DESTROYED }

  private class KeyframeParsedData {
    public ArrayList<Keyframe> mOpaKfList = new ArrayList<>();
    public ArrayList<Keyframe> mTranXKfList = new ArrayList<>();
    public ArrayList<Keyframe> mTranYKfList = new ArrayList<>();
    public ArrayList<Keyframe> mTranZKfList = new ArrayList<>();
    public ArrayList<Keyframe> mRotZKfList = new ArrayList<>();
    public ArrayList<Keyframe> mRotXKfList = new ArrayList<>();
    public ArrayList<Keyframe> mRotYKfList = new ArrayList<>();
    public ArrayList<Keyframe> mScaXKfList = new ArrayList<>();
    public ArrayList<Keyframe> mScaYKfList = new ArrayList<>();
    public ArrayList<Keyframe> mBColorKfList = new ArrayList<>();

    public boolean mHasPercentageTransform = false;
    public boolean mHasTransform = false;

    public Set<LynxAnimationPropertyType> mHasStartKeyframe =
        new HashSet<LynxAnimationPropertyType>();
    public Set<LynxAnimationPropertyType> mHasEndKeyframe =
        new HashSet<LynxAnimationPropertyType>();

    PropertyValuesHolder[] mViewHolders;
    PropertyValuesHolder[] mBGHolders;
  }

  private static class PauseTimeHelper {
    private long mPauseTime = sTimeNotInit;
    public void recordPauseTime() {
      if (mPauseTime == sTimeNotInit) {
        mPauseTime = System.currentTimeMillis();
      }
    }
    public long getPauseDuration() {
      if (mPauseTime == sTimeNotInit) {
        return 0;
      }
      long pauseDuration = System.currentTimeMillis() - mPauseTime;
      mPauseTime = sTimeNotInit;
      return pauseDuration;
    }
  }
  private PauseTimeHelper mPauseTimeHelper = new PauseTimeHelper();

  private static final String TAG = "LynxKeyframeAnimator";

  private WeakReference<LynxUI> mUI;
  private WeakReference<View> mView;

  private HashMap<String, Object> mPropertyOriginValue = new HashMap<String, Object>();

  private static final long sTimeNotInit = -1;
  private long mKeyframeStartTime = sTimeNotInit;

  private KeyframeParsedData mKeyframeParsedData = null;

  private ObjectAnimator[] mInternalAnimators = null;
  private LynxKFAnimatorState mState = LynxKFAnimatorState.IDLE;
  private AnimationInfo mInfo = null;

  public LynxKeyframeAnimator(View view, LynxUI ui) {
    mUI = new WeakReference<>(ui);
    mView = new WeakReference<>(view);
  }

  public void apply(AnimationInfo info) {
    LLog.DCHECK(mInfo == null || info.getName().equals(mInfo.getName()));

    LynxUI ui = getUI();
    if (ui == null) {
      return;
    }
    switch (mState) {
      case IDLE:
      case CANCELED: {
        if (info.isEqualTo(mInfo) && mState == LynxKFAnimatorState.IDLE
            && !shouldReInitTransform()) {
          return;
        }
        // As iteration count is begin from 0 in Android, so don't return directly when iteration
        // count is 0.
        if (info.getIterationCount() < 0 || info.getDuration() <= 0) {
          return;
        }
        applyAnimationInfo(info);
        break;
      }

      case PAUSED:
      case RUNNING: {
        if (info.isEqualTo(mInfo) && !shouldReInitTransform()) {
          return;
        }
        if (info.isOnlyPlayStateChanged(mInfo)) {
          if (mState == LynxKFAnimatorState.PAUSED) {
            resume(info);
          } else {
            pause(info);
          }
        } else {
          cancel();
          apply(info);
        }
        break;
      }
    }
  }

  private void sendCancelEvent() {
    if (mState == LynxKFAnimatorState.CANCELED || mState == LynxKFAnimatorState.RUNNING
        || mState == LynxKFAnimatorState.PAUSED) {
      LynxUI ui = getUI();
      AnimationInfo info = getAnimationInfo();
      String animationName = "";
      if (info != null) {
        animationName = info.getName();
      }
      KeyframeAnimationListener.sendAnimationEvent(
          ui, KeyframeAnimationListener.EVENT_CANCEL, animationName);
    }
  }

  public void destroy() {
    sendCancelEvent();
    cancel();
    restoreAllViewOriginValue();
    mState = LynxKFAnimatorState.DESTROYED;
  }

  private boolean shouldReInitTransform() {
    LynxUI lynxUI = mUI.get();
    return isPercentTransform() && null != lynxUI && lynxUI.hasSizeChanged();
  }

  // UI's property may be changed when keyframe is running, we should notify animator that property
  // has been updated in order to make sure UI's props correct after animator destoryed.
  public void notifyPropertyUpdated(String name, Object value) {
    if (mPropertyOriginValue.containsKey(name)) {
      mPropertyOriginValue.put(name, value);
    }
    if (name.equals(sBackgroundColorStr)) {
      if (mPropertyOriginValue.containsKey(sColorStr)) {
        mPropertyOriginValue.put(sColorStr, value);
      }
    }
  }

  public void onAttach() {
    addListenerToLastAnimator();
  }

  public void onDetach() {
    if (mInternalAnimators != null) {
      for (ObjectAnimator animator : mInternalAnimators) {
        animator.removeAllListeners();
      }
    }
  }

  public void detachFromUI() {
    cancel();
    restoreAllViewOriginValue();
    mUI = null;
    mView = null;
  }

  public void attachToUI(LynxUI ui) {
    mUI = new WeakReference<>(ui);
    mView = new WeakReference<>(ui.getView());
    applyAnimationInfo(mInfo);
  }

  public boolean isRunning() {
    return mState == LynxKFAnimatorState.RUNNING;
  }

  public AnimationInfo getAnimationInfo() {
    return mInfo;
  }

  private boolean isAnimationExpired(AnimationInfo info) {
    if (mKeyframeStartTime == sTimeNotInit) {
      return false;
    }
    // As iteration count is begin from 0 in Android, so iteration count should +1 when get
    // allDuration.
    long allDuration =
        (info.getIterationCount() >= StyleConstants.ANIMATIONITERATIONCOUNT_INFINITE - 1
                ? Long.MAX_VALUE
                : info.getDuration() * (info.getIterationCount() + 1) + info.getDelay());
    return (System.currentTimeMillis() - mKeyframeStartTime >= allDuration);
  }

  // Check whether is percentage transform keyframe.
  private boolean isPercentTransform() {
    if (mKeyframeParsedData != null && mKeyframeParsedData.mHasPercentageTransform) {
      return true;
    }
    TransformOrigin origin = getUI().getTransformOriginStr();
    if (origin != null && origin.hasPercent()) {
      return true;
    }
    return false;
  }

  // state can only be changed by these five functions(pause, resume, cancel, run, finish).
  private void pause(AnimationInfo info) {
    LLog.DCHECK(info.getPlayState() == ANIMATIONPLAYSTATE_PAUSED);
    LLog.DCHECK(mState == LynxKFAnimatorState.RUNNING);

    mState = LynxKFAnimatorState.PAUSED;
    if (mInternalAnimators != null) {
      // only work when android version >= 19
      if (Build.VERSION.SDK_INT >= 19) {
        for (ObjectAnimator animator : mInternalAnimators) {
          animator.pause();
        }
      }
    }
    if (!isAnimationExpired(info)) {
      mPauseTimeHelper.recordPauseTime();
    }
    mInfo = info;
  }

  private void resume(AnimationInfo info) {
    LLog.DCHECK(info.getPlayState() == ANIMATIONPLAYSTATE_RUNNING);
    LLog.DCHECK(mState == LynxKFAnimatorState.PAUSED);

    mState = LynxKFAnimatorState.RUNNING;
    if (mInternalAnimators != null) {
      // only work when android version >= 19
      if (Build.VERSION.SDK_INT >= 19) {
        for (ObjectAnimator animator : mInternalAnimators) {
          animator.resume();
        }
      }
    }
    mKeyframeStartTime += mPauseTimeHelper.getPauseDuration();
    mInfo = info;
  }

  private void cancel() {
    if (mState != LynxKFAnimatorState.RUNNING && mState != LynxKFAnimatorState.PAUSED) {
      return;
    }
    if (mInternalAnimators != null && mInternalAnimators.length > 0) {
      // only work when android version >= 19
      if (Build.VERSION.SDK_INT >= 19) {
        for (ObjectAnimator animator : mInternalAnimators) {
          animator.cancel();
        }
      }
    }
    mInternalAnimators = null;
    mState = LynxKFAnimatorState.CANCELED;
  }

  private void run() {
    LLog.DCHECK(mState == LynxKFAnimatorState.IDLE || mState == LynxKFAnimatorState.CANCELED);
    mState = LynxKFAnimatorState.RUNNING;
  }

  private void finish() {
    LLog.DCHECK(mState == LynxKFAnimatorState.RUNNING);
    mState = LynxKFAnimatorState.IDLE;
  }

  private void applyAnimationInfo(AnimationInfo info) {
    LLog.DCHECK(mState == LynxKFAnimatorState.IDLE || mState == LynxKFAnimatorState.CANCELED);

    View view = getView();
    LynxUI ui = getUI();
    if (view == null || ui == null || null == info) {
      return;
    }

    // Only need to parse keyframe once.
    if (mKeyframeParsedData == null || shouldReInitTransform()) {
      if (!parseKeyframes(ui, info)) {
        LLog.e("Lynx", "Keyframes input error.");
        return;
      }
    }

    // We will use 'mKeyframeStartTime' to get play time later, so should update
    // 'mKeyframeStartTime' ahead here.
    if (mKeyframeStartTime != sTimeNotInit && info.getPlayState() == ANIMATIONPLAYSTATE_RUNNING) {
      mKeyframeStartTime += mPauseTimeHelper.getPauseDuration();
    }

    int animatorCount = 0, arraySize = 1;
    BackgroundDrawable bgDrawable = getBackgroundDrawable();
    if (bgDrawable != null) {
      arraySize = 2;
    }
    ObjectAnimator[] animatorArray = new ObjectAnimator[arraySize];
    for (int i = 0; i < arraySize; ++i) {
      PropertyValuesHolder[] holders =
          (i == 1 ? mKeyframeParsedData.mBGHolders : mKeyframeParsedData.mViewHolders);
      if (holders == null)
        continue;

      Object targetObject = (i == 1 ? bgDrawable : view);
      ObjectAnimator animator = ObjectAnimator.ofPropertyValuesHolder(targetObject, holders);
      animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
        @Override
        public void onAnimationUpdate(ValueAnimator animation) {
          LynxUI lynxUI = mUI.get();
          if (lynxUI != null && lynxUI.getParent() instanceof UIShadowProxy) {
            ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
          }
        }
      });
      animatorArray[i] = animator;
      animator.setDuration(info.getDuration());
      animator.setRepeatCount(info.getIterationCount());
      if (AnimationInfo.isDirectionAlternate(info)) {
        animator.setRepeatMode(ValueAnimator.REVERSE);
      } else {
        animator.setRepeatMode(ValueAnimator.RESTART);
      }
      animator.setInterpolator(InterpolatorFactory.getInterpolator(info));
      if (info.getDelay() != 0 && AnimationInfo.isFillModeBackwards(info)) {
        // Use another animation's very first period to do this trick.
        // Better method can be used.
        ObjectAnimator oneFrameAnimator = animator.clone();
        oneFrameAnimator.setDuration(10000000);
        oneFrameAnimator.addListener(new StartListener());
        oneFrameAnimator.start();
      }
      ++animatorCount;
      if (info.getDelay() >= 0) {
        animator.setStartDelay(info.getDelay());
      } else {
        animator.setCurrentPlayTime(info.getDelay() * -1);
      }
      if (mKeyframeStartTime != sTimeNotInit) {
        long playTime = System.currentTimeMillis() - mKeyframeStartTime;
        if (playTime < info.getDelay()) {
          animator.setStartDelay(info.getDelay() - playTime);
        } else {
          animator.setCurrentPlayTime(playTime - info.getDelay());
        }
      }
      animator.start();
    }

    if (animatorCount == 0) {
      mInternalAnimators = null;
    } else if (animatorCount == animatorArray.length) {
      mInternalAnimators = animatorArray;
    } else {
      mInternalAnimators = new ObjectAnimator[animatorCount];
      int index = 0;
      for (ObjectAnimator animator : animatorArray) {
        if (animator != null) {
          mInternalAnimators[index++] = animator;
        }
      }
    }

    // add animation listener
    addListenerToLastAnimator();

    // mKeyframeStartTime only need to be updated once.
    if (mKeyframeStartTime == sTimeNotInit) {
      mKeyframeStartTime = System.currentTimeMillis();
    }

    if (!isAnimationExpired(info)) {
      // Send animation start event
      // only send start event when state is IDLE.
      if (mState == LynxKFAnimatorState.IDLE) {
        KeyframeAnimationListener.sendAnimationEvent(
            ui, KeyframeAnimationListener.EVENT_START, info.getName());
      }

      // Change animator state
      run();
      if (info.getPlayState() == ANIMATIONPLAYSTATE_PAUSED) {
        pause(info);
      }
    }
    mInfo = info;
  }

  @Nullable
  private LynxUI getUI() {
    return mUI.get();
  }

  @Nullable
  private View getView() {
    return mView.get();
  }

  private PropertyValuesHolder calHolder(ArrayList<Keyframe> list, String string) {
    Keyframe[] array = new Keyframe[list.size()];
    array = list.toArray(array);
    return PropertyValuesHolder.ofKeyframe(string, array);
  }

  private void calStartEnd(
      float time, LynxAnimationPropertyType type, KeyframeParsedData keyframeParsedData) {
    if (time == 0f) {
      keyframeParsedData.mHasStartKeyframe.add(type);
    }
    if (time == 1f) {
      keyframeParsedData.mHasEndKeyframe.add(type);
    }
  }

  private void restoreAllViewOriginValue() {
    View view = getView();
    LynxUI ui = getUI();
    if (ui == null || view == null) {
      return;
    }
    for (Map.Entry<String, Object> entry : mPropertyOriginValue.entrySet()) {
      switch (entry.getKey()) {
        case sAlphaStr:
          view.setAlpha((Float) entry.getValue());
          break;
        case sTransformStr:
          if (ui.getBackgroundManager() != null) {
            ui.getBackgroundManager().setTransform((List<TransformRaw>) entry.getValue());
          }
          break;
        case sBackgroundColorStr:
          view.setBackgroundColor((Integer) entry.getValue());
          break;
        case sColorStr:
          BackgroundDrawable bgDrawable = getBackgroundDrawable();
          bgDrawable.setColor((Integer) entry.getValue());
          break;
      }
    }
  }

  private void saveViewOriginValue(String propertyName, Object value) {
    // Just need to update origin value once.
    if (!mPropertyOriginValue.containsKey(propertyName)) {
      mPropertyOriginValue.put(propertyName, value);
    }
  }

  private boolean parseKeyframes(LynxUI ui, AnimationInfo info) {
    ReadableMap keyframesMap = ui.getKeyframes(info.getName());
    if (keyframesMap == null) {
      return false;
    }
    KeyframeParsedData keyframeParsedData = new KeyframeParsedData();

    ReadableMapKeySetIterator it = keyframesMap.keySetIterator();
    while (it.hasNextKey()) {
      String timeStr = it.nextKey();
      float currentMoment = Float.parseFloat(timeStr);
      if (AnimationInfo.isDirectionReverse(info)) {
        currentMoment = 1f - currentMoment;
      }
      ReadableMap currentStyle = keyframesMap.getMap(timeStr);
      ReadableMapKeySetIterator styleIt = currentStyle.keySetIterator();
      while (styleIt.hasNextKey()) {
        String styleName = styleIt.nextKey();
        if (styleName.equals("opacity")) {
          saveViewOriginValue(sAlphaStr, getView().getAlpha());
          calStartEnd(currentMoment, LynxAnimationPropertyType.OPACITY, keyframeParsedData);
          float styleValue = (float) currentStyle.getDouble(styleName);
          if (styleValue < 0 || styleValue > 1) {
            return false;
          }
          keyframeParsedData.mOpaKfList.add(Keyframe.ofFloat(currentMoment, styleValue));
        } else if (styleName.equals("transform")) {
          saveViewOriginValue(sTransformStr, ui.getTransformRaws());
          List<TransformRaw> transforms =
              TransformRaw.toTransformRaw(currentStyle.getArray(styleName));
          final TransformProps transformProps = TransformProps.processTransform(transforms,
              ui.getLynxContext().getUIBody().getFontSize(), ui.getFontSize(),
              ui.getLynxContext().getUIBody().getLatestWidth(),
              ui.getLynxContext().getUIBody().getLatestHeight(), ui.getLatestWidth(),
              ui.getLatestHeight());
          if (transformProps == null) {
            return false;
          }
          keyframeParsedData.mHasTransform = true;
          if (TransformRaw.hasPercent(transforms)) {
            keyframeParsedData.mHasPercentageTransform = true;
          }

          calStartEnd(currentMoment, LynxAnimationPropertyType.TRANSLATE_X, keyframeParsedData);
          keyframeParsedData.mTranXKfList.add(
              Keyframe.ofFloat(currentMoment, transformProps.getTranslationX()));

          calStartEnd(currentMoment, LynxAnimationPropertyType.TRANSLATE_Y, keyframeParsedData);
          keyframeParsedData.mTranYKfList.add(
              Keyframe.ofFloat(currentMoment, transformProps.getTranslationY()));

          if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            ui.getView().setOutlineProvider(null); // disable shadow
            calStartEnd(currentMoment, LynxAnimationPropertyType.TRANSLATE_Z, keyframeParsedData);
            keyframeParsedData.mTranZKfList.add(
                Keyframe.ofFloat(currentMoment, transformProps.getTranslationZ()));
          }

          calStartEnd(currentMoment, LynxAnimationPropertyType.ROTATE_Z, keyframeParsedData);
          keyframeParsedData.mRotZKfList.add(
              Keyframe.ofFloat(currentMoment, transformProps.getRotation()));

          calStartEnd(currentMoment, LynxAnimationPropertyType.ROTATE_X, keyframeParsedData);
          keyframeParsedData.mRotXKfList.add(
              Keyframe.ofFloat(currentMoment, transformProps.getRotationX()));

          calStartEnd(currentMoment, LynxAnimationPropertyType.ROTATE_Y, keyframeParsedData);
          keyframeParsedData.mRotYKfList.add(
              Keyframe.ofFloat(currentMoment, transformProps.getRotationY()));

          calStartEnd(currentMoment, LynxAnimationPropertyType.SCALE_X, keyframeParsedData);
          keyframeParsedData.mScaXKfList.add(
              Keyframe.ofFloat(currentMoment, transformProps.getScaleX()));

          calStartEnd(currentMoment, LynxAnimationPropertyType.SCALE_Y, keyframeParsedData);
          keyframeParsedData.mScaYKfList.add(
              Keyframe.ofFloat(currentMoment, transformProps.getScaleY()));

        } else if (styleName.equals("background-color")) {
          if (getBackgroundDrawable() == null) {
            saveViewOriginValue(sBackgroundColorStr, ui.getBackgroundColor());
          } else {
            saveViewOriginValue(sColorStr, ui.getBackgroundColor());
          }
          calStartEnd(currentMoment, LynxAnimationPropertyType.BG_COLOR, keyframeParsedData);
          int styleValue = currentStyle.getInt(styleName);
          keyframeParsedData.mBColorKfList.add(Keyframe.ofInt(currentMoment, styleValue));
        }
      }
    }

    int arraySize = 1;
    if (getBackgroundDrawable() != null) {
      arraySize = 2;
    }
    for (int i = 0; i < arraySize; ++i) {
      PropertyValuesHolder[] holders = calKfHolder(i, arraySize, keyframeParsedData);
      if (holders == null)
        continue;
      if (i == 0) {
        keyframeParsedData.mViewHolders = holders;
      } else if (i == 1) {
        keyframeParsedData.mBGHolders = holders;
      }
    }
    mKeyframeParsedData = keyframeParsedData;
    return true;
  }

  private PropertyValuesHolder[] calKfHolder(
      int index, int count, KeyframeParsedData keyframeParsedData) {
    View view = getView();
    LynxUI ui = getUI();
    if (view == null || ui == null) {
      return null;
    }
    ArrayList<PropertyValuesHolder> holderList = new ArrayList<>();
    PropertyValuesHolder holder;

    Comparator<Keyframe> comparator = new Comparator<Keyframe>() {
      @Override
      public int compare(Keyframe k1, Keyframe k2) {
        return Float.compare(k1.getFraction(), k2.getFraction());
      }
    };

    if (keyframeParsedData.mOpaKfList.size() != 0 && index == 0) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.OPACITY)) {
        keyframeParsedData.mOpaKfList.add(Keyframe.ofFloat(0, view.getAlpha()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.OPACITY)) {
        keyframeParsedData.mOpaKfList.add(Keyframe.ofFloat(1, view.getAlpha()));
      }
      Collections.sort(keyframeParsedData.mOpaKfList, comparator);
      holder = calHolder(keyframeParsedData.mOpaKfList, sAlphaStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mTranXKfList.size() != 0 && index == 0) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.TRANSLATE_X)) {
        keyframeParsedData.mTranXKfList.add(Keyframe.ofFloat(0, view.getTranslationX()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.TRANSLATE_X)) {
        keyframeParsedData.mTranXKfList.add(Keyframe.ofFloat(1, view.getTranslationX()));
      }
      Collections.sort(keyframeParsedData.mTranXKfList, comparator);
      holder = calHolder(keyframeParsedData.mTranXKfList, sTranslationXStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mTranYKfList.size() != 0 && index == 0) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.TRANSLATE_Y)) {
        keyframeParsedData.mTranYKfList.add(Keyframe.ofFloat(0, view.getTranslationY()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.TRANSLATE_Y)) {
        keyframeParsedData.mTranYKfList.add(Keyframe.ofFloat(1, view.getTranslationY()));
      }
      Collections.sort(keyframeParsedData.mTranYKfList, comparator);
      holder = calHolder(keyframeParsedData.mTranYKfList, sTranslationYStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mTranZKfList.size() != 0 && index == 0) {
      float originalTranZ = 0;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
        originalTranZ = view.getTranslationZ();
      }
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.TRANSLATE_Z)) {
        keyframeParsedData.mTranZKfList.add(Keyframe.ofFloat(0, originalTranZ));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.TRANSLATE_Z)) {
        keyframeParsedData.mTranZKfList.add(Keyframe.ofFloat(1, originalTranZ));
      }
      Collections.sort(keyframeParsedData.mTranZKfList, comparator);
      holder = calHolder(keyframeParsedData.mTranZKfList, sTranslationZStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mRotZKfList.size() != 0 && index == 0) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.ROTATE_Z)) {
        keyframeParsedData.mRotZKfList.add(Keyframe.ofFloat(0, view.getRotation()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.ROTATE_Z)) {
        keyframeParsedData.mRotZKfList.add(Keyframe.ofFloat(1, view.getRotation()));
      }
      Collections.sort(keyframeParsedData.mRotZKfList, comparator);
      holder = calHolder(keyframeParsedData.mRotZKfList, sRotationZStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mRotXKfList.size() != 0 && index == 0) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.ROTATE_X)) {
        keyframeParsedData.mRotXKfList.add(Keyframe.ofFloat(0, view.getRotationX()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.ROTATE_X)) {
        keyframeParsedData.mRotXKfList.add(Keyframe.ofFloat(1, view.getRotationX()));
      }
      Collections.sort(keyframeParsedData.mRotXKfList, comparator);
      holder = calHolder(keyframeParsedData.mRotXKfList, sRotationXStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mRotYKfList.size() != 0 && index == 0) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.ROTATE_Y)) {
        keyframeParsedData.mRotYKfList.add(Keyframe.ofFloat(0, view.getRotationY()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.ROTATE_Y)) {
        keyframeParsedData.mRotYKfList.add(Keyframe.ofFloat(1, view.getRotationY()));
      }
      Collections.sort(keyframeParsedData.mRotYKfList, comparator);
      holder = calHolder(keyframeParsedData.mRotYKfList, sRotationYStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mScaXKfList.size() != 0 && index == 0) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.SCALE_X)) {
        keyframeParsedData.mScaXKfList.add(Keyframe.ofFloat(0, view.getScaleX()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.SCALE_X)) {
        keyframeParsedData.mScaXKfList.add(Keyframe.ofFloat(1, view.getScaleX()));
      }
      Collections.sort(keyframeParsedData.mScaXKfList, comparator);
      holder = calHolder(keyframeParsedData.mScaXKfList, sScaleXStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mScaYKfList.size() != 0 && index == 0) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.SCALE_Y)) {
        keyframeParsedData.mScaYKfList.add(Keyframe.ofFloat(0, view.getScaleY()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.SCALE_Y)) {
        keyframeParsedData.mScaYKfList.add(Keyframe.ofFloat(1, view.getScaleY()));
      }
      Collections.sort(keyframeParsedData.mScaYKfList, comparator);
      holder = calHolder(keyframeParsedData.mScaYKfList, sScaleYStr);
      holderList.add(holder);
    }

    if (keyframeParsedData.mBColorKfList.size() != 0 && (index + 1 == count)) {
      if (!keyframeParsedData.mHasStartKeyframe.contains(LynxAnimationPropertyType.BG_COLOR)) {
        keyframeParsedData.mBColorKfList.add(Keyframe.ofInt(0, ui.getBackgroundColor()));
      }
      if (!keyframeParsedData.mHasEndKeyframe.contains(LynxAnimationPropertyType.BG_COLOR)) {
        keyframeParsedData.mBColorKfList.add(Keyframe.ofInt(1, ui.getBackgroundColor()));
      }
      Collections.sort(keyframeParsedData.mBColorKfList, comparator);
      if (index == 0) {
        holder = calHolder(keyframeParsedData.mBColorKfList, sBackgroundColorStr);
      } else {
        // use BackgroundDrawable, so we change its color
        holder = calHolder(keyframeParsedData.mBColorKfList, sColorStr);
      }
      holder.setEvaluator(new ArgbEvaluator());
      holderList.add(holder);
    }

    if (holderList.size() != 0) {
      PropertyValuesHolder[] array = new PropertyValuesHolder[holderList.size()];
      array = holderList.toArray(array);
      return array;
    } else {
      return null;
    }
  }

  private BackgroundDrawable getBackgroundDrawable() {
    LynxUI ui = getUI();
    if (null == ui) {
      return null;
    }
    BackgroundManager mgr = ui.getBackgroundManager();
    if (mgr != null) {
      return mgr.getDrawable();
    }
    return null;
  }

  private void addListenerToLastAnimator() {
    if (mInternalAnimators == null) {
      return;
    }
    int length = mInternalAnimators.length;
    if (length > 0) {
      // as all animator's time info is the same, we need only one end listener
      // We should add listener to last animator to ensure all animators completed when
      // onAnimationEnd has been invoked.
      mInternalAnimators[length - 1].addListener(new KeyframeAnimationListener(this));
    }
  }

  private static class StartListener extends AnimatorListenerAdapter {
    @Override
    public void onAnimationStart(Animator animation) {
      super.onAnimationStart(animation);
      animation.cancel();
    }
  }

  private static class KeyframeAnimationListener extends AnimatorListenerAdapter {
    private static final String EVENT_START = "animationstart";
    private static final String EVENT_ITERATION = "animationiteration";
    private static final String EVENT_END = "animationend";
    private static final String EVENT_CANCEL = "animationcancel";
    private static Map<String, Object> sEventParams = new HashMap<String, Object>();
    static {
      sEventParams.put("animation_type", "keyframe-animation");
      sEventParams.put("animation_name", "");
    }
    private static void sendAnimationEvent(
        @Nullable LynxUI ui, String event_name, String animation_name) {
      if (ui == null) {
        return;
      }
      if (ui.getEvents() != null && ui.getEvents().containsKey(event_name)) {
        sEventParams.put("animation_name", animation_name);
        ui.getLynxContext().getEventEmitter().sendCustomEvent(
            new LynxCustomEvent(ui.getSign(), event_name, sEventParams));
      }
    }

    WeakReference<LynxKeyframeAnimator> mLynxAnimatorRef;
    public KeyframeAnimationListener(LynxKeyframeAnimator lynxAnimator) {
      this.mLynxAnimatorRef = new WeakReference<>(lynxAnimator);
    }

    @Override
    public void onAnimationStart(Animator animator) {
      super.onAnimationStart(animator);
    }

    @Override
    public void onAnimationCancel(Animator animator) {
      super.onAnimationCancel(animator);
      if (animator != null) {
        // If animator be cancel, it's onAnimationEnd() callback also will be triggered.
        // Remove all listeners here to prevent onAnimationEnd() from being triggered.
        animator.removeAllListeners();
      }
    }

    @Override
    public void onAnimationRepeat(Animator animator) {
      super.onAnimationRepeat(animator);
      LynxKeyframeAnimator lynxAnimator = mLynxAnimatorRef.get();
      if (lynxAnimator == null) {
        return;
      }
      String animationName = "";
      AnimationInfo info = lynxAnimator.getAnimationInfo();
      if (info != null) {
        animationName = info.getName();
      }
      sendAnimationEvent(lynxAnimator.getUI(), EVENT_ITERATION, animationName);
    }

    @Override
    public void onAnimationEnd(Animator animator) {
      super.onAnimationEnd(animator);

      LynxKeyframeAnimator lynxAnimator = mLynxAnimatorRef.get();
      if (lynxAnimator == null) {
        return;
      }
      String animationName = "";
      AnimationInfo info = lynxAnimator.getAnimationInfo();
      if (info != null) {
        animationName = info.getName();
      }

      // Send animation end event and change animator's state only if animator's state is running.
      if (lynxAnimator.isRunning()) {
        LynxUI ui = lynxAnimator.getUI();
        sendAnimationEvent(ui, EVENT_END, animationName);

        if (null != ui) {
          ui.onAnimationEnd(animationName);
        }
        lynxAnimator.finish();
      }

      // Restore view's styles
      if (info != null && !AnimationInfo.isFillModeForwards(info)) {
        lynxAnimator.restoreAllViewOriginValue();
      }

      lynxAnimator.mInternalAnimators = null;
    }
  }

  public interface LynxAnimationListener {
    void onAnimationEnd(String value);
  }
}
