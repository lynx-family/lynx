// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;

public class CustomAccessibilityDelegateCompatTest {
  private LynxContext mContext;
  private LynxAccessibilityWrapper mWrapper;

  @Before
  public void setUp() throws Exception {
    mContext = spy(TestingUtils.getLynxContext());
    // Enable the helper (page config enableA11y) and make every ui important for accessibility.
    LynxAccessibilityHelper helper = mock(LynxAccessibilityHelper.class);
    when(helper.isImportantForAccessibility(any(LynxBaseUI.class))).thenReturn(true);
    when(helper.isImportantForAccessibility(anyInt())).thenReturn(true);
    mWrapper = mock(LynxAccessibilityWrapper.class);
    when(mWrapper.enableHelper()).thenReturn(true);
    when(mWrapper.getLynxAccessibilityHelper()).thenReturn(helper);
    when(mContext.getLynxAccessibilityWrapper()).thenReturn(mWrapper);
  }

  // Create the node info of a view with the given props, as name and value pairs.
  private AccessibilityNodeInfoCompat createNodeInfo(Object... props) {
    JavaOnlyMap map = new JavaOnlyMap();
    for (int i = 0; i < props.length; i += 2) {
      map.put((String) props[i], props[i + 1]);
    }
    UIView ui = new UIView(mContext);
    ui.updatePropertiesInterval(new StylesDiffMap(map));
    CustomAccessibilityDelegateCompat delegate = new CustomAccessibilityDelegateCompat(ui);
    AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain();
    delegate.onInitializeAccessibilityNodeInfo(ui.getView(), info);
    return info;
  }

  @Test
  public void switchKeepsItsLabelOutOfTheNodeText() {
    AccessibilityNodeInfoCompat info = createNodeInfo(PropsConstants.ACCESSIBILITY_LABEL, "Wi-Fi",
        PropsConstants.ACCESSIBILITY_ROLE_DESCRIPTION, "switch");
    assertEquals(android.widget.Switch.class.getName(), info.getClassName().toString());
    assertEquals("Wi-Fi", info.getContentDescription().toString());
    assertNull(info.getText());
  }

  @Test
  public void otherElementsKeepTheirLabelAsNodeText() {
    AccessibilityNodeInfoCompat info = createNodeInfo(PropsConstants.ACCESSIBILITY_LABEL, "Save",
        PropsConstants.ACCESSIBILITY_ROLE_DESCRIPTION, "button");
    assertEquals("Save", info.getText().toString());
  }

  @Test
  public void traitsTakeTheRoleFromTheFirstKnownValue() {
    AccessibilityNodeInfoCompat info = createNodeInfo(PropsConstants.ACCESSIBILITY_LABEL, "Bold",
        PropsConstants.ACCESSIBILITY_TRAITS, "button,selected");
    assertEquals(android.widget.Button.class.getName(), info.getClassName().toString());
    assertTrue(info.isSelected());
  }

  @Test
  public void traitsDisabledMarksTheNodeNotEnabled() {
    AccessibilityNodeInfoCompat info = createNodeInfo(PropsConstants.ACCESSIBILITY_LABEL, "Send",
        PropsConstants.ACCESSIBILITY_TRAITS, "button, disabled");
    assertEquals(android.widget.Button.class.getName(), info.getClassName().toString());
    assertFalse(info.isEnabled());
  }

  @Test
  public void singleTraitStillSetsTheRole() {
    AccessibilityNodeInfoCompat info = createNodeInfo(
        PropsConstants.ACCESSIBILITY_LABEL, "Close", PropsConstants.ACCESSIBILITY_TRAITS, "button");
    assertEquals(android.widget.Button.class.getName(), info.getClassName().toString());
    assertFalse(info.isSelected());
    assertTrue(info.isEnabled());
  }

  @Test
  public void switchRoleDescriptionIsCheckedWhenTraitsSaySelected() {
    AccessibilityNodeInfoCompat info = createNodeInfo(PropsConstants.ACCESSIBILITY_LABEL, "Wi-Fi",
        PropsConstants.ACCESSIBILITY_ROLE_DESCRIPTION, "switch",
        PropsConstants.ACCESSIBILITY_TRAITS, "selected");
    assertTrue(info.isCheckable());
    assertTrue(info.isChecked());
    assertFalse(info.isSelected());
  }

  @Test
  public void checkboxRoleDescriptionWithoutSelectedIsNotChecked() {
    AccessibilityNodeInfoCompat info = createNodeInfo(PropsConstants.ACCESSIBILITY_LABEL,
        "Remember me", PropsConstants.ACCESSIBILITY_ROLE_DESCRIPTION, "checkbox");
    assertTrue(info.isCheckable());
    assertFalse(info.isChecked());
  }

  // Apply props to the ui as the engine does, as name and value pairs.
  private static void applyProps(UIView ui, Object... props) {
    JavaOnlyMap map = new JavaOnlyMap();
    for (int i = 0; i < props.length; i += 2) {
      map.put((String) props[i], props[i + 1]);
    }
    ui.updateProperties(new StylesDiffMap(map));
  }

  @Test
  public void selectedTraitChangeNotifiesTheCheckedState() {
    UIView ui = new UIView(mContext);
    applyProps(ui, PropsConstants.ACCESSIBILITY_ROLE_DESCRIPTION, "switch",
        PropsConstants.ACCESSIBILITY_TRAITS, "button");
    applyProps(ui, PropsConstants.ACCESSIBILITY_TRAITS, "button,selected");
    // The same state again, and another prop: nothing changed.
    applyProps(ui, PropsConstants.ACCESSIBILITY_TRAITS, "button,selected");
    applyProps(ui, PropsConstants.ACCESSIBILITY_LABEL, "Wi-Fi");
    verify(mWrapper, times(1)).notifyAccessibilityCheckedChanged(ui);
    applyProps(ui, PropsConstants.ACCESSIBILITY_TRAITS, "button");
    verify(mWrapper, times(2)).notifyAccessibilityCheckedChanged(ui);
  }

  @Test
  public void removedTraitsClearTheRoleAndStates() {
    UIView ui = new UIView(mContext);
    applyProps(ui, PropsConstants.ACCESSIBILITY_LABEL, "Send", PropsConstants.ACCESSIBILITY_TRAITS,
        "button,selected,disabled");
    // A removed attribute arrives as a null value.
    applyProps(ui, PropsConstants.ACCESSIBILITY_TRAITS, null);
    AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain();
    new CustomAccessibilityDelegateCompat(ui).onInitializeAccessibilityNodeInfo(ui.getView(), info);
    assertFalse(android.widget.Button.class.getName().equals(String.valueOf(info.getClassName())));
    assertFalse(info.isSelected());
    assertTrue(info.isEnabled());
    // Selected came and went: two checked changes.
    verify(mWrapper, times(2)).notifyAccessibilityCheckedChanged(ui);
  }

  @Test
  public void removedRoleDescriptionAndStatusAreCleared() {
    UIView ui = new UIView(mContext);
    applyProps(ui, PropsConstants.ACCESSIBILITY_LABEL, "Night light",
        PropsConstants.ACCESSIBILITY_ROLE_DESCRIPTION, "switch",
        PropsConstants.ACCESSIBILITY_STATUS, "on");
    applyProps(ui, PropsConstants.ACCESSIBILITY_ROLE_DESCRIPTION, null,
        PropsConstants.ACCESSIBILITY_STATUS, null);
    AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain();
    new CustomAccessibilityDelegateCompat(ui).onInitializeAccessibilityNodeInfo(ui.getView(), info);
    assertFalse(info.isCheckable());
    assertEquals("Night light", info.getContentDescription().toString());
    assertEquals("Night light", info.getText().toString());
  }

  @Test
  public void removedActionsAreCleared() {
    UIView ui = new UIView(mContext);
    applyProps(ui, PropsConstants.ACCESSIBILITY_LABEL, "Photo",
        PropsConstants.ACCESSIBILITY_ACTIONS, JavaOnlyArray.of("Share", "Delete"));
    applyProps(ui, PropsConstants.ACCESSIBILITY_ACTIONS, null);
    assertNull(ui.getAccessibilityActions());
    AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain();
    new CustomAccessibilityDelegateCompat(ui).onInitializeAccessibilityNodeInfo(ui.getView(), info);
    for (AccessibilityNodeInfoCompat.AccessibilityActionCompat action : info.getActionList()) {
      assertNull(action.getLabel());
    }
  }
}
