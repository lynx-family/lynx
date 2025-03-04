// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import nativeConsole from '../../common/ttConsole';
/**
 * The Subject interface declares a set of methods for managing subscribers.
 */
interface Subject {
  registerObserver(observer: Function): void;
  removeObserver(observer: Function): void;
  notifyDataChange(value: any): void;
}

/**
 * The Subject owns some important state and notifies observers when the state
 * changes.
 */
class ShareDataSubject implements Subject {
  /**
   * @type {number} For the sake of simplicity, the Subject's state, essential
   * to all subscribers, is stored in this variable.
   */
  public state: number;

  /**
   * @type {Observer[]} List of subscribers.
   *
   */
  private observersFunc: Function[] = [];

  /**
   * The subscription management methods.
   */
  public registerObserver(observer: Function): void {
    const isExist = this.observersFunc.includes(observer);
    if (isExist) {
      return nativeConsole.log('Subject: Observer has been attached already.');
    }
    this.observersFunc.push(observer);
  }

  public removeObserver(observer: Function): void {
    // nativeConsole.log('Subject: Nonexistent observer.');
    const observerIndex = this.observersFunc.indexOf(observer);
    if (observerIndex === -1) {
      return nativeConsole.log('Subject: Nonexistent observer.');
    }

    this.observersFunc.splice(observerIndex, 1);
    //   nativeConsole.log('Subject: Detached an observer.');
  }

  public notifyDataChange(value: any): void {
    this.observersFunc.forEach((toObserver) => {
      if (typeof toObserver === 'function') {
        try {
          toObserver(value);
        } catch (error) {
          nativeConsole.log(
            'SharedData change and notifyDataChange error info:' + error
          );
        }
      }
    });
  }
}

export { ShareDataSubject };
