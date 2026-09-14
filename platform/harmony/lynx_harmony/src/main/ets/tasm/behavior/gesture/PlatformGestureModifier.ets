// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IContext } from '../../IContext';

interface PlatformGestureContext extends IContext {
  onPlatformTouchTestDone?(recognizers: Array<GestureRecognizer>): void;
  onPlatformGestureRecognizerJudgeBegin?(event: BaseGestureEvent,
    current: GestureRecognizer, others: Array<GestureRecognizer>): GestureJudgeResult;
  onPlatformScrollBoundary?(reached: boolean): void;
}

export class PlatformGestureModifier<T extends CommonAttribute> implements AttributeModifier<T> {
  private context: PlatformGestureContext;
  private observeTouchTest: boolean;

  constructor(context: IContext, observeTouchTest: boolean = false) {
    this.context = context as PlatformGestureContext;
    this.observeTouchTest = observeTouchTest;
  }

  applyNormalAttribute(instance: T): void {
    const context = this.context;
    if (this.observeTouchTest && context.onPlatformTouchTestDone) {
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

  protected hasPlatformScrollBoundaryCallback(): boolean {
    return this.context.onPlatformScrollBoundary !== undefined;
  }

  protected reportPlatformScrollBoundary(reached: boolean): void {
    this.context.onPlatformScrollBoundary?.(reached);
  }
}
