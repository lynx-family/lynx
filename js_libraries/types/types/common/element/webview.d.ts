// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, Callback } from '../events';
import { StandardProps } from '../props';

/**
 * Cookie Object
 * @PC
 */
export interface WebviewCookie {
  /**
   * The name of the cookie
   * @PC
   */
  name: string;
  /**
   * The value of the cookie
   * @PC
   */
  value: string;
  /**
   * The domain of the cookie
   * @PC
   */
  domain?: string;
  /**
   * Whether the cookie is HTTP only
   * @PC
   */
  hostOnly?: boolean;
  /**
   * The path of the cookie
   * @PC
   */
  path?: string;
  /**
   * Whether the cookie is marked as secure
   * @PC
   */
  secure?: boolean;
  /**
   * Whether the cookie is HTTP only
   * @PC
   */
  httpOnly?: boolean;
  /**
   * Whether the cookie is a session cookie or a persistent cookie with an expiration date
   * @PC
   */
  session?: boolean;
  /**
   * The expiration date of the cookie as the number of seconds since the UNIX epoch
   */
  expirationDate?: number;
  /**
   * The Same Site policy to apply to this cookie
   * @PC
   */
  sameSite: string;
}

export interface WebviewErrorEvent {
  /**
   * A string that represents the error message
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  errorMsg: string;
  /**
   * A number that represents the error code
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  errorCode: number;
}

export interface WebviewMessageEvent {
  /**
   * A string that represents the message
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  msg: string;
}

/**
 * Webview event
 * @PC
 */
export interface WebviewUrlEvent {
  /**
   * A string that represents the target url
   * @PC
   */
  url: string;
}

export interface WebviewProps extends Omit<StandardProps, 'binderror'> {
  /**
   * A string that represents the location of a resource on a remote server. Automatically trigger content refresh when the src changes
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  src?: string;
  /**
   * A string that represents the html content to load. Automatically trigger content refresh when the html changes. Priority lower than `src`.
   * @Android
   * @iOS
   * @Harmony
   * @since 3.6
   */
  html?: string;
  /**
   * Enable bounce effect
   * @defaultValue false
   * @iOS
   */
  bounces?: boolean;
  /**
   * Enable scrollbar
   * @defaultValue false
   * @iOS
   */
  'scroll-bar-enable'?: boolean;
  /**
   * Params for external webview implementation
   * @Android
   * @iOS
   */
  params?: object;
  /**
   * Specify the type of webview, it could be a implementation of a webview inject from LynxService
   * @defaultValue 'default'
   * @Android
   * @iOS
   * @Harmony
   */
  'webview-type'?: 'default' | string;

  /**
   * Enable WebView debugging in Android so that it can be debugged in Chrome DevTools
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'enable-debug'?: boolean;
  /**
   * Execute javascript when document ready
   * @since Lynx 3.5
   * @PC
   */
  initjs?: string;
  /**
   * Preset cookies
   * @since Lynx 3.5
   * @PC
   */
  cookies?: WebviewCookie[];
  /**
   * Whether enable offscreen rendering mode
   * @since Lynx 3.5
   * @defaultValue false
   * @PC
   */
  'use-osr'?: boolean;
  /**
   * Load success event
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindload?: (e: BaseEvent<'load'>) => void;
  /**
   * Error event
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  binderror?: (e: BaseEvent<'error', WebviewErrorEvent>) => void;
  /**
   * Message post from javascript
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindmessage?: (e: BaseEvent<'message', WebviewMessageEvent>) => void;
  /**
   * open window event
   * @since Lynx 3.5
   * @PC
   */
  bindopenwindow?: (e: BaseEvent<'openwindow', WebviewUrlEvent>) => void;
  /**
   * location change event
   * @since Lynx 3.5
   * @PC
   */
  bindlocationchange?: (e: BaseEvent<'locationchange', WebviewUrlEvent>) => void;
}

/**
 * Reload the webview
 * @Android
 * @iOS
 * @PC
 * @Harmony
 */
export interface WebviewReloadMethod extends BaseMethod {
  method: 'reload';
}

/**
 * Call js function
 * @Android
 * @iOS
 * @PC
 * @Harmony
 */
export interface WebviewEvalMethod extends BaseMethod {
  method: 'eval';
  params: {
    /**
     * Name of the function: `javascriptFunc(arg1, arg2)`.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    func: string;
  };
}

/**
 * Write any unwritten cookies data to disk for webview
 * @since Lynx 3.5
 * @PC
 */
export interface WebviewCookiesFlushStoreMethod extends BaseMethod {
  method: 'cookies.flushStore';
}

/**
 * Removes the cookies matching url and name
 * @since Lynx 3.5
 * @PC
 */
export interface WebviewCookiesRemoveMethod extends BaseMethod {
  method: 'cookies.remove';
  params: {
    /**
     * The URL associated with the cookie.
     * @PC
     */
    url: string;
    /**
     * The name of cookie to remove.
     * @PC
     */
    name?: string;
  };
}

/**
 * Set a cookie to webview
 * @since Lynx 3.5
 * @PC
 */
export interface WebviewCookiesSetMethod extends BaseMethod {
  method: 'cookies.set';
  params: {
    /**
     * The URL to associate the cookie with
     * @PC
     */
    url: string;
    /**
     * The name of the cookie
     * @PC
     */
    name: string;
    /**
     * The value of the cookie. Empty by default if omitted
     * @PC
     */
    value?: string;
    /**
     * The path of the cookie. Empty by default if omitted
     * @PC
     */
    path?: string;
    /**
     * The domain of the cookie. Empty by default if omitted
     * @PC
     */
    domain?: string;
    /**
     * Whether the cookie should be marked as secure
     * @defaultValue false
     * @PC
     */
    secure?: boolean;
    /**
     * Whether the cookie should be marked as HTTP only
     * @defaultValue false
     * @PC
     */
    httpOnly?: boolean;
    /**
     * The Same Site policy to apply to this cookie. Can be unspecified, no_restriction, lax or strict
     * @defaultValue 'lax'
     * @PC
     */
    sameSite?: string;
    /**
     * The expiration date of the cookie as the number of seconds since the UNIX epoch
     * @PC
     */
    expirationDate?: number;
  };
}

/**
 * Get cookies from webview
 * @since Lynx 3.5
 * @PC
 */
export interface WebviewCookiesGetMethod extends BaseMethod {
  method: 'cookies.get';
  params: {
    /**
     * Retrieves cookies which are associated with url. Empty implies retrieving cookies of all URLs
     * @PC
     */
    url?: string;
    /**
     * Filters cookies by name
     * @PC
     */
    name?: string;
    /**
     * Retrieves cookies whose path matches path
     * @PC
     */
    path?: string;
    /**
     * Retrieves cookies whose domains match or are subdomains of domains
     * @PC
     */
    domain?: string;
    /**
     * Filters cookies by their Secure property
     * @PC
     */
    secure?: boolean;
    /**
     * Filters cookies by httpOnly
     * @PC
     */
    httpOnly?: boolean;
    /**
     * Filters out session or persistent cookies
     * @PC
     */
    session?: boolean;
  };
  success?: Callback<WebviewCookie[]>;
}

export type WebviewUIMethods = WebviewReloadMethod | WebviewEvalMethod | WebviewCookiesFlushStoreMethod | WebviewCookiesRemoveMethod | WebviewCookiesSetMethod | WebviewCookiesGetMethod;
