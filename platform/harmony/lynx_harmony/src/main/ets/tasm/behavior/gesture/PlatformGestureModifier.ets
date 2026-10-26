// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import deviceInfo from '@ohos.deviceInfo';
import { IContext } from '../../IContext';

interface PlatformGestureContext extends IContext {
  onPlatformTouchTestDone?(recognizers: Array<GestureRecognizer>): void;
  onPlatformGestureRecognizerJudgeBegin?(event: BaseGestureEvent,
    current: GestureRecognizer, others: Array<GestureRecognizer>): GestureJudgeResult;
}

export class PlatformGestureModifier<T extends CommonAttribute> implements AttributeModifier<T> {
  private context: PlatformGestureContext;

  constructor(context: IContext) {
    this.context = context as PlatformGestureContext;
  }

  applyNormalAttribute(instance: T): void {
    if (deviceInfo.sdkApiVersion < 20) return;
    const context = this.context;
    if (context.onPlatformTouchTestDone) {
      instance.onTouchTestDone((_event: BaseGestureEvent,
        recognizers: Array<GestureRecognizer>): void => {
        context.onPlatformTouchTestDone!(recognizers);
      });
    }
    if (context.onPlatformGestureRecognizerJudgeBegin) {
      instance.onGestureRecognizerJudgeBegin((event: BaseGestureEvent,
        current: GestureRecognizer, others: Array<GestureRecognizer>): GestureJudgeResult => {
        return context.onPlatformGestureRecognizerJudgeBegin!(event, current, others);
      });
    }
  }
}
