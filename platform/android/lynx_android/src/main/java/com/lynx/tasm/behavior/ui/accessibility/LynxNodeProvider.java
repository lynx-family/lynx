// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import static android.view.View.IMPORTANT_FOR_ACCESSIBILITY_NO;
import static android.view.View.IMPORTANT_FOR_ACCESSIBILITY_YES;
import static android.view.View.VISIBLE;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityDelegate.DEBUG;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityDelegate.HOST_NODE_ID;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.*;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_CLICK;
import static com.lynx.tasm.event.LynxTouchEvent.EVENT_TAP;

import android.graphics.Rect;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityNodeInfo;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.core.view.accessibility.AccessibilityNodeProviderCompat;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.LynxViewVisibleHelper;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.behavior.ui.list.UIList;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * This is the provider that exposes the virtual view tree to accessibility services.
 */
public class LynxNodeProvider extends AccessibilityNodeProviderCompat {
  private static final String TAG = "LynxA11yNodeProvider";

  /**
   * Used to calculate mXPixelPerSection / mYPixelPerSection, and 50 is a relative reasonable
   * value.
   */
  public static final int SECTION_COUNT = 50;

  /** Accessibility delegate */
  private WeakReference<LynxAccessibilityDelegate> mDelegateWeakRef;

  /** Host ui */
  private UIGroup mHostUI;

  /** Host view */
  private View mHostView;

  /**
   * Sort children by coordinates, following top-left. Assign a segment number according to the top
   * value / pixelPerSection.
   */
  private int mXPixelPerSection;
  private int mYPixelPerSection;

  /**
   * Whether has hierarchy element in virtual a11y node tree. For supporting accessibility-elements
   * feature.
   */
  private boolean mHasHierarchyElement;

  /** All virtual a11y node */
  protected ArrayList<LynxCustomNodeInfo> mVirtualChildren;

  public LynxNodeProvider(LynxAccessibilityDelegate delegate) {
    if (delegate != null) {
      LLog.i(TAG, "Create LynxNodeProvider for " + delegate.getHostUI());
      mDelegateWeakRef = new WeakReference<>(delegate);
      mHostUI = delegate.getHostUI();
      mHostView = delegate.getHostView();
      mVirtualChildren = new ArrayList<>();
      mXPixelPerSection = mHostUI.getLynxContext().getScreenMetrics().widthPixels / SECTION_COUNT;
      mYPixelPerSection = mHostUI.getLynxContext().getScreenMetrics().heightPixels / SECTION_COUNT;
    }
  }

  @Nullable
  @Override
  public AccessibilityNodeInfoCompat createAccessibilityNodeInfo(int virtualViewId) {
    // Just in case, we add try-catch here.
    try {
      if (HOST_NODE_ID == virtualViewId) {
        return AccessibilityNodeInfoCompat.obtain(createNodeForHost());
      }
      AccessibilityNodeInfoCompat nodeInfoCompat =
          AccessibilityNodeInfoCompat.obtain(createNodeForChild(virtualViewId));
      // We set IMPORTANT_FOR_ACCESSIBILITY_NO to LynxUI's view after createNodeForChild.
      setImportantForAccessibilityNo(virtualViewId);
      return nodeInfoCompat;
    } catch (Exception exception) {
      LLog.e(TAG,
          "createAccessibilityNodeInfo with virtual view id = " + virtualViewId
              + " with virtual children size = " + mVirtualChildren.size()
              + ", and exception msg = " + exception.getMessage());
      return AccessibilityNodeInfoCompat.obtain();
    }
  }

  @Override
  public boolean performAction(int virtualViewId, int action, Bundle arguments) {
    if (virtualViewId < HOST_NODE_ID || virtualViewId >= mVirtualChildren.size()) {
      LLog.e(TAG, "performAction: check virtualViewId failed");
      return false;
    }
    if (mDelegateWeakRef != null && mDelegateWeakRef.get() != null) {
      LynxAccessibilityDelegate delegate = mDelegateWeakRef.get();
      switch (virtualViewId) {
        case HOST_NODE_ID:
          if (DEBUG) {
            LLog.i(TAG, "performAction for host: " + mHostView + ", " + action);
          }
          return delegate.performActionForHost(action, arguments);
        default:
          final LynxCustomNodeInfo node = mVirtualChildren.get(virtualViewId);
          if (DEBUG) {
            LLog.i(TAG, "performAction for child: " + virtualViewId + ", " + action);
          }
          return delegate.performActionForChild(virtualViewId, node.mUI, action, arguments);
      }
    }
    return false;
  }

  /**
   * Finds {@link AccessibilityNodeInfoCompat} by text and if virtualViewId is equal HOST_NODE_ID,
   * we will return all nodes which are descendant of the host view.
   *
   * @param text The searched text.
   * @param virtualViewId the virtual view id for item for which to construct a node.
   * @Returns List of node info.
   */
  @Nullable
  @Override
  public List<AccessibilityNodeInfoCompat> findAccessibilityNodeInfosByText(
      String text, int virtualViewId) {
    List<AccessibilityNodeInfoCompat> result = new ArrayList<>();
    if (text == null) {
      return result;
    }
    text = text.toLowerCase();
    if (virtualViewId == HOST_NODE_ID) {
      for (int i = 0; i < mVirtualChildren.size(); ++i) {
        if (mVirtualChildren.get(i).mUI != null) {
          CharSequence label = getAccessibilityLabel(mVirtualChildren.get(i).mUI);
          if (label != null) {
            String textToLowerCase = label.toString().toLowerCase();
            if (textToLowerCase.contains(text)) {
              result.add(createAccessibilityNodeInfo(i));
            }
          }
        }
      }
    } else if (virtualViewId > HOST_NODE_ID && virtualViewId < mVirtualChildren.size()) {
      LynxBaseUI ui = mVirtualChildren.get(virtualViewId).mUI;
      if (ui != null) {
        CharSequence label = getAccessibilityLabel(ui);
        if (label != null) {
          String textToLowerCase = label.toString().toLowerCase();
          if (textToLowerCase.contains(text)) {
            result.add(createAccessibilityNodeInfo(virtualViewId));
          }
        }
      }
    }
    return result;
  }

  /**
   * Constructs and returns an accessibility node info for the host view.
   *
   * @return an {@link AccessibilityNodeInfoCompat} for the specified host view.
   */
  protected AccessibilityNodeInfoCompat createNodeForHost() {
    final AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain(mHostView);
    ViewCompat.onInitializeAccessibilityNodeInfo(mHostView, info);
    findAllAccessibilityNode(mHostUI, mVirtualChildren);
    if (info.getChildCount() > 0 && mVirtualChildren.size() > 0) {
      LLog.e(TAG,
          "Views cannot have both real and virtual children, with real child count = "
              + info.getChildCount() + "and virtual child count = " + mVirtualChildren.size());
    }
    LLog.i(TAG, "createNodeForHost with child count = " + mVirtualChildren.size());
    // Add the virtual descendants.
    for (int i = 0; i < mVirtualChildren.size(); i++) {
      info.addChild(mHostView, i);
    }
    LynxBaseUI focusedUI = mDelegateWeakRef.get().getFocusedUI();
    int focusedId = mDelegateWeakRef.get().getAccessibilityFocusedVirtualViewId();
    if (focusedUI != null && focusedUI.getAccessibilityKeepFocused() && focusedId != HOST_NODE_ID) {
      for (int i = 0; i < mVirtualChildren.size(); ++i) {
        LynxBaseUI ui = mVirtualChildren.get(i).mUI;
        if (ui != null && ui == focusedUI && i != focusedId) {
          if (DEBUG) {
            LLog.i(TAG, "Focused id changed: " + i + " -> " + focusedId);
          }
          if (getScreenVisibleRectOfUI(ui, new Rect())) {
            performAction(i, AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS, null);
          }
        }
      }
    }
    if (DEBUG) {
      LLog.i(TAG, "create host with child count = " + mVirtualChildren.size());
    }
    return info;
  }

  /**
   * Constructs and returns an accessibility node info for the specified child item
   * and automatically manages accessibility actions.
   *
   * @param virtualViewId the virtual view id for item for which to construct a node
   * @return an {@link AccessibilityNodeInfoCompat} for the specified child item.
   */
  protected AccessibilityNodeInfoCompat createNodeForChild(int virtualViewId) {
    final AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain();
    if (virtualViewId == HOST_NODE_ID || virtualViewId >= mVirtualChildren.size()) {
      LLog.e(TAG, "createNodeForChild: check virtualViewId failed");
      return info;
    }
    // Default
    info.setEnabled(true);
    info.setFocusable(true);
    info.setClassName(mHostUI.getClass().getName());
    info.setBoundsInParent(INVALID_BOUNDS);
    info.setBoundsInScreen(INVALID_BOUNDS);
    info.setParent(mHostView);
    info.setSource(mHostView, virtualViewId);
    info.setPackageName(mHostView.getContext().getPackageName());
    final LynxCustomNodeInfo node = mVirtualChildren.get(virtualViewId);
    if (node.mUI != null) {
      // Note: Invoking setImportantForAccessibility() in createNodeForChild() is npe-risky because
      // setImportantForAccessibility() may trigger mView#sendAccessibilityEventUnchecked() and
      // invoke createNodeForHost() to clear mVirtualChildren that causes the node.mUI to be set to
      // null. For more detail can be found in
      // ViewRootImpl#SendWindowContentChangedAccessibilityEvent.
      LynxBaseUI parentUI = node.mUI;
      while (parentUI != null && parentUI != mHostUI) {
        if (parentUI instanceof UIGroup && parentUI.isScrollContainer()) {
          UIGroup group = (UIGroup) parentUI;
          View parentView = group.getAccessibilityHostView();
          if (parentView != null) {
            info.setParent(parentView);
            break;
          }
        }
        parentUI = (LynxBaseUI) parentUI.getParent();
      }
      String label = getAccessibilityLabelWithChild(node.mUI);
      // Note: currently do not set focusable to the value of node.mUI.isFocusable()
      // info.setFocusable(node.mUI.isFocusable());
      info.setText(label);
      info.setContentDescription(label);
      info.setClassName(node.mUI.getClass().getName());
      // The old code logic is that if ui set a11y-enable-tap and is clickable, info will add
      // ACTION_CLICK, this may cause additional click event.
      // We delete check clickable in new logic to enable virtual view consume ACTION_CLICK that
      // will not fire LynxView#onTouchEvent().
      info.setClickable(isUIClickable(node.mUI));
      if (node.mUI.getAccessibilityEnableTap()) {
        info.addAction(AccessibilityNodeInfo.ACTION_CLICK);
      }
      // Set bounds in parent
      info.setBoundsInParent(node.mUI.getBoundingClientRect());
      // Set visible to user
      Rect globalVisibleRect = new Rect();
      boolean isGlobalVisible = getScreenVisibleRectOfUI(node.mUI, globalVisibleRect);
      info.setVisibleToUser(isGlobalVisible);
      if (isGlobalVisible) {
        info.setBoundsInScreen(globalVisibleRect);
      }
      // Set accessibility focus state
      if (mDelegateWeakRef.get().getAccessibilityFocusedVirtualViewId() == virtualViewId) {
        info.setAccessibilityFocused(true);
        info.addAction(AccessibilityNodeInfoCompat.ACTION_CLEAR_ACCESSIBILITY_FOCUS);
      } else {
        info.setAccessibilityFocused(false);
        info.addAction(AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS);
      }
    }
    if (DEBUG) {
      Rect tempGlobalRect = new Rect();
      info.getBoundsInScreen(tempGlobalRect);
      LLog.i(TAG,
          "create child: " + virtualViewId
              + ", focused id: " + mDelegateWeakRef.get().getAccessibilityFocusedVirtualViewId()
              + ", focused ui: " + mDelegateWeakRef.get().getFocusedUI()
              + ", visible: " + info.isVisibleToUser() + ", global rect: " + tempGlobalRect
              + ", label: " + info.getText() + ", drawing order: " + info.getDrawingOrder());
    }
    return info;
  }

  /**
   * Set IMPORTANT_FOR_ACCESSIBILITY_NO to LynxUI's view.
   *
   * @param virtualViewId the virtual view id for node item
   */
  private void setImportantForAccessibilityNo(int virtualViewId) {
    if (virtualViewId == HOST_NODE_ID || virtualViewId >= mVirtualChildren.size()) {
      return;
    }
    final LynxCustomNodeInfo node = mVirtualChildren.get(virtualViewId);
    if (node != null && node.mUI instanceof LynxUI && ((LynxUI<?>) node.mUI).getView() != null) {
      // Note: If mUI is object of LynxUI, we should set IMPORTANT_FOR_ACCESSIBILITY_NO flag to
      // LynxUI's mView otherwise, focus will be preempted by the LynxUI's mView.
      View currentView = ((LynxUI<?>) node.mUI).getView();
      ViewCompat.setImportantForAccessibility(
          currentView, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO);
    }
  }

  /**
   * Find all a11y node for root ui.
   *
   * @param rootUI current root ui
   * @param nodeInfoList all a11y node info of root ui
   */
  private void findAllAccessibilityNode(
      final LynxBaseUI rootUI, final ArrayList<LynxCustomNodeInfo> nodeInfoList) {
    if (rootUI == null || nodeInfoList == null) {
      return;
    }
    // Step1. Clear all node info
    nodeInfoList.clear();
    // Step2. Find all node info of rootUI include container node.
    findAllAccessibilityNodeOfLynxDeepInternal(rootUI, nodeInfoList);
    // Step3. Sort children by coordinates, default from top to left.
    Collections.sort(nodeInfoList, new Comparator() {
      @Override
      public int compare(Object o1, Object o2) {
        Rect rect1 = ((LynxCustomNodeInfo) o1).mRectOnScreen;
        Rect rect2 = ((LynxCustomNodeInfo) o2).mRectOnScreen;
        if (rootUI.isAccessibilityDirectionVertical()) {
          int xCompare = rect1.left - rect2.left;
          int yCompare = mYPixelPerSection == 0
              ? (rect1.top - rect2.top)
              : (rect1.top / mYPixelPerSection - rect2.top / mYPixelPerSection);
          return yCompare == 0 ? xCompare : yCompare;
        } else {
          int xCompare = mXPixelPerSection == 0
              ? (rect1.left - rect2.left)
              : (rect1.left / mXPixelPerSection - rect2.left / mXPixelPerSection);
          int yCompare = rect1.top - rect2.top;
          return xCompare == 0 ? yCompare : xCompare;
        }
      }
    });
    if (mHasHierarchyElement) {
      findAllAccessibilityNodeOfHierarchy(nodeInfoList);
      mHasHierarchyElement = false;
    }
    findAllAccessibilityNodeOfContainerUI(rootUI, nodeInfoList);
  }

  private void findAllAccessibilityNodeOfContainerUI(
      final LynxBaseUI rootUI, final ArrayList<LynxCustomNodeInfo> nodeInfoList) {
    if (nodeInfoList == null || rootUI == null) {
      return;
    }
    if (rootUI instanceof UIList) {
      UIList list = (UIList) rootUI;
      list.initNodeInfo();
    }
    ArrayList<LynxCustomNodeInfo> newNodeInfoList = new ArrayList<>();
    for (LynxCustomNodeInfo nodeInfo : nodeInfoList) {
      if (nodeInfo.mIsNodeContainer) {
        ArrayList<LynxCustomNodeInfo> containerNodeInfoList = new ArrayList<>();
        findAllAccessibilityNode(nodeInfo.mUI, containerNodeInfoList);
        if (DEBUG) {
          LLog.i(TAG,
              "current node is container: " + nodeInfo.mUI
                  + ", with child count = " + containerNodeInfoList.size());
        }
        LynxBaseUI childUI = nodeInfo.mUI;
        if (childUI instanceof UIComponent && childUI.getParent() instanceof UIList) {
          UIComponent component = (UIComponent) childUI;
          UIList list = (UIList) childUI.getParent();
          String itemKey = component.getItemKey();
          list.updateNodeInfo(itemKey, containerNodeInfoList);
        } else {
          newNodeInfoList.addAll(containerNodeInfoList);
        }
      } else {
        newNodeInfoList.add(nodeInfo);
      }
    }
    nodeInfoList.clear();
    if (rootUI instanceof UIList) {
      UIList list = (UIList) rootUI;
      for (String itemKey : list.getComponentAccessibilityOrder()) {
        newNodeInfoList.addAll(list.getCustomNodeMap().get(itemKey));
      }
    }
    nodeInfoList.addAll(newNodeInfoList);
  }

  /**
   * Depth-first traversal of the ui tree, try to find all AccessibilityElement and
   * construct the corresponding LynxCustomNodeInfo.
   */
  private void findAllAccessibilityNodeOfLynxDeepInternal(
      LynxBaseUI rootUI, final ArrayList<LynxCustomNodeInfo> nodeInfoList) {
    // Note: We should ignore overlay ui using LynxViewVisibleHelper interface because overlay view
    // is in the different window with LynxView.
    if (nodeInfoList == null || rootUI == null
        || (rootUI instanceof LynxUI && !ViewCompat.isAttachedToWindow(((LynxUI) rootUI).getView()))
        || (rootUI instanceof LynxViewVisibleHelper)) {
      return;
    }
    ArrayList<String> accessibilityElements = rootUI.getAccessibilityElements();
    ArrayList<String> accessibilityElementsA11y = rootUI.getAccessibilityElementsA11y();
    if (accessibilityElements != null || accessibilityElementsA11y != null) {
      Rect outBounds = getBoundsOnScreenOfLynxBaseUI(rootUI);
      LynxCustomNodeInfo nodeInfo = new LynxCustomNodeInfo(rootUI, mHostUI, outBounds);
      nodeInfo.mIsHierarchy = true;
      nodeInfoList.add(nodeInfo);
      mHasHierarchyElement = true;
      return;
    }
    // Note: UIShadowProxy override getChildren() to return real children.
    // UIShadowProxy -> real ui -> real children
    for (int i = rootUI.getChildren().size() - 1; i >= 0; --i) {
      LynxBaseUI childUI = rootUI.getChildren().get(i);
      if (childUI instanceof LynxUI
          && !ViewCompat.isAttachedToWindow(((LynxUI) childUI).getView())) {
        continue;
      }
      if (childUI instanceof LynxUI && childUI.isAccessibilityHostUI()) {
        Rect outBounds = getBoundsOnScreenOfLynxBaseUI(childUI);
        LynxCustomNodeInfo nodeInfo = new LynxCustomNodeInfo(childUI, mHostUI, outBounds);
        nodeInfo.mIsNodeContainer = true;
        nodeInfoList.add(nodeInfo);
        continue;
      }
      findAllAccessibilityNodeOfLynxDeepInternal(childUI, nodeInfoList);
    }
    insertNodeInfo(rootUI, nodeInfoList);
  }

  /**
   * Traverse all LynxAccessibilityNodeInfo, if one's ui has accessibility-elements or
   * accessibility-elements-a11y which is used to mark the focus order, create all new
   * LynxAccessibilityNodeInfo and add to mChildren.
   */
  private void findAllAccessibilityNodeOfHierarchy(
      final ArrayList<LynxCustomNodeInfo> nodeInfoList) {
    ArrayList<LynxCustomNodeInfo> newNodeInfoList = new ArrayList<>();
    for (LynxCustomNodeInfo nodeInfo : nodeInfoList) {
      if (nodeInfo.mUI != null && nodeInfo.mIsHierarchy) {
        boolean enableA11y = true;
        ArrayList<String> elements = nodeInfo.mUI.getAccessibilityElementsA11y();
        if (elements == null) {
          elements = nodeInfo.mUI.getAccessibilityElements();
          enableA11y = false;
        }
        if (elements != null && mHostUI.getLynxContext() != null
            && mHostUI.getLynxContext().getLynxUIOwner() != null) {
          LynxUIOwner owner = mHostUI.getLynxContext().getLynxUIOwner();
          for (String elementId : elements) {
            LynxBaseUI childUI = enableA11y ? owner.findLynxUIByA11yId(elementId)
                                            : owner.findLynxUIByIdSelector(elementId);
            if (childUI == null
                || (childUI instanceof LynxUI
                    && !ViewCompat.isAttachedToWindow(((LynxUI) childUI).getView()))) {
              continue;
            }
            insertNodeInfo(childUI, newNodeInfoList);
          }
        }
      } else {
        newNodeInfoList.add(nodeInfo);
      }
    }
    nodeInfoList.clear();
    nodeInfoList.addAll(newNodeInfoList);
  }

  private void insertNodeInfo(
      final LynxBaseUI nodeUI, final ArrayList<LynxCustomNodeInfo> nodeInfoList) {
    if (nodeUI != null && nodeUI != mHostUI) {
      // Note: If ui is instance of UIShadowProxy, we should get the real ui which holds all
      // a11y properties.
      LynxBaseUI realUI = nodeUI;
      if (realUI instanceof UIShadowProxy) {
        realUI = ((UIShadowProxy) nodeUI).getChild();
      }
      if (isAccessibilityElement(realUI)) {
        Rect outBounds = getBoundsOnScreenOfLynxBaseUI(realUI);
        nodeInfoList.add(new LynxCustomNodeInfo(realUI, mHostUI, outBounds));
      }
    }
  }

  private void findAllAccessibilityNodeOfViews(
      View view, final ArrayList<LynxCustomNodeInfo> nodeInfoList) {
    boolean isAccessible = (view.getImportantForAccessibility() == IMPORTANT_FOR_ACCESSIBILITY_YES)
        || (view.getImportantForAccessibility() != IMPORTANT_FOR_ACCESSIBILITY_NO
            && !TextUtils.isEmpty(view.getContentDescription()));
    boolean isVisible = view.getVisibility() == VISIBLE;
    if (isAccessible && isVisible) {
      Rect rect = new Rect();
      getLeftAndTopOfBoundsInScreen(view, rect);
      nodeInfoList.add(new LynxCustomNodeInfo(view, rect));
    }
    // check children accessibility
    if (view instanceof ViewGroup) {
      ViewGroup group = (ViewGroup) view;
      for (int i = 0; i < group.getChildCount(); i++) {
        findAllAccessibilityNodeOfViews(group.getChildAt(i), nodeInfoList);
      }
    }
  }

  private static void getLeftAndTopOfBoundsInScreen(View view, Rect bounds) {
    bounds.set(0, 0, view.getRight() - view.getLeft(), view.getBottom() - view.getTop());
    int[] locationOnScreen = new int[2];
    view.getLocationOnScreen(locationOnScreen);
    bounds.offset(locationOnScreen[0], locationOnScreen[1]);
  }

  private String getAccessibilityLabelWithChild(LynxBaseUI ui) {
    if (!isAccessibilityElement(ui)) {
      return "";
    }
    CharSequence label = getAccessibilityLabel(ui);
    if (TextUtils.isEmpty(label)) {
      for (LynxBaseUI child : ui.getChildren()) {
        label = label + getAccessibilityLabel(child);
      }
    }
    return label.toString();
  }

  private static String getAccessibilityLabel(LynxBaseUI ui) {
    CharSequence label = ui.getAccessibilityLabel();
    if (label == null) {
      label = "";
    }
    return label.toString();
  }

  protected boolean isAccessibilityElement(LynxBaseUI ui) {
    if (ui == null) {
      return false;
    }
    if (ui.getAccessibilityElementStatus() == ACCESSIBILITY_ELEMENT_DEFAULT
        && mDelegateWeakRef.get() != null) {
      return mDelegateWeakRef.get().mEnableAccessibilityElement;
    }
    return ui.getAccessibilityElementStatus() == ACCESSIBILITY_ELEMENT_TRUE;
  }

  protected int findVirtualViewIdByUi(LynxBaseUI ui) {
    if (ui == null || mVirtualChildren == null) {
      return HOST_NODE_ID;
    }
    int targetVirtualViewId = HOST_NODE_ID;
    for (int i = mVirtualChildren.size() - 1; i >= 0; --i) {
      if (mVirtualChildren.get(i).mUI == ui) {
        return i;
      }
    }
    return targetVirtualViewId;
  }

  private boolean isUIClickable(LynxBaseUI ui) {
    return ui != null && ui.getEvents() != null
        && (ui.getEvents().containsKey(EVENT_CLICK) || ui.getEvents().containsKey(EVENT_TAP));
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
          parent = ((UIGroup) realLynxUI).getAccessibilityHostView();
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

  private boolean getScreenVisibleRectOfUI(LynxBaseUI currentUI, Rect rect) {
    if (currentUI == null) {
      return false;
    }
    Rect tempRect = new Rect();
    if (currentUI instanceof LynxUI) {
      LynxUI realLynxUI = (LynxUI) currentUI;
      View currentView = realLynxUI.getView();
      if (currentUI instanceof UIShadowProxy) {
        // get real ui of UIShadowProxy
        LynxBaseUI shadowChild = ((UIShadowProxy) currentUI).getChild();
        if (shadowChild instanceof LynxUI) {
          currentView = ((LynxUI) shadowChild).getView();
        }
      }
      boolean isCurrentViewVisible = currentView.getGlobalVisibleRect(rect);
      if (isCurrentViewVisible) {
        // Note: In the overlay scenario, if status-bar-translucent is set to false for overlay, the
        // coordinate of the DecorView to which the overlay belongs is not located at the upper-left
        // corner of the screen. There will be a distance between the y value of this coordinate and
        // the screen's top edge. So here we need to add this distance to the rect.
        int[] offsetOnScreen = new int[2];
        currentView.getLocationOnScreen(offsetOnScreen);
        rect.offset(offsetOnScreen[0] - rect.left, offsetOnScreen[1] - rect.top);
        return true;
      }
    } else if (currentUI instanceof LynxFlattenUI) {
      LynxBaseUI parentUI = currentUI;
      while (parentUI instanceof LynxFlattenUI && parentUI != mHostUI) {
        tempRect.left += parentUI.getOriginLeft();
        tempRect.top += parentUI.getOriginTop();
        parentUI = parentUI.getParentBaseUI();
      }
      if (parentUI != null && parentUI instanceof LynxUI) {
        View parentView = ((LynxUI) parentUI).getView();
        if (parentUI instanceof UIGroup) {
          parentView = ((UIGroup) parentUI).getAccessibilityHostView();
        }
        Rect parentLocalRect = new Rect();
        int[] parentOffsetOnScreen = new int[2];
        boolean isParentViewVisible = parentView.getLocalVisibleRect(parentLocalRect);
        if (isParentViewVisible) {
          tempRect.right = tempRect.left + currentUI.getWidth();
          tempRect.bottom = tempRect.top + currentUI.getHeight();
          if (tempRect.intersect(parentLocalRect)) {
            parentView.getLocationOnScreen(parentOffsetOnScreen);
            rect.set(tempRect);
            rect.offset(parentOffsetOnScreen[0] - parentView.getScrollX(),
                parentOffsetOnScreen[1] - parentView.getScrollY());
            return true;
          }
        }
      }
    }
    rect.set(0, 0, currentUI.getWidth(), currentUI.getHeight());
    return false;
  }

  public static class LynxCustomNodeInfo {
    LynxBaseUI mUI;
    UIGroup mHostUI;
    View mView;
    Rect mRectOnScreen;
    boolean mIsHierarchy;
    boolean mIsNodeContainer;

    public void invalid() {
      mUI = null;
      mHostUI = null;
      mView = null;
      mRectOnScreen = INVALID_BOUNDS;
    }

    public LynxCustomNodeInfo(LynxBaseUI ui, UIGroup hostUI, Rect rectOnScreen) {
      mUI = ui;
      mHostUI = hostUI;
      mView = null;
      mRectOnScreen = rectOnScreen;
    }

    public LynxCustomNodeInfo(View view, Rect rectOnScreen) {
      mUI = null;
      mHostUI = null;
      mView = view;
      mRectOnScreen = rectOnScreen;
    }
  }
}
