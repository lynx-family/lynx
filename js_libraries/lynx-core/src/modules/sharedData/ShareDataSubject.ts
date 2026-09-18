// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * The Subject interface declares a set of methods for managing subscribers.
 */
interface Subject {
  registerObserver(observer: Function, owner?: string): void;
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
   * Owner (native app id) of each entry in {@link observersFunc}, kept in
   * lockstep by index.
   *
   * The subject is group-wide and outlives every page. Under the new "shared
   * Isolate/VM + per-page isolated Context" scheme an observer is a function
   * object of the registering page's realm, so a page that goes away without
   * unregistering would keep its whole context alive. Recording the owner lets
   * {@link removeObserversOfOwner} drop them on destroy. Entries registered
   * without an owner are never auto-removed.
   */
  private observerOwners: (string | undefined)[] = [];

  /**
   * The subscription management methods.
   */
  public registerObserver(observer: Function, owner?: string): void {
    const isExist = this.observersFunc.includes(observer);
    if (isExist) {
      return nativeConsole.log('Subject: Observer has been attached already.');
    }
    this.observersFunc.push(observer);
    this.observerOwners.push(owner);
  }

  public removeObserver(observer: Function): void {
    // nativeConsole.log('Subject: Nonexistent observer.');
    const observerIndex = this.observersFunc.indexOf(observer);
    if (observerIndex === -1) {
      return nativeConsole.log('Subject: Nonexistent observer.');
    }

    this.observersFunc.splice(observerIndex, 1);
    this.observerOwners.splice(observerIndex, 1);
    //   nativeConsole.log('Subject: Detached an observer.');
  }

  /**
   * Drop every observer registered by the given owner. Called when an app is
   * destroyed so the group-wide subject stops referencing that page's realm.
   */
  public removeObserversOfOwner(owner: string): void {
    for (let i = this.observerOwners.length - 1; i >= 0; i--) {
      if (this.observerOwners[i] === owner) {
        this.observersFunc.splice(i, 1);
        this.observerOwners.splice(i, 1);
      }
    }
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
