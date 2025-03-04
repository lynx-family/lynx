// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export type ErrorName =
  | 'INTERNAL_PARSE_ERROR'
  | 'INTERNAL_RUNTIME_ERROR'
  | 'USER_VALIDATE_ERROR'
  | 'USER_RUNTIME_ERROR'
  | 'DATA_CHANGE_HANDLE_ERROR'
  | 'INVOKE_ERROR';

export type ErrorKind = 'INTERNAL_ERROR' | 'USER_ERROR';
export type ErrorEnv = 'SERVICE';

/**
 * The enum values should be sync with `lynx_error.h`.
 */
export const enum LynxErrorLevel {
  Fatal = 0,
  Error,
  Warn,
}

export abstract class BaseError extends Error {
  abstract kind: ErrorKind;
  abstract name: ErrorName;
  env?: ErrorEnv;
  constructor(message: string, stack?: string) {
    super(message);
    if (stack) {
      this.stack = stack;
    }
  }
}

export abstract class InternalError extends BaseError {
  kind = 'INTERNAL_ERROR' as const;
}

export abstract class UserError extends BaseError {
  kind = 'USER_ERROR' as const;
}

export class UserValidateError extends UserError {
  name = 'USER_VALIDATE_ERROR' as const;
}

/** error comes form use code */
export class UserRuntimeError extends UserError {
  name = 'USER_RUNTIME_ERROR' as const;
}

/**
 * error from internal framework
 */
export class InternalRuntimeError extends InternalError {
  name = 'INTERNAL_RUNTIME_ERROR' as const;
}

/**
 * error from lepusNG
 */
export class InternalLepusNgError extends Error {
  name: string;
  stack: string;
  constructor(message: string, stack?: string) {
    super(message);
    if (stack) {
      this.stack = stack;
    }
  }
}

/** error comes from jsb invoke  */
export class InvokeError extends InternalError {
  name = 'INVOKE_ERROR' as const;
}

export class AppServiceEngineKnownError extends Error {
  type: string;
  constructor(msg) {
    super(`APP-SERVICE-Engine: ${msg}`);
    this.type = 'AppServiceEngineKnownError';
  }
}

export interface sourceMapReleaseObj {
  name: string;
  stack: string; // detail stack of error
  message: string; // the sourceMapReleaseId, such as "d73160119ef7e77776246caca2a7b98e"
}
