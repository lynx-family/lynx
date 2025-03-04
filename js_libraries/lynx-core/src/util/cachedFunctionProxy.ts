// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Cache access to functions of the target object.
//
// When a function on target obj is accessed for the first time,
// the proxy obtains the function object and saves it,
// and returns the cached function object directly during subsequent access
// without accessing again.
export class CachedFunctionProxy<T> {
  private _cachedFunctions: Record<string, Function> = {};

  static create<T>(obj: T): T {
    return new CachedFunctionProxy(obj) as any;
  }

  constructor(obj: T) {
    for (const key in obj) {
      Object.defineProperty(this, key, {
        get() {
          if (this._cachedFunctions[key]) {
            return this._cachedFunctions[key];
          }
          const value = obj[key];
          if (typeof value === 'function') {
            this._cachedFunctions[key] = value;
          }
          return value;
        },
      });
    }
  }
}
