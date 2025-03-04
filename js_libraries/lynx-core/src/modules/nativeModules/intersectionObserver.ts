// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IntersectionObserver as IIntersectionObserver } from '@lynx-js/types';

export interface IntersectionObserverModule {
  createIntersectionObserver: Function;
  relativeTo: Function;
  relativeToViewport: Function;
  relativeToScreen: Function;
  observe: Function;
  disconnect: Function;
}

class IntersectionObservationTarget {
  private readonly _selector: string;
  private readonly _callback: Function;

  constructor(selector: string, callback: Function) {
    this._selector = selector;
    this._callback = callback;
  }

  invokeCallback(data: object): void {
    this._callback(data);
  }
}

export class IntersectionObserver implements IIntersectionObserver {
  private readonly _id: number;
  private readonly _intersectionObserverModule: IntersectionObserverModule;
  private readonly _manager: IntersectionObserverManager;
  private readonly _observationTargets: IntersectionObservationTarget[];
  private readonly _defaultMargins: object;

  constructor(
    id: number,
    intersectionObserverModule: IntersectionObserverModule,
    manager: IntersectionObserverManager
  ) {
    this._id = id;
    this._intersectionObserverModule = intersectionObserverModule;
    this._manager = manager;
    this._observationTargets = [];
    this._defaultMargins = {
      left: 0,
      right: 0,
      top: 0,
      bottom: 0,
    };
  }

  relativeTo(selector: string, margins?: {}): IntersectionObserver {
    this._intersectionObserverModule.relativeTo(
      this._id,
      selector,
      margins || this._defaultMargins
    );
    return this;
  }

  relativeToViewport(margins?: {}): IntersectionObserver {
    this._intersectionObserverModule.relativeToViewport(
      this._id,
      margins || this._defaultMargins
    );
    return this;
  }

  relativeToScreen(margins?: {}): IntersectionObserver {
    this._intersectionObserverModule.relativeToScreen(
      this._id,
      margins || this._defaultMargins
    );
    return this;
  }

  observe(selector: string, callback: Function): void {
    this._observationTargets.push(
      new IntersectionObservationTarget(selector, callback)
    );
    this._intersectionObserverModule.observe(
      this._id,
      selector,
      this._observationTargets.length - 1
    );
  }

  disconnect(): void {
    this._intersectionObserverModule.disconnect(this._id);
    this._manager.removeObserver(this._id);
  }

  invokeCallback(callbackId: number, data: object): void {
    if (callbackId < this._observationTargets.length) {
      this._observationTargets[callbackId].invokeCallback(data);
    }
  }
}

export class IntersectionObserverManager {
  private readonly _nativeModules: object;
  private _observerId: number;
  private _observers: object;
  private readonly _defaultOptions: object;

  constructor(nativeModules: object) {
    this._nativeModules = nativeModules;
    this._observerId = 0;
    this._observers = {};
    this._defaultOptions = {
      thresholds: [0],
      initialRatio: 0,
      observeAll: false,
    };
  }

  createIntersectionObserver(
    componentId: string,
    options?: object
  ): IntersectionObserver {
    let intersectionObserverModule = this._nativeModules[
      'IntersectionObserverModule'
    ];
    const observer = new IntersectionObserver(
      this._observerId,
      intersectionObserverModule,
      this
    );
    this._observers[this._observerId] = observer;
    intersectionObserverModule.createIntersectionObserver(
      this._observerId,
      componentId,
      options || this._defaultOptions
    );
    this._observerId++;
    return observer;
  }

  getObserver(observerId: number): IntersectionObserver {
    return this._observers[observerId];
  }

  removeObserver(observerId: number): void {
    this._observers[observerId] = null;
  }
}
