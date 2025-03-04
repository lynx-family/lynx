// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BodyMixin } from './BodyMixin';

type ResponseLynxExtension = Record<string, any>;

interface ResponseInitInner extends ResponseInit {
  url?: string;
  lynxExtension?: ResponseLynxExtension;
}

export function createResponseClass(Promise: PromiseConstructor): any {
  return class Response extends BodyMixin {
    private _url: string;
    private _status: number;
    private _statusText: string;
    private _ok: boolean;
    private _headers: Headers;
    private _lynxExtension: ResponseLynxExtension;

    get url() {
      return this._url;
    }

    get status() {
      return this._status;
    }

    get statusText() {
      return this._statusText;
    }

    get ok() {
      return this._ok;
    }

    get headers() {
      return this._headers;
    }

    get lynxExtension() {
      return this._lynxExtension;
    }

    constructor(bodyInit?: BodyInit, options?: ResponseInitInner) {
      super();
      options = options || {};

      this._status = options.status === undefined ? 200 : options.status;
      if (this._status < 200 || this._status > 599) {
        throw new RangeError(
          "Failed to construct 'Response': The status provided (0) is outside the range [200, 599]."
        );
      }
      this._ok = this._status >= 200 && this._status < 300;
      this._statusText =
        options.statusText === undefined ? '' : '' + options.statusText;
      this._headers = new Headers(options.headers);
      this._url = options.url || '';
      this._lynxExtension = options.lynxExtension || {};
      this.setBody(bodyInit);
    }

    public clone(): Response {
      const cloned = new Response(null, {
        status: this._status,
        statusText: this._statusText,
        headers: new Headers(this._headers),
        url: this._url,
      });

      cloned.setBody(this);

      return cloned;
    }
  };
}
