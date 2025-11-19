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
} from './common';

export interface ScrollViewProps extends StandardProps {
  /**
   * Replacement of scroll-x and scroll-y
   * @defaultValue 'vertical'
   * @since 3.0
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'scroll-orientation'?: 'vertical' | 'horizontal';

  /**
   * Enable bounce effect
   * @defaultValue true
   * @since 1.4
   * @iOS
   * @Harmony
   * @PC
   */
  bounces?: boolean;

  /**
   * Enable dragging
   * @defaultValue true
   * @since 1.4
   * @iOS
   * @Android 2.2
   * @Harmony
   * @PC
   */
  'enable-scroll'?: boolean;

  /**
   * Enable scrollbar
   * @defaultValue true
   * @since 1.4
   * @iOS
   * @Harmony
   * @PC
   */
  'scroll-bar-enable'?: boolean;

  /**
   * Set upper threshold to bindscrolltoupper event.
   * @defaultValue 0
   * @since 1.4
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'upper-threshold'?: number;

  /**
   * Set upper threshold to bindscrolltoupper event.
   * @defaultValue 0
   * @since 1.4
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'lower-threshold'?: number;

  /**
   * Initial scroll position, only effective once, in PX
   * @defaultValue 0
   * @since 2.17
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'initial-scroll-offset'?: number;

  /**
   * Scroll to specified child node on first screen, only effective once. All direct child nodes must be flatten=false.
   * @defaultValue 0
   * @since 2.17
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'initial-scroll-to-index'?: number;

  /**
   * This event is triggered when the upper/left edge of the scrolling area intersects with the visible area defined by the upperThreshold.
   * @defaultValue none
   * @since 1.4
   * @iOS
   * @Android
   * @Harmony
   */
  bindscrolltoupper?: (e: ScrollToUpperEvent) => void;

  /**
   * This event is triggered when the lower/right edge of the scrolling area intersects with the visible area defined by the lowerThreshold.
   * @defaultValue none
   * @since 1.4
   * @iOS
   * @Android
   * @Harmony
   */
  bindscrolltolower?: (e: ScrollToLowerEvent) => void;

  /**
   * This event is triggered when the scrollview is scrolling.
   * @defaultValue none
   * @since 1.4
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  bindscroll?: (e: ScrollEvent) => void;

  /**
   * This event is triggered when the scrollview's scroll ended.
   * @defaultValue none
   * @since 1.6
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  bindscrollend?: (e: ScrollEndEvent) => void;

  /**
   * This event is triggered when the scrollview's content size changed.
   * @defaultValue none
   * @since 1.6
   * @iOS
   * @Android
   * @Harmony
   */
  bindcontentsizechanged?: (e: ContentSizeChangedEvent) => void;
}

/**
 * Scroll to specified position
 * @Android
 * @iOS
 * @Harmony
 * @PC
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
 * @Android
 * @iOS
 * @Harmony
 * @PC
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
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
export interface ScrollViewAutoScrollMethod extends BaseMethod {
  method: 'autoScroll';
  params: {
    /**
     *  The distance of each second's scrolling, which supports positive and negative values. The unit of distance can be "px", "rpx", "ppx", or null (for iOS, the value must be greater than 1/screen.scale px).
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    rate: number;
    /**
     * Start/stop automatic scrolling.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    start: boolean;
  };
}

/**
 * Get scroll info
 * @Android
 * @iOS
 * @Harmony
 * @PC
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

export type ScrollViewUIMethods = ScrollViewScrollToMethod | ScrollViewScrollByMethod | ScrollViewAutoScrollMethod | ScrollViewGetScrollInfoMethod;
