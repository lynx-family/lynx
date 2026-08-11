// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent } from '../events';

/**
 * Scrollbar properties supported by Clay scrollable components.
 * @ClayWindows
 * @ClayMacOS
 */
export interface ScrollbarProps {
  /**
   * Whether to display the scrollbar.
   * @defaultValue false
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'enable-scrollbar'?: boolean;

  /**
   * Whether to hide the scrollbar automatically when it is inactive.
   * @defaultValue true
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-auto-hide'?: boolean;

  /**
   * Width of the scrollbar track, in px.
   * @defaultValue 12
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-width'?: number;

  /**
   * Width of the scrollbar thumb, in px.
   * @defaultValue 8
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-thumb-width'?: number;

  /**
   * Minimum length of the scrollbar thumb, in px.
   * @defaultValue 18
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-thumb-min-length'?: number;

  /**
   * Corner radius of the scrollbar thumb, in px.
   * @defaultValue 4
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-thumb-radius'?: number;

  /**
   * Color of the scrollbar thumb.
   * Supported formats are named colors, #RGB, #RRGGBB, #RRGGBBAA, and comma-separated rgb(), rgba(), hsl(), and hsla().
   * @defaultValue 'rgba(0, 0, 0, 0.4)'
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-thumb-color'?: string;

  /**
   * Color of the scrollbar thumb while it is being dragged.
   * Supported formats are named colors, #RGB, #RRGGBB, #RRGGBBAA, and comma-separated rgb(), rgba(), hsl(), and hsla().
   * @defaultValue 'rgba(0, 0, 0, 0.8)'
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-thumb-active-color'?: string;

  /**
   * Color of the scrollbar thumb while the pointer is hovering over it.
   * Supported formats are named colors, #RGB, #RRGGBB, #RRGGBBAA, and comma-separated rgb(), rgba(), hsl(), and hsla().
   * @defaultValue 'rgba(0, 0, 0, 0.8)'
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-thumb-hover-color'?: string;

  /**
   * Color of the scrollbar track.
   * Supported formats are named colors, #RGB, #RRGGBB, #RRGGBBAA, and comma-separated rgb(), rgba(), hsl(), and hsla().
   * @defaultValue 'transparent'
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-track-color'?: string;

  /**
   * Delay before automatically hiding the scrollbar, in ms.
   * @defaultValue 1000
   * @since 3.0
   * @ClayWindows
   * @ClayMacOS
   */
  'scroll-bar-auto-hide-delay'?: number;
}

export interface BaseScrollInfo {
  /**
   * scroll top from start
   */
  scrollTop: number;
  /**
   * scroll left from start
   */
  scrollLeft: number;
  /**
   * scroll content height
   */
  scrollHeight: number;
  /**
   * scroll content width
   */
  scrollWidth: number;
  /**
   * X-axis scroll delta for this scroll. It's always 0 in some non-scroll related events.
   */
  deltaX: number;
  /**
   * Y-axis scroll delta for this scroll. It's always 0 in some non-scroll related events.
   */
  deltaY: number;
}

export interface ScrollToLowerEvent extends BaseEvent<'scrolltolower', BaseScrollInfo> {}
export interface ScrollToUpperEvent extends BaseEvent<'scrolltoupper', BaseScrollInfo> {}
export interface ScrollEvent extends BaseEvent<'scroll', BaseScrollInfo> {}
export interface ScrollEndEvent extends BaseEvent<'scrollend', BaseScrollInfo> {}
export interface ContentSizeChangedEvent extends BaseEvent<'contentsizechanged', BaseScrollInfo> {}
export interface ScrollToUpperEdgeEvent extends BaseEvent<'scrolltoupperedge', BaseScrollInfo> {}
export interface ScrollToLowerEdgeEvent extends BaseEvent<'scrolltoloweredge', BaseScrollInfo> {}
export interface ScrollToNormalStateEvent extends BaseEvent<'scrolltonormalstate', BaseScrollInfo> {}
export interface ScrollToNormalStateEvent extends BaseEvent<'scrolltonormalstate', BaseScrollInfo> {}
