// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

export interface BlurViewProps extends StandardProps {
  /**
   * Gaussian blur radius.
   * @Android 4.0
   * @iOS 4.0
   * @Harmony 4.0
   * @ClayAndroid 4.0
   * @ClayIOS 4.0
   * @ClayHarmony 4.0
   * @defaultValue "0px"
   */
  'blur-radius'?: string;

  /**
   * The downsampling ratio of the blurred area is similar to reducing the blur of the background area first and then zooming in,
   * which slightly affects the blur effect, but can significantly improve performance.
   * @Android 4.0
   * @ClayAndroid 4.0
   * @defaultValue 6
   */
  'blur-sampling'?: number;

  /**
   * Whether to automatically update blur.
   * @Android 4.0
   * @ClayAndroid 4.0
   * @defaultValue true
   */
  'enable-auto-blur'?: boolean;

  /**
   * The raw id of the Android Lynx view to capture and blur.
   * Only the overlap with the `<blur-view>` is used as the blur source.
   * When omitted or unresolved, Android uses the default parent capture path.
   * @Android 4.0
   * @ClayAndroid 4.0
   */
  'android-capture-target'?: string;

  /**
   * Switches the Android internal blur-buffer refresh path.
   * @Android 4.0
   * @ClayAndroid 4.0
   * @experimental
   */
  'experimental-update-blur-radius'?: boolean;

  /**
   * It mainly affects the brightness of the blurred area:
   * light: the brightness is basically the same as the background;
   * dark: the brightness is darker than the background;
   * extra-light: the brightness is brighter than the background;
   * glass: a visual effect that renders a glass material;
   * glass-container: a UIGlassContainerEffect renders multiple glass elements into a combined effect.
   * @iOS 4.0
   * @ClayIOS 4.0
   * @defaultValue 'light'
   */
  'blur-effect'?: 'light' | 'extra-light' | 'dark' | 'glass' | 'glass-container';

  /**
   * The spacing specifies the distance between elements at which they begin to merge.
   * @iOS 4.0
   * @ClayIOS 4.0
   * @defaultValue 0
   */
  'spacing'?: number;

  /**
   * Enables interactive behavior for the glass effect.
   * @iOS 4.0
   * @ClayIOS 4.0
   * @defaultValue false
   */
  'glass-interactive'?: boolean;

  /**
   * The user interface style adopted by `<blur-view>`.
   * @iOS 4.1
   * @see {@link https://developer.apple.com/documentation/uikit/uiviewcontroller/overrideuserinterfacestyle?language=objc | Apple Developer Documentation}
   */
  'ios-user-interface-style'?: 'dark' | 'light';

  /**
   * A tint color applied to the glass effect.
   * @iOS 4.0
   * @ClayIOS 4.0
   * @defaultValue 'transparent'
   */
  'glass-tint-color'?: string;

  /**
   * The style of the glass effect.
   * @iOS 4.0
   * @ClayIOS 4.0
   * @defaultValue 'regular'
   */
  'glass-style'?: 'regular' | 'clear';
}
