// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { ImageProps } from './image';
/** Properties for images embedded in text. */
export interface InlineImageProps extends ImageProps {
  /**
   * Shift the inline image vertically relative to the text baseline.
   * A px string is converted to device pixels, a percentage is relative to font size,
   * and a unitless string is a font-size multiplier. Positive values move it down.
   * @Android 2.3
   * @defaultValue Unset; the stored vertical shift is 0.
   */
  'custom-baseline-shift'?: string;
}
