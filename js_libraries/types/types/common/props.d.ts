// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { CSSProperties } from './csstype';
import { LynxEventProps } from './events';

export interface StandardProps extends LynxEventProps {
  /**
   * The unique identifier of the element, ensuring the uniqueness of the entire page
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  id?: string;
  /** In React, similar to class, className is generally used as an alias for class
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  className?: string;
  /**
   * Used to specify one or more class names for the element. These class names can be used in CSS to apply styles
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  class?: string;
  /**  
   * Element display, All elements is displayed by Default
   * @deprecated Please sse `visibility:hidden` instead
   */
  hidden?: boolean;
  /**
   *  Animation Properties 
   */
  animation?: { actions: Record<string, unknown>[] };
  /**
   * Used to flatten the specified element, avoiding the creation of a real renderObject to accelerate rendering performance. Its default value is true on `<view>`
   * @defaultValue true
   * @Android
   */
  flatten?: boolean;
  /**
   * The unique identifier of the element, used for the client to find the View
   * @deprecated Please use `id` instead
   * @Android
   * @iOS
   */
  name?: string;
  /**
   * Used to control overlapRendering on Android
   * @defaultValue false
   * @Android
   */
  overlap?: boolean;
  /**
   * Controls whether to overlap the drawing of the background and content to achieve the correct alpha blend effect
   * @defaultValue false
   * @iOS
   */
  'overlap-ios'?: boolean;
  /** 
   * Layout only
   * @deprecated Legacy API
   */
  enableLayoutOnly?: boolean;
  /** 
   * Legacy API
   * @deprecated Legacy API
   */
  cssAlignWithLegacyW3C?: boolean;
  /**
   * Accessibility reading content
   * @Android
   * @iOS
   * @Harmony
   * @PC
   * @spec {@link https://developer.apple.com/documentation/objectivec/nsobject/1615181-accessibilitylabel?language=objc | iOS}
   * @spec {@link https://developer.android.com/reference/android/view/View.html#setContentDescription(java.lang.CharSequence) | Android}
   */
  'accessibility-label'?: string;
  /**
   * The combination of accessibility traits that best characterizes the accessibility element.
   * @defaultValue 'none'
   * @iOS
   * @Harmony
   * @Android
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
   * @Harmony
   * @PC
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

  /**
   * Whether the element is hidden from the accessibility tree.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   */
  'accessibility-elements-hidden'?: boolean;

  /**
   * Whether the element is the only focusable element in the accessibility tree.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   */
  'accessibility-exclusive-focus'?: boolean;

  /**
   * Used to specify the accessibility identifier of a UIView in iOS, which is only required when accessing the platform accessibility framework.
   * @iOS
   */
  'ios-platform-accessibility-id'?: string;

  /**
   * Direction used to start pan gesture interception. 0 means horizontal, and 1 means vertical.
   * @defaultValue undefined
   * @Android
   * @iOS
   * @since 4.1
   */
  'pan-intercept-direction'?: 0 | 1;

  /**
   * Scope used to decide which external gesture views can be intercepted. 0 means self, 1 means ancestors, 2 means descendants, 3 means self and ancestors, 4 means self and descendants, and 5 means all.
   * @defaultValue undefined
   * @Android
   * @iOS
   * @since 4.1
   */
  'pan-intercept-scope'?: 0 | 1 | 2 | 3 | 4 | 5;

  /**
   * UIGestureRecognizer's class name, used with `ios-pan-intercept-view-class` and `ios-pan-intercept-view-tag` to identify an external gesture that should be intercepted by pan-intercept.
   * @defaultValue undefined
   * @iOS
   * @since 4.1
   */
  'ios-pan-intercept-gesture-class'?: string;

  /**
   * UIView's class name, used with `ios-pan-intercept-gesture-class` and `ios-pan-intercept-view-tag` to identify the view bound to an external gesture that should be intercepted by pan-intercept.
   * @defaultValue undefined
   * @iOS
   * @since 4.1
   */
  'ios-pan-intercept-view-class'?: string;

  /**
   * UIView's tag, used with `ios-pan-intercept-gesture-class` and `ios-pan-intercept-view-class` to identify the view bound to an external gesture that should be intercepted by pan-intercept.
   * @defaultValue undefined
   * @iOS
   * @since 4.1
   */
  'ios-pan-intercept-view-tag'?: number;

  /** 
   * Control whether the component can receive focus
   * @defaultValue false
   * @PC
   */
  focusable?: boolean;

  /** 
   * Use two-dimensional coordinates such as "0, 0" to represent the focus priority of the x and y axes respectively. Nodes with the same priority will switch focus based on their position. 
   * @PC
   */
  'focus-index'?: string;

  /** 
   * Manually specify the node ID that will receive focus when the user presses the "up" arrow key
   * @PC
   */
  'next-focus-up'?: string;

  /** Manually specify the node ID that will receive focus when the user presses the "down" arrow key
   * @PC
   */
  'next-focus-down'?: string;

  /** Manually specify the node ID that will receive focus when the user presses the "left" arrow key 
   * @PC
   */
  'next-focus-left'?: string;

  /**
   * Manually specify the node id that receives focus when the user presses the "right" arrow key
   * @PC
   */
  'next-focus-right'?: string;

  /**
   * Custom timing flag
   * @Android
   * @iOS
   * @Harmony
   * @since Lynx 2.10
   */
  __lynx_timing_flag?: string;
  
  /**
   * Used to apply inline styles to the element
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  style?: string | CSSProperties;

  /**
   * We use CAShapeLayer to accelerate rendering of the component's backgrounds on iOS.
   * @deprecated CPU drawing path is no longer needed
   * @defaultValue true
   * @iOS
   * @since 3.1
   */
  'ios-background-shape-layer'?: boolean;

  /**
   * Specify whether the node needs exposure/un-exposure events
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */

  'exposure-id'?: string;

  /**
   * Specify the scene of the node that needs exposure/un-exposure events
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-scene'?: string;
  /**
   * Specify the top extension value of the screen boundary referenced by the target node in the exposure detection task, which affects the viewport intersection judgment of the target node. Each node can have its own screen boundary scaling value
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-screen-margin-top'?: `${number}px` | `${number}rpx`;

  /**
   * Specify the right extension value of the screen boundary referenced by the target node in the exposure detection task, which affects the viewport intersection judgment of the target node. Each node can have its own screen boundary scaling value
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-screen-margin-right'?: `${number}px` | `${number}rpx`;

  /**
   * Specify the bottom extension value of the screen boundary referenced by the target node in the exposure detection task, which affects the viewport intersection judgment of the target node. Each node can have its own screen boundary scaling value
  * @defaultValue "0px" 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-screen-margin-bottom'?: `${number}px` | `${number}rpx`;

  /**
   * Specify the left extension value of the screen boundary referenced by the target node in the exposure detection task, which affects the viewport intersection judgment of the target node. Each node can have its own screen boundary scaling value
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-screen-margin-left'?: `${number}px` | `${number}rpx`;

  /**
   * Specify the top boundary extension value of the target node itself in the exposure detection, which affects the viewport intersection judgment of the target node. Each node can have its own boundary extension value. Before using this feature, you also need to set `enable-exposure-ui-margin` for the current node.
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-ui-margin-top'?: `${number}px` | `${number}rpx`;

  /**
   * Specify the right boundary extension value of the target node itself in the exposure detection, which affects the viewport intersection judgment of the target node. Each node can have its own boundary extension value. Before using this feature, you also need to set `enable-exposure-ui-margin` for the current node.
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-ui-margin-right'?: `${number}px` | `${number}rpx`;

  /**
   * Specify the bottom boundary extension value of the target node itself in the exposure detection, which affects the viewport intersection judgment of the target node. Each node can have its own boundary extension value. Before using this feature, you also need to set `enable-exposure-ui-margin` for the current node.
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-ui-margin-bottom'?: `${number}px` | `${number}rpx`;

  /**
   * Specify the left boundary extension value of the target node itself in the exposure detection, which affects the viewport intersection judgment of the target node. Each node can have its own boundary extension value. Before using this feature, you also need to set `enable-exposure-ui-margin` for the current node.
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-ui-margin-left'?: `${number}px` | `${number}rpx`;

  /**
   * Specifies the viewport intersection ratio at which the target node can trigger an exposure event. An exposure event is triggered when the ratio is greater than this value, and an un-exposure event is triggered when the ratio is less than this value. By default, the exposure event is triggered as soon as the target node is visible.
   * @defaultValue "0%"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'exposure-area'?: `${number}%`;
  /**
   * Specifies whether the target node needs to extend the viewport intersection area for exposure detection. If set to true, the target node will be extended by the values specified in `exposure-ui-margin-top`, `exposure-ui-margin-right`, `exposure-ui-margin-bottom`, and `exposure-ui-margin-left`. The default value is `true` on Harmony
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'enable-exposure-ui-margin'?: boolean;

  /**
   * Specify whether the exposure detection task takes into account the viewport clipping of the parent node. When set to true, nodes outside the parent node viewport cannot trigger exposure, and when set to false, nodes outside the parent node viewport can trigger exposure.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   * @since 3.4
   */
  'enable-exposure-ui-clip'?: boolean;

  /**
   * Specifies whether the target node and its child nodes can respond to Lynx touch events. This property does not affect platform gestures (such as scrolling of the `<scroll-view>`)
   * @defaultValue true
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'user-interaction-enabled'?: boolean;
  /**
   * Specifies whether the target node and its child nodes can consume platform touch events. This property does not affect platform gestures (such as scrolling of the `<scroll-view>`). The default value is `true` on iOS and Harmony and `false` on Android.
   * @Android
   * @iOS
   * @Harmony
   */
  'native-interaction-enabled'?: boolean;
  /**
   * Specifies whether the target node blocks platform gestures outside Lynx when it is on the event response chain, which can achieve an effect like blocking the platform swipe-back gesture.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   */
  'block-native-event'?: boolean;
  /**
   * Specifies whether to block platform gestures outside Lynx when the target node is on the event response chain and the touch is within the specified area of the target node. This can achieve an effect similar to blocking the platform swipe-back gesture. The inner array is an array containing four numbers: x, y, width, and height, representing the position of the touch area within the target node.
   * @Android
   * @iOS
   */
  'block-native-event-areas'?: [`${number}px` | `${number}%`, `${number}px` | `${number}%`, `${number}px` | `${number}%`, `${number}px` | `${number}%`][];
  /**
   * Specifies whether the platform gestures should respond when the target node is on the event response chain and a swipe occurs at a specific angle. This does not affect Lynx touch events and can be used to implement a front - end scroll container that consumes swipes in a specified direction. The inner array contains two numbers, start and end, representing the start and end angles respectively.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'consume-slide-event'?: [number, number][];
  /**
   * Specifies whether the platform touch events should be dispatched to Lynx when the touch occurs on the target node. This can achieve an effect similar to displaying content without interaction. This property supports inheritance. This property can only ensure that Lynx does not consume platform touch events, but the parent view of LynxView may consume touch events, causing the penetration effect to fail.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   */
  'event-through'?: boolean;
  /**
   * Specifies whether the target node supports the :active pseudo-class to continue bubbling up the event response chain when a touch event occurs.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'enable-touch-pseudo-propagation'?: boolean;
  /**
   * Specify the touch event response hot zone of the target node without affecting platform gestures
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'hit-slop'?: `${number}px` | {top: `${number}px`, left: `${number}px`, right: `${number}px`, bottom: `${number}px`};
  /**
   * Specifies whether the target node should not抢占 focus when touched. By default, the node will抢占 focus when clicked, which can achieve an effect similar to preventing the keyboard from closing when clicking other areas. Additionally, it supports inheritance, meaning the default value of child nodes is the `ignore-focus` value of the parent node, and child nodes can override this value.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'ignore-focus'?: boolean;
  /** 
   * Specifies whether to force the triggering of touch events when the target node is on the event response chain, which can solve the problem of touch events not being triggered on iOS
   * @defaultValue false
   * @iOS
   */
  'ios-enable-simultaneous-touch'?: boolean;
  /**
   * Used in conjunction with `event-through` to specify the event penetration definition for the target node in specific areas. The inner array is in the form of [x, y, width, height], with units of px or %. The x/y positions are relative to the viewport coordinate system of the target node.
   * @iOS
   * @Android
   * @Harmony
   */
  'event-through-active-regions'?: [`${number}%` | `${number}px`, `${number}%` | `${number}px`, `${number}%` | `${number}px`, `${number}%` | `${number}px`][];
}

export interface NoProps {
  // empty props
}
