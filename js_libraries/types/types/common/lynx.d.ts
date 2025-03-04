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
}
