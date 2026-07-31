/**
 * @license MIT
 * https://github.com/mswjs/headers-polyfill/blob/main/LICENSE
 *
Copyright (c) 2020–present Artem Zakharchenko

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
export class Headers {
  private _headers_map: Map<string, string> = new Map();

  constructor(init?: HeadersInit) {
    if (init === null || typeof init === 'number') {
      throw new TypeError(`Headers init with null/number`);
    }
    if (init instanceof Headers) {
      for (const [key, value] of init) {
        this.append(key, value);
      }
    } else if (Array.isArray(init)) {
      init.forEach(([name, value]) => {
        this.append(name, Array.isArray(value) ? value.join(' ') : value);
      });
    } else if (init) {
      Object.getOwnPropertyNames(init).forEach((name) => {
        const value = init[name];
        this.append(name, Array.isArray(value) ? value.join(' ') : value);
      });
    }
  }

  [Symbol.toStringTag] = 'Headers';

  [Symbol.iterator]() {
    return this.entries();
  }

  *keys(): IterableIterator<string> {
    for (const [key, value] of this._headers_map) {
      yield key;
    }
  }

  *values(): IterableIterator<string> {
    for (const [key, value] of this._headers_map) {
      yield value;
    }
  }

  *entries(): IterableIterator<[string, string]> {
    for (const entry of this._headers_map) {
      yield entry;
    }
  }

  /**
   * Returns a boolean stating whether a `Headers` object contains a certain header.
   */
  has(name: string): boolean {
    return this._headers_map.has(name);
  }

  /**
   * Returns a `ByteString` sequence of all the values of a header with a given name.
   */
  get(name: string): string | null {
    return this._headers_map.get(name) ?? null;
  }

  /**
   * Sets a new value for an existing header inside a `Headers` object, or adds the header if it does not already exist.
   */
  set(name: string, value: string): void {
    this._headers_map.set(name, String(value));
  }

  /**
   * Appends a new value onto an existing header inside a `Headers` object, or adds the header if it does not already exist.
   */
  append(name: string, value: string): void {
    let resolvedValue = this.has(name) ? `${this.get(name)}, ${value}` : value;

    this.set(name, resolvedValue);
  }

  /**
   * Deletes a header from the `Headers` object.
   */
  delete(name: string): void {
    if (!this.has(name)) {
      return;
    }

    this._headers_map.delete(name);
  }

  /**
   * Traverses the `Headers` object,
   * calling the given callback for each header.
   */
  forEach<ThisArg = this>(
    callback: (
      this: ThisArg,
      value: string,
      name: string,
      parent: this
    ) => void,
    thisArg?: ThisArg
  ) {
    for (const [name, value] of this.entries()) {
      callback.call(thisArg, value, name, this);
    }
  }
}
