// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import static androidx.core.view.ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS;
import static androidx.core.view.accessibility.AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_ACCESSIBILITY_FOCUS;
import static androidx.core.view.accessibility.AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_CLICK;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_DEFAULT;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.INVALID_BOUNDS;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_CLICK;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TAP;

import android.graphics.Rect;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import androidx.annotation.NonNull;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import com.lynx.R;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * CustomAccessibilityDelegateCompat is a class for implementing accessibility and this delegate
 * contains methods that correspond to the accessibility methods in the view.
 * Note: the default implementation the delegate methods behaves exactly as the corresponding method
 * in the view for the case of no accessibility delegate been set.
 */
public class CustomAccessibilityDelegateCompat extends AccessibilityDelegateCompat {
  private static final String TAG = "CustomAccessibilityDelegateCompat";

  private static final boolean DEBUG = false;

  private static List<Integer> idList;

  /** Reference of host ui */
  private WeakReference<LynxUI> mWeakUI;

  /** The node bounds in screen coordinates. */
  private Rect mGlobalBoundRect = new Rect(INVALID_BOUNDS);

  /** The node bounds in parent coordinates. */
  private Rect mParentBoundRect = new Rect(INVALID_BOUNDS);

  public CustomAccessibilityDelegateCompat(LynxUI ui) {
    if (ui == null || ui.getView() == null) {
      LLog.e(TAG, "Construct with null ui or view");
      return;
    }
    mWeakUI = new WeakReference<>(ui);
    boolean isImportantForAccessibility = false;
    LynxAccessibilityHelper helper = getLynxAccessibilityHelper();
    if (helper != null) {
      // When initialize delegate, we should use ACCESSIBILITY_ELEMENT_DEFAULT which can apply
      // global config enableAccessibilityElement to the view.
      isImportantForAccessibility =
          helper.isImportantForAccessibility(ACCESSIBILITY_ELEMENT_DEFAULT);
    }
    ViewCompat.setImportantForAccessibility(ui.getView(),
        isImportantForAccessibility ? ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES
                                    : ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_AUTO);
  }

  /**
   * Initializes the node with information about the given host view.
   *
   * @param host The View hosting the delegate.
   * @param info The instance to initialize.
   */
  @Override
  public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info) {
    super.onInitializeAccessibilityNodeInfo(host, info);
    LynxAccessibilityWrapper wrapper = getLynxAccessibilityWrapper();
    if (wrapper == null) {
      return;
    }
    if (mWeakUI != null && mWeakUI.get() != null && wrapper.enableHelper()) {
      LynxUI ui = mWeakUI.get();
      LynxAccessibilityHelper helper = getLynxAccessibilityHelper();
      final boolean isImportant = helper.isImportantForAccessibility(ui);
      final CharSequence label = ui.getAccessibilityLabel();
      final boolean isClickable = isClickableUI();
      info.setScreenReaderFocusable(isImportant);
      info.setContentDescription(
          isImportant ? (concatA11yStatusAndLabel(ui.getAccessibilityStatus(), label)) : null);
      info.setText(isImportant ? label : null);
      info.setClickable(isClickable);
      ArrayList<String> customActions = ui.getAccessibilityActions();
      if (customActions != null) {
        int index = 0;
        for (String name : customActions) {
          info.addAction(new AccessibilityNodeInfoCompat.AccessibilityActionCompat(
              genA11yCustomActionID(index++), name));
        }
      } else {
        for (AccessibilityNodeInfoCompat.AccessibilityActionCompat action : info.getActionList()) {
          info.removeAction(action);
        }
      }
      if (isClickable) {
        info.addAction(ACTION_CLICK);
      } else {
        info.removeAction(ACTION_CLICK);
      }
      if (!isImportant) {
        info.removeAction(ACTION_ACCESSIBILITY_FOCUS);
        info.setVisibleToUser(false);
      }

      info.getBoundsInScreen(mGlobalBoundRect);
      info.getBoundsInParent(mParentBoundRect);
      setTraversalOrder(ui, info);
      setTraitsIfNeeded(ui, info);
      setRoleDescriptionIfNeeded(ui, info);
    }
    if (DEBUG) {
      LLog.i(TAG,
          "onInitializeAccessibilityNodeInfo: " + host + ", " + info.getContentDescription() + ", "
              + mGlobalBoundRect);
    }
  }

  private String concatA11yStatusAndLabel(CharSequence status, CharSequence label) {
    StringBuilder builder = new StringBuilder();
    if (status != null) {
      builder.append(status);
    }
    if (label != null) {
      if (builder.length() != 0) {
        builder.append(", ");
      }
      builder.append(label);
    }
    return builder.toString();
  }

  /**
   * Perform the accessibility action on the view. Here we only need to handle ACTION_CLICK and send
   * focus event to FE when handling ACTION_ACCESSIBILITY_FOCUS.
   *
   * @param host the View hosting the delegate.
   * @param action the action to perform.
   * @param args optional action arguments.
   * @return Whether the action was performed.
   */
  @Override
  public boolean performAccessibilityAction(View host, int action, Bundle args) {
    boolean res = super.performAccessibilityAction(host, action, args);
    if (action == ACTION_CLICK.getId()) {
      res = fireActionClick();
    } else if (action == ACTION_ACCESSIBILITY_FOCUS.getId()
        && getLynxAccessibilityWrapper() != null) {
      // Notify FE when an accessibility element focused.
      getLynxAccessibilityWrapper().onAccessibilityFocused(-1, mWeakUI.get());
    } else {
      ArrayList<String> actions = mWeakUI.get().getAccessibilityActions();
      if (actions != null) {
        int index = 0;
        for (String name : actions) {
          if (action == genA11yCustomActionID(index++)) {
            LynxDetailEvent event =
                new LynxDetailEvent(mWeakUI.get().getSign(), "accessibilityaction");
            event.addDetail("name", name);
            mWeakUI.get().getLynxContext().getEventEmitter().sendCustomEvent(event);
            break;
          }
        }
      }
    }
    if (DEBUG) {
      LLog.i(TAG, "performAccessibilityAction: " + host + ", " + action + ", " + res);
    }
    return res;
  }

  private int genA11yCustomActionID(int index) {
    if (idList == null) {
      idList = new ArrayList<>(Arrays.asList(R.id.custom_reserved_a11y_action_0_id,
          R.id.custom_reserved_a11y_action_1_id, R.id.custom_reserved_a11y_action_2_id,
          R.id.custom_reserved_a11y_action_3_id, R.id.custom_reserved_a11y_action_4_id,
          R.id.custom_reserved_a11y_action_5_id, R.id.custom_reserved_a11y_action_6_id,
          R.id.custom_reserved_a11y_action_7_id, R.id.custom_reserved_a11y_action_8_id,
          R.id.custom_reserved_a11y_action_9_id, R.id.custom_reserved_a11y_action_10_id,
          R.id.custom_reserved_a11y_action_11_id, R.id.custom_reserved_a11y_action_12_id,
          R.id.custom_reserved_a11y_action_13_id, R.id.custom_reserved_a11y_action_14_id,
          R.id.custom_reserved_a11y_action_15_id, R.id.custom_reserved_a11y_action_16_id,
          R.id.custom_reserved_a11y_action_17_id, R.id.custom_reserved_a11y_action_18_id,
          R.id.custom_reserved_a11y_action_19_id, R.id.custom_reserved_a11y_action_20_id,
          R.id.custom_reserved_a11y_action_21_id, R.id.custom_reserved_a11y_action_22_id,
          R.id.custom_reserved_a11y_action_23_id, R.id.custom_reserved_a11y_action_24_id,
          R.id.custom_reserved_a11y_action_25_id, R.id.custom_reserved_a11y_action_26_id,
          R.id.custom_reserved_a11y_action_27_id, R.id.custom_reserved_a11y_action_28_id,
          R.id.custom_reserved_a11y_action_29_id));
    }
    return idList.get(index);
  }

  /**
   * Set the accessibility-traits to the accessibility node info.
   */
  private void setTraitsIfNeeded(LynxUI ui, AccessibilityNodeInfoCompat info) {
    if (ui != null && info != null) {
      LynxAccessibilityHelper.LynxAccessibilityTraits traits = ui.getAccessibilityTraits();
      info.setClassName(LynxAccessibilityHelper.LynxAccessibilityTraits.getValue(traits));
      if (traits == LynxAccessibilityHelper.LynxAccessibilityTraits.NONE) {
        info.setRoleDescription("");
      }
    }
  }

  static final Map<String, String> mClassNameMap = new HashMap<String, String>() {
    {
      put("button", android.widget.Button.class.getName());
      put("switch", android.widget.Switch.class.getName());
      put("checkbox", android.widget.CheckBox.class.getName());
      put("image", android.widget.ImageView.class.getName());
      put("progressbar", android.widget.ProgressBar.class.getName());
    }
  };

  private void setRoleDescriptionIfNeeded(LynxUI ui, AccessibilityNodeInfoCompat info) {
    if (ui != null && info != null) {
      String description = ui.getAccessibilityRoleDescription();
      if (description != null) {
        if (mClassNameMap.containsKey(description)) {
          info.setClassName(mClassNameMap.get(description));
        } else {
          info.setRoleDescription(description);
        }
      }
    }
  }

  /**
   * If ui has props about box-shadow, we will create UIShadowProxy whose ShadowView has the same
   * dimension with parent ui. As a result, the screen reader always accesses children of the ui
   * with box-shadow first. Here we should invoke setTraversalOrder() with the accessibilityNodeInfo
   * to make sure not preferentially visited.
   *
   * @param ui current ui
   * @param info node info of the current ui
   */
  private void setTraversalOrder(@NonNull LynxUI ui, @NonNull AccessibilityNodeInfoCompat info) {
    LynxContext context = ui.getLynxContext();
    View host = ui.getView();
    if (context != null && host != null && context.getUIBody() != null) {
      UIBody.UIBodyView bodyView = context.getUIBody().getBodyView();
      ViewParent parentView = host.getParent();
      while (bodyView != null && parentView != null && parentView != bodyView) {
        if (parentView instanceof UIShadowProxy.ShadowView) {
          UIShadowProxy.ShadowView shadowView = (UIShadowProxy.ShadowView) parentView;
          if (shadowView.getParent() instanceof ViewGroup) {
            ViewGroup shadowViewParent = (ViewGroup) shadowView.getParent();
            final int childCount = shadowViewParent.getChildCount();
            for (int i = 0; i < childCount; ++i) {
              View child = shadowViewParent.getChildAt(i);
              if (child == shadowView) {
                // If child is the shadowView, no need to set traversal order.
                break;
              } else if (child != null) {
                // Make sure a screen-reader visit the content of child before the content of this
                // info.
                info.setTraversalAfter(child);
              }
            }
          }
          break;
        }
        parentView = parentView.getParent();
      }
    }
  }

  /**
   * Fire click or tap event on the host ui.
   *
   * @return Whether the action was performed.
   */
  private boolean fireActionClick() {
    LynxAccessibilityWrapper wrapper = getLynxAccessibilityWrapper();
    if (mWeakUI != null && mWeakUI.get() != null && wrapper != null && wrapper.enableHelper()) {
      LynxUI ui = mWeakUI.get();
      if (!mGlobalBoundRect.equals(INVALID_BOUNDS) && !mParentBoundRect.equals(INVALID_BOUNDS)) {
        LynxTouchEvent.Point globalPoint =
            new LynxTouchEvent.Point(mGlobalBoundRect.centerX(), mGlobalBoundRect.centerY());
        LynxTouchEvent.Point localPoint =
            new LynxTouchEvent.Point(mParentBoundRect.centerX(), mParentBoundRect.centerY());
        if (ui.getEvents() != null) {
          final Map<String, EventsListener> events = ui.getEvents();
          boolean consumeActionClick = false;
          if (events.containsKey(EVENT_TAP)) {
            ui.getLynxContext().getEventEmitter().sendTouchEvent(
                new LynxTouchEvent(ui.getSign(), EVENT_TAP, localPoint, localPoint, globalPoint));
            consumeActionClick = true;
          }
          if (events.containsKey(EVENT_CLICK)) {
            ui.getLynxContext().getEventEmitter().sendTouchEvent(
                new LynxTouchEvent(ui.getSign(), EVENT_CLICK, localPoint, localPoint, globalPoint));
            consumeActionClick = true;
          }
          return consumeActionClick;
        }
      }
    }
    return false;
  }

  private boolean isClickableUI() {
    LynxUI ui = getLynxUI();
    return ui != null && ui.getEvents() != null
        && (ui.getEvents().containsKey(EVENT_CLICK) || ui.getEvents().containsKey(EVENT_TAP));
  }

  private LynxAccessibilityHelper getLynxAccessibilityHelper() {
    LynxAccessibilityWrapper wrapper = getLynxAccessibilityWrapper();
    if (wrapper != null) {
      return wrapper.getLynxAccessibilityHelper();
    }
    return null;
  }

  private LynxAccessibilityWrapper getLynxAccessibilityWrapper() {
    if (getLynxUI() != null) {
      LynxContext context = getLynxUI().getLynxContext();
      if (context != null) {
        return context.getLynxAccessibilityWrapper();
      }
    }
    return null;
  }

  private LynxUI getLynxUI() {
    if (mWeakUI != null && mWeakUI.get() != null) {
      return mWeakUI.get();
    }
    return null;
  }
}
