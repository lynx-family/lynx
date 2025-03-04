// MIT License

// Copyright (c) 2017 molsson

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

import EventEmitter from '../event';

interface AbortEvent {
  type: 'abort';
  reason?: any;
}

export class AbortSignal extends EventEmitter {
  private _aborted: boolean;
  private _reason: any;

  public onabort: (...args: unknown[]) => void;

  get aborted() {
    return this._aborted;
  }

  get reason() {
    return this._reason;
  }

  private constructor() {
    super();
    this._aborted = false;
  }

  get [Symbol.toStringTag]() {
    return '[object AbortSignal]';
  }

  dispatchEvent(event: AbortEvent) {
    if (event.type === 'abort') {
      this._aborted = true;
      this._reason = event.reason;
      if (typeof this.onabort === 'function') {
        this.onabort.call(this, event);
      }
    }

    super.emit(event.type, event);
  }

  addEventListener(type: string, listener: (...args: unknown[]) => void) {
    super.addListener(type, listener);
  }

  static __create() {
    return new AbortSignal();
  }
}

export class AbortController {
  private _signal: AbortSignal;
  get signal() {
    return this._signal;
  }

  constructor() {
    this._signal = AbortSignal.__create();
  }

  abort(reason?: any) {
    let signalReason = reason;
    if (signalReason === undefined) {
      signalReason = new Error('This operation was aborted');
      signalReason.name = 'AbortError';
    }

    const event: AbortEvent = {
      type: 'abort',
      reason: signalReason,
    };

    this.signal.dispatchEvent(event);
  }

  get [Symbol.toStringTag]() {
    return '[object AbortController]';
  }
}
