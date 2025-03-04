// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
/**
 * @description Http Body
 * @since 2.18
 */
export interface Body {
  /**
   * @description body used
   * @see https://developer.mozilla.org/docs/Web/API/Request/bodyUsed
   * @since 2.18
   */
  readonly bodyUsed: boolean;
  /**
   * @description arrayBuffer()
   * @see https://developer.mozilla.org/docs/Web/API/Request/arrayBuffer
   * @since 2.18
   */
  arrayBuffer(): Promise<ArrayBuffer>;
  /**
   * @description json()
   * @see https://developer.mozilla.org/docs/Web/API/Request/json
   * @since 2.18
   */
  json(): Promise<any>;
  /**
   * @description text()
   * @see https://developer.mozilla.org/docs/Web/API/Request/text
   * @since 2.18
   */
  text(): Promise<string>;
}

/**
 * @description This Fetch API interface represents a resource request.
 * @see https://developer.mozilla.org/docs/Web/API/Request
 * @since 2.18
 */
export interface Request extends Body {
  /**
   * @description Returns a Headers object consisting of the headers associated with request. Note that headers added in the network layer by the user agent will not be accounted for in this object, e.g., the "Host" header.
   * @see https://developer.mozilla.org/docs/Web/API/Request/headers
   * @since 2.18
   */
  readonly headers: Headers;
  /**
   * @description Returns request's HTTP method, which is "GET" by default.
   * @see https://developer.mozilla.org/docs/Web/API/Request/method
   * @since 2.18
   */
  readonly method: string;
  /**
   * @description Returns the URL of request as a string.
   * @see https://developer.mozilla.org/docs/Web/API/Request/url
   * @since 2.18
   */
  readonly url: string;
  /**
   * @description clone()
   * @see https://developer.mozilla.org/docs/Web/API/Request/clone
   * @since 2.18
   */
  clone(): Request;
}

/**
 * @description This Fetch API interface represents a resource request.
 * @see https://developer.mozilla.org/docs/Web/API/Request
 * @since 2.18
 */
export declare var Request: {
  prototype: Request;
  new (input: RequestInfo | URL, init?: RequestInit): Request;
};

/**
 * @description This Fetch API interface represents the response to a request.
 * @see https://developer.mozilla.org/docs/Web/API/Response
 * @since 2.18
 */
export interface RequestInit {
  /**
   * @description A BodyInit object or null to set request's body.
   * @since 2.18
   */
  body?: BodyInit | null;
  /**
   * @description A Headers object, an object literal, or an array of two-item arrays to set request's headers.
   * @since 2.18
   */
  headers?: HeadersInit;
  /**
   * @description A string to set request's method.
   * @since 2.18
   */
  method?: string;
}

/**
 * @description This Fetch API interface represents the response to a request.
 * @see https://developer.mozilla.org/docs/Web/API/Response
 * @since 2.18
 */
export interface Response extends Body {
  /**
   * @description headers
   * @see https://developer.mozilla.org/docs/Web/API/Response/headers
   * @since 2.18
   */
  readonly headers: Headers;
  /**
   * @description ok
   * @see https://developer.mozilla.org/docs/Web/API/Response/ok
   * @since 2.18
   */
  readonly ok: boolean;
  /**
   * @description status
   * @see https://developer.mozilla.org/docs/Web/API/Response/status
   * @since 2.18
   */
  readonly status: number;
  /**
   * @description statusText
   * @see https://developer.mozilla.org/docs/Web/API/Response/statusText
   * @since 2.18
   */
  readonly statusText: string;
  /**
   * @description url
   * @see https://developer.mozilla.org/docs/Web/API/Response/url
   * @since 2.18
   */
  readonly url: string;
  /**
   * @description clone()
   * @see https://developer.mozilla.org/docs/Web/API/Response/clone
   * @since 2.18
   */
  clone(): Response;
}

/**
 * @description This Fetch API interface represents the response to a request.
 * @see https://developer.mozilla.org/docs/Web/API/Response
 * @since 2.18
 */
export declare var Response: {
  prototype: Response;
  new (body?: BodyInit | null, init?: ResponseInit): Response;
};
