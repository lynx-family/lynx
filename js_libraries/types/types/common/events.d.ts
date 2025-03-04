// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Element as MainThreadElement } from '../main-thread/element';

export interface Target {
  /** The id selector of the event target. */
  id: string;
  /** The unique identifier of the event target. */
  uid: number;
  /** The collection of custom attributes starting with data- on the event target. */
  dataset: {
    [key: string]: any;
  };
}

export interface BaseEventOrig<T, TargetType = Target> {
  /** Event type. */
  type: string;

  /** Timestamp when the event was generated. */
  timestamp: number;

  /** Collection of attribute values of the target that triggers the event. */
  target: TargetType;

  /** Collection of attribute values of the target that listens to the event. */
  currentTarget: TargetType;

  /** Additional information. */
  detail: T;

  /** Preventing elements from performing default behavior. */
  preventDefault: () => void;

  /** Prevent the event from bubbling up to the parent element, preventing any parent event handlers from being executed. */
  stopPropagation: () => void;
}

export interface Touch {
  /** The unique identifier of the finger touching the screen. */
  identifier: number;
  /** The current position of the touch point relative to the touched element's x-coordinate. */
  x: number;
  /** The current position of the touch point relative to the touched element's y-coordinate. */
  y: number;
  /** The current position of the touch point relative to the page's x-coordinate. */
  pageX: number;
  /** The current position of the touch point relative to the page's y-coordinate. */
  pageY: number;
  /** The current position of the touch point relative to the display area's x-coordinate. */
  clientX: number;
  /** The current position of the touch point relative to the display area's y-coordinate. */
  clientY: number;
}

export interface ChangedTouch {
  identifier: number;
  x: number;
  y: number;
}

export interface BaseCommonEvent<T> extends BaseEventOrig<any, T> {}
export interface CommonEvent extends BaseCommonEvent<Target | MainThreadElement> {}

export interface AppearanceEvent {
  type: 'nodeappear' | 'nodedisappear';
  detail: {
    position: number;
    key: string;
  };
}

export interface BaseTouchEvent<T> extends BaseEventOrig<any, T> {
  /** The touch points currently on the touch plane. */
  touches: Array<Touch>;

  /** The touch points whose state has changed compared to the last touch event. */
  changedTouches: Array<Touch>;

  detail: {
    /** The current position of the touch point relative to the touched element's x-coordinate. */
    x: number;
    /** The current position of the touch point relative to the touched element's y-coordinate. */
    y: number;
  }
}

export interface TouchEvent extends BaseTouchEvent<Target> {}

export interface BaseMouseEvent<T> extends BaseEventOrig<{}, T> {
  /** The currently pressed mouse button, if multiple buttons are pressed simultaneously, is the last one pressed. */
  button: number;
  /** The current mouse button being pressed, if multiple buttons are simultaneously pressed, it is a bit field composed of all button codes. */
  buttons: number;
  /** clientX */
  x: number;
  /** clientY */
  y: number;
  /** The current position of the cursor relative to the page's x-coordinate. */
  pageX: number;
  /** The current position of the cursor relative to the page's y-coordinate. */
  pageY: number;
  /** The current position of the cursor relative to the element's x-coordinate. */
  clientX: number;
  /** The current position of the cursor relative to the element's y-coordinate. */
  clientY: number;
}

export interface MouseEvent extends BaseMouseEvent<Target> {}

export interface BaseWheelEvent<T> extends BaseEventOrig<{}, T> {
  /** clientX */
  x: number;
  /** clientY */
  y: number;
  /** The current position of the cursor relative to the page's x-coordinate. */
  pageX: number;
  /** The current position of the cursor relative to the page's y-coordinate. */
  pageY: number;
  /** The current position of the cursor relative to the element's x-coordinate. */
  clientX: number;
  /** The current position of the cursor relative to the element's y-coordinate. */
  clientY: number;
  /** The distance of x-axis scrolling on the mouse wheel. */
  deltaX: number;
  /** The distance of y-axis scrolling on the mouse wheel. */
  deltaY: number;
}

export interface WheelEvent extends BaseWheelEvent<Target> {}

export interface BaseKeyEvent<T> extends BaseEventOrig<{}, T> {
  /** Button Name. */
  key: string;
}

export interface KeyEvent extends BaseKeyEvent<Target> {}

export interface BaseAnimationEvent<T> extends BaseEventOrig<{}, T> {
  params: {
    animation_type: 'keyframe-animation';
    animation_name: string;
    new_animator?: true;
  };
}

export interface AnimationEvent extends BaseAnimationEvent<Target | MainThreadElement> {}

export interface BaseTransitionEvent<T> extends BaseEventOrig<{}, T> {
  params:
    | {
        animation_type: 'transition-animation';
        animation_name: 'width' | 'height' | 'left' | 'top' | 'right' | 'bottom' | 'background-color' | 'opacity';
        new_animator: true;
      }
    | {
        new_animator: undefined;
        animation_name: undefined;
        animation_type:
          | 'transition-width'
          | 'transition-height'
          | 'transition-left'
          | 'transition-top'
          | 'transition-right'
          | 'transition-bottom'
          | 'transition-transform'
          | 'transition-background-color'
          | 'transition-opacity';
      };
}
export interface TransitionEvent extends BaseTransitionEvent<Target | MainThreadElement> {}

export interface ImageLoadEvent {
  detail: {
    width: number;
    height: number;
  };
}

export interface ImageErrorEvent {
  detail: {
    errMsg: string;
    error_code: number;
    lynx_categorized_code: number;
  };
}

export interface TextLineInfo {
  start: number;
  end: number;
  ellipsisCount: number;
}

export interface TextLayoutEventDetail {
  lineCount: number;
  lines: TextLineInfo[];
  size: {
    width: number;
    height: number;
  };
}

export interface AccessibilityActionDetailEvent<T> extends BaseEventOrig<{}, T> {
  detail: {
    /**
     * The name of the custom action.
     * @Android
     * @iOS
     * @spec {@link https://developer.apple.com/documentation/appkit/nsaccessibility/2869551-accessibilitycustomactions/ | iOS}
     * @spec {@link https://developer.android.com/reference/androidx/core/view/accessibility/AccessibilityNodeInfoCompat?hl=en#addAction(androidx.core.view.accessibility.AccessibilityNodeInfoCompat.AccessibilityActionCompat) | Android}
     */
    name: string;
  };
}

export interface LayoutChangeDetailEvent<T> extends BaseEventOrig<{}, T> {
  type: 'layoutchange';
  /**
   * @deprecated Use 'detail' field instead.
   * This field is only available on the Android platform.
   * */
  params: {
    width: number;
    height: number;
    left: number;
    top: number;
    right: number;
    bottom: number;
  };

  /**
   * This field is available on other platforms.
   * */
  detail: {
    /** The id selector of the target. */
    id: string;
    /** The width of the target. */
    width: number;
    /** The height of the target. */
    height: number;
    /** The position of the target's top border relative to the page's coordinate. */
    top: number;
    /** The position of the target's right border relative to the page's coordinate. */
    right: number;
    /** The position of the target's bottom border relative to the page's coordinate. */
    bottom: number;
    /** The position of the target's left border relative to the page's coordinate. */
    left: number;
    /** The collection of custom attributes starting with data- on the event target. */
    dataset: {
      [key: string]: any;
    };
  };
}

export interface UIAppearanceDetailEvent<T> extends BaseEventOrig<{}, T> {
  type: 'uiappear' | 'uidisappear';
  detail: {
    /** exposure-id set on the target. */
    'exposure-id': string;
    /** exposure-scene set on the target. */
    'exposure-scene': string;
    /** uid of the target */
    'unique-id': string;
    /** The collection of custom attributes starting with data- on the event target. */
    dataset: {
      [key: string]: any;
    };
  }
}

export type Callback<T = any> = (res: T) => void;

export interface BaseMethod {
  success?: Callback;
  fail?: Callback;
}

export interface LepusEventInstance {
  querySelector: (...params: any[]) => any;
  querySelectorAll: (...params: any[]) => any;
  requestAnimationFrame: (...params: any[]) => any;
  cancelAnimationFrame: (...params: any[]) => any;
  triggerEvent: (...params: any[]) => any;
  getStore: (...params: any[]) => any;
  setStore: (...params: any[]) => any;
  getData: (...params: any[]) => any;
  setData: (...params: any[]) => any;
  getProperties: (...params: any[]) => any;
}

export type EventHandler<T> = (event: T, instance?: LepusEventInstance) => void;

export interface BaseEvent<T = string, D = any> {
  /** Event type. */
  type: T;

  /** Timestamp when the event was generated. */
  timestamp: number;

  /** Collection of attribute values of the target that triggers the event. */
  target: Target;

  /** Collection of attribute values of the target that listens to the event. */
  currentTarget: Target;

  /** Additional information. */
  detail: D;
}

export interface LynxEvent<Target> {
  /**
   * Listening for background image loading success.
   * @since since Lynx 2.6
   */
  BGLoad?: EventHandler<ImageLoadEvent>;

  /**
   * Failed to load background image for listening.
   * @since since: Android: Lynx 2.6, iOS: Lynx 2.8
   */
  BGError?: EventHandler<ImageErrorEvent>;

  // NodeAppear?: EventHandler<ReactLynx.AppearanceEvent>;

  // NodeDisappear?: EventHandler<ReactLynx.AppearanceEvent>;

  /** Finger touch action begins. */
  TouchStart?: EventHandler<BaseTouchEvent<Target>>;

  /** Moving after touching with fingers. */
  TouchMove?: EventHandler<BaseTouchEvent<Target>>;

  /** Finger touch actions are interrupted by incoming call reminders and pop-up windows. */
  TouchCancel?: EventHandler<BaseTouchEvent<Target>>;

  /** Finger touch action ends. */
  TouchEnd?: EventHandler<BaseTouchEvent<Target>>;

  /** After touching the finger, if it leaves after more than 350ms and the event callback function is specified and triggered, the tap event will not be triggered. */
  LongPress?: EventHandler<BaseCommonEvent<Target>>;

  /** It will trigger during a transition animation start. */
  TransitionStart?: EventHandler<BaseTransitionEvent<Target>>;

  /** It will trigger when a transition animation is cancelled. */
  TransitionCancel?: EventHandler<BaseTransitionEvent<Target>>;

  /** It will trigger after the transition or createAnimation animation is finished. */
  TransitionEnd?: EventHandler<BaseTransitionEvent<Target>>;

  /** It will trigger at the beginning of an animation. */
  AnimationStart?: EventHandler<BaseAnimationEvent<Target>>;

  /** It will trigger during an animation iteration. */
  AnimationIteration?: EventHandler<BaseAnimationEvent<Target>>;

  /** It will trigger when an animation is cancelled. */
  AnimationCancel?: EventHandler<BaseAnimationEvent<Target>>;

  /** It will trigger upon completion of an animation. */
  AnimationEnd?: EventHandler<BaseAnimationEvent<Target>>;

  /** Mouse Clicked. */
  MouseDown?: EventHandler<BaseMouseEvent<Target>>;

  /** Mouse released. */
  MouseUp?: EventHandler<BaseMouseEvent<Target>>;

  /** Mouse movement. */
  MouseMove?: EventHandler<BaseMouseEvent<Target>>;

  /** Mouse click. */
  MouseClick?: EventHandler<BaseMouseEvent<Target>>;

  /** Double-click the mouse.  */
  MouseDblClick?: EventHandler<BaseMouseEvent<Target>>;

  /** Long press on the mouse. */
  MouseLongPress?: EventHandler<BaseMouseEvent<Target>>;

  /** Mouse (or touchpad) scrolling. */
  Wheel?: EventHandler<BaseWheelEvent<Target>>;

  /** Keyboard (or remote control) button pressed. */
  KeyDown?: EventHandler<BaseKeyEvent<Target>>;

  /** Keyboard (or remote control) key released. */
  KeyUp?: EventHandler<BaseKeyEvent<Target>>;

  /** Element gets focus. */
  Focus?: EventHandler<BaseCommonEvent<Target>>;

  /** Element loses focus. */
  Blur?: EventHandler<BaseCommonEvent<Target>>;

  /** layout info Change event */
  LayoutChange?: LayoutChangeEvent;

  /** UI appear event */
  UIAppear?: UIAppearanceEvent;

  /** UI disappear event */
  UIDisappear?: UIAppearanceEvent;
  /**
   * The text layout event is triggered when the text layout changes.
   * @since since: Android: Lynx 2.6, iOS: Lynx 2.8
   */

  /**
   * The custom actions of the current accessibility element is triggered.
   * @Android
   * @iOS
   * @spec {@link https://developer.apple.com/documentation/appkit/nsaccessibility/2869551-accessibilitycustomactions/ | iOS}
   * @spec {@link https://developer.android.com/reference/androidx/core/view/accessibility/AccessibilityNodeInfoCompat?hl=en#addAction(androidx.core.view.accessibility.AccessibilityNodeInfoCompat.AccessibilityActionCompat) | Android}
   */
  AccessibilityAction?: EventHandler<AccessibilityActionDetailEvent<Target>>;
}

export type LayoutChangeEvent = EventHandler<LayoutChangeDetailEvent<Target>>;

export type UIAppearanceEvent = EventHandler<UIAppearanceDetailEvent<Target>>;

/**
 * This type is different with LynxEvent that they only have `bind` and `catch` event. But not `on` Event.
 */
export interface LynxBindCatchEvent<Target = any> {
  /** Immediately lift your finger after touching. */
  Tap?: EventHandler<BaseTouchEvent<Target>>;

  /** After touching the finger, leave after more than 350ms (it is recommended to use the longpress event instead). */
  LongTap?: EventHandler<BaseTouchEvent<Target>>;
}

export type LynxEventPropsBase<Target> = {
  [K in keyof LynxEvent<Target> as Lowercase<`bind${K}` | `catch${K}` | `capture-bind${K}` | `capture-catch${K}` | `global-bind${K}`>]: LynxEvent<Target>[K];
} & {
  [K in keyof LynxBindCatchEvent<Target> as Lowercase<`bind${K}` | `catch${K}` | `capture-bind${K}` | `capture-catch${K}` | `global-bind${K}`>]: LynxBindCatchEvent<Target>[K];
};

export type LynxEventProps = LynxEventPropsBase<Target>;

export interface ITouchEvent extends BaseTouchEvent<Target> {}
export interface IMouseEvent extends BaseMouseEvent<Target> {}
export interface IWheelEvent extends BaseWheelEvent<Target> {}
export interface IKeyEvent extends BaseKeyEvent<Target> {}
