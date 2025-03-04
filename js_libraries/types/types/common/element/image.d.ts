// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, ImageErrorEvent, ImageLoadEvent } from '../events';
import { StandardProps } from '../props';

/**
 * Used to display images
 */
export interface ImageProps extends StandardProps {
  /**
   * Supports http/https/base64
   * @defaultValue ""
   * @since 0.2
   */
  src?: string;

  /**
   * Specifies image cropping/scaling mode
   * scaleToFill: Scales the image without preserving the aspect ratio, stretching the image to fill the element
   * aspectFit: Scales the image while preserving aspect ratio so that the long side is fully visible
   * aspectFill: Scales the image while preserving aspect ratio, ensuring the short side fills the element
   * center: Does not scale the image; image is centered
   * @defaultValue "scaleToFill"
   * @since 0.2
   */
  mode?: 'scaleToFill' | 'aspectFit' | 'aspectFill' | 'center';

  /**
   * ARGB_8888: 32-bit memory per pixel, supports semi-transparent images
   * RGB_565: 16-bit memory per pixel, reduces memory usage but loses transparency
   * @defaultValue "ARGB_8888"
   * @since 1.4
   */
  'image-config'?: 'ARGB_8888' | 'RGB_565';

  /**
   * Placeholder image, used same as src
   * @defaultValue ""
   * @since 1.4
   */
  placeholder?: string;

  /**
   * Image blur radius
   * @defaultValue "3px"
   * @since 0.2
   */
  'blur-radius'?: string;

  /**
   * Stretchable area for 9patch images, in percentage or decimal, four values for top, right, bottom, left
   * @defaultValue "0.2 10% 0.3 20%"
   * @since 1.4
   */
  'cap-insets'?: string;

  /**
   * Number of times an animated image plays
   * @defaultValue 1
   * @since 1.4
   */
  'loop-count'?: number;

  /**
   * Image won't load if its size is 0, but will load if prefetch-width is set
   * @defaultValue "100px"
   * @since 1.4
   */
  'prefetch-width'?: string;

  /**
   * Image won't load if its size is 0, but will load if prefetch-height is set
   * @defaultValue "100px"
   * @since 1.4
   */
  'prefetch-height'?: string;

  /**
   * If true, URL mapping is skipped. LynxView's custom ImageInterceptor won't work
   * @defaultValue false
   * @since 1.5
   */
  'skip-redirection'?: boolean;

  /**
   * Reduces the chance of OOM by downsampling large images, requires container support
   * @defaultValue false
   * @since iOS 2.0
   */
  downsampling?: boolean;

  /**
   * Disables unexpected iOS built-in animations
   * @defaultValue true
   * @since iOS 2.0
   */
  'implicit-animation'?: boolean;

  /**
   * Image load success event
   * @since 0.2
   */
  bindload?: (e: LoadEvent) => void;

  /**
   * Image load error event
   * @since 0.2
   */
  binderror?: (e: ErrorEvent) => void;

  /**
   * Add custom parameters to image
   * @since 2.17
   */
  'additional-custom-info'?: { [key: string]: string };
}

export type LoadEvent = BaseEvent<'load', ImageLoadEvent>;
export type ErrorEvent = BaseEvent<'error', ImageErrorEvent>;
