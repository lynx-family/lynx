// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, ImageErrorEvent, ImageLoadEvent } from '../events';
import { StandardProps } from '../props';

/**
 * Used to display images
 */
export interface ImageProps extends StandardProps {
  /**
   * Supports http/https/base64
   * @defaultValue undefined
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 0.2
   */
  src?: string;

  /**
   * Specifies image cropping/scaling mode
   * scaleToFill: Scales the image without preserving the aspect ratio, stretching the image to fill the element
   * aspectFit: Scales the image while preserving aspect ratio so that the long side is fully visible
   * aspectFill: Scales the image while preserving aspect ratio, ensuring the short side fills the element
   * center: Does not scale the image; image is centered
   * @defaultValue 'scaleToFill'
   * @since 0.2
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  mode?: 'scaleToFill' | 'aspectFit' | 'aspectFill' | 'center';

  /**
   * ARGB_8888: 32-bit memory per pixel, supports semi-transparent images
   * RGB_565: 16-bit memory per pixel, reduces memory usage but loses transparency
   * Support PC platform since 3.5
   * @defaultValue "ARGB_8888"
   * @Android
   * @PC
   * @since 1.4
   */
  'image-config'?: 'ARGB_8888' | 'RGB_565';

  /**
   * Placeholder image, used same as src
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 1.4
   */
  placeholder?: string;

  /**
   * Image blur radius
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 0.2
   */
  'blur-radius'?: string;

  /**
   * Stretchable area for 9patch images, in percentage or decimal, four values for top, right, bottom, left
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 1.4
   */
  'cap-insets'?: string;

  /**
   * Adjust the scale of stretchable area for 9patch images
   * @defaultValue 1
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 1.4
   */
  'cap-insets-scale'?: number;

  /**
   * Number of times an animated image plays, 0 stands for infinite 
   * @defaultValue 0
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 1.4
   */
  'loop-count'?: number;

  /**
   * Image won't load if its size is 0, but will load if prefetch-width is set
   * @defaultValue "0px"
   * @deprecated
   * @iOS
   * @Android
   * @since 1.4
   */
  'prefetch-width'?: string;

  /**
   * Image won't load if its size is 0, but will load if prefetch-height is set
   * @defaultValue "0px"
   * @deprecated
   * @iOS
   * @Android
   * @since 1.4
   */
  'prefetch-height'?: string;

  /**
   * When set to true and the `<image>` element has no width or height,
   * the size of the `<image>` will be automatically adjusted
   * to match the image's original dimensions after the image is successfully loaded,
   * ensuring that the aspect ratio is maintained.
   * @defaultValue false
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 2.6
   */
  'auto-size'?: boolean;

  /**
   * When set to true, the `<image>` will only clear the previously displayed image resource after a new image has successfully loaded.
   * The default behavior is to clear the image resource before starting a new load.
   * This can resolve flickering issues when the image src is switched and reloaded. It is not recommended to enable this in scenarios where there is node reuse in views like lists.
   * @defaultValue false
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 2.7
   */
  'defer-src-invalidation'?: boolean;

  /**
   * Specifies whether the animated image should start playing automatically once it is loaded.
   * @defaultValue true
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 2.11
   */
  'autoplay'?: boolean;

  /**
   * Changes the color of all non-transparent pixels to the tint-color specified. The value is a `<color>`.
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 2.12
   */
  'tint-color'?: string;

  /**
   * Image load success event
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 0.2
   */
  bindload?: (e: LoadEvent) => void;

  /**
   * Image load error event
   * @iOS
   * @Android
   * @Harmony
   * @PC
   * @since 0.2
   */
  binderror?: (e: ErrorEvent) => void;
}

export type LoadEvent = BaseEvent<'load', ImageLoadEvent>;
export type ErrorEvent = BaseEvent<'error', ImageErrorEvent>;

/**
 * Restart the animation playback method controlled by the front end, and the animation playback progress and loop count will be reset.
 * @iOS
 * @Android
 * @Harmony
 * @PC
 * @deprecated Deprecated. Some scenarios may not call back the call result. It is recommended to use resumeAnimation instead.
 */
export interface ImageStartAnimMethod extends BaseMethod {
  method: 'startAnimate';
}

/**
 * Resumes the animation, without resetting the loop-count.
 * @iOS
 * @Android
 * @Harmony
 * @PC
 * @since 2.11
 */
export interface ImageResumeAnimMethod extends BaseMethod {
  method: 'resumeAnimation';
}

/**
 * Pauses the animation, without resetting the loop-count.
 * @iOS
 * @Android
 * @Harmony
 * @PC
 * @since 2.11
 */
export interface ImagePauseAnimMethod extends BaseMethod {
  method: 'pauseAnimation';
}

/**
 * Stops the animation, and it will reset the loop-count.
 * @iOS
 * @Android
 * @Harmony
 * @PC
 * @since 2.11
 */
export interface ImageStopAnimMethod extends BaseMethod {
  method: 'stopAnimation';
}

export type ImageUIMethods = ImageStartAnimMethod | ImageResumeAnimMethod | ImagePauseAnimMethod | ImageStopAnimMethod;
