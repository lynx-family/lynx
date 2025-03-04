// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

declare let nativeConsole: typeof console & {
  profile(key: string): void;
  profileEnd(): void;
  alog?(message?: string): void;
  report?(message?: string): void;
};

declare let __BUILD_VERSION__: string;
declare let __VERSION__: string;
// `development` and `production` are replaced by rollup
// `test` is injected by jest.setup.ts
declare let NODE_ENV: 'development' | 'production' | 'test';
declare let __COMMIT_HASH__: string;
declare let __PKG_MODE__: string;
declare let __OPEN_INTERNAL_LOG__:boolean;
declare let LynxSDKCore: any;
declare let globComponentRegistPath: string;

type AnyFunction = (...args: any[]) => any;

type LynxSetTimeout = (
  callback: (...args: unknown[]) => unknown,
  number: number
) => number;
