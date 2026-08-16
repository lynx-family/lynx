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
   * @Android 4.0
   * @iOS 4.0
   * @Harmony 4.0
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  src?: string;
  /**
   * A string that represents the html content to load. Automatically trigger content refresh when the html changes. Priority lower than `src`.
   * @Android 4.0
   * @iOS 4.0
   * @Harmony 4.0
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   */
  html?: string;
  /**
   * Enable bounce effect
   * @iOS 4.0
   * @defaultValue false
   */
  bounces?: boolean;
  /**
   * Enable scrollbar
   * @iOS 4.0
   * @defaultValue false
   */
  'scroll-bar-enable'?: boolean;
  /**
   * Params for external webview implementation
   * @Android 4.0
   * @iOS 4.0
   */
  params?: object;
  /**
   * Specify the type of webview, it could be a implementation of a webview inject from LynxService
   * @Android 4.0
   * @iOS 4.0
   * @Harmony 4.0
   * @defaultValue 'default'
   */
  'webview-type'?: 'default' | string;

  /**
   * Enable WebView debugging in Android so that it can be debugged in Chrome DevTools
   * @Android 4.0
   * @iOS 4.0
   * @Harmony 4.0
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue false
   */
  'enable-debug'?: boolean;
  /**
   * Execute javascript when document ready
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  initjs?: string;
  /**
   * Preset cookies
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  cookies?: WebviewCookie[];
  /**
   * Whether enable offscreen rendering mode
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue false
   */
  'use-osr'?: boolean;
  /**
   * Load success event
   * @Android 4.0
   * @iOS 4.0
   * @Harmony 4.0
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  bindload?: (e: BaseEvent<'load'>) => void;
  /**
   * Error event
   * @Android 4.0
   * @iOS 4.0
   * @Harmony 4.0
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  binderror?: (e: BaseEvent<'error', WebviewErrorEvent>) => void;
  /**
   * Message post from javascript
   * @Android 4.0
   * @iOS 4.0
   * @Harmony 4.0
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  bindmessage?: (e: BaseEvent<'message', WebviewMessageEvent>) => void;
  /**
   * open window event
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  bindopenwindow?: (e: BaseEvent<'openwindow', WebviewUrlEvent>) => void;
  /**
   * location change event
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  bindlocationchange?: (e: BaseEvent<'locationchange', WebviewUrlEvent>) => void;
}

/**
 * Reload the webview
 * @Android 4.0
 * @iOS 4.0
 * @Harmony 4.0
 * @ClayAndroid 4.0
 * @ClayIOS 4.0
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface WebviewReloadMethod extends BaseMethod {
  method: 'reload';
}

/**
 * Call js function
 * @Android 4.0
 * @iOS 4.0
 * @Harmony 4.0
 * @ClayAndroid 4.0
 * @ClayIOS 4.0
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
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
 * @ClayAndroid 4.0
 * @ClayIOS 4.0
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface WebviewCookiesFlushStoreMethod extends BaseMethod {
  method: 'cookies.flushStore';
}

/**
 * Removes the cookies matching url and name
 * @ClayAndroid 4.0
 * @ClayIOS 4.0
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
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
 * @ClayAndroid 4.0
 * @ClayIOS 4.0
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
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
 * @ClayAndroid 4.0
 * @ClayIOS 4.0
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
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
