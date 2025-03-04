// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BeforePublishEvent as IBeforePublishEvent } from '@lynx-js/types';
import EventEmitter from './eventEmitter';

export class AopManager {
  public _beforePublishEvent: BeforePublishEvent;

  constructor() {
    this._beforePublishEvent = new BeforePublishEvent();
  }
}

export class BeforePublishEvent
  extends EventEmitter
  implements IBeforePublishEvent {
  add(
    eventName: string,
    callback: (...args: unknown[]) => void,
    context?: object
  ): BeforePublishEvent {
    super.addListener(eventName, callback, context);
    return this;
  }

  remove(
    eventName: string,
    callback: (...args: unknown[]) => void
  ): BeforePublishEvent {
    super.removeListener(eventName, callback);
    return this;
  }
}
