// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType, expectTypeOf } from 'vitest';
import {
  IntrinsicElements,
  BaseTouchEvent,
  BaseMouseEvent,
  BaseWheelEvent,
  BaseKeyEvent,
  BaseAnimationEvent,
  BaseCommonEvent,
  BaseTransitionEvent,
  LayoutChangeDetailEvent,
  UIAppearanceDetailEvent,
  BaseImageLoadEvent,
  BaseImageErrorEvent,
  AccessibilityActionDetailEvent,
  Target,
  BoundingClientRectMethod,
  TakeScreenShotMethod,
  SetFocusMethod,
  RequestAccessibilityFocusMethod,
  IsAnimatingMethod,
  ScrollIntoViewMethod,
} from '../../types';

// Props Types Check
let a;

// Props Types Check
{
  // Basic props
  <view id="test" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['id']);

  <view className="test-class" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['className']);

  <view class="test-class" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['class']);

  <view hidden={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['hidden']);

  <view flatten={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['flatten']);

  <view overlap={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['overlap']);

  <view overlap-ios={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['overlap-ios']);

  // Accessibility props
  <view accessibility-label="test label" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['accessibility-label']);

  <view accessibility-traits="button" />;
  assertType<
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
    | 'none'
    | undefined
  >(a as IntrinsicElements['view']['accessibility-traits']);

  <view accessibility-element={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['accessibility-element']);

  <view accessibility-value="test value" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['accessibility-value']);

  <view accessibility-heading={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['accessibility-heading']);

  <view accessibility-role-description="switch" />;
  assertType<'switch' | 'checkbox' | 'image' | 'progressbar' | string | undefined>(a as IntrinsicElements['view']['accessibility-role-description']);

  <view accessibility-actions={['action1', 'action2']} />;
  assertType<string[] | undefined>(a as IntrinsicElements['view']['accessibility-actions']);

  <view accessibility-elements-hidden={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['accessibility-elements-hidden']);

  <view accessibility-exclusive-focus={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['accessibility-exclusive-focus']);

  <view ios-platform-accessibility-id="test-id" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['ios-platform-accessibility-id']);

  // Pan intercept props
  <view pan-intercept-direction={0} />;
  <view pan-intercept-direction={1} />;
  assertType<0 | 1 | undefined>(a as IntrinsicElements['view']['pan-intercept-direction']);
  // @ts-expect-error: pan-intercept-direction only accepts 0 or 1
  <view pan-intercept-direction={2} />;

  <view pan-intercept-scope={0} />;
  <view pan-intercept-scope={1} />;
  <view pan-intercept-scope={2} />;
  <view pan-intercept-scope={3} />;
  <view pan-intercept-scope={4} />;
  <view pan-intercept-scope={5} />;
  assertType<0 | 1 | 2 | 3 | 4 | 5 | undefined>(a as IntrinsicElements['view']['pan-intercept-scope']);
  // @ts-expect-error: pan-intercept-scope only accepts 0, 1, 2, 3, 4, or 5
  <view pan-intercept-scope={6} />;

  <view ios-pan-intercept-gesture-class="UIPanGestureRecognizer" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['ios-pan-intercept-gesture-class']);
  // @ts-expect-error: ios-pan-intercept-gesture-class only accepts string
  <view ios-pan-intercept-gesture-class={1} />;

  <view ios-pan-intercept-view-class="UIView" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['ios-pan-intercept-view-class']);
  // @ts-expect-error: ios-pan-intercept-view-class only accepts string
  <view ios-pan-intercept-view-class={1} />;

  <view ios-pan-intercept-view-tag={1001} />;
  assertType<number | undefined>(a as IntrinsicElements['view']['ios-pan-intercept-view-tag']);
  // @ts-expect-error: ios-pan-intercept-view-tag only accepts number
  <view ios-pan-intercept-view-tag="1001" />;

  // Focus props
  <view focusable={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['focusable']);

  <view focus-index="0, 0" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['focus-index']);

  <view next-focus-up="up-id" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['next-focus-up']);

  <view next-focus-down="down-id" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['next-focus-down']);

  <view next-focus-left="left-id" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['next-focus-left']);

  <view next-focus-right="right-id" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['next-focus-right']);

  // Timing flag
  <view __lynx_timing_flag="test-flag" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['__lynx_timing_flag']);

  // Style
  <view style="color: red;" />;
  <view style={{ color: 'red' }} />;
  assertType<string | import('../../types/common/csstype').CSSProperties | undefined>(a as IntrinsicElements['view']['style']);

  // iOS background shape layer
  <view ios-background-shape-layer={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['ios-background-shape-layer']);

  // Exposure props
  <view exposure-id="test" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['exposure-id']);

  <view exposure-scene="test" />;
  assertType<string | undefined>(a as IntrinsicElements['view']['exposure-scene']);

  <view exposure-area="50%" />;
  assertType<`${number}%` | undefined>(a as IntrinsicElements['view']['exposure-area']);

  <view enable-exposure-ui-margin={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['enable-exposure-ui-margin']);

  <view enable-exposure-ui-clip={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['enable-exposure-ui-clip']);

  <view exposure-screen-margin-top="10px" />;
  assertType<`${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['view']['exposure-screen-margin-top']);

  <view exposure-screen-margin-right="10px" />;
  assertType<`${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['view']['exposure-screen-margin-right']);

  <view exposure-screen-margin-bottom="10px" />;
  assertType<`${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['view']['exposure-screen-margin-bottom']);

  <view exposure-screen-margin-left="10px" />;
  assertType<`${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['view']['exposure-screen-margin-left']);

  <view exposure-ui-margin-top="10px" />;
  assertType<`${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['view']['exposure-ui-margin-top']);

  <view exposure-ui-margin-right="10px" />;
  assertType<`${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['view']['exposure-ui-margin-right']);

  <view exposure-ui-margin-bottom="10px" />;
  assertType<`${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['view']['exposure-ui-margin-bottom']);

  <view exposure-ui-margin-left="10px" />;
  assertType<`${number}px` | `${number}rpx` | undefined>(a as IntrinsicElements['view']['exposure-ui-margin-left']);

  // Interaction props
  <view user-interaction-enabled={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['user-interaction-enabled']);

  <view native-interaction-enabled={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['native-interaction-enabled']);

  <view block-native-event={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['block-native-event']);

  <view block-native-event-areas={[['0px', '0px', '100%', '300px']]} />;
  assertType<[`${number}px` | `${number}%`, `${number}px` | `${number}%`, `${number}px` | `${number}%`, `${number}px` | `${number}%`][] | undefined>(
    a as IntrinsicElements['view']['block-native-event-areas']
  );

  <view
    consume-slide-event={[
      [-135, -180],
      [135, 180],
      [-45, 45],
    ]}
  />;
  assertType<[number, number][] | undefined>(a as IntrinsicElements['view']['consume-slide-event']);

  <view event-through={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['event-through']);

  <view enable-touch-pseudo-propagation={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['enable-touch-pseudo-propagation']);

  <view hit-slop="10px" />;
  assertType<`${number}px` | { top: `${number}px`; left: `${number}px`; right: `${number}px`; bottom: `${number}px` } | undefined>(a as IntrinsicElements['view']['hit-slop']);
  // @ts-expect-error: type error
  <view hit-slop={{ top: 10, bottom: 10, left: 10, right: 10 }} />;

  <view ignore-focus={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['ignore-focus']);

  <view ios-enable-simultaneous-touch={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['view']['ios-enable-simultaneous-touch']);

  <view event-through-active-regions={[['0px', '0px', '100%', '300px']]} />;
  assertType<[`${number}%` | `${number}px`, `${number}%` | `${number}px`, `${number}%` | `${number}px`, `${number}%` | `${number}px`][] | undefined>(
    a as IntrinsicElements['view']['event-through-active-regions']
  );
  // @ts-expect-error: event-through-active-regions only accept string array
  <view event-through-active-regions={[[0, 0, 100, 300]]} />;
  // @ts-expect-error: event-through-active-regions only accept string array of 4 elements.
  <view event-through-active-regions={[['0px', '0px', '100%']]} />;
}

// Events types check
function noop() {}
{
  // Touch events
  <view
    bindtouchstart={(e: BaseTouchEvent<Target>) => {
      assertType<Array<import('../../types/common/events').Touch>>(e.touches);
      assertType<Array<import('../../types/common/events').Touch>>(e.changedTouches);
      assertType<number>(e.detail.x);
      assertType<number>(e.detail.y);
    }}
  />;

  <view
    bindtouchmove={(e: BaseTouchEvent<Target>) => {
      assertType<Array<import('../../types/common/events').Touch>>(e.touches);
      assertType<Array<import('../../types/common/events').Touch>>(e.changedTouches);
      assertType<number>(e.detail.x);
      assertType<number>(e.detail.y);
    }}
  />;

  <view
    bindtouchend={(e: BaseTouchEvent<Target>) => {
      assertType<Array<import('../../types/common/events').Touch>>(e.touches);
      assertType<Array<import('../../types/common/events').Touch>>(e.changedTouches);
      assertType<number>(e.detail.x);
      assertType<number>(e.detail.y);
    }}
  />;

  <view
    bindtouchcancel={(e: BaseTouchEvent<Target>) => {
      assertType<Array<import('../../types/common/events').Touch>>(e.touches);
      assertType<Array<import('../../types/common/events').Touch>>(e.changedTouches);
      assertType<number>(e.detail.x);
      assertType<number>(e.detail.y);
    }}
  />;

  <view
    bindlongpress={(e: BaseTouchEvent<Target>) => {
      assertType<Array<import('../../types/common/events').Touch>>(e.touches);
      assertType<Array<import('../../types/common/events').Touch>>(e.changedTouches);
      assertType<number>(e.detail.x);
      assertType<number>(e.detail.y);
    }}
  />;

  // Tap events
  <view
    bindtap={(e: BaseTouchEvent<Target>) => {
      assertType<string>(e.type);
      assertType<number>(e.timestamp);
      assertType<Array<import('../../types/common/events').Touch>>(e.touches);
      assertType<Array<import('../../types/common/events').Touch>>(e.changedTouches);
      assertType<number>(e.detail.x);
      assertType<number>(e.detail.y);
    }}
  />;

  <view
    bindlongtap={(e: BaseTouchEvent<Target>) => {
      assertType<string>(e.type);
      assertType<number>(e.timestamp);
      assertType<Array<import('../../types/common/events').Touch>>(e.touches);
      assertType<Array<import('../../types/common/events').Touch>>(e.changedTouches);
      assertType<number>(e.detail.x);
      assertType<number>(e.detail.y);
    }}
  />;

  // Mouse events (PC)
  <view
    bindmousedown={(e: BaseMouseEvent<Target>) => {
      assertType<number>(e.button);
      assertType<number>(e.buttons);
      assertType<number>(e.x);
      assertType<number>(e.y);
      assertType<number>(e.pageX);
      assertType<number>(e.pageY);
      assertType<number>(e.clientX);
      assertType<number>(e.clientY);
    }}
  />;

  <view
    bindmouseup={(e: BaseMouseEvent<Target>) => {
      assertType<number>(e.button);
      assertType<number>(e.buttons);
      assertType<number>(e.x);
      assertType<number>(e.y);
      assertType<number>(e.pageX);
      assertType<number>(e.pageY);
      assertType<number>(e.clientX);
      assertType<number>(e.clientY);
    }}
  />;

  <view
    bindmousemove={(e: BaseMouseEvent<Target>) => {
      assertType<number>(e.button);
      assertType<number>(e.buttons);
      assertType<number>(e.x);
      assertType<number>(e.y);
      assertType<number>(e.pageX);
      assertType<number>(e.pageY);
      assertType<number>(e.clientX);
      assertType<number>(e.clientY);
    }}
  />;

  <view
    bindmouseclick={(e: BaseMouseEvent<Target>) => {
      assertType<number>(e.button);
      assertType<number>(e.buttons);
      assertType<number>(e.x);
      assertType<number>(e.y);
      assertType<number>(e.pageX);
      assertType<number>(e.pageY);
      assertType<number>(e.clientX);
      assertType<number>(e.clientY);
    }}
  />;

  <view
    bindmousedblclick={(e: BaseMouseEvent<Target>) => {
      assertType<number>(e.button);
      assertType<number>(e.buttons);
      assertType<number>(e.x);
      assertType<number>(e.y);
      assertType<number>(e.pageX);
      assertType<number>(e.pageY);
      assertType<number>(e.clientX);
      assertType<number>(e.clientY);
    }}
  />;

  <view
    bindmouselongpress={(e: BaseMouseEvent<Target>) => {
      assertType<number>(e.button);
      assertType<number>(e.buttons);
      assertType<number>(e.x);
      assertType<number>(e.y);
      assertType<number>(e.pageX);
      assertType<number>(e.pageY);
      assertType<number>(e.clientX);
      assertType<number>(e.clientY);
    }}
  />;

  // Wheel event (PC)
  <view
    bindwheel={(e: BaseWheelEvent<Target>) => {
      assertType<number>(e.x);
      assertType<number>(e.y);
      assertType<number>(e.pageX);
      assertType<number>(e.pageY);
      assertType<number>(e.clientX);
      assertType<number>(e.clientY);
      assertType<number>(e.deltaX);
      assertType<number>(e.deltaY);
    }}
  />;

  // Key events (PC)
  <view
    bindkeydown={(e: BaseKeyEvent<Target>) => {
      assertType<string>(e.key);
    }}
  />;

  <view
    bindkeyup={(e: BaseKeyEvent<Target>) => {
      assertType<string>(e.key);
    }}
  />;

  // Focus events (PC)
  <view
    bindfocus={(e: BaseCommonEvent<Target>) => {
      assertType<string>(e.type);
      assertType<number>(e.timestamp);
    }}
  />;

  <view
    bindblur={(e: BaseCommonEvent<Target>) => {
      assertType<string>(e.type);
      assertType<number>(e.timestamp);
    }}
  />;

  // Animation events
  <view
    bindanimationstart={(e: BaseAnimationEvent<Target>) => {
      assertType<{
        animation_type: 'keyframe-animation';
        animation_name: string;
        new_animator?: true;
      }>(e.params);
      assertType<'keyframe-animation'>(e.params.animation_type);
      assertType<string>(e.params.animation_name);
      assertType<true | undefined>(e.params.new_animator);
    }}
  />;

  <view
    bindanimationend={(e: BaseAnimationEvent<Target>) => {
      assertType<{
        animation_type: 'keyframe-animation';
        animation_name: string;
        new_animator?: true;
      }>(e.params);
      assertType<'keyframe-animation'>(e.params.animation_type);
      assertType<string>(e.params.animation_name);
      assertType<true | undefined>(e.params.new_animator);
    }}
  />;

  <view
    bindanimationiteration={(e: BaseAnimationEvent<Target>) => {
      assertType<{
        animation_type: 'keyframe-animation';
        animation_name: string;
        new_animator?: true;
      }>(e.params);
      assertType<'keyframe-animation'>(e.params.animation_type);
      assertType<string>(e.params.animation_name);
      assertType<true | undefined>(e.params.new_animator);
    }}
  />;

  <view
    bindanimationcancel={(e: BaseAnimationEvent<Target>) => {
      assertType<{
        animation_type: 'keyframe-animation';
        animation_name: string;
        new_animator?: true;
      }>(e.params);
      assertType<'keyframe-animation'>(e.params.animation_type);
      assertType<string>(e.params.animation_name);
      assertType<true | undefined>(e.params.new_animator);
    }}
  />;

  // Transition events
  <view
    bindtransitionstart={(e: BaseTransitionEvent<Target>) => {
      assertType<
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
          }
      >(e.params);
    }}
  />;

  <view
    bindtransitionend={(e: BaseTransitionEvent<Target>) => {
      assertType<
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
          }
      >(e.params);
    }}
  />;

  <view
    bindtransitioncancel={(e: BaseTransitionEvent<Target>) => {
      assertType<
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
          }
      >(e.params);
    }}
  />;

  // Layout and UI events
  <view
    bindlayoutchange={(e: LayoutChangeDetailEvent<Target>) => {
      assertType<'layoutchange'>(e.type);
      assertType<{
        width: number;
        height: number;
        left: number;
        top: number;
        right: number;
        bottom: number;
      }>(e.params);
      assertType<{
        id: string;
        width: number;
        height: number;
        top: number;
        right: number;
        bottom: number;
        left: number;
        dataset: { [key: string]: any };
      }>(e.detail);
      assertType<string>(e.detail.id);
      assertType<number>(e.detail.width);
      assertType<number>(e.detail.height);
      assertType<number>(e.detail.top);
      assertType<number>(e.detail.left);
      assertType<{ [key: string]: any }>(e.detail.dataset);
    }}
  />;

  <view
    binduiappear={(e: UIAppearanceDetailEvent<Target>) => {
      assertType<'uiappear' | 'uidisappear'>(e.type);
      assertType<{
        'exposure-id': string;
        'exposure-scene': string;
        'unique-id': string;
        dataset: { [key: string]: any };
      }>(e.detail);
      assertType<string>(e.detail['exposure-id']);
      assertType<string>(e.detail['exposure-scene']);
      assertType<string>(e.detail['unique-id']);
      assertType<{ [key: string]: any }>(e.detail.dataset);
    }}
  />;

  <view
    binduidisappear={(e: UIAppearanceDetailEvent<Target>) => {
      assertType<'uiappear' | 'uidisappear'>(e.type);
      assertType<{
        'exposure-id': string;
        'exposure-scene': string;
        'unique-id': string;
        dataset: { [key: string]: any };
      }>(e.detail);
      assertType<string>(e.detail['exposure-id']);
      assertType<string>(e.detail['exposure-scene']);
      assertType<string>(e.detail['unique-id']);
      assertType<{ [key: string]: any }>(e.detail.dataset);
    }}
  />;

  // Background events
  <view
    bindbgload={(e: BaseImageLoadEvent<Target>) => {
      assertType<number>(e.width);
      assertType<number>(e.height);
    }}
  />;

  <view
    bindbgerror={(e: BaseImageErrorEvent<Target>) => {
      assertType<string>(e.type);
      assertType<number>(e.timestamp);
    }}
  />;

  // Accessibility events
  <view
    bindaccessibilityaction={(e: AccessibilityActionDetailEvent<Target>) => {
      assertType<{
        name: string;
      }>(e.detail);
      assertType<string>(e.detail.name);
    }}
  />;
}

// Methods types check
function invoke<T extends BoundingClientRectMethod | TakeScreenShotMethod | SetFocusMethod | RequestAccessibilityFocusMethod | IsAnimatingMethod | ScrollIntoViewMethod>(
  _param: T
) {}

{
  // BoundingClientRect method
  invoke({
    method: 'boundingClientRect',
    params: {
      relativeTo: 'screen',
      androidEnableTransformProps: true,
    },
    success: (result) => {
      assertType<string>(result.id);
      assertType<object>(result.dataset);
      assertType<number>(result.left);
      assertType<number>(result.right);
      assertType<number>(result.top);
      assertType<number>(result.bottom);
      assertType<number>(result.width);
      assertType<number>(result.height);
    },
  });

  // TakeScreenshot method
  invoke({
    method: 'takeScreenshot',
    params: {
      format: 'jpeg',
      scale: 0.8,
    },
    success: (result) => {
      assertType<string>(result.data);
    },
  });

  // SetFocus method
  invoke({
    method: 'setFocus',
    params: {
      focus: true,
      scroll: false,
    },
  });

  // RequestAccessibilityFocus method
  invoke({
    method: 'requestAccessibilityFocus',
  });

  // IsAnimating method
  invoke({
    method: 'isAnimating',
    success: (result) => {
      assertType<boolean>(result.data);
    },
  });

  // ScrollIntoView method
  invoke({
    method: 'scrollIntoView',
    params: {
      scrollIntoViewOptions: {
        block: 'center',
        inline: 'start',
        behavior: 'smooth',
      },
    },
  });

  // Test method types
  let methodType: unknown;
  expectTypeOf<'boundingClientRect' | 'takeScreenshot' | 'setFocus' | 'requestAccessibilityFocus' | 'isAnimating' | 'scrollIntoView'>(
    methodType as
      | BoundingClientRectMethod['method']
      | TakeScreenShotMethod['method']
      | SetFocusMethod['method']
      | RequestAccessibilityFocusMethod['method']
      | IsAnimatingMethod['method']
      | ScrollIntoViewMethod['method']
  );
}
