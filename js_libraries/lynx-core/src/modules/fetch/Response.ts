// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BodyMixin } from './BodyMixin';
import { Headers } from './Headers';

type ResponseLynxExtension = Record<string, any>;
type ResponseTypeInner = 'default' | 'error';

interface ResponseInitInner extends ResponseInit {
  url?: string;
  lynxExtension?: ResponseLynxExtension;
}

const NULL_BODY_STATUS = [204, 205, 304];

function hasBody(body?: BodyInit | null): boolean {
  return body !== undefined && body !== null;
}

function isNullBodyStatus(status: number): boolean {
  return NULL_BODY_STATUS.indexOf(status) !== -1;
}

function isInvalidStatusText(statusText: string): boolean {
  for (let index = 0; index < statusText.length; index++) {
    const code = statusText.charCodeAt(index);
    if (code === 0x0a || code === 0x0d || code > 0xff) {
      return true;
    }
  }
  return false;
}

function asHeadersInit(headers: Headers): HeadersInit {
  return (headers as any) as globalThis.Headers;
}

function serializeJson(data: any): string {
  const body = JSON.stringify(data);
  if (body === undefined) {
    throw new TypeError('The provided value cannot be converted to JSON.');
  }
  return body;
}

export class Response extends BodyMixin {
  private _url: string;
  private _status: number;
  private _statusText: string;
  private _ok: boolean;
  private _headers: Headers;
  private _lynxExtension: ResponseLynxExtension;
  private _type: ResponseTypeInner;

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

  get type() {
    return this._type;
  }

  get headers() {
    return this._headers;
  }

  get lynxExtension() {
    return this._lynxExtension;
  }

  static json(data: any, init?: ResponseInitInner): Response {
    const body = serializeJson(data);
    return new Response(body, init);
  }

  static error(): Response {
    return new Response(null, undefined, undefined, 'error');
  }

  constructor(
    bodyInit?: BodyInit,
    options?: ResponseInitInner,
    enableFetchAPIStandardStreaming?: boolean,
    responseType: ResponseTypeInner = 'default'
  ) {
    super();
    options = options || {};
    this._type = responseType;

    if (this._type === 'error') {
      this._status = 0;
      this._ok = false;
      this._statusText = '';
      this._headers = new Headers();
      this._url = '';
      this._lynxExtension = {};
      return;
    }

    this._status = options.status === undefined ? 200 : options.status;
    if (this._status < 200 || this._status > 599) {
      throw new RangeError(
        `Failed to construct 'Response': The status provided (${this._status}) is outside the range [200, 599].`
      );
    }
    this._ok = this._status >= 200 && this._status < 300;
    this._statusText =
      options.statusText === undefined ? '' : '' + options.statusText;
    if (isInvalidStatusText(this._statusText)) {
      throw new TypeError(
        "Failed to construct 'Response': The statusText provided is invalid."
      );
    }
    if (isNullBodyStatus(this._status) && hasBody(bodyInit)) {
      throw new TypeError(
        "Failed to construct 'Response': Response with null body status cannot have body."
      );
    }
    this._headers = new Headers(options.headers);
    this._url = options.url || '';
    this._lynxExtension = options.lynxExtension || {};
    this.setBody(bodyInit, enableFetchAPIStandardStreaming);
  }

  public clone(): Response {
    if (this._type === 'error') {
      return Response.error();
    }

    const cloned = new Response(null, {
      status: this._status,
      statusText: this._statusText,
      headers: asHeadersInit(new Headers(asHeadersInit(this._headers))),
      url: this._url,
    });

    cloned.setBody(this);

    return cloned;
  }
}
