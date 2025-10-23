// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface TextInfo {
  fontSize: string;
  fontFamily?: string;
  maxWidth?: string;
  maxLine?: number;
}

export interface TextMetrics {
  width: number;
  content?: Array<string>;
}

export type DispatchEventResult = 
  // 'NotCanceled'
  // Event was not canceled by event handler or default event handler.
  | 0
  // 'CanceledByEventHandler'
  // Event was canceled by event handler; i.e. a script handler calling
  // preventDefault.
  | 1
  // 'CanceledByDefaultEventHandler'
  // Event was canceled by the default event handler; i.e. executing the default
  // action.  This result should be used sparingly as it deviates from the DOM
  // Event Dispatch model. Default event handlers really shouldn't be invoked
  // inside of dispatch.
  | 2
  // 'CanceledBeforeDispatch'
  // Event was canceled but suppressed before dispatched to event handler.  This
  // result should be used sparingly; and its usage likely indicates there is
  // potential for a bug. Trusted events may return this code; but untrusted
  // events likely should always execute the event handler the developer intends
  // to execute.
  | 3;

export interface MessageEvent {
  type: string;
  data: any;
  origin?: string;
}

export interface ContextProxy {
  onTriggerEvent?: (event: MessageEvent) => void;

  postMessage(message: any): void;
  dispatchEvent(event: MessageEvent): DispatchEventResult;
  addEventListener(type: string, listener: (event: MessageEvent) => void): void;
  removeEventListener(
    type: string,
    listener: (event: MessageEvent) => void
  ): void;
}

export interface BundleInfo {
  url: string;
  code: number;
  error_msg: string;
}

export interface ResponseHandler {
  wait: (timeout: number) => BundleInfo;
  then: (info: BundleInfo) => {}
}

/*
 *@description Common Lynx type
 */
export interface CommonLynx {
  getTextInfo(text: string, info: TextInfo): TextMetrics;

  /**
   * @description proactively report error
   * @param error errorInfo
   * @param options level warning or error
   * @since main-thread:3.0; background-thread:2.3;
   */
  reportError(error: string | Error, options?: { level?: 'error' | 'warning' }): void;

  /**
   * @description get project's targetSdkVersion config.
   * @since main-thread:3.0, background-thread: 2.6
   */
  targetSdkVersion?: string;

  getDevtool(): ContextProxy;
  getCoreContext(): ContextProxy;
  getJSContext(): ContextProxy;
  getUIContext(): ContextProxy;
  getNative(): ContextProxy;
  getEngine(): ContextProxy;
  fetchBundle(url: string, options?: {}): ResponseHandler;
  stopExposure(options?: { sendEvent: boolean }): void;
  resumeExposure(): void;
}
