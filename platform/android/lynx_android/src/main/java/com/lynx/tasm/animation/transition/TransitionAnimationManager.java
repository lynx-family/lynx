// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation.transition;

import static com.lynx.tasm.animation.AnimationConstant.ALL_PLATFORM_TRANSITION_PROPS_ARR;
import static com.lynx.tasm.animation.AnimationConstant.PROP_BOTTOM;
import static com.lynx.tasm.animation.AnimationConstant.PROP_HEIGHT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_LEFT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_RIGHT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_TOP;
import static com.lynx.tasm.animation.AnimationConstant.PROP_WIDTH;
import static com.lynx.tasm.animation.AnimationConstant.TRAN_PROP_ALL;
import static com.lynx.tasm.animation.AnimationConstant.TRAN_PROP_LEGACY_ALL_1;
import static com.lynx.tasm.animation.AnimationConstant.TRAN_PROP_LEGACY_ALL_2;
import static com.lynx.tasm.animation.AnimationConstant.TRAN_PROP_LEGACY_ALL_3;
import static com.lynx.tasm.behavior.StyleConstants.VISIBILITY_VISIBLE;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ArgbEvaluator;
import android.animation.FloatEvaluator;
import android.animation.ValueAnimator;
import android.graphics.Rect;
import android.os.Build;
import android.util.SparseArray;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.animation.AnimationConstant;
import com.lynx.tasm.animation.AnimationInfo;
import com.lynx.tasm.animation.InterpolatorFactory;
import com.lynx.tasm.animation.PropertyFactory;
import com.lynx.tasm.behavior.PropertyIDConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.behavior.ui.utils.TransformProps;
import com.lynx.tasm.behavior.ui.utils.TransformRaw;
import com.lynx.tasm.event.LynxCustomEvent;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TransitionAnimationManager {
  private int mLatestX = 0;
  private int mLatestY = 0;
  private int mLatestWidth = 0;
  private int mLatestHeight = 0;

  private boolean mHasVisibilityTransition;
  private boolean mHasAlphaTransition;
  private final LynxBaseUI mDelegateUI;

  @NonNull private final HashMap<Integer, Animator> mRunningAnimators;
  @NonNull private final HashMap<Integer, Animator> mIdleAnimators;

  @NonNull private SparseArray<AnimationInfo> mAnimationInfo;

  public TransitionAnimationManager(@Nullable LynxBaseUI ui) {
    mAnimationInfo = new SparseArray<>();
    mDelegateUI = ui;
    mRunningAnimators = new HashMap<>();
    mIdleAnimators = new HashMap<>();
  }

  public static boolean hasTransitionAnimation(StylesDiffMap map) {
    return map.hasKey(AnimationConstant.TRANSITION) || map.hasKey(PropertyIDConstants.Transition);
  }

  public boolean isShouldTransitionType(int type) {
    if (type == TRAN_PROP_LEGACY_ALL_1 || type == TRAN_PROP_LEGACY_ALL_2
        || type == TRAN_PROP_LEGACY_ALL_3 || type == TRAN_PROP_ALL) {
      return true;
    }
    for (int iter : ALL_PLATFORM_TRANSITION_PROPS_ARR) {
      if (type == iter) {
        return true;
      }
    }
    return false;
  }
  public boolean initializeFromConfig(final @Nullable ReadableMap config) {
    if (config == null) {
      return false;
    }
    ReadableArray tranArr = config.getArray(AnimationConstant.TRANSITION);
    if (tranArr == null) {
      endAllAnimators();
      return false;
    }
    SparseArray<AnimationInfo> oldAnimationInfos = mAnimationInfo;
    mAnimationInfo = new SparseArray<AnimationInfo>();
    // FIXME(liyanbo): this will remove after DrawInfo patch.
    //  see computed_css_style.cc TransitionTolepus.
    for (int i = 0; i < tranArr.size(); i++) {
      ReadableArray array = tranArr.getArray(i);
      int property = array.getInt(0);
      if (!isShouldTransitionType(property)) {
        continue;
      }
      AnimationInfo info = new AnimationInfo();
      info.setProperty(property);
      info.setDuration((long) array.getDouble(1));
      int next = info.setTimingFunction(array, 2);
      info.setDelay((long) array.getDouble(next));
      info.setOrderIndex(i);
      int info_type = info.getProperty();
      if (info_type == TRAN_PROP_LEGACY_ALL_1 || info_type == TRAN_PROP_LEGACY_ALL_2
          || info_type == TRAN_PROP_LEGACY_ALL_3 || info_type == TRAN_PROP_ALL) {
        mAnimationInfo.clear();
        int index = 0;
        for (int value : ALL_PLATFORM_TRANSITION_PROPS_ARR) {
          AnimationInfo item = new AnimationInfo(info);
          item.setOrderIndex(index++);
          item.setProperty(value);
          mAnimationInfo.put(item.getProperty(), item);
        }
        break;
      }
      mAnimationInfo.put(info.getProperty(), info);
    }

    // When both PROP_LEFT and PROP_RIGHT exist, keep the one that was added to mAnimationInfo
    // later.
    AnimationInfo.removeDuplicateAnimation(mAnimationInfo, PROP_LEFT, PROP_RIGHT);

    // When both PROP_TOP and PROP_BOTTOM exist, keep the one that was added to mAnimationInfo
    // later.
    AnimationInfo.removeDuplicateAnimation(mAnimationInfo, PROP_TOP, PROP_BOTTOM);

    // cancel animations that were removed from css
    if (oldAnimationInfos != null) {
      for (int i = 0; i < oldAnimationInfos.size(); i++) {
        int prop = oldAnimationInfos.keyAt(i);
        if (mAnimationInfo.indexOfKey(prop) < 0) {
          endTransitionAnimator(prop);
        }
      }
    }
    return mAnimationInfo.size() != 0;
  }

  public void endAllAnimators() {
    // mRunningAnimators will be modified in animation end callback, which will cause a
    // ConcurrentModificationException. So make a shallow copy here.
    HashMap<Integer, Animator> runningAnimatorsTemp =
        (HashMap<Integer, Animator>) mRunningAnimators.clone();
    for (Animator ani : runningAnimatorsTemp.values()) {
      ani.cancel();
    }
    mRunningAnimators.clear();
    mIdleAnimators.clear();
  }

  public void endAllLayoutAnimators() {
    endTransitionAnimator(PROP_LEFT);
    endTransitionAnimator(PROP_RIGHT);
    endTransitionAnimator(PROP_TOP);
    endTransitionAnimator(PROP_BOTTOM);
    endTransitionAnimator(PROP_HEIGHT);
    endTransitionAnimator(PROP_WIDTH);
  }

  public void endTransitionAnimator(Integer propertyID) {
    Animator transformAni = mRunningAnimators.get(propertyID);
    if (transformAni != null) {
      transformAni.cancel();
      mRunningAnimators.remove(propertyID);
    }
  }

  public boolean containTransition(int property) {
    return mAnimationInfo.size() != 0 && mAnimationInfo.get(property) != null;
  }

  public boolean containLayoutTransition() {
    return mAnimationInfo.size() != 0
        && (containsAnimation(PROP_LEFT) || containsAnimation(PROP_RIGHT)
            || containsAnimation(AnimationConstant.PROP_TOP)
            || containsAnimation(AnimationConstant.PROP_BOTTOM)
            || containsAnimation(AnimationConstant.PROP_WIDTH)
            || containsAnimation(AnimationConstant.PROP_HEIGHT));
  }

  public boolean hasAnimationRunning() {
    return !mRunningAnimators.isEmpty();
  }

  public boolean applyPropertyTransition(
      final LynxBaseUI ui, final int property, final Object value) {
    if (mAnimationInfo.size() == 0 || mAnimationInfo.get(property) == null) {
      return false;
    }

    final LynxBaseUI transitionUI = mDelegateUI != null ? mDelegateUI : ui;
    AnimationInfo info = mAnimationInfo.get(property);
    switch (property) {
      case AnimationConstant.PROP_BACKGROUND_COLOR: {
        final int newColor = (Integer) value;
        ValueAnimator animator = ValueAnimator.ofObject(
            new ArgbEvaluator(), transitionUI.getLynxBackground().getBackgroundColor(), newColor);
        mIdleAnimators.put(AnimationConstant.PROP_BACKGROUND_COLOR, animator);
        animator.setDuration(Math.round(info.getDuration()));
        animator.setInterpolator(InterpolatorFactory.getInterpolator(info));
        animator.setStartDelay(info.getDelay());
        animator.addListener(new TransitionListener(transitionUI, property) {
          @Override
          public void onAnimationEnd(Animator animation) {
            super.onAnimationEnd(animation);
            transitionUI.getLynxBackground().setBackgroundColor(newColor);
            transitionUI.invalidate();
            mRunningAnimators.remove(AnimationConstant.PROP_BACKGROUND_COLOR);
          }
        });
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
          @Override
          public void onAnimationUpdate(ValueAnimator animation) {
            Object value = animation.getAnimatedValue();
            if (null != value) {
              int newValue = (Integer) value;
              transitionUI.getLynxBackground().setBackgroundColor(newValue);
              transitionUI.invalidate();
            }
          }
        });
      } break;

      case AnimationConstant.PROP_TRANSFORM: {
        // TODO: add API in LynxBaseUI to support FlattenUI transition
        if (!(transitionUI instanceof LynxUI)) {
          break;
        }
        final LynxUI lynxUI = (LynxUI) transitionUI;
        List<TransformRaw> rawTransforms = (List<TransformRaw>) value;
        final TransformProps transformProps = TransformProps.processTransform(rawTransforms,
            lynxUI.getLynxContext().getUIBody().getFontSize(), lynxUI.getFontSize(),
            lynxUI.getLynxContext().getUIBody().getLatestWidth(),
            lynxUI.getLynxContext().getUIBody().getLatestHeight(), lynxUI.getLatestWidth(),
            lynxUI.getLatestHeight());
        if (transformProps == null) {
          break;
        }

        final float rawTranslateX = lynxUI.getTranslationX();
        final float rawTranslateY = lynxUI.getTranslationY();
        float translateZ = 0.0f;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
          translateZ = lynxUI.getView().getTranslationZ();
        } else {
          translateZ = lynxUI.getTranslationZ();
        }
        final float rawTranslateZ = translateZ;
        final float rawRotation = lynxUI.getView().getRotation();
        final float rawRotationX = lynxUI.getView().getRotationX();
        final float rawRotationY = lynxUI.getView().getRotationY();
        final float rawScaleX = lynxUI.getView().getScaleX();
        final float rawScaleY = lynxUI.getView().getScaleY();

        ValueAnimator animator = ValueAnimator.ofInt(0);
        mIdleAnimators.put(AnimationConstant.PROP_TRANSFORM, animator);
        animator.setDuration(info.getDuration());
        animator.setInterpolator(InterpolatorFactory.getInterpolator(info));
        animator.setStartDelay(info.getDelay());
        animator.addListener(new TransitionListener(lynxUI, property) {
          @Override
          public void onAnimationEnd(Animator animation) {
            super.onAnimationEnd(animation);
            lynxUI.getView().setTranslationX(transformProps.getTranslationX());
            lynxUI.getView().setTranslationY(transformProps.getTranslationY());
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
              lynxUI.getView().setTranslationZ(transformProps.getTranslationZ());
            }
            if (lynxUI.getParent() instanceof UIShadowProxy) {
              ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
            }
            lynxUI.getView().setRotation(transformProps.getRotation());
            lynxUI.getView().setRotationX(transformProps.getRotationX());
            lynxUI.getView().setRotationY(transformProps.getRotationY());
            lynxUI.getView().setScaleX(transformProps.getScaleX());
            lynxUI.getView().setScaleY(transformProps.getScaleY());
            mRunningAnimators.remove(AnimationConstant.PROP_TRANSFORM);
          }
        });
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
          @Override
          public void onAnimationUpdate(ValueAnimator animation) {
            float fraction = animation.getAnimatedFraction();
            // TODO: use matrix to apply correct value
            float newTransX =
                rawTranslateX + fraction * (transformProps.getTranslationX() - rawTranslateX);
            lynxUI.getView().setTranslationX(newTransX);
            float newTransY =
                rawTranslateY + fraction * (transformProps.getTranslationY() - rawTranslateY);
            lynxUI.getView().setTranslationY(newTransY);
            float newTransZ =
                rawTranslateZ + fraction * (transformProps.getTranslationZ() - rawTranslateZ);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
              lynxUI.getView().setTranslationZ(newTransZ);
            }
            float newRot = rawRotation + fraction * (transformProps.getRotation() - rawRotation);
            lynxUI.getView().setRotation(newRot);
            float newRotX =
                rawRotationX + fraction * (transformProps.getRotationX() - rawRotationX);
            lynxUI.getView().setRotationX(newRotX);
            float newRotY =
                rawRotationY + fraction * (transformProps.getRotationY() - rawRotationY);
            lynxUI.getView().setRotationY(newRotY);
            float newScaleX = rawScaleX + fraction * (transformProps.getScaleX() - rawScaleX);
            lynxUI.getView().setScaleX(newScaleX);
            float newScaleY = rawScaleY + fraction * (transformProps.getScaleY() - rawScaleY);
            lynxUI.getView().setScaleY(newScaleY);

            if (lynxUI.getParent() instanceof UIShadowProxy) {
              ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
            }
          }
        });
      } break;

      case AnimationConstant.PROP_OPACITY: {
        // TODO: can move Opacity from LynxUI to LynxBaseUI?
        if (mHasVisibilityTransition)
          break;
        final float newAlpha = Math.min((float) value, 1.0f);
        final LynxUI lynxUI = (LynxUI) transitionUI;
        ValueAnimator animator =
            ValueAnimator.ofObject(new FloatEvaluator(), lynxUI.getView().getAlpha(), newAlpha);
        mIdleAnimators.put(AnimationConstant.PROP_OPACITY, animator);
        animator.setDuration(info.getDuration());
        animator.setInterpolator(InterpolatorFactory.getInterpolator(info));
        animator.setStartDelay(info.getDelay());
        animator.addListener(new TransitionListener(lynxUI, property) {
          @Override
          public void onAnimationEnd(Animator animation) {
            super.onAnimationEnd(animation);
            lynxUI.getView().setAlpha(newAlpha);
            if (lynxUI.getParent() instanceof UIShadowProxy) {
              ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
            }
            mRunningAnimators.remove(AnimationConstant.PROP_OPACITY);
          }
        });
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
          @Override
          public void onAnimationUpdate(ValueAnimator animation) {
            Object value = animation.getAnimatedValue();
            if (null != value) {
              float newValue = (float) value;
              lynxUI.getView().setAlpha(newValue);
              if (lynxUI.getParent() instanceof UIShadowProxy) {
                ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
              }
            }
          }
        });
        mHasAlphaTransition = true;
      } break;

      case AnimationConstant.PROP_VISIBILITY: {
        // TODO: move visibility from LynxUI to LynxBaseUI
        if (mHasAlphaTransition)
          break;

        final LynxUI lynxUI = transitionUI.getParent() instanceof UIShadowProxy
            ? (LynxUI) (transitionUI.getParent())
            : (LynxUI) transitionUI;

        final int newVisibility = (Integer) value;
        final int startVisibility = lynxUI.getView().getVisibility();
        final int endVisibility =
            newVisibility == VISIBILITY_VISIBLE ? View.VISIBLE : View.INVISIBLE;

        if (startVisibility == endVisibility)
          break;

        mHasVisibilityTransition = true;
        final float alphaBeforeTransition = lynxUI.getView().getAlpha();
        float startAlpha = alphaBeforeTransition;
        float endAlpha = 1.0f;

        if (startVisibility == View.VISIBLE) {
          endAlpha = 0.0f;
        } else if ((startVisibility == View.INVISIBLE || startVisibility == View.GONE)
            && endVisibility == View.VISIBLE) {
          startAlpha = 0.0f;
          lynxUI.getView().setAlpha(0);
        }

        final ValueAnimator animator =
            ValueAnimator.ofObject(new FloatEvaluator(), startAlpha, endAlpha);

        animator.setDuration(info.getDuration());
        animator.setInterpolator(InterpolatorFactory.getInterpolator(info));
        animator.setStartDelay(info.getDelay());
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
          @Override
          public void onAnimationUpdate(ValueAnimator animation) {
            Object value = animation.getAnimatedValue();
            if (null != value) {
              lynxUI.getView().setAlpha((float) value);
            }
          }
        });
        animator.addListener(new TransitionListener(transitionUI, property) {
          @Override
          public void onAnimationEnd(Animator animation) {
            super.onAnimationEnd(animation);
            lynxUI.setVisibilityForView(endVisibility);
            lynxUI.getView().setAlpha(alphaBeforeTransition);
            mRunningAnimators.remove(AnimationConstant.PROP_VISIBILITY);
          }
          @Override
          public void onAnimationStart(Animator animation) {
            super.onAnimationStart(animation);
            lynxUI.getView().setVisibility(View.VISIBLE);
            mHasVisibilityTransition = false;
          }
          @Override
          public void onAnimationCancel(Animator animation) {
            super.onAnimationCancel(animation);
            mHasVisibilityTransition = false;
          }
        });
        mIdleAnimators.put(AnimationConstant.PROP_VISIBILITY, animator);
      } break;

      default:
        break;
    }

    return true;
  }

  private ValueAnimator createLayoutAnimator(int propertyId, final LynxBaseUI transitionUI,
      int startValue, int endValue, AnimationInfo info, boolean shouldSendAnimationEvent) {
    ValueAnimator animator = ValueAnimator.ofInt(startValue, endValue);
    animator.setDuration(info.getDuration());
    animator.setInterpolator(InterpolatorFactory.getInterpolator(info));
    animator.setStartDelay(info.getDelay());
    animator.addListener(
        new LayoutTransitionListener(transitionUI, propertyId, this, shouldSendAnimationEvent));

    switch (propertyId) {
      case AnimationConstant.PROP_LEFT:
      case AnimationConstant.PROP_RIGHT: {
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
          @Override
          public void onAnimationUpdate(ValueAnimator animation) {
            Object value = animation.getAnimatedValue();
            if (null != value) {
              int newX = (Integer) value;
              transitionUI.setLeft(newX);
              final LynxUI lynxUI = (LynxUI) transitionUI;
              if (lynxUI.getParent() instanceof UIShadowProxy) {
                ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
              }
            }
          }
        });
        break;
      }
      case AnimationConstant.PROP_TOP:
      case AnimationConstant.PROP_BOTTOM: {
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
          @Override
          public void onAnimationUpdate(ValueAnimator animation) {
            Object value = animation.getAnimatedValue();
            if (null != value) {
              int newY = (Integer) value;
              transitionUI.setTop(newY);
              final LynxUI lynxUI = (LynxUI) transitionUI;
              if (lynxUI.getParent() instanceof UIShadowProxy) {
                ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
              }
            }
          }
        });
        break;
      }

      case AnimationConstant.PROP_WIDTH: {
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
          @Override
          public void onAnimationUpdate(ValueAnimator animation) {
            Object value = animation.getAnimatedValue();
            if (null != value) {
              int newW = (Integer) value;
              transitionUI.setWidth(newW);
              final LynxUI lynxUI = (LynxUI) transitionUI;
              if (lynxUI.getParent() instanceof UIShadowProxy) {
                ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
              }
            }
          }
        });
        break;
      }

      case AnimationConstant.PROP_HEIGHT: {
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
          @Override
          public void onAnimationUpdate(ValueAnimator animation) {
            Object value = animation.getAnimatedValue();
            if (null != value) {
              int newH = (Integer) value;
              transitionUI.setHeight(newH);
              final LynxUI lynxUI = (LynxUI) transitionUI;
              if (lynxUI.getParent() instanceof UIShadowProxy) {
                ((UIShadowProxy) (lynxUI.getParent())).notifyAnimating();
              }
            }
          }
        });
        break;
      }
      default:
        break;
    }

    return animator;
  }

  public void applyLayoutTransition(final LynxBaseUI ui, final int x, final int y, final int width,
      final int height, final int paddingLeft, final int paddingTop, final int paddingRight,
      final int paddingBottom, final int marginLeft, final int marginTop, final int marginRight,
      final int marginBottom, final int borderLeftWidth, final int borderTopWidth,
      final int borderRightWidth, final int borderBottomWidth, final Rect bound) {
    if (!(ui instanceof LynxUI)) {
      return;
    }

    final LynxBaseUI transitionUI = mDelegateUI != null ? mDelegateUI : ui;
    boolean xPosChange = transitionUI.getOriginLeft() != x;
    boolean yPosChange = transitionUI.getOriginTop() != y;
    boolean widthChange = transitionUI.getWidth() != width;
    boolean heightChange = transitionUI.getHeight() != height;
    boolean xRightNotChange =
        (transitionUI.getOriginLeft() + transitionUI.getWidth()) == (x + width);
    boolean yBottomNotChange =
        (transitionUI.getOriginTop() + transitionUI.getHeight()) == (y + height);

    transitionUI.onBeforeAnimation(
        x, y, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom);

    boolean hasLeft =
        containsAnimation(AnimationConstant.PROP_LEFT) || containsAnimation(PROP_RIGHT);
    boolean hasTop = containsAnimation(PROP_TOP) || containsAnimation(PROP_BOTTOM);
    boolean hasWidth = containsAnimation(PROP_WIDTH);
    boolean hasHeight = containsAnimation(PROP_HEIGHT);

    int startX = x;
    if (hasLeft && xPosChange || hasWidth && xRightNotChange) {
      startX = transitionUI.getOriginLeft();
    }
    int startY = y;
    if (hasTop && yPosChange || hasHeight && yBottomNotChange) {
      startY = transitionUI.getOriginTop();
    }
    int startWidth = widthChange && hasWidth ? transitionUI.getWidth() : width;
    int startHeight = heightChange && hasHeight ? transitionUI.getHeight() : height;

    // We end all layout animators here. All necessary layout animators will be added again later.
    // Because the end callback of the android animator will be triggered synchronically, and we
    // will change the layout info of LynxUI in the end callback, so we should end all layout
    // animators after the begin layout info has be constructed.
    endAllLayoutAnimators();

    // update begin layout info before animation
    transitionUI.updateLayout(startX, startY, startWidth, startHeight, paddingLeft, paddingTop,
        paddingRight, paddingBottom, marginLeft, marginTop, marginRight, marginBottom,
        borderLeftWidth, borderTopWidth, borderRightWidth, borderBottomWidth, bound);

    HashMap<Integer, Animator> layoutAnimators = new HashMap<Integer, Animator>();

    for (int i = 0; i < mAnimationInfo.size(); i++) {
      AnimationInfo info = mAnimationInfo.valueAt(i);
      if ((info.getProperty() & AnimationConstant.PROP_OF_LAYOUT) == 0)
        continue;

      switch (info.getProperty()) {
        case AnimationConstant.PROP_LEFT:
        case AnimationConstant.PROP_RIGHT: {
          if (xPosChange) {
            ValueAnimator animator = createLayoutAnimator(
                info.getProperty(), transitionUI, transitionUI.getOriginLeft(), x, info, true);
            layoutAnimators.put(info.getProperty(), animator);
          }
          break;
        }

        case AnimationConstant.PROP_TOP:
        case AnimationConstant.PROP_BOTTOM: {
          if (yPosChange) {
            ValueAnimator animator = createLayoutAnimator(
                info.getProperty(), transitionUI, transitionUI.getOriginTop(), y, info, true);
            layoutAnimators.put(info.getProperty(), animator);
          }
          break;
        }

        case AnimationConstant.PROP_WIDTH: {
          if (widthChange) {
            final AnimatorSet animationSet = new AnimatorSet();
            List<Animator> animators = new ArrayList<>();
            ValueAnimator widthAnimator = createLayoutAnimator(AnimationConstant.PROP_WIDTH,
                transitionUI, transitionUI.getWidth(), width, info, true);
            animators.add(widthAnimator);

            // If xPosChange and xRightNotChange and don't have left
            // animation, should add an additional left animation to ensure the animation is
            // correct.
            boolean needAdditionalAnimator = (xPosChange && xRightNotChange && !hasLeft);
            if (needAdditionalAnimator) {
              ValueAnimator leftAnimator = createLayoutAnimator(AnimationConstant.PROP_LEFT,
                  transitionUI, transitionUI.getOriginLeft(), x, info, false);
              animators.add(leftAnimator);
            }
            animationSet.playTogether(animators);
            layoutAnimators.put(AnimationConstant.PROP_WIDTH, animationSet);
          }
          break;
        }

        case AnimationConstant.PROP_HEIGHT: {
          if (heightChange) {
            final AnimatorSet animationSet = new AnimatorSet();
            List<Animator> animators = new ArrayList<>();
            ValueAnimator heightAnimator = createLayoutAnimator(AnimationConstant.PROP_HEIGHT,
                transitionUI, transitionUI.getHeight(), height, info, true);
            animators.add(heightAnimator);

            // If yPosChange and yBottomNotChange and don't have top
            // animation, should add a additional top animation to ensure the animation is correct.
            boolean needAdditionalAnimator = (yPosChange && yBottomNotChange && !hasTop);
            if (needAdditionalAnimator) {
              ValueAnimator topAnimator = createLayoutAnimator(AnimationConstant.PROP_TOP,
                  transitionUI, transitionUI.getOriginTop(), y, info, false);
              animators.add(topAnimator);
            }
            animationSet.playTogether(animators);
            layoutAnimators.put(AnimationConstant.PROP_HEIGHT, animationSet);
          }
          break;
        }
        default:
          break;
      }
    }

    if (!layoutAnimators.isEmpty()) {
      mIdleAnimators.putAll(layoutAnimators);
    }

    mLatestX = x;
    mLatestY = y;
    mLatestWidth = width;
    mLatestHeight = height;
  }

  public void applyTransformTransition(LynxBaseUI ui) {
    final LynxBaseUI transitionUI = mDelegateUI != null ? mDelegateUI : ui;
    applyPropertyTransition(
        transitionUI, AnimationConstant.PROP_TRANSFORM, transitionUI.getTransformRaws());
  }

  public void startTransitions() {
    if (mIdleAnimators.isEmpty()) {
      return;
    }

    for (Map.Entry<Integer, Animator> entry : mIdleAnimators.entrySet()) {
      Animator ani = mRunningAnimators.get(entry.getKey());
      if (ani != null) {
        ani.cancel();
      }
      // Transition is a special keyframe animation.
      // We need to make sure that the fillmode of the transition animation is both, otherwise the
      // animation will be weird when its delay is not zero.
      ensureFillModeBoth(entry.getValue());
      entry.getValue().start();
      mRunningAnimators.put(entry.getKey(), entry.getValue());
    }
    mIdleAnimators.clear();
  }

  private boolean containsAnimation(int prop) {
    return mAnimationInfo.indexOfKey(prop) >= 0;
  }

  private static class TransitionListener extends AnimatorListenerAdapter {
    private static final String sEventStart = "transitionstart";
    private static final String sEventEnd = "transitionend";
    private static final Map<String, Object> sEventParams = new HashMap<String, Object>();
    static {
      sEventParams.put("animation_type", "transition");
    }

    WeakReference<LynxBaseUI> mUI;
    int mPropName;

    public TransitionListener(LynxBaseUI ui, int prop) {
      this.mUI = new WeakReference<>(ui);
      this.mPropName = prop;
    }

    @Override
    public void onAnimationStart(Animator animation) {
      super.onAnimationStart(animation);
      sendAnimationEvent(sEventStart);
    }

    @Override
    public void onAnimationRepeat(Animator animation) {
      super.onAnimationRepeat(animation);
    }

    @Override
    public void onAnimationEnd(Animator animation) {
      super.onAnimationEnd(animation);
      sendAnimationEvent(sEventEnd);
    }

    private void sendAnimationEvent(String eventName) {
      LynxBaseUI ui = mUI.get();
      if (ui == null) {
        return;
      }

      LynxBaseUI targetUI = ui instanceof UIShadowProxy ? ((UIShadowProxy) ui).getChild() : ui;

      if (targetUI.getEvents() != null && targetUI.getEvents().containsKey(eventName)) {
        sEventParams.put(
            "animation_type", "transition-" + PropertyFactory.propertyToString(mPropName));
        targetUI.getLynxContext().getEventEmitter().sendCustomEvent(
            new LynxCustomEvent(targetUI.getSign(), eventName, sEventParams));
      }
    }
  }

  private static class LayoutTransitionListener extends TransitionListener {
    WeakReference<TransitionAnimationManager> mManager;
    // This flag is used to identify whether to send animation events to the front end
    boolean mShouldSendAnimationEvent = true;

    public LayoutTransitionListener(LynxBaseUI ui, int prop, TransitionAnimationManager manager,
        boolean shouldSendAnimationEvent) {
      super(ui, prop);
      this.mManager = new WeakReference<>(manager);
      mShouldSendAnimationEvent = shouldSendAnimationEvent;
    }

    @Override
    public void onAnimationStart(Animator animation) {
      if (mShouldSendAnimationEvent) {
        super.onAnimationStart(animation);
      }
    }

    @Override
    public void onAnimationEnd(Animator animation) {
      // If we need to send animation end event to the front end, then we need to call the
      // corresponding method of the parent class
      if (mShouldSendAnimationEvent) {
        super.onAnimationEnd(animation);
      }
      TransitionAnimationManager manager = mManager.get();
      LynxBaseUI ui = mUI.get();
      if (ui == null || manager == null) {
        return;
      }
      manager.mRunningAnimators.remove(mPropName);
      int x = ui.getOriginLeft();
      int y = ui.getOriginTop();
      int width = ui.getWidth();
      int height = ui.getHeight();
      switch (mPropName) {
        case PROP_LEFT:
        case PROP_RIGHT: {
          x = manager.mLatestX;
          break;
        }
        case PROP_TOP:
        case PROP_BOTTOM: {
          y = manager.mLatestY;
          break;
        }
        case PROP_WIDTH: {
          width = manager.mLatestWidth;
          break;
        }
        case PROP_HEIGHT: {
          height = manager.mLatestHeight;
          break;
        }
      }
      ui.updateLayout(x, y, width, height, ui.getPaddingLeft(), ui.getPaddingTop(),
          ui.getPaddingRight(), ui.getPaddingBottom(), ui.getMarginLeft(), ui.getMarginTop(),
          ui.getMarginRight(), ui.getMarginBottom(), ui.getBorderLeftWidth(),
          ui.getBorderTopWidth(), ui.getBorderRightWidth(), ui.getBorderBottomWidth(),
          ui.getBound());
    }
  }

  public void onDestroy() {
    endAllAnimators();
  }

  // The fill-mode of Android animator is already forwards, we only need to implement backward to
  // achieve the effect of both
  private void ensureFillModeBoth(Animator animator) {
    Animator oneFrameAnimator = animator.clone();
    oneFrameAnimator.setDuration(10000000);
    oneFrameAnimator.setStartDelay(0);
    if (oneFrameAnimator instanceof AnimatorSet) {
      AnimatorSet animatorSet = (AnimatorSet) oneFrameAnimator;
      ArrayList<Animator> animators = animatorSet.getChildAnimations();
      for (Animator ani : animators) {
        ani.removeAllListeners();
      }
    }
    oneFrameAnimator.removeAllListeners();
    oneFrameAnimator.addListener(new StartListenerForFillModeBoth());
    oneFrameAnimator.start();
  }

  private static class StartListenerForFillModeBoth extends AnimatorListenerAdapter {
    @Override
    public void onAnimationStart(Animator animation) {
      super.onAnimationStart(animation);
      animation.cancel();
    }
  }
}
