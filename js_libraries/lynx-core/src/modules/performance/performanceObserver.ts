// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import EventEmitter from '../event';
import {
  PerformanceObserver as IPerformanceObserver,
  PerformanceCallback,
  PerformanceEntry,
} from '@lynx-js/types';

const ListenerKeys = {
  onPerformance: 'lynx.performance.onPerformanceEvent',
};

export class PerformanceObserver implements IPerformanceObserver {
  _emitter: EventEmitter;
  _observedNames: string[];
  _onPerformance: PerformanceCallback;
  constructor(emitter: EventEmitter, callback: PerformanceCallback) {
    this._emitter = emitter;
    this._onPerformance = callback;
    this._observedNames = [];
  }

  observe(names: string[]): void {
    // The previous observe must be closed using the disconnect method before re-observing.
    if (this._observedNames.length > 0) {
      return;
    }

    this._observedNames = names;
    this._emitter.addListener(
      ListenerKeys.onPerformance,
      this.onPerformanceEvent.bind(this)
    );
  }

  disconnect(): void {
    this._observedNames = [];
    this._emitter.removeListener(
      ListenerKeys.onPerformance,
      this.onPerformanceEvent.bind(this)
    );
  }

  onPerformanceEvent(entry: PerformanceEntry): void {
    if (this._observedNames.length === 0) {
      return;
    }

    let entryName = entry.entryType + '.' + entry.name;
    if (
      this._observedNames.includes(entryName) ||
      this._observedNames.includes(entry.entryType)
    ) {
      this._onPerformance(entry);
    }
  }
}
