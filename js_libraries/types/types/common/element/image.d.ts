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
   * @Android 0.1
   * @iOS 1.5
   * @Harmony 2.14
   * @Web
   * @ClayAndroid 2.14
   * @ClayIOS 2.8
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   * @defaultValue undefined
   */
  'src'?: string;

  /**
   * Specifies image cropping/scaling mode
   * `scaleToFill`: Scales the image without preserving the aspect ratio, stretching the image to fill the element.
   * `aspectFit`: Scales the image while preserving aspect ratio so that the long side is fully visible.
   * `aspectFill`: Scales the image while preserving aspect ratio, ensuring the short side fills the element.
   * `center`: Does not scale the image; image is centered.
   * @Android 0.1
   * @iOS 1.5
   * @Harmony 2.15
   * @Web
   * @ClayAndroid 2.1
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 2.14
   * @defaultValue 'scaleToFill'
   */
  'mode'?: 'scaleToFill' | 'aspectFit' | 'aspectFill' | 'center';

  /**
   * `ARGB_8888`: 32-bit memory per pixel, supports semi-transparent images.
   * `RGB_565`: 16-bit memory per pixel, reduces memory usage but loses transparency.
   *
   * Support PC platform since 3.5
   *
   * :::note
   *
   * Affects the actual memory usage of the image bitmap.
   *
   * Taking an image with a resolution of 1024*768 as an example, the actual memory usage is (1024 * 768 * bits per pixel / 8) Bytes.
   *
   * The default is ARGB_8888. Frontend developers can optimize image memory usage by setting it to RGB_565.
   *
   * `ARGB_8888`: Each pixel occupies 32 bits of memory and includes an alpha channel.
   * `RGB_565`: Each pixel occupies 16 bits of memory, which reduces memory usage but results in the loss of transparency.
   *
   * Setting RGB_565 may affect the display of `<image>`'s border-radius. You can set border-radius on the parent view of the `<image>` and add the clip-radius attribute to the parent view.
   *
   * It is not recommended to use RGB_565 when mode="aspectFit", as it may cause black borders in the cropped area.
   *
   * :::
   *
   * @Android 1.5
   * @ClayAndroid 3.5
   * @ClayIOS 3.5
   * @ClayMacOS 3.5
   * @ClayWindows 3.5
   * @ClayHarmony 4.0
   * @defaultValue "ARGB_8888"
   */
  'image-config'?: 'ARGB_8888' | 'RGB_565';

  /**
   * Placeholder image, used same as src
   * @Android 1.4
   * @iOS 1.5
   * @Harmony 2.15
   * @Web
   * @ClayAndroid 2.14
   * @ClayIOS 2.8
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  'placeholder'?: string;

  /**
   * Image blur radius
   * @Android 0.1
   * @iOS 1.5
   * @Harmony 2.15
   * @Web
   * @ClayAndroid 2.14
   * @ClayIOS 2.8
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  'blur-radius'?: string;

  /**
   * Stretchable area for 9patch images, in percentage or decimal, four values for top, right, bottom, left
   *
   * :::note
   * Using cap-insets does not require the original image to be a 9-patch image.
   * :::
   *
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.14
   */
  'cap-insets'?: string;

  /**
   * Adjust the scale of stretchable area for 9patch images
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.14
   * @defaultValue 1
   */
  'cap-insets-scale'?: number;

  /**
   * Number of times an animated image plays, 0 stands for infinite
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.18
   * @defaultValue 0
   */
  'loop-count'?: number;

  /**
   * Image won't load if its size is 0, but will load if prefetch-width is set
   * @Android 1.5
   * @iOS 1.5
   * @defaultValue "0px"
   * @deprecated
   */
  'prefetch-width'?: string;

  /**
   * Image won't load if its size is 0, but will load if prefetch-height is set
   * @Android 1.5
   * @iOS 1.5
   * @defaultValue "0px"
   * @deprecated
   */
  'prefetch-height'?: string;

  /**
   * When set to true and the `<image>` element has no width or height,
   * the size of the `<image>` will be automatically adjusted
   * to match the image's original dimensions after the image is successfully loaded,
   * ensuring that the aspect ratio is maintained.
   * @Android 2.0
   * @iOS 1.5
   * @Harmony 2.15
   * @Web
   * @ClayAndroid 2.18
   * @ClayIOS 2.8
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   * @ClayHarmony 2.15
   * @defaultValue false
   */
  'auto-size'?: boolean;

  /**
   * When set to true, the `<image>` will only clear the previously displayed image resource after a new image has successfully loaded.
   * The default behavior is to clear the image resource before starting a new load.
   * This can resolve flickering issues when the image src is switched and reloaded. It is not recommended to enable this in scenarios where there is node reuse in views like lists.
   * @Android 1.5
   * @iOS 2.8
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.17
   * @defaultValue false
   */
  'defer-src-invalidation'?: boolean;

  /**
   * Specifies whether the animated image should start playing automatically once it is loaded.
   * @Android 1.5
   * @iOS 2.11
   * @Harmony 3.4
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 3.3
   * @defaultValue true
   */
  'autoplay'?: boolean;

  /**
   * Changes the color of all non-transparent pixels to the tint-color specified. The value is a `<color>`.
   * @Android 1.5
   * @iOS 2.11
   * @Harmony 3.4
   * @ClayAndroid 3.7
   * @ClayIOS 3.7
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 3.1
   */
  'tint-color'?: string;

  /**
   * Image load success event
   * @Android 0.1
   * @iOS 1.5
   * @Harmony 2.16
   * @Web
   * @ClayAndroid 2.14
   * @ClayIOS 2.8
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  bindload?: (e: LoadEvent) => void;

  /**
   * Image load error event
   * @Android 0.1
   * @iOS 1.5
   * @Harmony 2.15
   * @Web
   * @ClayAndroid 2.1
   * @ClayIOS 2.8
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  binderror?: (e: ErrorEvent) => void;

  /**
   * Triggered when the animated image starts playing.
   * @Android 2.11
   * @iOS 3.2
   * @Harmony 3.3
   * @ClayAndroid 3.1
   * @ClayIOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 3.3
   */
  bindstartplay?: (e: BaseEvent) => void;

  /**
   * Triggered when one loop of the animated image finishes playing.
   * @Android 2.11
   * @iOS 3.2
   * @Harmony 3.3
   * @ClayAndroid 3.1
   * @ClayIOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 3.3
   */
  bindcurrentloopcomplete?: (e: BaseEvent) => void;

  /**
   * Triggered when the animated image finishes playing all `loop-count` loops. If `loop-count` is not set, this callback will not be triggered.
   * @Android 2.11
   * @iOS 3.2
   * @Harmony 3.3
   * @ClayAndroid 3.1
   * @ClayIOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 3.3
   */
  bindfinalloopcomplete?: (e: BaseEvent) => void;
}

export type LoadEvent = BaseEvent<'load', ImageLoadEvent>;
export type ErrorEvent = BaseEvent<'error', ImageErrorEvent>;

/**
 * Restart the animation playback method controlled by the front end, and the animation playback progress and loop count will be reset.
 * @Android 1.0
 * @iOS 1.0
 * @Harmony 3.4
 * @ClayAndroid 1.5
 * @ClayIOS 1.5
 * @ClayMacOS 1.5
 * @ClayWindows 1.5
 * @ClayHarmony 2.18
 * @deprecated Deprecated. Some scenarios may not call back the call result. It is recommended to use resumeAnimation instead.
 */
export interface ImageStartAnimMethod extends BaseMethod {
  method: 'startAnimate';
}

/**
 * Resumes the animation, without resetting the loop-count.
 * @Android 1.0
 * @iOS 1.0
 * @Harmony 3.4
 * @ClayAndroid 1.5
 * @ClayIOS 1.5
 * @ClayMacOS 1.5
 * @ClayWindows 1.5
 * @ClayHarmony 2.18
 */
export interface ImageResumeAnimMethod extends BaseMethod {
  method: 'resumeAnimation';
}

/**
 * Pauses the animation, without resetting the loop-count.
 * @Android 1.0
 * @iOS 1.0
 * @Harmony 3.4
 * @ClayAndroid 1.5
 * @ClayIOS 1.5
 * @ClayMacOS 1.5
 * @ClayWindows 1.5
 * @ClayHarmony 2.18
 */
export interface ImagePauseAnimMethod extends BaseMethod {
  method: 'pauseAnimation';
}

/**
 * Stops the animation, and it will reset the loop-count.
 * @Android 1.0
 * @iOS 1.0
 * @Harmony 3.4
 * @ClayAndroid 1.5
 * @ClayIOS 1.5
 * @ClayMacOS 1.5
 * @ClayWindows 1.5
 * @ClayHarmony 2.18
 */
export interface ImageStopAnimMethod extends BaseMethod {
  method: 'stopAnimation';
}

export type ImageUIMethods = ImageStartAnimMethod | ImageResumeAnimMethod | ImagePauseAnimMethod | ImageStopAnimMethod;
