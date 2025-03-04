// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { EventEmitter as IEventEmitter } from '@lynx-js/types';
import { CallLynxSetModule } from '../nativeModules';

export default class EventEmitter implements IEventEmitter {
  // eslint-disable-next-line @typescript-eslint/prefer-readonly
  private _events: Map<
    string,
    { listener: (...args: unknown[]) => void; context?: object }[]
  >;
  private _internal_callLynxSetModule?: CallLynxSetModule;
  constructor(callLynxSetModule?: CallLynxSetModule) {
    this._internal_callLynxSetModule = callLynxSetModule;
    this._events = new Map();
  }

  getEventsSize(eventType: string): number {
    return this._events.get(eventType)?.length;
  }

  setCallLynxSetModule(callLynxSetModule?: CallLynxSetModule) {
    this._internal_callLynxSetModule = callLynxSetModule;
  }

  addListener(
    eventName: string,
    listener: (...args: unknown[]) => void,
    context?: object
  ): void {
    const event = this._events.get(eventName);
    // TODO: removed this api design after spring
    if (eventName == 'keyboardstatuschanged') {
      if (this._internal_callLynxSetModule) {
        this._internal_callLynxSetModule('switchKeyBoardDetect', [true]);
      }
    }
    if (event) {
      event.push({
        listener,
        context,
      });
    } else {
      this._events.set(eventName, [
        {
          listener,
          context,
        },
      ]);
    }
  }

  removeListener(
    eventName: string,
    listener: (...args: unknown[]) => void
  ): void {
    if (typeof listener !== 'function') {
      throw new Error('removeListener only takes instances of Function');
    }
    const events = this._events.get(eventName);
    let index = 0;
    if (Array.isArray(events)) {
      const flag = events.some((item) => {
        if (listener === item.listener) {
          return true;
        }
        index++;
      });
      flag && events.splice(index, 1);
    }

    // TODO: removed this api design after spring
    if (eventName == 'keyboardstatuschanged') {
      if (this._internal_callLynxSetModule) {
        this._internal_callLynxSetModule('switchKeyBoardDetect', [false]);
      }
    }
  }

  emit(eventName: string, data: unknown): void {
    const events = this._events.get(eventName);
    if (Array.isArray(events)) {
      events.forEach((item) => {
        const { listener, context } = item;
        if (typeof listener === 'function') {
          listener.apply(context || this, data);
        }
      });
    }
  }

  removeAllListeners(eventName?: string): void {
    if (typeof eventName === 'string') {
      this._events.delete(eventName);
      return;
    }

    // clear all
    this._events = new Map();
  }

  trigger(eventName: string, params: string | Record<any, any>): void {
    // for api usage;
    const events = this._events.get(eventName);
    if (Array.isArray(events)) {
      if (typeof params === 'string') {
        params = JSON.parse(params);
      }
      events.forEach((item) => {
        const { listener, context } = item;
        if (typeof listener === 'function') {
          listener.call(context || this, params);
        }
      });
    }
  }

  toggle(eventName: string, ...data: unknown[]): void {
    this.emit(eventName, data);
  }
}

export function createEventEmitter() {
  return new EventEmitter();
}
