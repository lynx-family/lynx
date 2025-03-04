// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation.layout;

import android.graphics.Rect;
import android.util.SparseArray;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.event.LynxCustomEvent;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public class LayoutAnimationManager {
  private WeakReference<LynxUI> mUI;
  // Latest layout info of UI
  // TODO(WUJINTIAN): Move those latest layout info to LynxBaseUI
  private int mX, mY, mWidth, mHeight;
  private int mPaddingLeft, mPaddingTop, mPaddingRight, mPaddingBottom;
  private int mBorderLeftWidth, mBorderTopWidth, mBorderRightWidth, mBorderBottomWidth;
  private int mMarginLeft, mMarginTop, mMarginRight, mMarginBottom;
  private Rect mBound;

  private LynxOnAttachStateChangeListener mOnAttachStateChangeListener;

  @Nullable private AbstractLayoutAnimation mLayoutCreateAnimation;
  @Nullable private AbstractLayoutAnimation mLayoutUpdateAnimation;
  @Nullable private AbstractLayoutAnimation mLayoutDeleteAnimation;

  private final SparseArray<LayoutHandlingAnimation> mLayoutHandlers = new SparseArray<>(0);
  private float mOriginAlpha = -1.0f;

  private boolean mLayerTypeChanged = false;

  public void updateAlpha(float alpha) {
    mOriginAlpha = alpha;
  }

  private LynxUI getUI() {
    if (mUI != null) {
      return mUI.get();
    }
    return null;
  }

  private SparseArray<LayoutHandlingAnimation> getLayoutHandlers() {
    return mLayoutHandlers;
  }

  /**
   * Update layout of given view, via immediate update or animation depending on the current batch
   * layout animation configuration supplied during initialization. Handles create and update
   * animations.
   */
  public void applyLayoutUpdate(LynxUI ui, int x, int y, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, final Rect bound) {
    View view = ui.getView();

    mUI = new WeakReference<>(ui);
    updateLatestLayoutInfo(x, y, width, height, paddingLeft, paddingTop, paddingRight,
        paddingBottom, marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth,
        borderTopWidth, borderRightWidth, borderBottomWidth, bound);
    if (mOriginAlpha == -1.0f && view != null) {
      mOriginAlpha = view.getAlpha();
    }
    final int reactTag = ui.getSign();

    // Update an ongoing animation if possible, otherwise the layout update would be ignored as
    // the existing animation would still animate to the old layout.
    LayoutHandlingAnimation existingAnimation = mLayoutHandlers.get(reactTag);
    if (existingAnimation != null) {
      if (view != null && view.getAnimation() != null) {
        existingAnimation.onLayoutUpdate(x, y, width, height);
        return;
      } else {
        mLayoutHandlers.remove(reactTag);
      }
    }

    // Use Update when both width and height are 0, otherwise use Create.
    AbstractLayoutAnimation layoutAnimation = (ui.getWidth() == 0 && ui.getHeight() == 0)
        ? mLayoutCreateAnimation
        : mLayoutUpdateAnimation;

    Animation animation = null;
    if (mLayoutDeleteAnimation != null && mLayoutDeleteAnimation.isValid() && width == 0
        && height == 0) {
      deleteView(ui);
      return;
    } else if ((mLayoutDeleteAnimation == null || !mLayoutDeleteAnimation.isValid()) && width == 0
        && height == 0) {
      ui.updateLayout(x, y, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom,
          marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth, borderTopWidth,
          borderRightWidth, borderBottomWidth, bound);
      return;
    } else if (layoutAnimation != null) {
      animation = layoutAnimation.createAnimation(ui, x, y, width, height, paddingLeft, paddingTop,
          paddingRight, paddingBottom, marginLeft, marginTop, marginRight, marginBottom,
          borderLeftWidth, borderTopWidth, borderRightWidth, borderBottomWidth, bound,
          mOriginAlpha);
    }

    if (animation instanceof TranslateAnimation && ui.getParent() instanceof UIShadowProxy) {
      view = ((LynxUI) ui.getParent()).getView();
    }

    if (animation == null) {
      if (mOriginAlpha != -1.0f) {
        view.setAlpha(mOriginAlpha);
      }
      if (mUI != null) {
        mUI.clear();
      }

      ui.updateLayout(x, y, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom,
          marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth, borderTopWidth,
          borderRightWidth, borderBottomWidth, bound);
      return;
    }

    if (ui.getWidth() == 0 && ui.getHeight() == 0) {
      animation.setAnimationListener(
          new LayoutAnimationListener(this, LayoutAnimationListenerUtils.LAYOUT_ANIMATION_CREATE));
      addOnAttachStateChangeListenerToView(
          view, LayoutAnimationListenerUtils.LAYOUT_ANIMATION_CREATE);
    } else {
      animation.setAnimationListener(
          new LayoutAnimationListener(this, LayoutAnimationListenerUtils.LAYOUT_ANIMATION_UPDATE));
      addOnAttachStateChangeListenerToView(
          view, LayoutAnimationListenerUtils.LAYOUT_ANIMATION_UPDATE);
    }
    if (animation instanceof LayoutHandlingAnimation) {
      // Some phones do not trigger drawing automatically and require manual request.
      ui.requestLayout();
    } else {
      ui.updateLayout(x, y, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom,
          marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth, borderTopWidth,
          borderRightWidth, borderBottomWidth, bound);
    }
    ui.onBeforeAnimation(x, y, width, height, paddingLeft, paddingTop, paddingRight, paddingBottom);
    view.startAnimation(animation);
  }

  public void applyLatestLayoutInfoToUI() {
    if (mUI != null && mUI.get() != null) {
      mUI.get().updateLayout(mX, mY, mWidth, mHeight, mPaddingLeft, mPaddingTop, mPaddingRight,
          mPaddingBottom, mMarginLeft, mMarginTop, mMarginRight, mMarginBottom, mBorderLeftWidth,
          mBorderTopWidth, mBorderRightWidth, mBorderBottomWidth, mBound);
    }
  }

  /**
   * Animate a view deletion using the layout animation configuration supplied during
   * initialization.
   */
  public void deleteView(final LynxUI ui) {
    final View view = ui.getView();
    if (view == null) {
      return;
    }
    mUI = new WeakReference<>(ui);
    Animation animation =
        mLayoutDeleteAnimation.createAnimation(ui, view.getLeft(), view.getTop(), view.getWidth(),
            view.getHeight(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect(), mOriginAlpha);

    if (animation == null) {
      ui.updateLayout(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect());
      return;
    }

    animation.setAnimationListener(
        new LayoutAnimationListener(this, LayoutAnimationListenerUtils.LAYOUT_ANIMATION_DELETE));
    addOnAttachStateChangeListenerToView(
        view, LayoutAnimationListenerUtils.LAYOUT_ANIMATION_DELETE);

    view.startAnimation(animation);
  }

  private static class LayoutAnimationListenerUtils {
    private static final String EVENT_START = "animationstart";
    private static final String EVENT_END = "animationend";
    private static final String LAYOUT_ANIMATION_CREATE = "layout-animation-create";
    private static final String LAYOUT_ANIMATION_UPDATE = "layout-animation-update";
    private static final String LAYOUT_ANIMATION_DELETE = "layout-animation-delete";
    private static Map<String, Object> sEventParams = new HashMap<String, Object>();
    static {
      sEventParams.put("animation_type", "");
    }

    public static void sendAnimationEvent(LynxUI ui, String event_name, String animation_type) {
      if (ui == null) {
        return;
      }
      if (ui.getEvents() != null && ui.getEvents().containsKey(event_name)) {
        sEventParams.put("animation_type", animation_type);
        ui.getLynxContext().getEventEmitter().sendCustomEvent(
            new LynxCustomEvent(ui.getSign(), event_name, sEventParams));
      }
    }
  }

  private static class LayoutAnimationListener implements Animation.AnimationListener {
    private WeakReference<LayoutAnimationManager> mManagerRef;
    private String mLayoutAnimationType;
    public LayoutAnimationListener(LayoutAnimationManager manager, String animation_type) {
      this.mManagerRef = new WeakReference<>(manager);
      ;
      this.mLayoutAnimationType = animation_type;
    }

    @Override
    public void onAnimationStart(Animation animation) {
      LayoutAnimationManager manager = mManagerRef.get();
      if (manager == null) {
        return;
      }
      LynxUI ui = manager.getUI();
      if (ui == null) {
        return;
      }
      View view = ui.getView();
      LayoutAnimationListenerUtils.sendAnimationEvent(
          ui, LayoutAnimationListenerUtils.EVENT_START, mLayoutAnimationType);

      if (mLayoutAnimationType == LayoutAnimationListenerUtils.LAYOUT_ANIMATION_CREATE
          || mLayoutAnimationType == LayoutAnimationListenerUtils.LAYOUT_ANIMATION_UPDATE) {
        if (animation instanceof LayoutHandlingAnimation) {
          manager.getLayoutHandlers().put(ui.getSign(), (LayoutHandlingAnimation) animation);
        } else if (animation instanceof OpacityAnimation) {
          if (view.getLayerType() == View.LAYER_TYPE_NONE) {
            manager.mLayerTypeChanged = true;
            view.setLayerType(View.LAYER_TYPE_HARDWARE, null);
          }
        }
      }

      else if (mLayoutAnimationType == LayoutAnimationListenerUtils.LAYOUT_ANIMATION_DELETE) {
        if (animation instanceof OpacityAnimation) {
          if (view.getLayerType() == View.LAYER_TYPE_NONE) {
            manager.mLayerTypeChanged = true;
            view.setLayerType(View.LAYER_TYPE_HARDWARE, null);
          }
        }
      }
    }

    @Override
    public void onAnimationRepeat(Animation animation) {}

    @Override
    public void onAnimationEnd(Animation animation) {
      LayoutAnimationManager manager = mManagerRef.get();
      if (manager == null) {
        return;
      }
      LynxUI ui = manager.getUI();
      if (ui == null) {
        return;
      }
      View view = ui.getView();

      manager.onAnimationEnd(mLayoutAnimationType);

      if (mLayoutAnimationType == LayoutAnimationListenerUtils.LAYOUT_ANIMATION_CREATE
          || mLayoutAnimationType == LayoutAnimationListenerUtils.LAYOUT_ANIMATION_UPDATE) {
        if (animation instanceof LayoutHandlingAnimation) {
          manager.getLayoutHandlers().remove(ui.getSign());
        } else if (animation instanceof OpacityAnimation) {
          if (manager.mLayerTypeChanged) {
            if (view != null) {
              view.setLayerType(View.LAYER_TYPE_NONE, null);
            }
            manager.mLayerTypeChanged = false;
          }
        }
      }

      else if (mLayoutAnimationType == LayoutAnimationListenerUtils.LAYOUT_ANIMATION_DELETE) {
        if (animation instanceof OpacityAnimation) {
          if (manager.mLayerTypeChanged) {
            view.setLayerType(View.LAYER_TYPE_NONE, null);
            manager.mLayerTypeChanged = false;
          }
        }
        ui.updateLayout(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect());
      }
    }
  }

  @NonNull
  public AbstractLayoutAnimation getLayoutCreateAnimation() {
    if (null == mLayoutCreateAnimation) {
      mLayoutCreateAnimation = new LayoutCreateAnimation();
    }
    return mLayoutCreateAnimation;
  }

  @NonNull
  public AbstractLayoutAnimation getLayoutUpdateAnimation() {
    if (null == mLayoutUpdateAnimation) {
      mLayoutUpdateAnimation = new LayoutUpdateAnimation();
    }
    return mLayoutUpdateAnimation;
  }

  @NonNull
  public AbstractLayoutAnimation getLayoutDeleteAnimation() {
    if (null == mLayoutDeleteAnimation) {
      mLayoutDeleteAnimation = new LayoutDeleteAnimation();
    }
    return mLayoutDeleteAnimation;
  }

  public boolean isValid() {
    return (mLayoutCreateAnimation != null && mLayoutCreateAnimation.isValid())
        || (mLayoutDeleteAnimation != null && mLayoutDeleteAnimation.isValid())
        || (mLayoutUpdateAnimation != null && mLayoutUpdateAnimation.isValid());
  }

  private static class LynxOnAttachStateChangeListener implements View.OnAttachStateChangeListener {
    private WeakReference<LayoutAnimationManager> mManagerRef;
    public String mLayoutAnimationType;
    public LynxOnAttachStateChangeListener(LayoutAnimationManager manager, String animationType) {
      this.mManagerRef = new WeakReference<>(manager);
      this.mLayoutAnimationType = animationType;
    }
    @Override
    public void onViewAttachedToWindow(View v) {}

    @Override
    public void onViewDetachedFromWindow(View v) {
      LayoutAnimationManager manager = mManagerRef.get();
      if (manager != null) {
        manager.onAnimationEnd(mLayoutAnimationType);
      }
    }
  }

  private void addOnAttachStateChangeListenerToView(View view, String animationType) {
    LynxUI ui = getUI();
    if (ui == null) {
      return;
    }

    if (mOnAttachStateChangeListener != null
        && mOnAttachStateChangeListener.mLayoutAnimationType.equals(animationType)) {
      return;
    }
    if (mOnAttachStateChangeListener != null) {
      view.removeOnAttachStateChangeListener(mOnAttachStateChangeListener);
    }
    mOnAttachStateChangeListener = new LynxOnAttachStateChangeListener(this, animationType);
    view.addOnAttachStateChangeListener(mOnAttachStateChangeListener);
  }

  private void onAnimationEnd(String animationType) {
    LynxUI ui = getUI();
    if (ui == null) {
      return;
    }
    View view = ui.getView();

    if (mOnAttachStateChangeListener != null) {
      if (view != null) {
        view.removeOnAttachStateChangeListener(mOnAttachStateChangeListener);
      }
      mOnAttachStateChangeListener = null;
    }

    applyLatestLayoutInfoToUI();

    LayoutAnimationListenerUtils.sendAnimationEvent(
        ui, LayoutAnimationListenerUtils.EVENT_END, animationType);
  }

  public void updateLatestLayoutInfo(int x, int y, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, final Rect bound) {
    mX = x;
    mY = y;
    mWidth = width;
    mHeight = height;
    mPaddingLeft = paddingLeft;
    mPaddingTop = paddingTop;
    mPaddingRight = paddingRight;
    mPaddingBottom = paddingBottom;
    mMarginLeft = marginLeft;
    mMarginTop = marginTop;
    mMarginRight = marginRight;
    mMarginBottom = marginBottom;
    mBorderLeftWidth = borderLeftWidth;
    mBorderTopWidth = borderTopWidth;
    mBorderRightWidth = borderRightWidth;
    mBorderBottomWidth = borderBottomWidth;
    mBound = bound;
  }

  // End the layout animation currently on the UI and update the final layout to the UI.
  private void endAnimation(String animationType) {
    if (mOnAttachStateChangeListener == null
        || !mOnAttachStateChangeListener.mLayoutAnimationType.equals(animationType)) {
      return;
    }
    if (mUI != null && mUI.get() != null) {
      View view = mUI.get().getView();
      if (view != null && view.getAnimation() != null) {
        // clear animation will also send animation end event
        view.clearAnimation();
        applyLatestLayoutInfoToUI();
      }
    }
  }

  public void setLayoutAnimationCreateDuration(double duration) {
    getLayoutCreateAnimation().setDuration((long) duration);
    if (!getLayoutCreateAnimation().isValid()) {
      endAnimation(LayoutAnimationListenerUtils.LAYOUT_ANIMATION_CREATE);
    }
  }

  public void setLayoutAnimationUpdateDuration(double duration) {
    getLayoutUpdateAnimation().setDuration((long) duration);
    if (!getLayoutUpdateAnimation().isValid()) {
      endAnimation(LayoutAnimationListenerUtils.LAYOUT_ANIMATION_UPDATE);
    }
  }

  public void setLayoutAnimationDeleteDuration(double duration) {
    getLayoutDeleteAnimation().setDuration((long) duration);
    if (!getLayoutDeleteAnimation().isValid()) {
      endAnimation(LayoutAnimationListenerUtils.LAYOUT_ANIMATION_DELETE);
    }
  }
}
