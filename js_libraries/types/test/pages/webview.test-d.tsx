// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { BaseEvent, IntrinsicElements, WebviewErrorEvent, WebviewMessageEvent, WebviewUrlEvent, WebviewUIMethods } from '../../types';

// Props Types Check
let a: unknown;
{
  <webview src="https://example.com" />;
  assertType<string | undefined>(a as IntrinsicElements['webview']['src']);

  <webview html="<html><body>Hello</body></html>" />;
  assertType<string | undefined>(a as IntrinsicElements['webview']['html']);

  <webview bounces={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['webview']['bounces']);

  <webview scroll-bar-enable={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['webview']['scroll-bar-enable']);

  <webview params={{ key: 'value' }} />;
  assertType<object | undefined>(a as IntrinsicElements['webview']['params']);

  <webview webview-type="custom" />;
  assertType<'default' | string | undefined>(a as IntrinsicElements['webview']['webview-type']);

  <webview enable-debug={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['webview']['enable-debug']);

  <webview initjs="console.log('hello')" />;
  assertType<string | undefined>(a as IntrinsicElements['webview']['initjs']);

  <webview use-osr={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['webview']['use-osr']);

  <webview cookies={[{ name: 'test', value: 'value', sameSite: 'lax' }]} />;
}

// Events types check
function noop() {}
{
  <webview bindtap={noop} />;

  <webview
    bindload={(e) => {
      assertType<'load'>(e.type);
    }}
  />;

  <webview
    binderror={(e: BaseEvent<'error', WebviewErrorEvent>) => {
      assertType<string>(e.detail.errorMsg);
      assertType<number>(e.detail.errorCode);
    }}
  />;

  <webview
    bindmessage={(e: BaseEvent<'message', WebviewMessageEvent>) => {
      assertType<string>(e.detail.msg);
    }}
  />;

  <webview
    bindopenwindow={(e: BaseEvent<'openwindow', WebviewUrlEvent>) => {
      assertType<string>(e.detail.url);
    }}
  />;

  <webview
    bindlocationchange={(e: BaseEvent<'locationchange', WebviewUrlEvent>) => {
      assertType<string>(e.detail.url);
    }}
  />;
}

// UIMethods types check
function invoke<T extends keyof { 'webview': WebviewUIMethods }>(_param: { 'webview': WebviewUIMethods }[T]) {}

{
  invoke<'webview'>({
    method: 'reload',
    success: () => {},
  });

  invoke<'webview'>({
    method: 'eval',
    params: {
      func: 'console.log("hello")',
    },
    success: () => {},
  });

  invoke<'webview'>({
    method: 'cookies.flushStore',
    success: () => {},
  });

  invoke<'webview'>({
    method: 'cookies.remove',
    params: {
      url: 'https://example.com',
      name: 'test-cookie',
    },
    success: () => {},
  });

  invoke<'webview'>({
    method: 'cookies.set',
    params: {
      url: 'https://example.com',
      name: 'test-cookie',
      value: 'test-value',
      path: '/',
      domain: 'example.com',
      secure: true,
      httpOnly: false,
      sameSite: 'lax',
      expirationDate: 1234567890,
    },
    success: () => {},
  });

  invoke<'webview'>({
    method: 'cookies.get',
    params: {
      url: 'https://example.com',
    },
    success: (cookies) => {
      assertType<
        Array<{
          name: string;
          value: string;
          sameSite: string;
          domain?: string;
          path?: string;
          secure?: boolean;
          httpOnly?: boolean;
          session?: boolean;
          expirationDate?: number;
          hostOnly?: boolean;
        }>
      >(cookies);
    },
  });
}
