
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod } from '../events';
import { StandardProps } from '../props';


export interface RefreshStartRefresh {
  /**
   * Indicates whether the startrefresh event is triggered by manual drag
   * @Android
   * @iOS
   * @Harmony
   * @Web
   **/
  isManual: boolean;
}

export interface RefreshHeaderOffset {
  /**
   * Indicates if `<refresh-header>` is being dragged
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   **/
  isDragging: boolean;
  /**
   * The ratio of the pull-down movement distance to its own height
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   **/
  offsetPercent: number;
}

export enum RefreshState {
    IDLE, 
    OVER_DRAG_RELEASE, 
    REFRESHING, 
}

export interface RefreshStateChange {
  /**
   * Indicates if `<refresh-header>` is being dragged
   * @Android
   * @iOS
   * @Harmony
   */
  state: RefreshState;
}

export interface RefreshProps extends StandardProps {
  /**
   * Determines if dragging down or calling autoStartRefresh can trigger the startrefresh event.
   * @defaultValue true
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   */
  'enable-refresh'?: boolean;

  /**
   * Triggered when enable-refresh is true, and dragging down or calling autoStartRefresh (enters refresh state).
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   */
  bindstartrefresh?: (e: RefreshStartRefreshEvent) => void;


  /**
   * Triggered during movement when `<refresh-header>` is exposed.
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   */
  bindheaderoffset?: (e: RefreshHeaderOffsetEvent) => void;

  /**
   * Triggered when `<refresh-header>` state changes.
   * @Android
   * @iOS
   * @Harmony
   */
  bindrefreshstatechange?: (e: RefreshStateChangeEvent) => void;
}

export type RefreshStartRefreshEvent = BaseEvent<'bindstartrefresh', RefreshStartRefresh>;
export type RefreshHeaderOffsetEvent = BaseEvent<'bindheaderoffset', RefreshHeaderOffset>;
export type RefreshStateChangeEvent = BaseEvent<'bindrefreshstatechange', RefreshStateChange>;


export interface RefreshHeaderProps extends StandardProps {}

/**
 * Called after the startrefresh event to end the refresh state, making `<refresh-header>` rebound.
 * @Android
 * @iOS
 * @Harmony
 * @Web
 * @Clay
 */
export interface RefreshFinishRefreshMethod extends BaseMethod {
  method: 'finishRefresh';
}



/**
 * When enable-refresh is true, call this method to expose the entire `<refresh-header>`, triggering the startrefresh event, after which `<refresh-header>` will attach to the top edge of the refresh's viewport.
 * @Android
 * @iOS
 * @Harmony
 * @Web
 * @Clay
 */
export interface RefreshAutoStartRefreshMethod extends BaseMethod {
  method: 'autoStartRefresh';
}

export type RefreshUIMethods =
  | RefreshFinishRefreshMethod
  | RefreshAutoStartRefreshMethod;
