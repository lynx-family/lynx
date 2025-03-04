// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxSetTimeout } from '../common';
import { AnimationElement } from './animation';
import { EventEmitter } from './event';
import { NodesRef } from './nodes-ref';

/**
 * @description LynxRuntimeCommonMethod is the public interface that app context implements
 */
export interface LynxRuntimeCommonMethod {
  setTimeout: LynxSetTimeout;
  setInterval: LynxSetTimeout;
  clearTimeout(timeoutId: number): void;
  clearInterval(intervalId: number): void;
  getNodeRef(id: string): NodesRef;
  GlobalEventEmitter: EventEmitter;
  getJSModule(name: string): EventEmitter;
  getElementById(id: string): AnimationElement;
  selectComponent(name: string, completeCb: (...args: any[]) => void): void;
}

export interface LynxComponentRuntimeMethod extends LynxRuntimeCommonMethod {
  triggerEvent(evtName: string, ...evtPrams: any[]): void;
  getNodeRefFromRoot(id: string): NodesRef;
}
