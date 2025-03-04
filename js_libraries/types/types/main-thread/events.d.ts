// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxEventPropsBase, BaseTouchEvent, BaseMouseEvent, BaseWheelEvent, BaseKeyEvent } from '../common/events';
import { Element } from './element';

// worklet event
export interface TouchEvent extends BaseTouchEvent<Element> {}
export interface MouseEvent extends BaseMouseEvent<Element> {}
export interface WheelEvent extends BaseWheelEvent<Element> {}
export interface KeyEvent extends BaseKeyEvent<Element> {}
type LynxWorkletEventPropsImpl = {
  [K in keyof LynxEventPropsBase<Element> as Lowercase<`main-thread:${K}`>]: LynxEventPropsBase<Element>[K];
};
export interface LynxWorkletEventProps extends LynxWorkletEventPropsImpl {}

declare module '../common/props.d.ts' {
  interface StandardProps extends LynxWorkletEventProps {}
}
