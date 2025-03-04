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
   * @defaultValue ""
   * @since 0.2
   */
  'src'?: string;

  /**
   * Specifies image cropping/scaling mode
   * scaleToFill: Scales the image without preserving the aspect ratio, stretching the image to fill the element
   * aspectFit: Scales the image while preserving aspect ratio so that the long side is fully visible
   * aspectFill: Scales the image while preserving aspect ratio, ensuring the short side fills the element
   * center: Does not scale the image; image is centered
   * @defaultValue "scaleToFill"
   * @since 0.2
   */
  'mode'?: 'scaleToFill' | 'aspectFit' | 'aspectFill' | 'center';

  /**
   * Specifies the BoxBlur radius for the image
   * @defaultValue "0px"
   * @since 0.2
   */
  'blur-radius'?: string;

  /**
   * Specifies the shadow style for the image
   * @defaultValue ""
   * @since 0.2
   */
  'drop-shadow'?: string;

  /**
   * Image load success event
   * @since 0.2
   */
  'bindload'?: (e: FilterImageLoadEvent) => void;

  /**
   * Image load error event
   * @since 0.2
   */
  'binderror'?: (e: FilterImageErrorEvent) => void;
}
