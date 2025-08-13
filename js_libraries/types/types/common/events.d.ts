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

export interface BaseTouchEvent<T> extends BaseEventOrig<any, T> {
  /** 
   * Collection of touch points currently on the touch plane.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  touches: Array<Touch>;

  /** 
   * Collection of touch points whose state has changed compared to the last touch event.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  changedTouches: Array<Touch>;

  detail: {
    /** 
     * The current position of the touch point relative to the page's x-coordinate.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    x: number;
    /** 
     * The current position of the touch point relative to the page's y-coordinate.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    y: number;
  }
}

export interface TouchEvent extends BaseTouchEvent<Target> {}

export interface BaseMouseEvent<T> extends BaseEventOrig<{}, T> {
  /** 
   * The currently pressed mouse button, if multiple buttons are pressed simultaneously, is the last one pressed. 
   * @PC
   */
  button: number;
  /** 
   * The current mouse button being pressed, if multiple buttons are simultaneously pressed, it is a bit field composed of all button codes. 
   * @PC
   */
  buttons: number;
  /** 
   * clientX 
   * @PC
   */
  x: number;
  /** 
   * clientY 
   * @PC
   */
  y: number;
  /** 
   * The current position of the cursor relative to the page's x-coordinate. 
   * @PC
   */
  pageX: number;
  /** 
   * The current position of the cursor relative to the page's y-coordinate. 
   * @PC
   */
  pageY: number;
  /** 
   * The current position of the cursor relative to the element's x-coordinate. 
   * @PC
   */
  clientX: number;
  /** 
   * The current position of the cursor relative to the element's y-coordinate. 
   * @PC
   */
  clientY: number;
}

export interface MouseEvent extends BaseMouseEvent<Target> {}

export interface BaseWheelEvent<T> extends BaseEventOrig<{}, T> {
  /** 
   * clientX 
   * @PC
   */
  x: number;
  /** 
   * clientY 
   * @PC
   */
  y: number;
  /** 
   * The current position of the cursor relative to the page's x-coordinate. 
   * @PC
   */
  pageX: number;
  /** 
   * The current position of the cursor relative to the page's y-coordinate. 
   * @PC
   */
  pageY: number;
  /** 
   * The current position of the cursor relative to the element's x-coordinate. 
   * @PC
   */ 
  clientX: number;
  /** 
   * The current position of the cursor relative to the element's y-coordinate. 
   * @PC
   */
  clientY: number;
  /** 
   * The distance of x-axis scrolling on the mouse wheel. 
   * @PC
   */ 
  deltaX: number;
  /** 
   * The distance of y-axis scrolling on the mouse wheel. 
   * @PC
   */
  deltaY: number;
}

export interface WheelEvent extends BaseWheelEvent<Target> {}

export interface BaseKeyEvent<T> extends BaseEventOrig<{}, T> {
  /** 
   * Button Name
   * @PC 
   */
  key: string;
}

export interface KeyEvent extends BaseKeyEvent<Target> {}

export interface BaseAnimationEvent<T> extends BaseEventOrig<{}, T> {
  /** 
   * Animation params.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  params: {
    /**
     * Animation type.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    animation_type: 'keyframe-animation';
    /**
     * Animation name.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    animation_name: string;
    /**
     * If new animator enabled.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    new_animator?: true;
  };
}

export interface AnimationEvent extends BaseAnimationEvent<Target> {}

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
export interface TransitionEvent extends BaseTransitionEvent<Target> {}

export interface BaseImageLoadEvent<T> extends BaseEventOrig<{}, T> {
  /**
   * Image width. In pixels.
   * @Android
   * @iOS
   */
  width: number;
  /**
   * Image height. In pixels. 
   * @Android
   * @iOS
   */
  height: number;
}

export interface ImageLoadEvent extends BaseImageLoadEvent<Target> {
  /**
   * Image start loading timestamp, Unit (ms).
   * @Android
   * @iOS
   * @since 3.6
   */
  load_start?: number;

  /**
   * Image loading completion timestamp, Unit (ms). It mainly includes image downloading and image decoding.
   * @Android
   * @iOS
   * @since 3.6
   */
  load_finish?: number;

  /**
   * Image loading duration (load_finish - load_start), Unit (ms).
   * @Android
   * @iOS
   * @since 3.6
   */
  cost?: number;

  /**
   * image URI.
   * @Android
   * @iOS
   * @since 3.6
   */
  src?: string;

  /**
   * image view width. Unit: (px)
   * @Android
   * @iOS
   * @since 3.6
   */
  view_width?: number,

  /**
   * image view height. Unit: (px)
   * @Android
   * @iOS
   * @since 3.6
   */
  view_height?: number;

  /**
   * Image memory size, Unit: Byte (B).
   * @Android
   * @iOS
   * @since 3.6
   */
  memory_cost?: number;

  /**
   * Source of image, network download, memory cache or disk cache.
   * @Android
   * @iOS
   * @since 3.6
   */
  origin?: number;
}


export interface BaseImageErrorEvent<T> extends BaseEventOrig<{}, T> {
  /**
   * Error message.
   * @Android
   * @iOS
   */
  errMsg: string;
  /**
   * Error code.
   * @Android
   * @iOS
   */
  error_code: number;
  /**
   * Categorized error code.
   * @Android
   * @iOS
   */
  lynx_categorized_code: number;
}

export interface ImageErrorEvent extends BaseImageErrorEvent<Target> {
    errMsg: string;
    error_code: number;
    lynx_categorized_code: number;
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

export interface TextSelectionChangeEventDetail {
  start: number;
  end: number;
  direction: 'forward' | 'backward';
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
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  detail: {
    /**
     * The id selector of the target.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    id: string;
    /** 
     * The width of the target. In pixels.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    width: number;
    /** The height of the target. In pixels.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */ 
    height: number;
    /** 
     * The position of the target's top border relative to the page's coordinate. In pixels.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    top: number;
    /** 
     * The position of the target's right border relative to the page's coordinate. In pixels.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    right: number;
    /** 
     * The position of the target's bottom border relative to the page's coordinate. In pixels.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    bottom: number;
    /** 
     * The position of the target's left border relative to the page's coordinate. In pixels.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    left: number;
    /** 
     * The collection of custom attributes starting with data- on the event target.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    dataset: {
      [key: string]: any;
    };
  };
}

export interface UIAppearanceDetailEvent<T> extends BaseEventOrig<{}, T> {
  type: 'uiappear' | 'uidisappear';
  detail: {
    /** 
     * exposure-id set on the target.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    'exposure-id': string;
    /** 
     * exposure-scene set on the target.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    'exposure-scene': string;
    /** 
     * uid of the target
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    'unique-id': string;
    /** 
     * The collection of custom attributes starting with data- on the event target.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
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
  /** 
   * Event type.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  type: T;

  /** 
   * Timestamp when the event was generated.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  timestamp: number;

  /** 
   * Collection of attribute values of the target that triggers the event.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  target: Target;

  /** 
   * Collection of attribute values of the target that listens to the event.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  currentTarget: Target;

  /** 
   * Additional information.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  detail: D;
}

export interface LynxEvent<T> {
  /**
   * Listening for background image loading success.
   * @Android
   * @iOS
   * @since 2.6
   */
  BGLoad?: EventHandler<BaseImageLoadEvent<T>>;

  /**
   * Failed to load background image for listening.
   * @Android
   * @iOS
   * @since 2.8
   */
  BGError?: EventHandler<BaseImageErrorEvent<T>>;

  // NodeAppear?: EventHandler<ReactLynx.AppearanceEvent>;

  // NodeDisappear?: EventHandler<ReactLynx.AppearanceEvent>;

  /** 
   * Finger touch action begins. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  TouchStart?: EventHandler<BaseTouchEvent<T>>;

  /** 
   * Moving after touching with fingers.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  TouchMove?: EventHandler<BaseTouchEvent<T>>;

  /** 
   * Finger touch actions are interrupted by incoming call reminders and pop-up windows. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  TouchCancel?: EventHandler<BaseTouchEvent<T>>;

  /** 
   * Finger touch action ends. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  TouchEnd?: EventHandler<BaseTouchEvent<T>>;

  /** 
   * After touching the finger, if it leaves after more than 350ms and the event callback function is specified and triggered, the tap event will not be triggered. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  LongPress?: EventHandler<BaseTouchEvent<T>>;

  /** 
   * It will trigger during a transition animation start. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */ 
  TransitionStart?: EventHandler<BaseTransitionEvent<T>>;

  /** 
   * It will trigger when a transition animation is cancelled. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  TransitionCancel?: EventHandler<BaseTransitionEvent<T>>;

  /** 
   * It will trigger after the transition or createAnimation animation is finished. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  TransitionEnd?: EventHandler<BaseTransitionEvent<T>>;

  /** 
   * It will trigger at the beginning of an animation. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  AnimationStart?: EventHandler<BaseAnimationEvent<T>>;

  /** 
   * It will trigger during an animation iteration. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  AnimationIteration?: EventHandler<BaseAnimationEvent<T>>;

  /** 
   * It will trigger when an animation is cancelled. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  AnimationCancel?: EventHandler<BaseAnimationEvent<T>>;

  /** 
   * It will trigger upon completion of an animation. 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  AnimationEnd?: EventHandler<BaseAnimationEvent<T>>;

  /** 
   * Indicates that a mouse button (primary or secondary) is pressed. The target is the UI that contains the mouse pointer and is closest to the user.
   * @PC
   */
  MouseDown?: EventHandler<BaseMouseEvent<T>>;

  /** 
   * Indicates that a mouse button (primary or secondary) is released. The target is the UI that contains the mouse pointer and is closest to the user.
   * @PC
   */
  MouseUp?: EventHandler<BaseMouseEvent<T>>;

  /** 
   * Mouse movement.
   * @PC
   */
  MouseMove?: EventHandler<BaseMouseEvent<T>>;

  /** 
   * Mouse click.
   * @PC
   */
  MouseClick?: EventHandler<BaseMouseEvent<T>>;

  /** 
   * Double-click the mouse.
   * @PC  
   */
  MouseDblClick?: EventHandler<BaseMouseEvent<T>>;

  /** 
   * Long press on the mouse.
   * @PC
   */
  MouseLongPress?: EventHandler<BaseMouseEvent<T>>;

  /** 
   * Mouse (or touchpad) scrolling.
   * @PC
   */
  Wheel?: EventHandler<BaseWheelEvent<T>>;

  /** 
   * Keyboard (or remote control) button pressed.
   * @PC
   */
  KeyDown?: EventHandler<BaseKeyEvent<T>>;

  /** 
   * Keyboard (or remote control) key released.
   * @PC
   */
  KeyUp?: EventHandler<BaseKeyEvent<T>>;

  /** 
   * Element gets focus.
   * @PC
   */
  Focus?: EventHandler<BaseCommonEvent<T>>;

  /** 
   * Element loses focus.
   * @PC
   */
  Blur?: EventHandler<BaseCommonEvent<T>>;

  /** 
   * Layout info changed event 
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  LayoutChange?: EventHandler<LayoutChangeDetailEvent<T>>;

  /**
   * Element appear event
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  UIAppear?: EventHandler<UIAppearanceDetailEvent<T>>;

  /** 
   * Element disappear event
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  UIDisappear?: EventHandler<UIAppearanceDetailEvent<T>>;

  /**
   * The custom actions of the current accessibility element is triggered.
   * @Android
   * @iOS
   * @spec {@link https://developer.apple.com/documentation/appkit/nsaccessibility/2869551-accessibilitycustomactions/ | iOS}
   * @spec {@link https://developer.android.com/reference/androidx/core/view/accessibility/AccessibilityNodeInfoCompat?hl=en#addAction(androidx.core.view.accessibility.AccessibilityNodeInfoCompat.AccessibilityActionCompat) | Android}
   */
  AccessibilityAction?: EventHandler<AccessibilityActionDetailEvent<T>>;
}

export interface LayoutChangeEvent extends LayoutChangeDetailEvent<Target> {}
export interface UIAppearanceEvent extends UIAppearanceDetailEvent<Target> {}

/**
 * This type is different with LynxEvent that they only have `bind` and `catch` event. But not `on` Event.
 */
export interface LynxBindCatchEvent<T = any> {
  /**
   * Triggered when a finger clicks on the touch plane. This event and the `longpress` event are mutually exclusive in event listening. That is, if the front-end listens to both events simultaneously, the two events will not be triggered at the same time, and the `longpress` event takes precedence.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  Tap?: EventHandler<BaseTouchEvent<T>>;

  /** 
   * After touching the finger, leave after more than 350ms
   * @deprecated It is recommended to use the longpress event instead
   */
  LongTap?: EventHandler<BaseTouchEvent<T>>;
}

type PrefixedEvent<E> = {
  bind?: E;
  catch?: E;
  'capture-bind'?: E;
  'capture-catch'?: E;
  'global-bind'?: E;
}

// Helper interfaces for each event, providing explicit properties
// for bind, catch, capture-bind, capture-catch, and global-bind prefixes.
interface BGLoadProps<T> { bindbgload?: LynxEvent<T>['BGLoad']; catchbgload?: LynxEvent<T>['BGLoad']; 'capture-bindbgload'?: LynxEvent<T>['BGLoad']; 'capture-catchbgload'?: LynxEvent<T>['BGLoad']; 'global-bindbgload'?: LynxEvent<T>['BGLoad']; }
interface BGErrorProps<T> { bindbgerror?: LynxEvent<T>['BGError']; catchbgerror?: LynxEvent<T>['BGError']; 'capture-bindbgerror'?: LynxEvent<T>['BGError']; 'capture-catchbgerror'?: LynxEvent<T>['BGError']; 'global-bindbgerror'?: LynxEvent<T>['BGError']; }
interface TouchStartProps<T> { bindtouchstart?: LynxEvent<T>['TouchStart']; catchtouchstart?: LynxEvent<T>['TouchStart']; 'capture-bindtouchstart'?: LynxEvent<T>['TouchStart']; 'capture-catchtouchstart'?: LynxEvent<T>['TouchStart']; 'global-bindtouchstart'?: LynxEvent<T>['TouchStart']; }
interface TouchMoveProps<T> { bindtouchmove?: LynxEvent<T>['TouchMove']; catchtouchmove?: LynxEvent<T>['TouchMove']; 'capture-bindtouchmove'?: LynxEvent<T>['TouchMove']; 'capture-catchtouchmove'?: LynxEvent<T>['TouchMove']; 'global-bindtouchmove'?: LynxEvent<T>['TouchMove']; }
interface TouchCancelProps<T> { bindtouchcancel?: LynxEvent<T>['TouchCancel']; catchtouchcancel?: LynxEvent<T>['TouchCancel']; 'capture-bindtouchcancel'?: LynxEvent<T>['TouchCancel']; 'capture-catchtouchcancel'?: LynxEvent<T>['TouchCancel']; 'global-bindtouchcancel'?: LynxEvent<T>['TouchCancel']; }
interface TouchEndProps<T> { bindtouchend?: LynxEvent<T>['TouchEnd']; catchtouchend?: LynxEvent<T>['TouchEnd']; 'capture-bindtouchend'?: LynxEvent<T>['TouchEnd']; 'capture-catchtouchend'?: LynxEvent<T>['TouchEnd']; 'global-bindtouchend'?: LynxEvent<T>['TouchEnd']; }
interface LongPressProps<T> { bindlongpress?: LynxEvent<T>['LongPress']; catchlongpress?: LynxEvent<T>['LongPress']; 'capture-bindlongpress'?: LynxEvent<T>['LongPress']; 'capture-catchlongpress'?: LynxEvent<T>['LongPress']; 'global-bindlongpress'?: LynxEvent<T>['LongPress']; }
interface TransitionStartProps<T> { bindtransitionstart?: LynxEvent<T>['TransitionStart']; catchtransitionstart?: LynxEvent<T>['TransitionStart']; 'capture-bindtransitionstart'?: LynxEvent<T>['TransitionStart']; 'capture-catchtransitionstart'?: LynxEvent<T>['TransitionStart']; 'global-bindtransitionstart'?: LynxEvent<T>['TransitionStart']; }
interface TransitionCancelProps<T> { bindtransitioncancel?: LynxEvent<T>['TransitionCancel']; catchtransitioncancel?: LynxEvent<T>['TransitionCancel']; 'capture-bindtransitioncancel'?: LynxEvent<T>['TransitionCancel']; 'capture-catchtransitioncancel'?: LynxEvent<T>['TransitionCancel']; 'global-bindtransitioncancel'?: LynxEvent<T>['TransitionCancel']; }
interface TransitionEndProps<T> { bindtransitionend?: LynxEvent<T>['TransitionEnd']; catchtransitionend?: LynxEvent<T>['TransitionEnd']; 'capture-bindtransitionend'?: LynxEvent<T>['TransitionEnd']; 'capture-catchtransitionend'?: LynxEvent<T>['TransitionEnd']; 'global-bindtransitionend'?: LynxEvent<T>['TransitionEnd']; }
interface AnimationStartProps<T> { bindanimationstart?: LynxEvent<T>['AnimationStart']; catchanimationstart?: LynxEvent<T>['AnimationStart']; 'capture-bindanimationstart'?: LynxEvent<T>['AnimationStart']; 'capture-catchanimationstart'?: LynxEvent<T>['AnimationStart']; 'global-bindanimationstart'?: LynxEvent<T>['AnimationStart']; }
interface AnimationIterationProps<T> { bindanimationiteration?: LynxEvent<T>['AnimationIteration']; catchanimationiteration?: LynxEvent<T>['AnimationIteration']; 'capture-bindanimationiteration'?: LynxEvent<T>['AnimationIteration']; 'capture-catchanimationiteration'?: LynxEvent<T>['AnimationIteration']; 'global-bindanimationiteration'?: LynxEvent<T>['AnimationIteration']; }
interface AnimationCancelProps<T> { bindanimationcancel?: LynxEvent<T>['AnimationCancel']; catchanimationcancel?: LynxEvent<T>['AnimationCancel']; 'capture-bindanimationcancel'?: LynxEvent<T>['AnimationCancel']; 'capture-catchanimationcancel'?: LynxEvent<T>['AnimationCancel']; 'global-bindanimationcancel'?: LynxEvent<T>['AnimationCancel']; }
interface AnimationEndProps<T> { bindanimationend?: LynxEvent<T>['AnimationEnd']; catchanimationend?: LynxEvent<T>['AnimationEnd']; 'capture-bindanimationend'?: LynxEvent<T>['AnimationEnd']; 'capture-catchanimationend'?: LynxEvent<T>['AnimationEnd']; 'global-bindanimationend'?: LynxEvent<T>['AnimationEnd']; }
interface MouseDownProps<T> { bindmousedown?: LynxEvent<T>['MouseDown']; catchmousedown?: LynxEvent<T>['MouseDown']; 'capture-bindmousedown'?: LynxEvent<T>['MouseDown']; 'capture-catchmousedown'?: LynxEvent<T>['MouseDown']; 'global-bindmousedown'?: LynxEvent<T>['MouseDown']; }
interface MouseUpProps<T> { bindmouseup?: LynxEvent<T>['MouseUp']; catchmouseup?: LynxEvent<T>['MouseUp']; 'capture-bindmouseup'?: LynxEvent<T>['MouseUp']; 'capture-catchmouseup'?: LynxEvent<T>['MouseUp']; 'global-bindmouseup'?: LynxEvent<T>['MouseUp']; }
interface MouseMoveProps<T> { bindmousemove?: LynxEvent<T>['MouseMove']; catchmousemove?: LynxEvent<T>['MouseMove']; 'capture-bindmousemove'?: LynxEvent<T>['MouseMove']; 'capture-catchmousemove'?: LynxEvent<T>['MouseMove']; 'global-bindmousemove'?: LynxEvent<T>['MouseMove']; }
interface MouseClickProps<T> { bindmouseclick?: LynxEvent<T>['MouseClick']; catchmouseclick?: LynxEvent<T>['MouseClick']; 'capture-bindmouseclick'?: LynxEvent<T>['MouseClick']; 'capture-catchmouseclick'?: LynxEvent<T>['MouseClick']; 'global-bindmouseclick'?: LynxEvent<T>['MouseClick']; }
interface MouseDblClickProps<T> { bindmousedblclick?: LynxEvent<T>['MouseDblClick']; catchmousedblclick?: LynxEvent<T>['MouseDblClick']; 'capture-bindmousedblclick'?: LynxEvent<T>['MouseDblClick']; 'capture-catchmousedblclick'?: LynxEvent<T>['MouseDblClick']; 'global-bindmousedblclick'?: LynxEvent<T>['MouseDblClick']; }
interface MouseLongPressProps<T> { bindmouselongpress?: LynxEvent<T>['MouseLongPress']; catchmouselongpress?: LynxEvent<T>['MouseLongPress']; 'capture-bindmouselongpress'?: LynxEvent<T>['MouseLongPress']; 'capture-catchmouselongpress'?: LynxEvent<T>['MouseLongPress']; 'global-bindmouselongpress'?: LynxEvent<T>['MouseLongPress']; }
interface WheelProps<T> { bindwheel?: LynxEvent<T>['Wheel']; catchwheel?: LynxEvent<T>['Wheel']; 'capture-bindwheel'?: LynxEvent<T>['Wheel']; 'capture-catchwheel'?: LynxEvent<T>['Wheel']; 'global-bindwheel'?: LynxEvent<T>['Wheel']; }
interface KeyDownProps<T> { bindkeydown?: LynxEvent<T>['KeyDown']; catchkeydown?: LynxEvent<T>['KeyDown']; 'capture-bindkeydown'?: LynxEvent<T>['KeyDown']; 'capture-catchkeydown'?: LynxEvent<T>['KeyDown']; 'global-bindkeydown'?: LynxEvent<T>['KeyDown']; }
interface KeyUpProps<T> { bindkeyup?: LynxEvent<T>['KeyUp']; catchkeyup?: LynxEvent<T>['KeyUp']; 'capture-bindkeyup'?: LynxEvent<T>['KeyUp']; 'capture-catchkeyup'?: LynxEvent<T>['KeyUp']; 'global-bindkeyup'?: LynxEvent<T>['KeyUp']; }
interface FocusProps<T> { bindfocus?: LynxEvent<T>['Focus']; catchfocus?: LynxEvent<T>['Focus']; 'capture-bindfocus'?: LynxEvent<T>['Focus']; 'capture-catchfocus'?: LynxEvent<T>['Focus']; 'global-bindfocus'?: LynxEvent<T>['Focus']; }
interface BlurProps<T> { bindblur?: LynxEvent<T>['Blur']; catchblur?: LynxEvent<T>['Blur']; 'capture-bindblur'?: LynxEvent<T>['Blur']; 'capture-catchblur'?: LynxEvent<T>['Blur']; 'global-bindblur'?: LynxEvent<T>['Blur']; }
interface LayoutChangeProps<T> { bindlayoutchange?: LynxEvent<T>['LayoutChange']; catchlayoutchange?: LynxEvent<T>['LayoutChange']; 'capture-bindlayoutchange'?: LynxEvent<T>['LayoutChange']; 'capture-catchlayoutchange'?: LynxEvent<T>['LayoutChange']; 'global-bindlayoutchange'?: LynxEvent<T>['LayoutChange']; }
interface UIAppearProps<T> { binduiappear?: LynxEvent<T>['UIAppear']; catchuiappear?: LynxEvent<T>['UIAppear']; 'capture-binduiappear'?: LynxEvent<T>['UIAppear']; 'capture-catchuiappear'?: LynxEvent<T>['UIAppear']; 'global-binduiappear'?: LynxEvent<T>['UIAppear']; }
interface UIDisappearProps<T> { binduidisappear?: LynxEvent<T>['UIDisappear']; catchuidisappear?: LynxEvent<T>['UIDisappear']; 'capture-binduidisappear'?: LynxEvent<T>['UIDisappear']; 'capture-catchuidisappear'?: LynxEvent<T>['UIDisappear']; 'global-binduidisappear'?: LynxEvent<T>['UIDisappear']; }
interface AccessibilityActionProps<T> { bindaccessibilityaction?: LynxEvent<T>['AccessibilityAction']; catchaccessibilityaction?: LynxEvent<T>['AccessibilityAction']; 'capture-bindaccessibilityaction'?: LynxEvent<T>['AccessibilityAction']; 'capture-catchaccessibilityaction'?: LynxEvent<T>['AccessibilityAction']; 'global-bindaccessibilityaction'?: LynxEvent<T>['AccessibilityAction']; }
interface TapProps<T> { bindtap?: LynxBindCatchEvent<T>['Tap']; catchtap?: LynxBindCatchEvent<T>['Tap']; 'capture-bindtap'?: LynxBindCatchEvent<T>['Tap']; 'capture-catchtap'?: LynxBindCatchEvent<T>['Tap']; 'global-bindtap'?: LynxBindCatchEvent<T>['Tap']; }
interface LongTapProps<T> { bindlongtap?: LynxBindCatchEvent<T>['LongTap']; catchlongtap?: LynxBindCatchEvent<T>['LongTap']; 'capture-bindlongtap'?: LynxBindCatchEvent<T>['LongTap']; 'capture-catchlongtap'?: LynxBindCatchEvent<T>['LongTap']; 'global-bindlongtap'?: LynxBindCatchEvent<T>['LongTap']; }

/**
 * A combination of all possible event properties, statically defined for IDE-friendliness.
 * This replaces the previous dynamic mapped type.
 */
export type LynxEventPropsBase<T> = BGLoadProps<T> &
  BGErrorProps<T> &
  TouchStartProps<T> &
  TouchMoveProps<T> &
  TouchCancelProps<T> &
  TouchEndProps<T> &
  LongPressProps<T> &
  TransitionStartProps<T> &
  TransitionCancelProps<T> &
  TransitionEndProps<T> &
  AnimationStartProps<T> &
  AnimationIterationProps<T> &
  AnimationCancelProps<T> &
  AnimationEndProps<T> &
  MouseDownProps<T> &
  MouseUpProps<T> &
  MouseMoveProps<T> &
  MouseClickProps<T> &
  MouseDblClickProps<T> &
  MouseLongPressProps<T> &
  WheelProps<T> &
  KeyDownProps<T> &
  KeyUpProps<T> &
  FocusProps<T> &
  BlurProps<T> &
  LayoutChangeProps<T> &
  UIAppearProps<T> &
  UIDisappearProps<T> &
  AccessibilityActionProps<T> &
  TapProps<T> &
  LongTapProps<T>;

export type LynxEventProps = LynxEventPropsBase<Target>;

export interface ITouchEvent extends BaseTouchEvent<Target> {}
export interface IMouseEvent extends BaseMouseEvent<Target> {}
export interface IWheelEvent extends BaseWheelEvent<Target> {}
export interface IKeyEvent extends BaseKeyEvent<Target> {}

interface LynxMessageEvents {
  // from native context
  __GlobalEvent: {
    data: [
      // name
      string,
      // params
      any,
    ];
    origin: 'NATIVE';
  };
  __DestroyLifetime: {
    data: [
      // appGUID
      number,
    ];
    origin: 'NATIVE';
  }

  // from engine context
  __RenderPage: {
    data: [
      // data
      object,
      // renderOptions
      object,
    ];
    origin: 'ENGINE';
  };
  __UpdatePage: {
    data: [
      // data
      object,
      // updateOptions
      object,
    ];
    origin: 'ENGINE';
  };
  __UpdateGlobalProps: {
    data: [
      // data
      Object
    ];
    origin: 'ENGINE';
  };
  __RemoveComponents: {
    data: [];
    origin: 'ENGINE';
  };
  __SSRHydrate: {
    data: [
      // customHydrateInfo
      string,
      // listIDs
      number[],
    ];
    origin: 'ENGINE';
  };
}

type LynxMessageEventType = keyof LynxMessageEvents;

type LynxMessageEventsWithType = {
  [k in LynxMessageEventType]: LynxMessageEvents[k] & {
    type: k;
  };
};

export type LynxMessageEvent = LynxMessageEventsWithType[LynxMessageEventType];
