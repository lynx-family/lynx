// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface LynxStreamDelegate {

  /**
   * streaming load process started;
   *
   *
   */
  onStart(contentOffset: number): void;

  /**
   * streaming load process return a part of data;
   *
   */
  onData(data: ArrayBuffer, offset: number, length: number): void;

  /**
   * streaming load process ended success;
   */
  onEnd(): void;

  /**
   * streaming load process ended with error;
   */
  onError(error: string): void;
}