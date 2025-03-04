// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { CSSProperties } from './csstype';
import { LynxEventProps } from './events';

export interface StandardProps extends LynxEventProps {
  /**  The unique identifier of the component, ensuring the uniqueness of the entire page. */
  id?: string;
  /** In React, similar to class, className is generally used as an alias for class.*/
  className?: string;
  class?: string;
  /**  Component Display, All Components Display by Default. */
  hidden?: boolean;
  /** Animation Properties */
  animation?: { actions: Record<string, unknown>[] };
  /** "flatten" attribute only works for Lynx. */
  flatten?: boolean;
  name?: string;
  overlap?: boolean;
  enableLayoutOnly?: boolean;
  cssAlignWithLegacyW3C?: boolean;
  /**
   * Accessibility reading content
   * @Android
   * @iOS
   * @spec {@link https://developer.apple.com/documentation/objectivec/nsobject/1615181-accessibilitylabel?language=objc | iOS}
   * @spec {@link https://developer.android.com/reference/android/view/View.html#setContentDescription(java.lang.CharSequence) | Android}
   */
  'accessibility-label'?: string;
  /**
   * The combination of accessibility traits that best characterizes the accessibility element.
   * @defaultValue 'none'
   * @iOS
   * @spec {@link https://developer.apple.com/documentation/objectivec/nsobject/1615202-accessibilitytraits?language=objc | iOS}
   */
  'accessibility-traits'?:
    | 'text'
    | 'image'
    | 'button'
    | 'link'
    | 'header'
    | 'search'
    | 'selected'
    | 'playable'
    | 'keyboard'
    | 'summary'
    | 'disabled'
    | 'updating'
    | 'adjustable'
    | 'tabbar'
    | 'none';
  /**
   * A Boolean value that indicates whether the element is an accessibility element that an assistive app can access.
   * @iOS
   * @Android
   * @spec {@link https://developer.apple.com/documentation/objectivec/nsobject/1615141-isaccessibilityelement?language=objc | iOS}
   * @spec {@link https://developer.android.com/reference/android/view/View.html#setImportantForAccessibility(int) | Android}
   */
  'accessibility-element'?: boolean;
  /**
   * A localized string that contains the value of the accessibility element.
   * @iOS
   * @Android
   * @spec {@link https://developer.apple.com/documentation/objectivec/nsobject/1615117-accessibilityvalue?language=objc | iOS}
   * @spec {@link https://developer.android.com/reference/androidx/core/view/accessibility/AccessibilityNodeInfoCompat?hl=en#setStateDescription(java.lang.CharSequence) | Android}
   */
  'accessibility-value'?: string;
  /**
   * Sets whether the node represents a heading.
   * @defaultValue false
   * @Android
   * @spec {@link https://developer.android.com/reference/androidx/core/view/accessibility/AccessibilityNodeInfoCompat?hl=en#setHeading(boolean) | Android}
   */
  'accessibility-heading'?: boolean;
  /**
   * Sets the class this node comes from, or, sets the custom role description.
   * @Android
   * @spec {@link https://developer.android.com/reference/androidx/core/view/accessibility/AccessibilityNodeInfoCompat?hl=en#setClassName(java.lang.CharSequence) | Android}
   * @spec {@link https://developer.android.com/reference/androidx/core/view/accessibility/AccessibilityNodeInfoCompat?hl=en#setRoleDescription(java.lang.CharSequence) | Android}
   */
  'accessibility-role-description'?: 'switch' | 'checkbox' | 'image' | 'progressbar' | string;
  /**
   * The custom actions of the current accessibility element.
   * @Android
   * @iOS
   * @spec {@link https://developer.apple.com/documentation/appkit/nsaccessibility/2869551-accessibilitycustomactions/ | iOS}
   * @spec {@link https://developer.android.com/reference/androidx/core/view/accessibility/AccessibilityNodeInfoCompat?hl=en#addAction(androidx.core.view.accessibility.AccessibilityNodeInfoCompat.AccessibilityActionCompat) | Android}
   */
  'accessibility-actions'?: string[];

  /** Control whether the component can receive focus. Default is false */
  focusable?: boolean;
  /** Use two-dimensional coordinates such as "0, 0" to represent the focus priority of the x and y axes respectively. Nodes with the same priority will switch focus based on their position.  */
  'focus-index'?: string;
  /** Manually specify the node ID that will receive focus when the user presses the "up" arrow key.  */
  'next-focus-up'?: string;
  /** Manually specify the node ID that will receive focus when the user presses the "down" arrow key.  */
  'next-focus-down'?: string;
  /** Manually specify the node ID that will receive focus when the user presses the "left" arrow key.*/
  'next-focus-left'?: string;
  /**
   * Manually specify the node id that receives focus when the user presses the "right" arrow key.
   */
  'next-focus-right'?: string;

  /**
   *  Custom Timing Flag
   * @since Lynx 2.10
   */
  __lynx_timing_flag?: string;

  style?: string | CSSProperties;
}

export interface NoProps {
  // empty props
}
