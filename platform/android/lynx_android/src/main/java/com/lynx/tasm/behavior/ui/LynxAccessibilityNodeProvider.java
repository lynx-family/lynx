// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static android.view.View.IMPORTANT_FOR_ACCESSIBILITY_NO;
import static android.view.View.IMPORTANT_FOR_ACCESSIBILITY_YES;
import static android.view.View.VISIBLE;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_DEFAULT;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_TRUE;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_CLICK;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TAP;

import android.app.Service;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeProvider;
import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * To support android accessilibity-service
 * This is the provider that exposes the virtual View tree to accessibility
 * services. From the perspective of an accessibility service the
 * AccessibilityNodeInfos it receives while exploring the sub-tree
 * rooted at this View will be the same as the ones it received while
 * exploring a View containing a sub-tree composed of real Views.
 */
public class LynxAccessibilityNodeProvider extends AccessibilityNodeProvider {
  private static String TAG = "LynxAccessibilityNodeProvider";

  private final int SECTION_COUNT = 50;
  private int mPixelPerSection;

  private final UIGroup mHost;
  private final ArrayList<LynxAccessibilityNodeInfo> mChildren = new ArrayList<>();
  private final View mHostView;
  private final AccessibilityManager mAccessibilityManager;
  private LynxAccessibilityNodeInfo mLastHoveredChild;
  private boolean mSendingHoverAccessibilityEvents;
  private boolean mHasHierarchyElement = false;

  // default value AccessibilityElement for all LynxUI
  boolean mEnableAccessibilityElement = true;
  // enable overlap for all LynxUI. default, show accessibility for overlap ui
  boolean mEnableOverlapForAccessibilityElement = true;

  public LynxAccessibilityNodeProvider(UIGroup host) {
    mHost = host;
    mHostView = mHost.getRealParentView();
    mAccessibilityManager =
        (AccessibilityManager) host.mContext.getSystemService(Service.ACCESSIBILITY_SERVICE);
    mPixelPerSection = host.getLynxContext().getScreenMetrics().heightPixels / SECTION_COUNT;
  }

  @Override
  public AccessibilityNodeInfo createAccessibilityNodeInfo(int virtualViewId) {
    AccessibilityNodeInfo info = null;
    LLog.i(TAG, "createAccessibilityNodeInfo: " + virtualViewId);
    if (virtualViewId == View.NO_ID) {
      info = AccessibilityNodeInfo.obtain(mHostView);
      findAllAccessibilityNodeOfLynx();
      mHostView.onInitializeAccessibilityNodeInfo(info);
      for (int i = 0; i < mChildren.size(); i++) {
        info.addChild(mHostView, i);
      }

      Rect hostViewRect = new Rect();
      getLeftAndTopOfBoundsInScreen(mHostView, hostViewRect);
      hostViewRect.set(hostViewRect.left, hostViewRect.top, hostViewRect.left + mHost.getWidth(),
          hostViewRect.top + mHost.getHeight());
    } else { // create AccessibilityNodeInfo for every LynxBaseUI
      if (virtualViewId < 0 || virtualViewId >= mChildren.size()) {
        return null;
      }
      LynxAccessibilityNodeInfo lynxNode = mChildren.get(virtualViewId);
      info = AccessibilityNodeInfo.obtain(mHostView, virtualViewId);
      mHostView.onInitializeAccessibilityNodeInfo(info);

      if (lynxNode.mUI != null) {
        lynxNode.mRectOnScreen = getBoundsOnScreenOfLynxBaseUI(lynxNode.mUI);
        info.setBoundsInScreen(lynxNode.mRectOnScreen);
        info.setClassName(lynxNode.mUI.getClass().getName());
        String label = getAccessibilityLabelWithChild(lynxNode.mUI);
        info.setContentDescription(label);
        info.setText(label);
        info.setScrollable(lynxNode.mUI.isScrollable());
        info.setLongClickable(lynxNode.mUI.isLongClickable());
        info.setFocusable(lynxNode.mUI.isFocusable());
        info.setClickable(isUIClickable(lynxNode.mUI));
        LLog.i(TAG, "Label for UI: " + virtualViewId + ", " + label);
        if (lynxNode.mUI.getAccessibilityEnableTap() && isUIClickable(lynxNode.mUI)) {
          info.addAction(AccessibilityNodeInfo.ACTION_CLICK);
        }
      } else if (lynxNode.mView != null && ViewCompat.isAttachedToWindow(lynxNode.mView)) {
        lynxNode.mView.onInitializeAccessibilityNodeInfo(info);
        info.setSource(mHostView, virtualViewId);
      }
      info.setParent(mHostView);
      info.addAction(mLastHoveredChild != lynxNode
              ? AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS
              : AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS);
      info.setAccessibilityFocused(mLastHoveredChild == lynxNode);
      info.setFocused(mLastHoveredChild == lynxNode);
      info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
      info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
      info.setVisibleToUser(true);
    }
    return info;
  }

  /* copy from Android SDK demo */
  @Override
  public List<AccessibilityNodeInfo> findAccessibilityNodeInfosByText(
      String searched, int virtualViewId) {
    List<AccessibilityNodeInfo> result = new ArrayList<AccessibilityNodeInfo>();
    if (searched == null) {
      return result;
    }
    searched = searched.toLowerCase();
    if (virtualViewId == View.NO_ID) {
      for (int i = 0; i < mChildren.size(); ++i) {
        if (mChildren.get(i).mUI != null) {
          CharSequence label = getAccessibilityLabel(mChildren.get(i).mUI);
          if (label != null) {
            String textToLowerCase = label.toString().toLowerCase();
            if (textToLowerCase.contains(searched)) {
              result.add(createAccessibilityNodeInfo(i));
            }
          }
        }
      }
    } else {
      if (virtualViewId <= 0 || virtualViewId >= mChildren.size()) {
        return result;
      }
      LynxBaseUI ui = mChildren.get(virtualViewId).mUI;
      if (ui != null) {
        CharSequence label = getAccessibilityLabel(ui);
        if (label != null) {
          String textToLowerCase = label.toString().toLowerCase();
          if (textToLowerCase.contains(searched)) {
            result.add(createAccessibilityNodeInfo(virtualViewId));
          }
        }
      }
    }
    return result;
  }

  @Override
  public boolean performAction(int virtualViewId, int action, Bundle arguments) {
    LynxAccessibilityNodeInfo node = null;
    LLog.i(TAG, "performAction on virtualViewId " + virtualViewId + " action " + action);
    if (virtualViewId == View.NO_ID) {
      return false;
    } else {
      if (virtualViewId < 0 || virtualViewId >= mChildren.size()) {
        return false;
      }
    }
    switch (action) {
      case AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS:
        sendAccessibilityEventForLynxUI(
            virtualViewId, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED);
        sendAccessibilityEventForLynxUI(virtualViewId, AccessibilityEvent.TYPE_VIEW_SELECTED);
        return true;
      case AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS:
        sendAccessibilityEventForLynxUI(
            virtualViewId, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED);
        return true;
      case AccessibilityNodeInfo.ACTION_CLICK:
        if (virtualViewId >= 0) {
          node = mChildren.get(virtualViewId);
          LynxBaseUI ui = node.mUI;
          if (ui != null && ui.getLynxContext() != null
              && ui.getLynxContext().getEventEmitter() != null && ui.getAccessibilityEnableTap()) {
            final Rect rect = node.mRectOnScreen;
            LynxTouchEvent.Point globalPoint =
                new LynxTouchEvent.Point(rect.centerX(), rect.centerY());
            LynxTouchEvent.Point localPoint =
                new LynxTouchEvent.Point(rect.centerX() - rect.left, rect.centerY() - rect.top);
            if (ui.mEvents != null) {
              if (ui.mEvents.containsKey(EVENT_TAP)) {
                ui.getLynxContext().getEventEmitter().sendTouchEvent(new LynxTouchEvent(
                    ui.getSign(), EVENT_TAP, localPoint, localPoint, globalPoint));
              } else if (ui.mEvents.containsKey(EVENT_CLICK)) {
                ui.getLynxContext().getEventEmitter().sendTouchEvent(new LynxTouchEvent(
                    ui.getSign(), EVENT_CLICK, localPoint, localPoint, globalPoint));
              }
            }
            return true;
          }
        }
      case AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD:
        break;
      case AccessibilityNodeInfo.ACTION_SCROLL_FORWARD:
        break;
    }

    return false;
  }

  public boolean onHover(View view, MotionEvent event) {
    // the event will hit one of child, or UIGroup self
    EventTarget target = mHost.hitTest((int) event.getX(), (int) event.getY());
    LLog.i(TAG,
        "onHover with target = " + target + " event: [" + (int) event.getX() + ", "
            + (int) event.getY() + "]");
    while (target != null && !(target instanceof LynxBaseUI)) {
      target = target.parent();
    }
    if (target == null || !(target instanceof LynxBaseUI)) {
      return false;
    }

    LynxBaseUI childUI = (LynxBaseUI) target;
    // if node is not accessible, using it parent
    while (!isAccessibilityElement(childUI)) {
      childUI = childUI.getParentBaseUI();
      if (childUI == null) {
        return false;
      }
    }

    int firstVirtualViewId = -1; // default value is UIGroup self
    LynxAccessibilityNodeInfo targetNode = null;
    for (int i = mChildren.size() - 1; i >= 0; --i) {
      if (mChildren.get(i).mUI == childUI) {
        firstVirtualViewId = i;
        break;
      }
    }
    Rect rect = new android.graphics.Rect();
    getLeftAndTopOfBoundsInScreen(mHostView, rect);
    int x = (int) event.getX() + rect.left;
    int y = (int) event.getY() + rect.top;
    int virtualViewId = firstVirtualViewId;
    for (int i = mChildren.size() - 1; i >= firstVirtualViewId && i >= 0; --i) {
      if (mChildren.get(i).mRectOnScreen.contains(x, y)) {
        virtualViewId = i;
        break;
      }
    }

    if (virtualViewId < 0) {
      return false;
    }
    LLog.i(TAG, "onHover confirm virtualViewId = " + virtualViewId);
    final int action = event.getAction();
    switch (action) {
      case MotionEvent.ACTION_HOVER_ENTER: {
        event.setAction(MotionEvent.ACTION_HOVER_ENTER);
        onHoverLynxUI(virtualViewId, event);
        mLastHoveredChild = targetNode;
        break;
      }
      case MotionEvent.ACTION_HOVER_MOVE: {
        if (targetNode == mLastHoveredChild) {
          onHoverLynxUI(virtualViewId, event);
        } else {
          event.setAction(MotionEvent.ACTION_HOVER_ENTER);
          onHoverLynxUI(virtualViewId, event);
          mLastHoveredChild = targetNode;
        }
        break;
      }
      case MotionEvent.ACTION_HOVER_EXIT: {
        mLastHoveredChild = null;
        onHoverLynxUI(virtualViewId, event);
        break;
      }
    }
    return true;
  }

  private void onHoverLynxUI(int virtualViewId, MotionEvent event) {
    //  each virtual View should fire a corresponding accessibility event whose source is that
    //  virtual view. Accessibility services get the event source as the entry point of the
    //  APIs for querying the window content.
    final int action = event.getAction();

    if (!mSendingHoverAccessibilityEvents) {
      if ((action == MotionEvent.ACTION_HOVER_ENTER || action == MotionEvent.ACTION_HOVER_MOVE)) {
        sendAccessibilityEventForLynxUI(virtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
        mSendingHoverAccessibilityEvents = true;
      }
    } else {
      if (action == MotionEvent.ACTION_HOVER_EXIT || (action == MotionEvent.ACTION_HOVER_MOVE)) {
        mSendingHoverAccessibilityEvents = false;
        sendAccessibilityEventForLynxUI(virtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_EXIT);
      }
    }

    switch (action) {
      case MotionEvent.ACTION_HOVER_ENTER:
        mHostView.setHovered(true);
        break;
      case MotionEvent.ACTION_HOVER_EXIT:
        mHostView.setHovered(false);
        break;
    }
  }

  private void sendAccessibilityEventForLynxUI(int virtualViewId, int eventType) {
    if (mAccessibilityManager.isTouchExplorationEnabled()) {
      AccessibilityEvent event = AccessibilityEvent.obtain(eventType);
      LynxAccessibilityNodeInfo node = null;
      if (virtualViewId >= 0) { // event from child
        node = mChildren.get(virtualViewId);
        if (node.mUI != null) {
          event.setPackageName(mHostView.getContext().getPackageName());
          event.setClassName(node.mUI.getClass().getName());
          event.setEnabled(true);
          String label = getAccessibilityLabelWithChild(node.mUI);
          event.setContentDescription(label);
        } else if (node.mView != null) {
          node.mView.onInitializeAccessibilityEvent(event);
        } else {
          return; // never reach
        }
        event.setSource(mHostView, virtualViewId);
        mHostView.invalidate();
        if (mHostView.getParent() == null) {
          LLog.e(TAG, "sendAccessibilityEventForLynxUI failed, parent is null.");
          return;
        }
        mHostView.getParent().requestSendAccessibilityEvent(mHostView, event);
      }
    }
  }

  /* travel lynxUI tree deep, try find all AccessibilityElement which is on top level of UI (visible
   * to user) */
  private void findAllAccessibilityNodeOfLynxDeep(LynxBaseUI node, List<Rect> topRects) {
    ArrayList<String> accessibilityElements = node.getAccessibilityElements();
    if (accessibilityElements != null) {
      Rect outBounds = getBoundsOnScreenOfLynxBaseUI(node);
      LynxAccessibilityNodeInfo nodeInfo = new LynxAccessibilityNodeInfo(node, outBounds);
      nodeInfo.mIsHierarchy = true;
      mChildren.add(nodeInfo);
      mHasHierarchyElement = true;
      return;
    }
    // Note: UIShadowProxy override getChildren() to return real children.
    // UIShadowProxy -> real ui -> real children
    for (int i = node.getChildren().size() - 1; i >= 0; --i) {
      LynxBaseUI child = node.getChildren().get(i);
      // Note: We should ignore overlay ui using LynxViewVisibleHelper interface because overlay
      // view is in the different window with LynxView.
      if ((child instanceof LynxUI && !ViewCompat.isAttachedToWindow(((LynxUI) child).getView()))
          || (child instanceof LynxViewVisibleHelper)) {
        continue;
      }
      findAllAccessibilityNodeOfLynxDeep(child, topRects);
    }
    if (node != mHost) {
      LynxBaseUI realUI = node;
      if (node instanceof UIShadowProxy) {
        realUI = ((UIShadowProxy) node).getChild();
      }
      if (isAccessibilityElement(realUI)) {
        Rect outBounds = getBoundsOnScreenOfLynxBaseUI(realUI);
        if (!mEnableOverlapForAccessibilityElement) {
          // overlay means another UI occlude this ui
          boolean overlap = false;
          for (Rect tmp : topRects) {
            if (tmp.contains(outBounds)) {
              overlap = true;
              break;
            }
          }
          if (!overlap) {
            mChildren.add(new LynxAccessibilityNodeInfo(realUI, outBounds));
          }
          topRects.add(outBounds);
        } else {
          mChildren.add(new LynxAccessibilityNodeInfo(realUI, outBounds));
        }
      }
      // for leaf node, find it all children android view
      if (realUI instanceof LynxUI && realUI.mChildren.isEmpty()) {
        LynxUI leaf = (LynxUI) realUI;
        if (leaf.getView() instanceof ViewGroup) {
          ViewGroup group = (ViewGroup) leaf.getView();
          for (int j = 0; j < group.getChildCount(); j++) {
            findAllAccessibilityNodeOfViews(group.getChildAt(j));
          }
        }
      }
    }
  }

  // Traverse all LynxAccessibilityNodeInfo, if one's ui has accessibility-elements which is used to
  // mark the focus order, create all new LynxAccessibilityNodeInfo and add to mChildren.
  private void findAllAccessibilityNodeOfHierarchy() {
    ArrayList<LynxAccessibilityNodeInfo> newNodeInfoList = new ArrayList<>();
    for (LynxAccessibilityNodeInfo nodeInfo : mChildren) {
      if (nodeInfo.mUI != null && nodeInfo.mIsHierarchy) {
        ArrayList<String> accessibilityElements = nodeInfo.mUI.getAccessibilityElements();
        if (accessibilityElements != null && mHost.getLynxContext() != null
            && mHost.getLynxContext().getLynxUIOwner() != null) {
          LynxUIOwner owner = mHost.getLynxContext().getLynxUIOwner();
          for (String elementId : accessibilityElements) {
            LynxBaseUI childUI = owner.findLynxUIByIdSelector(elementId);
            if (childUI == null
                || (childUI instanceof LynxUI
                    && !ViewCompat.isAttachedToWindow(((LynxUI) childUI).getView()))) {
              continue;
            }
            LynxBaseUI realUI = childUI;
            if (childUI instanceof UIShadowProxy) {
              realUI = ((UIShadowProxy) childUI).getChild();
            }
            if (isAccessibilityElement(realUI)) {
              Rect outBounds = getBoundsOnScreenOfLynxBaseUI(realUI);
              newNodeInfoList.add(new LynxAccessibilityNodeInfo(realUI, outBounds));
            }
          }
        }
      } else {
        newNodeInfoList.add(nodeInfo);
      }
    }
    mChildren.clear();
    mChildren.addAll(newNodeInfoList);
  }

  private void findAllAccessibilityNodeOfLynx() {
    mChildren.clear();
    findAllAccessibilityNodeOfLynxDeep(mHost, new ArrayList<Rect>());

    // sort children by coordinates, follow top-left
    // Assign a segment number according to the top value / mPixelPerSection
    Collections.sort(mChildren, new Comparator() {
      @Override
      public int compare(Object o1, Object o2) {
        Rect rect1 = ((LynxAccessibilityNodeInfo) o1).mRectOnScreen;
        Rect rect2 = ((LynxAccessibilityNodeInfo) o2).mRectOnScreen;
        int yCompare = mPixelPerSection == 0
            ? (rect1.top - rect2.top)
            : (rect1.top / mPixelPerSection - rect2.top / mPixelPerSection);
        int xCompare = rect1.left - rect2.left;
        return yCompare == 0 ? xCompare : yCompare;
      }
    });
    if (mHasHierarchyElement) {
      findAllAccessibilityNodeOfHierarchy();
      mHasHierarchyElement = false;
    }
  }

  private void findAllAccessibilityNodeOfViews(View view) {
    boolean visible = view.getVisibility() == VISIBLE;
    boolean isAccessible = (view.getImportantForAccessibility() == IMPORTANT_FOR_ACCESSIBILITY_YES)
        || (view.getImportantForAccessibility() != IMPORTANT_FOR_ACCESSIBILITY_NO
            && !TextUtils.isEmpty(view.getContentDescription()));
    if (visible && isAccessible) {
      // this view is accessible
      Rect rect = new Rect();
      getLeftAndTopOfBoundsInScreen(view, rect);
      mChildren.add(new LynxAccessibilityNodeInfo(view, rect));
    }
    // check children accessibility
    if (view instanceof ViewGroup) {
      ViewGroup group = (ViewGroup) view;
      for (int i = 0; i < group.getChildCount(); i++) {
        findAllAccessibilityNodeOfViews(group.getChildAt(i));
      }
    }
  }

  /* copy from Flutter, find left and top of bounds of view on Screen*/
  private static void getLeftAndTopOfBoundsInScreen(View view, Rect bounds) {
    bounds.set(0, 0, view.getRight() - view.getLeft(), view.getBottom() - view.getTop());
    int[] locationOnScreen = new int[2];
    view.getLocationOnScreen(locationOnScreen);
    bounds.offset(locationOnScreen[0], locationOnScreen[1]);
  }

  private Rect getBoundsOnScreenOfLynxBaseUI(LynxBaseUI ui) {
    Rect outBounds = new Rect();
    if (ui instanceof LynxUI) {
      // get bounds from LynxUI's mView
      LynxUI realLynxUI = (LynxUI) ui;
      View currentView = realLynxUI.getView();
      if (ui instanceof UIShadowProxy) {
        // get real ui of UIShadowProxy
        LynxBaseUI shadowChild = ((UIShadowProxy) ui).getChild();
        if (shadowChild instanceof LynxUI) {
          currentView = ((LynxUI) shadowChild).getView();
        }
      }
      getLeftAndTopOfBoundsInScreen(currentView, outBounds);
      outBounds.set(outBounds.left, outBounds.top, outBounds.left + ui.getWidth(),
          outBounds.top + ui.getHeight());
    } else if (ui instanceof LynxFlattenUI) {
      // get bounds from parent of LynxFlattenUI, then offset
      LynxBaseUI realLynxUI = ui.getParentBaseUI();
      while (realLynxUI != null && !(realLynxUI instanceof LynxUI)) {
        realLynxUI = realLynxUI.getParentBaseUI();
      }
      if (realLynxUI != null) {
        View parent = ((LynxUI) realLynxUI).getView();
        if (realLynxUI instanceof UIGroup) {
          parent = ((UIGroup) realLynxUI).getRealParentView();
        }
        getLeftAndTopOfBoundsInScreen(parent, outBounds);
        outBounds.offset(-parent.getScrollX(), -parent.getScrollY());
        outBounds.offset(ui.getLeft(), ui.getTop());
        outBounds.set(outBounds.left, outBounds.top, outBounds.left + ui.getWidth(),
            outBounds.top + ui.getHeight());
      }
    }
    return outBounds;
  }

  /* read the accessibility label from Front-end */
  private static String getAccessibilityLabel(LynxBaseUI ui) {
    CharSequence label = ui.getAccessibilityLabel();
    if (label == null) {
      label = "";
    }
    return label.toString();
  }

  /* read the accessibility label includes children */
  private String getAccessibilityLabelWithChild(LynxBaseUI ui) {
    if (!isAccessibilityElement(ui)) {
      return "";
    }
    CharSequence label = getAccessibilityLabel(ui);
    if (TextUtils.isEmpty(label)) {
      for (LynxBaseUI child : ui.mChildren) {
        label = label + getAccessibilityLabel(child);
      }
    }
    return label.toString();
  }

  private boolean isUIClickable(LynxBaseUI ui) {
    return ui != null && ui.mEvents != null
        && (ui.mEvents.containsKey(EVENT_CLICK) || ui.mEvents.containsKey(EVENT_TAP));
  }

  private boolean isAccessibilityElement(LynxBaseUI ui) {
    if (ui == null) {
      return false;
    }
    if (ui.getAccessibilityElementStatus() == ACCESSIBILITY_ELEMENT_DEFAULT) {
      return mEnableAccessibilityElement;
    }
    return ui.getAccessibilityElementStatus() == ACCESSIBILITY_ELEMENT_TRUE;
  }

  public void setConfigEnableAccessibilityElement(boolean configEnableAccessibilityElement) {
    mEnableAccessibilityElement = configEnableAccessibilityElement;
  }

  public void setConfigEnableOverlapForAccessibilityElement(
      boolean configEnableOverlapForAccessibilityElement) {
    mEnableOverlapForAccessibilityElement = configEnableOverlapForAccessibilityElement;
  }

  private static class LynxAccessibilityNodeInfo {
    LynxBaseUI mUI;
    View mView;
    Rect mRectOnScreen;
    boolean mIsHierarchy;

    public LynxAccessibilityNodeInfo(@NonNull LynxBaseUI ui, @NonNull Rect rect) {
      mUI = ui;
      mView = null;
      mRectOnScreen = rect;
    }

    public LynxAccessibilityNodeInfo(@NonNull View view, @NonNull Rect rect) {
      mUI = null;
      mView = view;
      mRectOnScreen = rect;
    }
  }
}
