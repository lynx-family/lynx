// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import static com.lynx.tasm.event.LynxTouchEvent.EVENT_CLICK;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TAP;

import android.app.Service;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.Layout;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewParent;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.ViewParentCompat;
import androidx.core.view.accessibility.AccessibilityEventCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.core.view.accessibility.AccessibilityNodeProviderCompat;
import androidx.core.view.accessibility.AccessibilityRecordCompat;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.text.IUIText;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxTouchEvent;
import java.util.Map;

/**
 * LynxAccessibilityDelegate is a class for implementing accessibility support in custom view
 * that represent a collection of view-like logical items.
 */
public class LynxAccessibilityDelegate extends AccessibilityDelegateCompat {
  public static final String TAG = "LynxA11yDelegate";

  public static final boolean DEBUG = false;

  /** Invalid virtual node identifier. */
  public static final int INVALID_NODE_ID = Integer.MIN_VALUE;

  /** Virtual node id for the host view */
  public static final int HOST_NODE_ID = View.NO_ID;

  /** System accessibility manager */
  private final AccessibilityManager mAccessibilityManager;

  /** Host ui */
  private final UIGroup mHostUI;

  /** Host view */
  private final View mHostView;

  /** Id for the virtual node that holds accessibility focus */
  private int mAccessibilityFocusedVirtualViewId = INVALID_NODE_ID;

  /** Id for the virtual node that holds accessibility focus */
  private int mHoveredVirtualId = INVALID_NODE_ID;

  /** Custom virtual node that is currently hovered */
  private LynxNodeProvider mNodeProvider;

  /** Default value AccessibilityElement for all LynxUI */
  protected boolean mEnableAccessibilityElement = true;

  /** Current focused UI */
  private LynxBaseUI mFocusedUI;

  public LynxAccessibilityDelegate(UIGroup hostUI) {
    if (hostUI == null || hostUI.getAccessibilityHostView() == null) {
      throw new IllegalArgumentException("host ui or host view is null");
    }
    mHostUI = hostUI;
    mHostView = hostUI.getAccessibilityHostView();
    mAccessibilityManager = (AccessibilityManager) mHostUI.getLynxContext().getSystemService(
        Service.ACCESSIBILITY_SERVICE);
    // Note: we move creating node provider from getAccessibilityNodeProvider() to constructor
    // method which prevents the mNodeProvider being null when some methods are called, eg:
    // ViewGroup#dispatchHoverEvent()
    mNodeProvider = new LynxNodeProvider(this);
    // Note: Host view must be focusable so that we can delegate to virtual views.
    mHostView.setFocusable(true);
    if (ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_AUTO
        == ViewCompat.getImportantForAccessibility(mHostView)) {
      ViewCompat.setImportantForAccessibility(
          mHostView, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
    }
  }

  @Override
  public AccessibilityNodeProviderCompat getAccessibilityNodeProvider(View host) {
    if (mNodeProvider == null) {
      mNodeProvider = new LynxNodeProvider(this);
    }
    return mNodeProvider;
  }

  /**
   * If view register the delegate, the hover event dispatched by the delegate. Mainly capabilities:
   *
   * 1. Locate the specific ui according to the {@link MotionEvent} using Lynx event distribution
   * mechanism(hitTest).
   * 2. After determining the node that responds to the event, create and send the {@link
   * AccessibilityEvent} to the system.
   *
   * @param event The motion event to be dispatched.
   * @return True if the event was handled by the view, false otherwise.
   */
  public boolean dispatchHoverEvent(MotionEvent event) {
    if (!isSystemAccessibilityEnable()) {
      return false;
    }
    int touchX = (int) event.getX();
    int touchY = (int) event.getY();
    EventTarget target = mHostUI.hitTest(touchX, touchY);
    if (DEBUG) {
      LLog.i(TAG, "dispatchHoverEvent = " + target + " event: [" + touchX + ", " + touchY + "]");
    }
    while (target != null && !(target instanceof LynxBaseUI)) {
      target = target.parent();
    }
    if (target == null || !(target instanceof LynxBaseUI)) {
      return false;
    }
    // If ui is not accessible, using it's parent.
    LynxBaseUI ui = (LynxBaseUI) target;
    while (!mNodeProvider.isAccessibilityElement(ui)) {
      ui = ui.getParentBaseUI();
      if (ui == null) {
        return false;
      }
    }
    final int targetVirtualViewId = mNodeProvider.findVirtualViewIdByUi(ui);
    if (targetVirtualViewId == HOST_NODE_ID) {
      return false;
    }
    if (DEBUG) {
      LLog.i(TAG, "dispatchHoverEvent confirm hover id = " + targetVirtualViewId);
    }
    switch (event.getAction()) {
      case MotionEvent.ACTION_HOVER_ENTER:
        if (mHoveredVirtualId != targetVirtualViewId) {
          sendEventForVirtualView(targetVirtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
          sendEventForVirtualView(mHoveredVirtualId, AccessibilityEvent.TYPE_VIEW_HOVER_EXIT);
          mHoveredVirtualId = targetVirtualViewId;
        }
        return true;
      case MotionEvent.ACTION_HOVER_EXIT:
        if (mHoveredVirtualId != INVALID_NODE_ID) {
          updateHoveredVirtualView(INVALID_NODE_ID);
        }
        return true;
      default:
        return false;
    }
  }

  private void updateHoveredVirtualView(int virtualViewId) {
    if (mHoveredVirtualId == virtualViewId) {
      return;
    }
    sendEventForVirtualView(virtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
    sendEventForVirtualView(mHoveredVirtualId, AccessibilityEvent.TYPE_VIEW_HOVER_EXIT);
    mHoveredVirtualId = virtualViewId;
  }

  protected boolean performActionForHost(int action, Bundle arguments) {
    return ViewCompat.performAccessibilityAction(mHostView, action, arguments);
  }

  protected boolean performActionForChild(
      int virtualViewId, LynxBaseUI ui, int action, Bundle arguments) {
    switch (action) {
      case AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS:
        return requestAccessibilityFocus(virtualViewId);
      case AccessibilityNodeInfoCompat.ACTION_CLEAR_ACCESSIBILITY_FOCUS:
        return clearAccessibilityFocus(virtualViewId);
      case AccessibilityNodeInfo.ACTION_CLICK:
        return fireActionClick(virtualViewId);
      case android.R.id.accessibilityActionShowOnScreen:
        return requestUIRectOnScreen(ui, true, action, arguments);
      default:
        return false;
    }
  }

  /**
   * Fire click or tap event on specified ui
   *
   * @param virtualViewId the identifier of the virtual view on which to fire click or tap event
   */
  private boolean fireActionClick(int virtualViewId) {
    if (virtualViewId >= 0) {
      LynxNodeProvider.LynxCustomNodeInfo node = mNodeProvider.mVirtualChildren.get(virtualViewId);
      LynxBaseUI ui = node.mUI;
      if (ui != null && ui.getLynxContext() != null && ui.getLynxContext().getEventEmitter() != null
          && ui.getAccessibilityEnableTap()) {
        final Rect rect = node.mRectOnScreen;
        LynxTouchEvent.Point globalPoint = new LynxTouchEvent.Point(rect.centerX(), rect.centerY());
        LynxTouchEvent.Point localPoint =
            new LynxTouchEvent.Point(rect.centerX() - rect.left, rect.centerY() - rect.top);
        if (ui.getEvents() != null) {
          Map<String, EventsListener> events = ui.getEvents();
          if (events.containsKey(EVENT_TAP)) {
            ui.getLynxContext().getEventEmitter().sendTouchEvent(
                new LynxTouchEvent(ui.getSign(), EVENT_TAP, localPoint, localPoint, globalPoint));
          }
          if (events.containsKey(EVENT_CLICK)) {
            ui.getLynxContext().getEventEmitter().sendTouchEvent(
                new LynxTouchEvent(ui.getSign(), EVENT_CLICK, localPoint, localPoint, globalPoint));
          }
        }
        return true;
      }
    }
    return false;
  }

  /**
   * Request that ui can be visible on the screen and scrolling if necessary just enough.
   *
   * @param ui current ui
   * @param smooth animated scrolling
   */
  private boolean requestUIRectOnScreen(
      LynxBaseUI ui, boolean smooth, int action, Bundle arguments) {
    if (ui == null) {
      return false;
    }
    if (ui instanceof LynxUI) {
      View currentView = ((LynxUI<?>) ui).getView();
      return currentView.performAccessibilityAction(action, arguments);
    } else if (ui instanceof LynxFlattenUI) {
      boolean scrolled = false;
      LynxBaseUI childUI = ui;
      LynxBaseUI parentUI = ui.getParentBaseUI();
      Rect localPosition = new Rect(0, 0, ui.getWidth(), ui.getHeight());
      Rect tempRect = new Rect();
      while (parentUI != null && parentUI != mHostUI) {
        tempRect.set(localPosition);
        scrolled |= parentUI.requestChildUIRectangleOnScreen(childUI, tempRect, smooth);
        localPosition.offset(childUI.getOriginLeft() - childUI.getScrollX(),
            childUI.getOriginTop() - childUI.getScrollY());
        childUI = parentUI;
        parentUI = childUI.getParentBaseUI();
      }
      return scrolled;
    }
    return false;
  }

  /**
   * Attempts to give accessibility focus to a virtual view.
   *
   * @param ui the LynxBaseUI on which to place accessibility focus
   * @return whether this LynxBaseUI actually took accessibility focus
   */
  public boolean requestAccessibilityFocus(LynxBaseUI ui) {
    if (ui == null || !isSystemAccessibilityEnable() || !mNodeProvider.isAccessibilityElement(ui)) {
      return false;
    }
    int targetVirtualViewId = mNodeProvider.findVirtualViewIdByUi(ui);
    if (targetVirtualViewId == HOST_NODE_ID) {
      return false;
    }
    return requestAccessibilityFocus(targetVirtualViewId);
  }

  /**
   * Attempts to give accessibility focus to a virtual view.
   *
   * @param virtualViewId the identifier of the virtual view on which to place accessibility focus
   * @return whether this virtual view actually took accessibility focus
   */
  private boolean requestAccessibilityFocus(int virtualViewId) {
    if (!isSystemAccessibilityEnable()) {
      return false;
    }
    // TODO(dingwang.wxx): Check virtual view visibility.
    if (mAccessibilityFocusedVirtualViewId != virtualViewId) {
      // Clear focus from the previously focused view, if applicable.
      if (mAccessibilityFocusedVirtualViewId != INVALID_NODE_ID) {
        clearAccessibilityFocus(mAccessibilityFocusedVirtualViewId);
      }
      // Set focus on the new view.
      mAccessibilityFocusedVirtualViewId = virtualViewId;
      mFocusedUI = mNodeProvider.mVirtualChildren.get(mAccessibilityFocusedVirtualViewId).mUI;
      mHostView.invalidate();
      sendEventForVirtualView(
          virtualViewId, AccessibilityEventCompat.TYPE_VIEW_ACCESSIBILITY_FOCUSED);
      LynxAccessibilityWrapper wrapper = getLynxAccessibilityWrapper();
      if (wrapper != null) {
        wrapper.onAccessibilityFocused(mAccessibilityFocusedVirtualViewId, mFocusedUI);
      }
      return true;
    }
    return false;
  }

  /**
   * Attempts to clear accessibility focus from a virtual view.
   *
   * @param virtualViewId the identifier of the virtual view from which to clear accessibility focus
   * @return whether this virtual view actually cleared accessibility focus
   */
  private boolean clearAccessibilityFocus(int virtualViewId) {
    if (mAccessibilityFocusedVirtualViewId == virtualViewId) {
      mAccessibilityFocusedVirtualViewId = INVALID_NODE_ID;
      mHostView.invalidate();
      mFocusedUI = null;
      sendEventForVirtualView(
          virtualViewId, AccessibilityEventCompat.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED);
      return true;
    }
    return false;
  }

  /**
   * Populates an event of the specified type with information about an item and attempts to send it
   * up through the view hierarchy.
   *
   * @param virtualViewId the identifier of the virtual view for which to send an event
   * @param eventType the type of event to send
   * @return whether the event was sent successfully
   */
  private final boolean sendEventForVirtualView(int virtualViewId, int eventType) {
    if ((virtualViewId == INVALID_NODE_ID) || !isSystemAccessibilityEnable()) {
      return false;
    }

    final ViewParent parent = mHostView.getParent();
    if (parent == null) {
      return false;
    }

    final AccessibilityEvent event = createEvent(virtualViewId, eventType);
    return ViewParentCompat.requestSendAccessibilityEvent(parent, mHostView, event);
  }

  /**
   * Constructs and returns an AccessibilityEvent for the specified virtual view id, which includes
   * the host view (HOST_ID).
   *
   * @param virtualViewId the identifier of the virtual view for which to send an event
   * @param eventType the type of event to send
   * @return AccessibilityEvent populated with information
   */
  private AccessibilityEvent createEvent(int virtualViewId, int eventType) {
    switch (virtualViewId) {
      case HOST_NODE_ID:
        return createEventForHost(eventType);
      default:
        return createEventForChild(virtualViewId, eventType);
    }
  }

  private AccessibilityEvent createEventForHost(int eventType) {
    final AccessibilityEvent event = AccessibilityEvent.obtain(eventType);
    mHostView.onInitializeAccessibilityEvent(event);
    return event;
  }

  private AccessibilityEvent createEventForChild(int virtualViewId, int eventType) {
    final AccessibilityEvent event = AccessibilityEvent.obtain(eventType);
    final AccessibilityNodeInfoCompat node = mNodeProvider.createNodeForChild(virtualViewId);
    if (node != null) {
      event.getText().add(node.getText());
      event.setContentDescription(node.getContentDescription());
      event.setScrollable(node.isScrollable());
      event.setPassword(node.isPassword());
      event.setEnabled(node.isEnabled());
      event.setChecked(node.isChecked());
      event.setClassName(node.getClassName());
    }
    AccessibilityRecordCompat.setSource(event, mHostView, virtualViewId);
    event.setPackageName(mHostView.getContext().getPackageName());
    return event;
  }

  private boolean isSystemAccessibilityEnable() {
    LynxAccessibilityWrapper wrapper = getLynxAccessibilityWrapper();
    if (wrapper != null) {
      return wrapper.isSystemAccessibilityEnable();
    }
    return false;
  }

  private LynxAccessibilityWrapper getLynxAccessibilityWrapper() {
    if (mHostUI != null && mHostUI.getLynxContext() != null) {
      return mHostUI.getLynxContext().getLynxAccessibilityWrapper();
    }
    return null;
  }

  protected UIGroup getHostUI() {
    return this.mHostUI;
  }

  protected View getHostView() {
    return this.mHostView;
  }

  protected int getAccessibilityFocusedVirtualViewId() {
    return this.mAccessibilityFocusedVirtualViewId;
  }

  protected LynxBaseUI getFocusedUI() {
    return this.mFocusedUI;
  }

  public void setConfigEnableAccessibilityElement(boolean value) {
    this.mEnableAccessibilityElement = value;
  }
}
