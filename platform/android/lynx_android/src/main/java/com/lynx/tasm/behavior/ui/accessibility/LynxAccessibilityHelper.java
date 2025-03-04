// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_DEFAULT;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_TRUE;

import android.os.Build;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIGroup;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;

public class LynxAccessibilityHelper {
  private static final String TAG = "LynxAccessibilityHelper";

  /** Reference of host ui */
  private WeakReference<UIGroup> mWeakHostUI;

  /** The value of default accessibility-element from page config. */
  private boolean mConfigEnableAccessibilityElement = false;

  /** The array of ui which set accessibility-elements. */
  private ArrayList<WeakReference<LynxBaseUI>> mAccessibilityElementsUI = null;

  /** The array of ui which set accessibility-elements-a11y. */
  private ArrayList<WeakReference<LynxBaseUI>> mAccessibilityElementsA11yUI = null;

  /** The map of ui which set accessibility-exclusive-focus. */
  private HashMap<Integer, WeakReference<LynxBaseUI>> mExclusiveUIMap = null;

  /** The enum of accessibility-traits. */
  public enum LynxAccessibilityTraits {
    NONE,
    IMAGE,
    BUTTON;

    private static String TRAITS_CLASS_NONE = "android.view.View";
    private static String TRAITS_CLASS_IMAGE = "android.widget.ImageView";
    private static String TRAITS_CLASS_BUTTON = "android.widget.Button";

    public static String getValue(LynxAccessibilityTraits traits) {
      switch (traits) {
        case IMAGE:
          return TRAITS_CLASS_IMAGE;
        case BUTTON:
          return TRAITS_CLASS_BUTTON;
        default:
          return TRAITS_CLASS_NONE;
      }
    }

    public static LynxAccessibilityTraits fromValue(String value) {
      if (value != null) {
        for (LynxAccessibilityTraits traits : LynxAccessibilityTraits.values()) {
          if (traits.name().equalsIgnoreCase(value)) {
            return traits;
          }
        }
      }
      return NONE;
    }
  }

  public LynxAccessibilityHelper(UIGroup hostUI) {
    if (hostUI == null || hostUI.getView() == null) {
      LLog.e(TAG, "Construct LynxAccessibilityHelper with null host ui or view");
      return;
    }
    mWeakHostUI = new WeakReference<>(hostUI);
    mAccessibilityElementsA11yUI = new ArrayList<>();
    mAccessibilityElementsUI = new ArrayList<>();
    mExclusiveUIMap = new HashMap<>();
  }

  /**
   * Attempts to give accessibility focus to a view.
   *
   * @param ui the LynxBaseUI on which to place accessibility focus
   * @return whether this LynxBaseUI actually took accessibility focus
   */
  public boolean requestAccessibilityFocus(LynxBaseUI ui) {
    if (!(ui instanceof LynxUI) || ((LynxUI<?>) ui).getView() == null) {
      return false;
    }
    if (!isImportantForAccessibility(ui)) {
      return false;
    }
    View focusView = ((LynxUI<?>) ui).getView();
    focusView.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
    return true;
  }

  /**
   * Traverse all elements within mAccessibilityElementsA11yUI and mAccessibilityElementsUI, if one
   * has accessibility-elements or accessibility-elements-a11y which is used to mark the focus
   * order, find relative views and set correct accessibility traversal orders.
   */
  void applyAccessibilityElements() {
    if (!mAccessibilityElementsA11yUI.isEmpty()) {
      for (WeakReference<LynxBaseUI> ref : mAccessibilityElementsA11yUI) {
        if (ref != null && ref.get() != null) {
          setAccessibilityElementsInternal(ref.get());
        }
      }
      mAccessibilityElementsA11yUI.clear();
    }
    if (!mAccessibilityElementsUI.isEmpty()) {
      for (WeakReference<LynxBaseUI> ref : mAccessibilityElementsUI) {
        if (ref != null && ref.get() != null) {
          setAccessibilityElementsInternal(ref.get());
        }
      }
      mAccessibilityElementsUI.clear();
    }
  }

  public void setConfigEnableAccessibilityElement(final boolean configEnableAccessibilityElement) {
    mConfigEnableAccessibilityElement = configEnableAccessibilityElement;
  }

  private void setAccessibilityElementsInternal(final LynxBaseUI root) {
    // setAccessibilityTraversalBefore requires API 5.1
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
      LynxUIOwner lynxUIOwner = getLynxUIOwner();
      if (root == null || lynxUIOwner == null
          || (root.getAccessibilityElementsA11y() == null
              && root.getAccessibilityElements() == null)) {
        return;
      }
      setChildrenNoImportantForAccessibility(root);
      // Note: Preferentially apply accessibility-elements-a11y prop.
      boolean useA11yId = true;
      ArrayList<String> elementIds = root.getAccessibilityElementsA11y();
      if (elementIds == null) {
        elementIds = root.getAccessibilityElements();
        useA11yId = false;
      }
      final ArrayList<LynxUI> accessibilityElementsUI = new ArrayList<>();
      for (String id : elementIds) {
        if (id != null) {
          LynxBaseUI ui = useA11yId ? lynxUIOwner.findLynxUIByA11yId(id)
                                    : lynxUIOwner.findLynxUIByIdSelector(id);
          if (ui instanceof LynxUI && ((LynxUI<?>) ui).getView() != null) {
            View currentView = ((LynxUI<?>) ui).getView();
            ViewCompat.setImportantForAccessibility(
                currentView, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
            accessibilityElementsUI.add((LynxUI) ui);
          }
        }
      }
      if (accessibilityElementsUI.isEmpty() || accessibilityElementsUI.size() == 1) {
        // No need to set traversal order.
        return;
      }
      final int count = accessibilityElementsUI.size();
      for (int i = 0; i < count - 1; ++i) {
        LynxUI currentUI = accessibilityElementsUI.get(i);
        LynxUI nextUI = accessibilityElementsUI.get(i + 1);
        View currentView = currentUI.getView();
        View nextView = nextUI.getView();
        currentView.setId(currentUI.getSign());
        nextView.setId(nextUI.getSign());
        currentView.setAccessibilityTraversalBefore(nextUI.getSign());
      }
    }
  }

  private void setChildrenNoImportantForAccessibility(final LynxBaseUI root) {
    if (root == null || root.getChildren() == null) {
      return;
    }
    // Note: UIShadowProxy override getChildren() to return real children.
    // UIShadowProxy -> real ui -> real children
    final int childrenCount = root.getChildren().size();
    for (int i = 0; i < childrenCount; ++i) {
      LynxBaseUI child = root.getChildAt(i);
      if (child instanceof LynxUI) {
        View childView = ((LynxUI<?>) child).getView();
        ViewCompat.setImportantForAccessibility(
            childView, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO);
      }
      setChildrenNoImportantForAccessibility(child);
    }
  }

  void addAccessibilityElementsUI(final LynxBaseUI root) {
    if (root != null) {
      mAccessibilityElementsUI.add(new WeakReference<>(root));
    }
  }

  void addAccessibilityElementsA11yUI(final LynxBaseUI root) {
    if (root != null) {
      mAccessibilityElementsA11yUI.add(new WeakReference<>(root));
    }
  }

  public void addUIToExclusiveMap(LynxBaseUI ui) {
    final int sign = ui.getSign();
    mExclusiveUIMap.put(sign, new WeakReference<>(ui));
  }

  public void removeUIFromExclusiveMap(LynxBaseUI ui) {
    final int sign = ui.getSign();
    if (mExclusiveUIMap == null || mExclusiveUIMap.get(sign) == null) {
      return;
    }
    LynxBaseUI findUI = mExclusiveUIMap.get(sign).get();
    if (findUI != null && findUI == ui) {
      mExclusiveUIMap.remove(sign);
    }
  }

  public boolean isImportantForAccessibility(final int status) {
    return status == ACCESSIBILITY_ELEMENT_DEFAULT ? mConfigEnableAccessibilityElement
                                                   : (status == ACCESSIBILITY_ELEMENT_TRUE);
  }

  public boolean isImportantForAccessibility(final LynxBaseUI ui) {
    if (ui == null) {
      return false;
    }
    boolean res = isImportantForAccessibility(ui.getAccessibilityElementStatus());
    UIGroup hostUI = getHostUI();
    // Note: Here we only need to handle the condition of res == true
    if (res && ui != null && hostUI != null && !mExclusiveUIMap.isEmpty()) {
      LynxBaseUI parentUI = ui;
      while (parentUI != null && parentUI != hostUI) {
        if (mExclusiveUIMap.containsKey(parentUI.getSign())) {
          return true;
        }
        parentUI = parentUI.getParentBaseUI();
      }
      return false;
    }
    return res;
  }

  private LynxUIOwner getLynxUIOwner() {
    if (mWeakHostUI != null && mWeakHostUI.get() != null) {
      LynxBaseUI hostUI = mWeakHostUI.get();
      if (hostUI != null && hostUI.getLynxContext() != null) {
        return hostUI.getLynxContext().getLynxUIOwner();
      }
    }
    return null;
  }

  private UIGroup getHostUI() {
    if (mWeakHostUI != null) {
      return mWeakHostUI.get();
    }
    return null;
  }
}
