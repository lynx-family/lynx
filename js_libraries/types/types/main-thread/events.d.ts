// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxEventPropsBase, BaseTouchEvent, BasePointerEvent, BaseMouseEvent, BaseWheelEvent, BaseKeyEvent, BaseAnimationEvent, BaseTransitionEvent, LayoutChangeDetailEvent, UIAppearanceDetailEvent  } from '../common/events';
import { Element } from './element';

// worklet event
export interface TouchEvent extends BaseTouchEvent<Element> {}
export interface PointerEvent extends BasePointerEvent<Element> {}
export interface MouseEvent extends BaseMouseEvent<Element> {}
export interface WheelEvent extends BaseWheelEvent<Element> {}
export interface KeyEvent extends BaseKeyEvent<Element> {}

export interface AnimationEvent extends BaseAnimationEvent<Element> {}
export interface TransitionEvent extends BaseTransitionEvent<Element> {}

export interface LayoutChangeEvent extends LayoutChangeDetailEvent<Element> {}
export interface UIAppearanceEvent extends UIAppearanceDetailEvent<Element> {}

type LynxWorkletEventPropsImpl = {
  [K in keyof LynxEventPropsBase<Element> as Lowercase<`main-thread:${K}`>]: LynxEventPropsBase<Element>[K];
};
export interface LynxWorkletEventProps extends LynxWorkletEventPropsImpl {}

declare module '../common/props' {
  interface StandardProps extends LynxWorkletEventProps {}
}
