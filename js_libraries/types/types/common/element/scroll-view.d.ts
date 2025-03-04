// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, EventHandler } from '../events';
import { StandardProps } from '../props';
import { AutoScrollMethod } from './list';
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
   * Scroll horizontaly.
   * @defaultValue false
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  'scroll-x'?: boolean;

  /**
   * Scroll vertically
   * @defaultValue false
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  'scroll-y'?: boolean;

  /**
   * Replacement of scroll-x and scroll-y
   * @since 3.0
   * @iOS
   * @Android
   * @H
   */
  'scroll-orientation'?: 'vertical' | 'horizontal';

  /**
   * Enable bounce effect
   * @defaultValue false
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  bounces?: boolean;

  /**
   * Enable dragging
   * @defaultValue false
   * @since 1.4
   * @iOS
   * @Android 2.2
   * @H
   */
  'enable-scroll'?: boolean;

  /**
   * Enable scrollbar
   * @defaultValue false
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  'scroll-bar-enable'?: boolean;

  /**
   * Not recommended to use. Please use upper-threshold-item-count instead. Set upper threshold to bindscrolltoupper event.
   * @defaultValue 0
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  'upper-threshold'?: number;

  /**
   * Not recommended to use. Please use lower-threshold-item-count instead. Set upper threshold to bindscrolltoupper event.
   * @defaultValue 0
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  'lower-threshold'?: number;

  /**
   * Please use initial-scroll-offset or ScrollTo method. Set the content offset from the top.
   * @defaultValue 0
   * @since 1.4
   * @deprecated 2.17
   * @iOS
   * @Android
   * @H
   */
  'scroll-top'?: number;

  /**
   * Please use initial-scroll-offset or ScrollTo method. Set the content offset from the left.
   * @defaultValue 0
   * @since 1.4
   * @deprecated 2.17
   * @iOS
   * @Android
   * @H
   */
  'scroll-left'?: number;

  /**
   * Initial scroll position, only effective once
   * @defaultValue 0
   * @since 2.17
   * @iOS
   * @Android
   * @H
   */
  'initial-scroll-offset'?: number;

  /**
   * Please use initial-scroll-index or ScrollTo method。Set the first item at the first screen
   * @defaultValue 0
   * @since 2.1
   * @deprecated 2.17
   * @iOS
   * @Android
   */
  'scroll-to-index'?: number;

  /**
   * Scroll to specified child node on first screen, only effective once. All direct child nodes must be flatten=false.
   * @defaultValue 0
   * @since 2.17
   * @iOS
   * @Android
   * @H
   */
  'initial-scroll-to-index'?: number;

  /**
   * On iOS, force-can-scroll should be used with ios-block-gesture-class and ios-recognized-view-tag. Can be used alone on Android.
   * @defaultValue false
   * @since 2.10.1
   * @iOS
   * @Android
   * @H
   */
  'force-can-scroll'?: boolean;

  /**
   * Force-can-scroll should be used with ios-block-gesture-class、ios-recognized-view-tag. Specify the class name of scrollable container that should be blocked by force-can-scroll. Given by container's developer.
   * @defaultValue none
   * @since 2.10.1
   * @iOS
   */
  'ios-block-gesture-class'?: string;

  /**
   *  force-can-scroll should be used with ios-block-gesture-class、ios-recognized-view-tag. Specify scrollable container's tag, the UIView's tag. Set and given by container's developer.
   * @defaultValue none
   * @since 2.10.1
   * @iOS
   */
  'ios-recognized-view-tag'?: number;

  /**
   * This event is triggered when the upper/left edge of the scrolling area intersects with the visible area defined by the upperThreshold.
   * @defaultValue none
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  bindscrolltoupper?: EventHandler<ScrollToUpperEvent>;

  /**
   * This event is triggered when the lower/right edge of the scrolling area intersects with the visible area defined by the lowerThreshold.
   * @defaultValue none
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  bindscrolltolower?: EventHandler<ScrollToLowerEvent>;

  /**
   * This event is triggered when the scrollview is scrolling.
   * @defaultValue none
   * @since 1.4
   * @iOS
   * @Android
   * @H
   */
  bindscroll?: EventHandler<ScrollEvent>;

  /**
   * This event is triggered when the scrollview's scroll ended.
   * @defaultValue none
   * @since 1.6
   * @iOS
   * @Android
   * @H
   */
  bindscrollend?: EventHandler<ScrollEndEvent>;

  /**
   * This event is triggered when the scrollview's content size changed.
   * @defaultValue none
   * @since 1.6
   * @iOS
   * @Android
   * @H
   */
  bindcontentsizechanged?: EventHandler<ContentSizeChangedEvent>;

  /**
   * scrollview scrolls to upper edge
   * @defaultValue none
   * @since 3.0
   * @iOS
   * @Android
   * @H
   */
  bindscrolltoupperedge?: EventHandler<ScrollToUpperEdgeEvent>;

  /**
   * scrollview scrolls to lower edge
   * @defaultValue none
   * @since 3.0
   * @iOS
   * @Android
   * @H
   */
  bindscrolltoloweredge?: EventHandler<ScrollToLowerEdgeEvent>;

  /**
   * scrollview scrolls to normal position. Not at upper or lower edge
   * @defaultValue none
   * @since 3.0
   * @iOS
   * @Android
   * @H
   */
  bindscrolltonormalstate?: EventHandler<ScrollToNormalStateEvent>;
}

export interface ScrollToMethod extends BaseMethod {
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

export type ScrollViewUIMethods = ScrollToMethod | AutoScrollMethod;
