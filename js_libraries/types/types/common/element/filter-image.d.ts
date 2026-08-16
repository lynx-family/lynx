// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

/**
 * Provides image blur and shadow features, similar to the usage of images. Specially, shadow and blur effects can be displayed inside the padding area.
 */
export interface FilterImageLoadEvent {
  detail: {
    width: number;
    height: number;
  };
}

export interface FilterImageErrorEvent {
  detail: {
    errMsg: string;
  };
}

export interface FilterImageProps extends StandardProps {
  /**
   * Supports http/https/base64
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.14
   * @Web
   * @defaultValue ""
   */
  'src'?: string;

  /**
   * Specifies image cropping/scaling mode
   * scaleToFill: Scales the image without preserving the aspect ratio, stretching the image to fill the element
   * aspectFit: Scales the image while preserving aspect ratio so that the long side is fully visible
   * aspectFill: Scales the image while preserving aspect ratio, ensuring the short side fills the element
   * center: Does not scale the image; image is centered
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.15
   * @Web
   * @defaultValue "scaleToFill"
   */
  'mode'?: 'scaleToFill' | 'aspectFit' | 'aspectFill' | 'center';

  /**
   * Specifies the BoxBlur radius for the image
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.15
   * @Web
   * @defaultValue "0px"
   */
  'blur-radius'?: string;

  /**
   * Specifies the shadow style for the image
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 3.1
   * @Web
   * @defaultValue ""
   */
  'drop-shadow'?: string;

  /**
   * Image load success event
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.16
   * @Web
   */
  'bindload'?: (e: FilterImageLoadEvent) => void;

  /**
   * Image load error event
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.15
   * @Web
   */
  'binderror'?: (e: FilterImageErrorEvent) => void;
}
