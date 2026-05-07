// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

export interface BlurViewProps extends StandardProps {
  /**
   * Gaussian blur radius.
   * @defaultValue "0px"
   * @Android
   * @iOS
   * @Harmony
   */
  'blur-radius'?: string;

  /**
   * The downsampling ratio of the blurred area is similar to reducing the blur of the background area first and then zooming in,
   * which slightly affects the blur effect, but can significantly improve performance.
   * @defaultValue 6
   * @Android
   */
  'blur-sampling'?: number;

  /**
   * Whether to automatically update blur.
   * @defaultValue true
   * @Android
   */
  'enable-auto-blur'?: boolean;

  /**
   * The raw id of the Android Lynx view to capture and blur.
   * Only the overlap with the `<blur-view>` is used as the blur source.
   * When omitted or unresolved, Android uses the default parent capture path.
   * @Android
   * @since 3.9
   */
  'android-capture-target'?: string;

  /**
   * Switches the Android internal blur-buffer refresh path.
   * @Android
   * @experimental
   * @since 3.4
   */
  'experimental-update-blur-radius'?: boolean;

  /**
   * It mainly affects the brightness of the blurred area:
   * light: the brightness is basically the same as the background;
   * dark: the brightness is darker than the background;
   * extra-light: the brightness is brighter than the background;
   * glass: a visual effect that renders a glass material;
   * glass-container: a UIGlassContainerEffect renders multiple glass elements into a combined effect.
   * @defaultValue 'light'
   * @iOS
   */
  'blur-effect'?: 'light' | 'extra-light' | 'dark' | 'glass' | 'glass-container';

  /**
   * The spacing specifies the distance between elements at which they begin to merge.
   * @defaultValue 0
   * @iOS
   */
  'spacing'?: number;

  /**
   * Enables interactive behavior for the glass effect.
   * @defaultValue false
   * @iOS
   * @since 3.8
   */
  'glass-interactive'?: boolean;

  /**
   * A tint color applied to the glass effect.
   * @defaultValue 'transparent'
   * @iOS
   * @since 3.8
   */
  'glass-tint-color'?: string;

  /**
   * The style of the glass effect.
   * @defaultValue 'regular'
   * @iOS
   * @since 3.8
   */
  'glass-style'?: 'regular' | 'clear';
}
