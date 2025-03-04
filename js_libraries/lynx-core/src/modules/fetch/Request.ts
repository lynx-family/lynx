// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BodyMixin } from './BodyMixin';
import { Headers } from './Headers';
import { AbortController, AbortSignal } from './AbortController';

type RequestLynxExtension = Record<string, any>;

interface RequestInitInner extends RequestInit {
  lynxExtension?: RequestLynxExtension;
}

export function createRequestClass(Promise: PromiseConstructor): any {
  return class Request extends BodyMixin {
    private _url: string;
    private _headers: Headers;
    private _method: string;
    private _signal: AbortSignal;
    private _lynxExtension: RequestLynxExtension;

    get url() {
      return this._url;
    }

    get headers() {
      return this._headers;
    }

    get method() {
      return this._method;
    }

    get signal() {
      return this._signal;
    }

    get lynxExtension() {
      return this._lynxExtension;
    }

    constructor(input: RequestInfo, options?: RequestInitInner) {
      super();
      options = options || {};

      if (input instanceof Request) {
        if (input.bodyUsed) {
          throw new TypeError('Already read');
        }
        this._url = input.url;
        if (!options.headers) {
          this._headers = new Headers(input.headers as globalThis.Headers);
        }
        this._method = input.method;
        this._signal = (input.signal as any) as AbortSignal;
      } else {
        this._url = String(input);
      }

      if (options.headers || !this.headers) {
        this._headers = new Headers(options.headers);
      }
      this._method = options.method || this.method || 'GET';
      this._method = this._method.toUpperCase();

      if ((this.method === 'GET' || this.method === 'HEAD') && options.body) {
        throw new TypeError('Body not allowed for GET or HEAD requests');
      }

      if (typeof options.signal !== 'undefined') {
        this._signal = (options.signal as any) as AbortSignal;
      }
      this._signal = this._signal || AbortSignal.__create();

      this._lynxExtension = options.lynxExtension || {};

      if (!this._headers.get('Content-Type')) {
        if (typeof options.body === 'string') {
          this._headers.set('Content-Type', 'text/plain;charset=UTF-8');
        } else if (
          globalThis.URLSearchParams &&
          options.body instanceof URLSearchParams
        ) {
          this._headers.set(
            'Content-Type',
            'application/x-www-form-urlencoded;charset=UTF-8'
          );
        } else if (options.body instanceof ArrayBuffer) {
        } else {
          this._headers.set('Content-Type', 'text/plain;charset=UTF-8');
        }
      }

      this.setBody(options.body);
    }

    public clone(): Request {
      const cloned = new Request(this as any, {
        method: this.method,
      });

      cloned.setBody(this);
      return cloned;
    }
  };
}
