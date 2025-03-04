// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent } from '../events';
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
