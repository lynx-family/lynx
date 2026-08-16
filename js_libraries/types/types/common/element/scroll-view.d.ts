// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, EventHandler, Callback } from '../events';
import { StandardProps } from '../props';
import {
  ContentSizeChangedEvent,
  ScrollEndEvent,
  BaseScrollInfo,
  ScrollEvent,
  ScrollToNormalStateEvent,
  ScrollToLowerEvent,
  ScrollToUpperEvent,
  ScrollToUpperEdgeEvent,
  ScrollToLowerEdgeEvent,
  ScrollbarProps,
} from './common';

export interface ScrollViewProps extends StandardProps, ScrollbarProps {
  /**
   * Replacement of scroll-x and scroll-y
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 3.5
   * @defaultValue 'vertical'
   */
  'scroll-orientation'?: 'vertical' | 'horizontal';

  /**
   * Enable bounce effect
   * @iOS 1.5
   * @Harmony 3.4
   * @ClayIOS 3.2
   * @ClayMacOS 3.2
   * @ClayWindows 3.2
   * @ClayHarmony 3.5
   * @defaultValue true
   */
  bounces?: boolean;

  /**
   * Enable dragging
   * @Android 2.2
   * @iOS 2.18
   * @Harmony 2.14
   * @Web
   * @ClayAndroid 3.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 3.5
   * @defaultValue true
   */
  'enable-scroll'?: boolean;

  /**
   * Enables the experimental Android overflow clipping implementation for scroll-view.
   * @Android 4.2
   * @defaultValue true
   * @experimental
   */
  'experimental-android-enable-new-overflow'?: boolean;

  /**
   * Enable scrollbar
   * @Android 3.5
   * @iOS 2.18
   * @Harmony 3.1
   * @Web
   * @ClayAndroid 4.0
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 4.0
   * @defaultValue true
   */
  'scroll-bar-enable'?: boolean;

  /**
   * Set upper threshold to bindscrolltoupper event.
   * @Android 3.5
   * @iOS 2.18
   * @Harmony 3.5
   * @Web
   * @ClayAndroid 3.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 3.5
   * @defaultValue 0
   */
  'upper-threshold'?: number;

  /**
   * Set upper threshold to bindscrolltoupper event.
   * @Android 3.5
   * @iOS 2.18
   * @Harmony 3.5
   * @Web
   * @ClayAndroid 3.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 3.5
   * @defaultValue 0
   */
  'lower-threshold'?: number;

  /**
   * Initial scroll position, only effective once, in PX
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 3.2
   * @ClayIOS 3.2
   * @ClayMacOS 3.2
   * @ClayWindows 3.2
   * @ClayHarmony 3.0
   * @defaultValue 0
   */
  'initial-scroll-offset'?: number;

  /**
   * Scroll to specified child node on first screen, only effective once. All direct child nodes must be flatten=false.
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 3.4
   * @Web
   * @defaultValue 0
   */
  'initial-scroll-to-index'?: number;

  /**
   * This event is triggered when the upper/left edge of the scrolling area intersects with the visible area defined by the upperThreshold.
   * @Android 0.2
   * @iOS 3.4
   * @Harmony 2.18
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 2.18
   * @defaultValue none
   */
  bindscrolltoupper?: (e: ScrollToUpperEvent) => void;

  /**
   * This event is triggered when the lower/right edge of the scrolling area intersects with the visible area defined by the lowerThreshold.
   * @Android 0.2
   * @iOS 3.4
   * @Harmony 2.18
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 2.18
   * @defaultValue none
   */
  bindscrolltolower?: (e: ScrollToLowerEvent) => void;

  /**
   * This event is triggered when the scrollview is scrolling.
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.16
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 2.14
   * @defaultValue none
   */
  bindscroll?: (e: ScrollEvent) => void;

  /**
   * This event is triggered when the scrollview's scroll ended.
   * @Android 3.5
   * @iOS 3.4
   * @Harmony 2.18
   * @Web
   * @ClayAndroid 3.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 2.18
   * @defaultValue none
   */
  bindscrollend?: (e: ScrollEndEvent) => void;

  /**
   * This event is triggered when the scrollview's content size changed.
   * @Android 1.3
   * @iOS 4.1
   * @Harmony 2.18
   * @ClayAndroid 2.15
   * @ClayIOS 2.8
   * @ClayMacOS 2.15
   * @ClayWindows 2.15
   * @ClayHarmony 2.15
   * @defaultValue none
   */
  bindcontentsizechanged?: (e: ContentSizeChangedEvent) => void;
}

/**
 * Scroll to specified position
 * @Android 0.1
 * @iOS 2.18
 * @Harmony 3.5
 * @Web
 * @ClayAndroid 1.5
 * @ClayIOS 2.8
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 * @ClayHarmony 2.14
 */
export interface ScrollViewScrollToMethod extends BaseMethod {
  method: 'scrollTo';
  params: {
    /**
     * Offset relative to target node
     */
    offset?: number;

    /**
     * Enable scroll animation
     */
    smooth?: boolean;

    /**
     * Target item index
     * @defaultValue 0
     */
    index?: number;
  };
}

/**
 * Scroll by specified offset
 * @Android 1.5
 * @iOS 2.18
 * @Harmony 3.5
 * @Web
 */
export interface ScrollViewScrollByMethod extends BaseMethod {
  method: 'scrollBy';
  params: {
    /**
     * Offset to scroll
     */
    offset?: number;
  };
}



/**
 * Automatic scrolling
 * @Android 2.0
 * @iOS 2.18
 * @Harmony 2.16
 * @Web
 * @ClayAndroid 2.14
 * @ClayIOS 2.8
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 * @ClayHarmony 2.14
 */
export interface ScrollViewAutoScrollMethod extends BaseMethod {
  method: 'autoScroll';
  params: {
    /**
     *  The distance of each second's scrolling, which supports positive and negative values. The unit of distance can be "px", "rpx", "ppx", or null (for iOS, the value must be greater than 1/screen.scale px).
     * @Android
     * @Web
     * @iOS
     * @Harmony
     * @PC
     */
    rate: number;
    /**
     * Start/stop automatic scrolling.
     * @Android
     * @Web
     * @iOS
     * @Harmony
     * @PC
     */
    start: boolean;
  };
}

/**
 * Get scroll info
 * @Android 2.13
 * @iOS 2.18
 * @Harmony 4.0
 * @ClayAndroid 3.1
 * @ClayIOS 2.16
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 * @ClayHarmony 4.0
 */
export interface ScrollViewGetScrollInfoMethod extends BaseMethod {
  method: 'getScrollInfo';
  success?: Callback<{
    /**
     * Content offset on X-axis, in PX
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    scrollX: number;
    /**
     * Content offset on Y-axis, in PX
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    scrollY: number;
    /**
     * Total scrollable range along orientation, in PX
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    scrollRange: number;
  }>;
}

/**
 * Take a screenshot of the full scrollable content.
 * @Android 4.0
 * @iOS 4.1
 * @Harmony 4.0
 */
export interface ScrollViewTakeContentScreenshotMethod extends BaseMethod {
  method: 'takeContentScreenshot';
  params?: {
    /**
     * Specify the image format.
     * @defaultValue 'jpeg'
     * @Android
     * @iOS
     * @Harmony
     */
    format?: 'jpeg' | 'png';
    /**
     * Specify the output scale.
     * @defaultValue 1
     * @Android
     * @iOS
     * @Harmony
     */
    scale?: number;
  };
  success?: Callback<{
    /**
     * The base64-encoded string of the screenshot image.
     * @Android
     * @iOS
     * @Harmony
     */
    data: string;
    /**
     * Width of the screenshot image.
     * @Android
     * @iOS
     * @Harmony
     */
    width: number;
    /**
     * Height of the screenshot image.
     * @Android
     * @iOS
     * @Harmony
     */
    height: number;
  }>;
}

export type ScrollViewUIMethods =
  | ScrollViewScrollToMethod
  | ScrollViewScrollByMethod
  | ScrollViewAutoScrollMethod
  | ScrollViewGetScrollInfoMethod
  | ScrollViewTakeContentScreenshotMethod;
