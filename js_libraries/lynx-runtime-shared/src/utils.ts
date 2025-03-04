// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export function hasProperty(object, property): boolean {
  // return Object.prototype.hasOwnProperty.call(Object.getPrototypeOf(object), property)
  return Object.prototype.hasOwnProperty.call(object || {}, property);
}

export function getDataType(data: any): string {
  const type = typeof data;
  if (type !== 'object') return type;
  if (Array.isArray(data)) return 'array';
  if (data == null) return 'null';
  if (data instanceof Date) return 'date';
  if (data instanceof RegExp) return 'regExp';
  return 'object';
}

export function isString(val: unknown): val is string {
  return typeof val === 'string';
}

export function isObject(val: unknown): boolean {
  return getDataType(val) === 'object';
}

export function isFunction(obj: unknown): obj is AnyFunction {
  const dataType = getDataType(obj);
  return dataType === 'function';
}

export function isArray(array: unknown): array is Array<unknown> {
  return getDataType(array) === 'array';
}

export function isNull(o: unknown): o is null {
  return o === null;
}

export function isUndefined(o: unknown): o is undefined {
  return o === void 0;
}

export function isNullOrUndef(o: unknown): o is null | undefined {
  return isUndefined(o) || isNull(o);
}

export function isError(o: unknown): o is Error {
  switch (Object.prototype.toString.call(o)) {
    case '[object Error]':
      return true;
    case '[object Exception]':
      return true;
    case '[object DOMException]':
      return true;
    default:
      return isInstanceOf(o, Error);
  }
}

export function isInstanceOf<T extends Function>(o: unknown, base: T): o is T {
  try {
    return o instanceof base;
  } catch (_e) {
    return false;
  }
}

export class ThirdScriptError extends Error {
  type: string;
  constructor(msg: any) {
    super(`${msg}`);
    this.type = 'ThirdScriptError';
  }
}

export class AppServiceSdkKnownError extends Error {
  type: string;
  constructor(msg) {
    super(`APP-SERVICE-SDK: + ${msg}`);
    this.type = 'AppServiceSdkKnownError';
  }
}

export function guid(): string {
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (char) => {
    const rand = (16 * Math.random()) | 0;
    return (char === 'x' ? rand : (3 & rand) | 8).toString(16);
  });
}

export function noop(): void {}

export function forEachRight<T>(arr: Array<T>, cb: (value: T) => void): void {
  if (Array.isArray(arr)) {
    let len = arr.length;
    for (let index = len - 1; index >= 0; index--) {
      cb(arr[index]);
    }
  } else {
    throw new Error('forEachRight ERROR: first params must be array.');
  }
}

export function callbackMerge(_cbs): void {
  if (Array.isArray(_cbs)) {
    for (let i = 0, len = _cbs.length; i < len; ++i) {
      _cbs[i]();
    }
  }
}
