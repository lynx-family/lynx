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
   * Current scale factor. Greater than 1 means zoom in. Less than 1 means zoom out.
   * @PC
   */
  scale: number;
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
  /**
   * Whether the key is being held down and is automatically repeating.
   * @PC
   */
  repeat: boolean;
  /**
   * Whether the Alt key is pressed.
   * @PC
   */
  altKey: boolean;
  /**
   * Whether the Shift key is pressed.
   * @PC
   */
  shiftKey: boolean;
  /**
   * Whether the Control key is pressed.
   * @PC
   */
  ctrlKey: boolean;
  /**
   * Whether the Meta key is pressed.
   * @PC
   */
  metaKey: boolean;
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
   * @Android 3.6
   * @iOS 3.6
   */
  load_start?: number;

  /**
   * Image loading completion timestamp, Unit (ms). It mainly includes image downloading and image decoding.
   * @Android 3.6
   * @iOS 3.6
   */
  load_finish?: number;

  /**
   * Image loading duration (load_finish - load_start), Unit (ms).
   * @Android 3.6
   * @iOS 3.6
   */
  cost?: number;

  /**
   * image URI.
   * @Android 3.6
   * @iOS 3.6
   */
  src?: string;

  /**
   * image view width. Unit: (px)
   * @Android 3.6
   * @iOS 3.6
   */
  view_width?: number,

  /**
   * image view height. Unit: (px)
   * @Android 3.6
   * @iOS 3.6
   */
  view_height?: number;

  /**
   * Image memory size, Unit: Byte (B).
   * @Android 3.6
   * @iOS 3.6
   */
  memory_cost?: number;

  /**
   * Source of image, network download, memory cache or disk cache.
   * @Android 3.6
   * @iOS 3.6
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
   * @Android 2.6
   * @iOS 2.6
   */
  BGLoad?: EventHandler<BaseImageLoadEvent<T>>;

  /**
   * Failed to load background image for listening.
   * @Android 2.8
   * @iOS 2.8
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
   * Indicates that a pointing device is moved onto the element.
   * @PC
   */
  MouseEnter?: EventHandler<BaseMouseEvent<T>>;

  /** 
   * Indicates that a pointing device is moved off the element.
   * @PC
   */
  MouseLeave?: EventHandler<BaseMouseEvent<T>>;

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
   * Zoom gesture in the trackpaad
   * @PC
   */
  Zoom?: EventHandler<BaseMouseEvent<T>>;

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
/**
   * @Android 0.1
   * @iOS 0.1
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bind?: E;
/**
   * @Android 0.1
   * @iOS 0.1
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catch?: E;
/**
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.16
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-bind'?: E;
/**
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.16
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-catch'?: E;
/**
   * @Android 2.6
   * @iOS 2.6
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 2.6
   * @ClayIOS 2.6
   * @ClayMacOS 2.6
   * @ClayWindows 2.6
   * @ClayHarmony 2.17
   */
  'global-bind'?: E;
}

// Helper interfaces for each event, providing explicit properties
// for bind, catch, capture-bind, capture-catch, and global-bind prefixes.
interface BGLoadProps<T> {
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  bindbgload?: LynxEvent<T>['BGLoad'];
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  catchbgload?: LynxEvent<T>['BGLoad'];
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  'capture-bindbgload'?: LynxEvent<T>['BGLoad'];
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  'capture-catchbgload'?: LynxEvent<T>['BGLoad'];
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  'global-bindbgload'?: LynxEvent<T>['BGLoad']; }
interface BGErrorProps<T> {
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  bindbgerror?: LynxEvent<T>['BGError'];
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  catchbgerror?: LynxEvent<T>['BGError'];
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  'capture-bindbgerror'?: LynxEvent<T>['BGError'];
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  'capture-catchbgerror'?: LynxEvent<T>['BGError'];
  /**
   * @Android 3.0
   * @iOS 2.18
   * @Harmony 3.8
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 4.1
   */
  'global-bindbgerror'?: LynxEvent<T>['BGError']; }
interface TouchStartProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindtouchstart?: LynxEvent<T>['TouchStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchtouchstart?: LynxEvent<T>['TouchStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindtouchstart'?: LynxEvent<T>['TouchStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchtouchstart'?: LynxEvent<T>['TouchStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindtouchstart'?: LynxEvent<T>['TouchStart']; }
interface TouchMoveProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindtouchmove?: LynxEvent<T>['TouchMove'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchtouchmove?: LynxEvent<T>['TouchMove'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindtouchmove'?: LynxEvent<T>['TouchMove'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchtouchmove'?: LynxEvent<T>['TouchMove'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindtouchmove'?: LynxEvent<T>['TouchMove']; }
interface TouchCancelProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindtouchcancel?: LynxEvent<T>['TouchCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchtouchcancel?: LynxEvent<T>['TouchCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindtouchcancel'?: LynxEvent<T>['TouchCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchtouchcancel'?: LynxEvent<T>['TouchCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindtouchcancel'?: LynxEvent<T>['TouchCancel']; }
interface TouchEndProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindtouchend?: LynxEvent<T>['TouchEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchtouchend?: LynxEvent<T>['TouchEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindtouchend'?: LynxEvent<T>['TouchEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchtouchend'?: LynxEvent<T>['TouchEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindtouchend'?: LynxEvent<T>['TouchEnd']; }
interface LongPressProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindlongpress?: LynxEvent<T>['LongPress'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchlongpress?: LynxEvent<T>['LongPress'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindlongpress'?: LynxEvent<T>['LongPress'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchlongpress'?: LynxEvent<T>['LongPress'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindlongpress'?: LynxEvent<T>['LongPress']; }
interface TransitionStartProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindtransitionstart?: LynxEvent<T>['TransitionStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchtransitionstart?: LynxEvent<T>['TransitionStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindtransitionstart'?: LynxEvent<T>['TransitionStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchtransitionstart'?: LynxEvent<T>['TransitionStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindtransitionstart'?: LynxEvent<T>['TransitionStart']; }
interface TransitionCancelProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   */
  bindtransitioncancel?: LynxEvent<T>['TransitionCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   */
  catchtransitioncancel?: LynxEvent<T>['TransitionCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   */
  'capture-bindtransitioncancel'?: LynxEvent<T>['TransitionCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   */
  'capture-catchtransitioncancel'?: LynxEvent<T>['TransitionCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   */
  'global-bindtransitioncancel'?: LynxEvent<T>['TransitionCancel']; }
interface TransitionEndProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindtransitionend?: LynxEvent<T>['TransitionEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchtransitionend?: LynxEvent<T>['TransitionEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindtransitionend'?: LynxEvent<T>['TransitionEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchtransitionend'?: LynxEvent<T>['TransitionEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindtransitionend'?: LynxEvent<T>['TransitionEnd']; }
interface AnimationStartProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindanimationstart?: LynxEvent<T>['AnimationStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchanimationstart?: LynxEvent<T>['AnimationStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindanimationstart'?: LynxEvent<T>['AnimationStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchanimationstart'?: LynxEvent<T>['AnimationStart'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindanimationstart'?: LynxEvent<T>['AnimationStart']; }
interface AnimationIterationProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindanimationiteration?: LynxEvent<T>['AnimationIteration'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchanimationiteration?: LynxEvent<T>['AnimationIteration'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindanimationiteration'?: LynxEvent<T>['AnimationIteration'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchanimationiteration'?: LynxEvent<T>['AnimationIteration'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindanimationiteration'?: LynxEvent<T>['AnimationIteration']; }
interface AnimationCancelProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindanimationcancel?: LynxEvent<T>['AnimationCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchanimationcancel?: LynxEvent<T>['AnimationCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindanimationcancel'?: LynxEvent<T>['AnimationCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchanimationcancel'?: LynxEvent<T>['AnimationCancel'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindanimationcancel'?: LynxEvent<T>['AnimationCancel']; }
interface AnimationEndProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindanimationend?: LynxEvent<T>['AnimationEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchanimationend?: LynxEvent<T>['AnimationEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindanimationend'?: LynxEvent<T>['AnimationEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchanimationend'?: LynxEvent<T>['AnimationEnd'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindanimationend'?: LynxEvent<T>['AnimationEnd']; }
interface MouseDownProps<T> {
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  bindmousedown?: LynxEvent<T>['MouseDown'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  catchmousedown?: LynxEvent<T>['MouseDown'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-bindmousedown'?: LynxEvent<T>['MouseDown'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-catchmousedown'?: LynxEvent<T>['MouseDown'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'global-bindmousedown'?: LynxEvent<T>['MouseDown']; }
interface MouseUpProps<T> {
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  bindmouseup?: LynxEvent<T>['MouseUp'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  catchmouseup?: LynxEvent<T>['MouseUp'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-bindmouseup'?: LynxEvent<T>['MouseUp'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-catchmouseup'?: LynxEvent<T>['MouseUp'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'global-bindmouseup'?: LynxEvent<T>['MouseUp']; }
interface MouseMoveProps<T> {
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  bindmousemove?: LynxEvent<T>['MouseMove'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  catchmousemove?: LynxEvent<T>['MouseMove'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-bindmousemove'?: LynxEvent<T>['MouseMove'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-catchmousemove'?: LynxEvent<T>['MouseMove'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'global-bindmousemove'?: LynxEvent<T>['MouseMove']; }
interface MouseEnterProps<T> {
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  bindmouseenter?: LynxEvent<T>['MouseEnter'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  catchmouseenter?: LynxEvent<T>['MouseEnter'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-bindmouseenter'?: LynxEvent<T>['MouseEnter'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-catchmouseenter'?: LynxEvent<T>['MouseEnter'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'global-bindmouseenter'?: LynxEvent<T>['MouseEnter']; }
interface MouseLeaveProps<T> {
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  bindmouseleave?: LynxEvent<T>['MouseLeave'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  catchmouseleave?: LynxEvent<T>['MouseLeave'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-bindmouseleave'?: LynxEvent<T>['MouseLeave'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-catchmouseleave'?: LynxEvent<T>['MouseLeave'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'global-bindmouseleave'?: LynxEvent<T>['MouseLeave']; }
interface MouseClickProps<T> {
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  bindmouseclick?: LynxEvent<T>['MouseClick'];
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  catchmouseclick?: LynxEvent<T>['MouseClick'];
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-bindmouseclick'?: LynxEvent<T>['MouseClick'];
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-catchmouseclick'?: LynxEvent<T>['MouseClick'];
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'global-bindmouseclick'?: LynxEvent<T>['MouseClick']; }
interface MouseDblClickProps<T> {
  /**
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  bindmousedblclick?: LynxEvent<T>['MouseDblClick'];
  /**
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  catchmousedblclick?: LynxEvent<T>['MouseDblClick'];
  /**
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  'capture-bindmousedblclick'?: LynxEvent<T>['MouseDblClick'];
  /**
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  'capture-catchmousedblclick'?: LynxEvent<T>['MouseDblClick'];
  /**
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  'global-bindmousedblclick'?: LynxEvent<T>['MouseDblClick']; }
interface MouseLongPressProps<T> {
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  bindmouselongpress?: LynxEvent<T>['MouseLongPress'];
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  catchmouselongpress?: LynxEvent<T>['MouseLongPress'];
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-bindmouselongpress'?: LynxEvent<T>['MouseLongPress'];
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-catchmouselongpress'?: LynxEvent<T>['MouseLongPress'];
  /**
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'global-bindmouselongpress'?: LynxEvent<T>['MouseLongPress']; }
interface WheelProps<T> {
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 4.1
   */
  bindwheel?: LynxEvent<T>['Wheel'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 4.1
   */
  catchwheel?: LynxEvent<T>['Wheel'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 4.1
   */
  'capture-bindwheel'?: LynxEvent<T>['Wheel'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 4.1
   */
  'capture-catchwheel'?: LynxEvent<T>['Wheel'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 4.1
   */
  'global-bindwheel'?: LynxEvent<T>['Wheel']; }
interface ZoomProps<T> {
  /**
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  bindzoom?: LynxEvent<T>['Zoom'];
  /**
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  catchzoom?: LynxEvent<T>['Zoom'];
  /**
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  'capture-bindzoom'?: LynxEvent<T>['Zoom'];
  /**
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  'capture-catchzoom'?: LynxEvent<T>['Zoom'];
  /**
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   */
  'global-bindzoom'?: LynxEvent<T>['Zoom']; }
interface KeyDownProps<T> {
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  bindkeydown?: LynxEvent<T>['KeyDown'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  catchkeydown?: LynxEvent<T>['KeyDown'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-bindkeydown'?: LynxEvent<T>['KeyDown'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-catchkeydown'?: LynxEvent<T>['KeyDown'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'global-bindkeydown'?: LynxEvent<T>['KeyDown']; }
interface KeyUpProps<T> {
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  bindkeyup?: LynxEvent<T>['KeyUp'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  catchkeyup?: LynxEvent<T>['KeyUp'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-bindkeyup'?: LynxEvent<T>['KeyUp'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'capture-catchkeyup'?: LynxEvent<T>['KeyUp'];
  /**
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'global-bindkeyup'?: LynxEvent<T>['KeyUp']; }
interface FocusProps<T> {
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  bindfocus?: LynxEvent<T>['Focus'];
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  catchfocus?: LynxEvent<T>['Focus'];
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-bindfocus'?: LynxEvent<T>['Focus'];
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-catchfocus'?: LynxEvent<T>['Focus'];
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'global-bindfocus'?: LynxEvent<T>['Focus']; }
interface BlurProps<T> {
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  bindblur?: LynxEvent<T>['Blur'];
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  catchblur?: LynxEvent<T>['Blur'];
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-bindblur'?: LynxEvent<T>['Blur'];
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'capture-catchblur'?: LynxEvent<T>['Blur'];
  /**
   * @Android 2.16
   * @iOS 3.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 4.1
   * @ClayMacOS 3.6
   * @ClayWindows 3.6
   * @ClayHarmony 2.17
   */
  'global-bindblur'?: LynxEvent<T>['Blur']; }
interface LayoutChangeProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindlayoutchange?: LynxEvent<T>['LayoutChange'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchlayoutchange?: LynxEvent<T>['LayoutChange'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindlayoutchange'?: LynxEvent<T>['LayoutChange'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchlayoutchange'?: LynxEvent<T>['LayoutChange'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindlayoutchange'?: LynxEvent<T>['LayoutChange']; }
interface UIAppearProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  binduiappear?: LynxEvent<T>['UIAppear'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchuiappear?: LynxEvent<T>['UIAppear'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-binduiappear'?: LynxEvent<T>['UIAppear'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchuiappear'?: LynxEvent<T>['UIAppear'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-binduiappear'?: LynxEvent<T>['UIAppear']; }
interface UIDisappearProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  binduidisappear?: LynxEvent<T>['UIDisappear'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchuidisappear?: LynxEvent<T>['UIDisappear'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-binduidisappear'?: LynxEvent<T>['UIDisappear'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchuidisappear'?: LynxEvent<T>['UIDisappear'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-binduidisappear'?: LynxEvent<T>['UIDisappear']; }
interface AccessibilityActionProps<T> {
  /**
   * @Android 2.14
   * @iOS 2.14
   */
  bindaccessibilityaction?: LynxEvent<T>['AccessibilityAction'];
  /**
   * @Android 2.14
   * @iOS 2.14
   */
  catchaccessibilityaction?: LynxEvent<T>['AccessibilityAction'];
  /**
   * @Android 2.14
   * @iOS 2.14
   */
  'capture-bindaccessibilityaction'?: LynxEvent<T>['AccessibilityAction'];
  /**
   * @Android 2.14
   * @iOS 2.14
   */
  'capture-catchaccessibilityaction'?: LynxEvent<T>['AccessibilityAction'];
  /**
   * @Android 2.14
   * @iOS 2.14
   */
  'global-bindaccessibilityaction'?: LynxEvent<T>['AccessibilityAction']; }
interface TapProps<T> {
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  bindtap?: LynxBindCatchEvent<T>['Tap'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  catchtap?: LynxBindCatchEvent<T>['Tap'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-bindtap'?: LynxBindCatchEvent<T>['Tap'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'capture-catchtap'?: LynxBindCatchEvent<T>['Tap'];
  /**
   * @Android 1.0
   * @iOS 1.0
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   */
  'global-bindtap'?: LynxBindCatchEvent<T>['Tap']; }
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
  MouseEnterProps<T> &
  MouseLeaveProps<T> &
  MouseClickProps<T> &
  MouseDblClickProps<T> &
  MouseLongPressProps<T> &
  WheelProps<T> &
  ZoomProps<T> &
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

export enum MemoryPressureLevel {
  MEMORY_PRESSURE_LEVEL_NONE = 0,
  MEMORY_PRESSURE_LEVEL_MODERATE = 1,
  MEMORY_PRESSURE_LEVEL_CRITICAL = 2,
  kMaxValue = MEMORY_PRESSURE_LEVEL_CRITICAL,
}

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
  __OnLowMemory: {
    data: [MemoryPressureLevel];
    origin: 'NATIVE';
  };

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
