// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BodyMixin } from './BodyMixin';
import { Headers } from './Headers';
import { AbortSignal } from './AbortController';

type RequestLynxExtension = Record<string, any>;

interface RequestInitInner extends RequestInit {
  lynxExtension?: RequestLynxExtension;
}

const FORBIDDEN_METHODS = ['CONNECT', 'TRACE', 'TRACK'];

function hasBody(body?: BodyInit | null): boolean {
  return body !== undefined && body !== null;
}

function isForbiddenMethod(method: string): boolean {
  return FORBIDDEN_METHODS.indexOf(method.toUpperCase()) !== -1;
}

export class Request extends BodyMixin {
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
      this.setBody(input._arrayBuffer);
    } else {
      this._url = String(input);
    }

    if (options.headers || !this.headers) {
      this._headers = new Headers(options.headers);
    }
    this._method = options.method || this.method || 'GET';
    this._method = this._method.toUpperCase();

    if (isForbiddenMethod(this.method)) {
      throw new TypeError(`'${this.method}' HTTP method is unsupported.`);
    }

    if (
      (this.method === 'GET' || this.method === 'HEAD') &&
      hasBody(options.body)
    ) {
      throw new TypeError('Body not allowed for GET or HEAD requests');
    }

    if (typeof options.signal !== 'undefined') {
      this._signal = (options.signal as any) as AbortSignal;
    }
    this._signal = this._signal || AbortSignal.__create();

    this._lynxExtension = options.lynxExtension || {};

    this.setBody(options.body);
  }

  public clone(): Request {
    const cloned = new Request(this as any, {
      method: this.method,
    });

    cloned.setBody(this);
    return cloned;
  }
}
