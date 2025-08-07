// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface ITextCodecHelper {
  /**
   * @description decode array buffer to string
   * @since 3.4
   */
  decode(input: ArrayBuffer): string;
  /**
   * @description encode string to array buffer
   * @since 3.4
   */
  encode(input: string): ArrayBuffer;
}
