// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod } from '../events';
import { StandardProps } from '../props';

export interface ViewPagerChange {
  /**
   * Current index
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  index: number;

  /**
   * If this change event is caused by the user's dragging
   * @Android 2.18
   * @iOS 2.18
   * @Harmony 2.18
   * @PC 2.18
   */
  isDragged: boolean;
}

export interface ViewPagerOffsetChange {
  /**
   * The scrolling offset
   * @Android
   * @iOS
   * @PC
   * @Harmony
   */
  offset: number;
}

export interface ViewPagerProps extends StandardProps {
  /**
   * UIView's tag, be used to identify the UIView of the UIGestureRecognizer which identified by `ios-recognized-gesture-class`. This property is designed to let an UIPanGesture work together with viewpager.
   * @iOS 3.9
   * @defaultValue 0
   */
  'ios-recognized-view-tag'?: number;

  /**
   * UIGestureRecognizer's class name, be used to identify the UIGestureRecognizer which may be recognized simultaneously with viewpager. This property is designed to let an UIPanGesture work together with viewpager.
   * @iOS 3.9
   */
  'ios-recognized-gesture-class'?: string;

  /**
   * Select the specified page at initialization, specifically referring to the first time when there are children.
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @ClayMacOS 3.9
   * @ClayWindows 3.9
   * @defaultValue 0
   */
  'initial-select-index'?: number;

  /**
   * Enable horizontal scroll gesture
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @ClayMacOS 3.9
   * @ClayWindows 3.9
   * @defaultValue true
   */
  'enable-scroll'?: boolean;

  /**
   * Is a spring effect needed. Note that this effect is not available on Android. On PC, only macOS support this feature.
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @ClayMacOS 3.9
   * @ClayWindows 3.9
   * @defaultValue true
   */
  bounces?: boolean;

  /**
   * When the finger touches within [0, value] from the left edge of the screen, it doesn't respond to the horizontal swipe gesture, but it can directly respond to the iOS right swipe to return gesture. However, it's not applicable to Android.
   * @iOS 3.9
   * @defaultValue 0
   */
  'ios-gesture-offset'?: number;

  /**
   * When enabled, when sliding to the head or tail, it can respond to the horizontal swipe events of the outer container (horizontal UIScrollView)
   * @iOS 3.9
   * @defaultValue false
   */
  'ios-gesture-direction'?: boolean;

  /**
   * Whether to enable the lazy load mode based on early exposure, it needs to be used in conjunction with the lazyComponent.
   * @Android 3.9
   * @iOS 3.9
   * @experimental
   * @defaultValue false
   */
  'keep-item-view'?: boolean;

  /**
   * On Android, this attribute is used to control the interaction behavior when scrolling to the edges. Setting it to true enables the default bounce effect, while setting it to false disables the bounce effect. The default value is false.
   * @Android 3.9
   * @defaultValue false
   */
  'android-always-overscroll'?: boolean;

  /**
   * On Android, this attribute is used to control if at start/end of viewpager and set true, gesture can not pass to parent.
   * @Android 3.9
   * @defaultValue false
   */
  'android-force-can-scroll'?: boolean;

  /**
   * Page switch event, it will only be triggered when the UI completely switches to the next page (from 100% to 200%).
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @ClayMacOS 3.9
   * @ClayWindows 3.9
   */
  bindchange?: (e: ViewPagerChangeEvent) => void;

  /**
   * Page switch event, it will be triggered when the page is about to switch, and at this time the UI has not switched to the next page yet, so the offset is still in the range of 0~100% (from page 0 to page 1) or 100%~200% (from page 1 to page 2).
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @ClayMacOS 3.9
   * @ClayWindows 3.9
   */
  bindwillchange?: (e: ViewPagerWillChangeEvent) => void;

  /**
   * Page switch progress callback, the range is the index of each page, for example, from page 0 to page 1, it's 0~1, from the first page to the second page, it's 1~2.
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 3.9
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @ClayMacOS 3.9
   * @ClayWindows 3.9
   */
  bindoffsetchange?: (e: ViewPagerOffsetChangeEvent) => void;
}

export type ViewPagerChangeEvent = BaseEvent<'bindchange', ViewPagerChange>;
export type ViewPagerWillChangeEvent = BaseEvent<'bindwillchange', ViewPagerChange>;
export type ViewPagerOffsetChangeEvent = BaseEvent<'bindchange', ViewPagerOffsetChange>;

export interface ViewPagerItemProps extends StandardProps {}

/**
 * Slide to the specified page.
 * @Android 3.9
 * @iOS 3.9
 * @Harmony 3.9
 * @ClayAndroid 3.9
 * @ClayIOS 3.9
 * @ClayMacOS 3.9
 * @ClayWindows 3.9
 */
export interface ViewPagerSelectTabMethod extends BaseMethod {
  method: 'selectTab';
  params: {
    /**
     * The index to be scrolled to.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    index: number;

    /**
     * If a animation effect needed. The default setting is true.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    smooth: boolean;
  };
}

export type ViewPagerUIMethods = ViewPagerSelectTabMethod;
