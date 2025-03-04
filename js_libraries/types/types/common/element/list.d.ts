// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { AppearanceEvent, BaseEvent, BaseMethod, EventHandler } from '../events';
import { StandardProps } from '../props';

export enum ListEventSource {
  DIFF = 0,
  LAYOUT = 1,
  SCROLL = 2,
}

/** The enum value of list batch render strategy.
 * DEFAULT: The default value with the normal list's rendering process.
 * BATCH_RENDER: Use batch render and the list will batch render multiple items.
 * ASYNC_RESOLVE_PROPERTY: Use batch render and enable async resolve property of the list item.
 * ASYNC_RESOLVE_PROPERTY_AND_ELEMENT_TREE (Recommended): Use batch render and enable async resolve property and element tree of the list item.
 * @Android
 * @iOS
 */
export enum BatchRenderStrategy {
  DEFAULT = 0,
  BATCH_RENDER = 1,
  ASYNC_RESOLVE_PROPERTY = 2,
  ASYNC_RESOLVE_PROPERTY_AND_ELEMENT_TREE = 3
}

export interface ListScrollInfo {
  /**
   * scroll left from start
   * @Android
   * @iOS
   */
  scrollLeft: number;
  /**
   * scroll top from start
   * @Android
   * @iOS
   */
  scrollTop: number;
  /**
   * scroll content width
   * @Android
   * @iOS
   */
  scrollWidth: number;
  /**
   * scroll content height
   * @Android
   * @iOS
   */
  scrollHeight: number;
  /**
   * list width
   * @Android
   * @iOS
   */
  listWidth: number;
  /**
   * list height
   * @Android
   * @iOS
   */
  listHeight: number;
  /**
   * X-axis scroll delta for this scroll. It's always 0 in some non-scroll related events.
   * @Android
   * @iOS
   */
  deltaX: number;
  /**
   * Y-axis scroll delta for this scroll. It's always 0 in some non-scroll related events.
   * @Android
   * @iOS
   */
  deltaY: number;
  /**
   * Source of the event.
   * @Android
   * @iOS
   */
  eventSource: ListEventSource;
}

export interface ListScrollEvent extends BaseEvent<'scroll', ListScrollInfo> {}
export interface ListScrollToLowerEvent extends BaseEvent<'scrolltolower', ListScrollInfo> {}
export interface ListScrollToUpperEvent extends BaseEvent<'scrolltoupper', ListScrollInfo> {}
export interface ListScrollToUpperEdgeEvent extends BaseEvent<'scrolltoupperedge', ListScrollInfo> {}
export interface ListScrollToLowerEdgeEvent extends BaseEvent<'scrolltoloweredge', ListScrollInfo> {}
export interface ListScrollToNormalStateEvent extends BaseEvent<'scrolltonormalstate', ListScrollInfo> {}

export interface ListItemSnapAlignment {
  /**
   * Paging factor, 0.0 means align to the top, while 1.0 means align to the bottom.
   * @Android
   * @iOS
   */
  factor: number;
  /**
   * In addition to the parameter of factor, an offset for alignment can be set, in px.
   * @Android
   * @iOS
   */
  offset: number;
}

export interface ListItemInfo {
  /**
   * Height of the item, in px.
   * @Android
   * @iOS
   * @H
   */
  height: number;
  /**
   * Width of the item, in px.
   * @Android
   * @iOS
   * @H
   */
  width: number;
  /**
   * ItemKey of the list-item.
   * @Android
   * @iOS
   * @H
   */
  itemKey: string;
  /**
   * When the list is running in multi-thread mode, this property indicates whether the item is currently being rendered and has not finished rendering. In this state, the item will have a default height that is the same as the list's height or an estimated height if one has been set.
   * @Android
   * @iOS
   * @H
   */
  isBinding: boolean;
  /**
   * The x coordinate of the item's top-left corner, in px. The axis is based on the total length of the content rather than the visible area or screen.
   * @Android
   * @iOS
   * @H
   */
  originX: number;
  /**
   * The y coordinate of the item's top-left corner, in px. The axis is based on the total length of the content rather than the visible area or screen.
   * @Android
   * @iOS
   * @H
   */
  originY: number;
  /**
   * Show if the list-item is currently being updated in this layout.
   * @Android
   * @iOS
   * @H
   */
  updated: boolean;
}

export interface ListAttachedCell {
  /**
   * id
   * @iOS
   * @Android
   */
  id: string;
  /**
   * bottom
   * @iOS
   * @Android
   */
  bottom: number;
  /**
   * top
   * @iOS
   * @Android
   */
  top: number;
  /**
   * left
   * @iOS
   * @Android
   */
  left: number;
  /**
   * right
   * @iOS
   * @Android
   */
  right: number;
  /**
   * postion
   * @iOS
   * @Android
   */
  position: number;
}

export interface ListDetail {
  /**
   * scroll top from start
   * @iOS
   * @Android
   */
  scrollTop: number;
  /**
   * scrolling delta at y axis
   * @iOS
   * @Android
   */
  deltaY: number;
  /**
   * attached cells
   * @iOS
   * @Android
   */
  attachedCells: ListAttachedCell[];
}

export interface ListEvent<T> extends BaseEvent<T, ListDetail> {}

export enum ListScrollState {
  SCROLL_STATE_STOP = 1,
  SCROLL_STATE_DRAGGING = 2,
  SCROLL_STATE_DECELERATE = 3,
}

export interface ListScrollStateChangeEvent extends BaseEvent<'scrollstatechange', {}> {
  detail: {
    /**
     * scroll state
     * @iOS
     * @Android
     */
    state: ListScrollState;
  };
}

export interface ListLayoutFinishEvent extends BaseEvent<'layoutcomplete', {}> {
  detail: {
    /**
     * timestamp, in ms
     * @iOS
     * @Android
     */
    timestamp: number;
    /**
     * All the component names after this layout
     * @iOS
     * @Android
     */
    cells: string[];
  };
}

interface ListDebugInfoEvent extends BaseEvent<'debuginfo', {}> {
  detail: {};
}

export interface ListSnapEvent extends BaseEvent<'snap', {}> {
  detail: {
    /**
     * Will snap to a position.
     * @iOS
     * @Android
     */
    position: number;
    /**
     * Current scrolling left offset, in px.
     * @iOS
     * @Android
     */
    currentScrollLeft: number;
    /**
     * Current scrolling top offset, in px.
     * @iOS
     * @Android
     */
    currentScrollTop: number;
    /**
     * Snap scrolling left offset, in px.
     * @iOS
     * @Android
     */
    targetScrollLeft: number;
    /**
     * Snap scrolling top offset, in px.
     * @iOS
     * @Android
     */
    targetScrollTop: number;
  };
}

export interface LayoutCompleteEvent extends BaseEvent<'layoutcomplete', {}> {
  /**
   * Layout details
   * @iOS
   * @Android
   * @H
   * @Since 2.15
   */
  detail: {
    /**
     * Connected to a setState.
     * @iOS
     * @Android
     * @H
     */
    'layout-id': number;
    /**
     * The diffResult of current layout change. If this diffResult is undefined, it means that the layout is triggered by a list-item update.
     * @iOS
     * @Android
     * @H
     */
    diffResult?: {
      insertions: number[];
      // eslint-disable-next-line
      move_from: number[];
      // eslint-disable-next-line
      move_to: number[];
      removals: number[];
      // eslint-disable-next-line
      update_from: number[];
      // eslint-disable-next-line
      update_to: number[];
    };
    /**
     * Cell info of the list after layout.
     * @iOS
     * @Android
     * @H
     */
    visibleCellsAfterUpdate?: ListItemInfo[];
    /**
     * Cell info of the list before layout.
     * @iOS
     * @Android
     * @H
     */
    visibleCellsBeforeUpdate?: ListItemInfo[];
    /**
     * The scroll info after layout.
     * @iOS
     * @Android
     * @H
     */
    scrollInfo: ListScrollInfo;
    /**
     * The unit of event's parameters.
     * @iOS
     * @Android
     * @H
     */
    eventUnit: string;
  };
}

/**
 * list. A list does not support adaptive height and the 'height' property must be specified for the <list> tag in order to set its height.
 */
export interface ListProps extends StandardProps {
  /**
   * Whether to display the scrollbar of the list
   * @defaultValue true
   * @iOS
   */
  'scroll-bar-enable'?: boolean;

  /**
   * Control whether list should produce bounces effect when scrolling past its content boundaries.
   * @defaultValue true
   * @iOS
   */
  bounces?: boolean;

  /**
   * z-index is undefined by default. When enabled, z-index will be assigned in ascending order for each item.
   * @defaultValue false
   * @iOS
   */
  'ios-index-as-z-index'?: boolean;

  /**
   * To solve the flickering issue of list, it's recommended to enable it.
   * @defaultValue false
   * @iOS
   */
  'ios-fixed-content-offset'?: boolean;

  /**
   * To solve the flickering issue of list, it's only necessary to enable when using bottom-up layout. It's recommended to understand the principle before using.
   * @defaultValue false
   * @iOS
   */
  'ios-fix-offset-from-start'?: boolean;

  /**
   * Interrupt List rendering switch. It is usually used when creating drag-and-drop effects to prevent the list from refreshing child nodes.
   * @defaultValue false
   * @Android
   */
  'no-invalidate'?: boolean;

  /**
   * Under asynchronous layout, it is recommended to enable this property if a child node renders blank.
   * @defaultValue false
   * @Android
   */
  'component-init-measure'?: boolean;

  /**
   * Control whether dataSet can be used as the unique identifier. It will call RecyclerView.setHasStableIds() internally.
   * @defaultValue false
   * @Android
   */
  'android-diffable'?: boolean;

  /**
   * ”upperBounce“ can only forbid bounces effect at top or left (right for RTL); ”lowerBounce“ can only forbid bounces effect at bottom or right (left for RTL). iOS uses bounces to implement refresh-view. This prop can be used when you need a refresh-view without bounces on the other side.
   * @defaultValue "none"
   * @iOS
   */
  'ios-forbid-single-sided-bounce'?: 'upperBounce' | 'lowerBounce' | 'none';

  /**
   * Control whether list should produce over-scroll effect when scrolling past its content boundaries.
   * @defaultValue false
   * @Android
   */
  'over-scroll'?: boolean;

  /**
   * Whether to allow scrolling for a list.
   * @defaultValue true
   * @Android
   * @iOS
   */
  'enable-scroll'?: boolean;

  /**
   * Whether to allow scrolling for a list. It's recommended to enable 'enable-scroll=true' and disable 'touch-scroll' on Android.
   * @defaultValue true
   * @Android
   */
  'touch-scroll'?: boolean;

  /**
   * layout type of the list
   * @defaultValue  "single"
   * @Android
   * @iOS
   */
  'list-type'?: 'single' | 'flow' | 'waterfall';

  /**
   * The number of columns for a list, only effective when list-type is 'flow' or 'waterfall'.
   * @defaultValue 0
   * @Android
   * @iOS
   */
  'column-count'?: number;

  /**
   * Control the horizontal and vertical scrolling direction of a list. If it is horizontal scrolling, you need to also set the CSS layout property 'style="display:linear; linear-orientation:horizontal"
   * @defaultValue undefined
   * @Android
   * @iOS
   */
  'vertical-orientation'?: boolean;

  /**
   * Replacement of vertical-orientation
   * @since 3.0
   * @iOS
   * @Android
   * @H
   */
  'scroll-orientation'?: 'vertical' | 'horizontal';

  /**
   * Control the list to fill the elements from the bottom. It is used in reverse layout scenarios such as message list.
   * @defaultValue false
   * @Android
   */
  'android-stack-from-end'?: boolean;

  /**
   * Control the switch of RTL (Right-to-Left) mode, the order of vertical two-column layout will also be mirrored and upside-down.
   * @defaultValue false
   * @Android
   * @iOS
   */
  'enable-rtl'?: boolean;

  /**
   * The ceiling or floor effect is not compatible with flatten.
   * @defaultValue  false
   * @Android
   * @iOS
   */
  sticky?: boolean;

  /**
   * The distance between the ceiling or floor position and the top or bottom of the list, in pixels.
   * @defaultValue 0
   * @Android
   * @iOS
   */
  'sticky-offset'?: number;

  /**
   * When enabled, the upper or lower element will also be shifted together with the bounces effect.
   * @defaultValue false
   * @iOS
   */
  'sticky-with-bounces'?: boolean;

  /**
   * Rollback to the older version's sticky feature.
   * @defaultValue  false
   * @Android
   * @iOS
   * @deprecated since 2.14
   */
  'use-old-sticky'?: boolean;

  /**
   * To solve the conflict between list and x-refresh-view
   * @defaultValue false
   * @experimental
   * @iOS
   * @since 2.18
   */
  'experimental-disable-filter-scroll'?: boolean;

  /**
   * Limit the inertia scrolling distance of asynchronous list to this value multiplied by the height (width) of the list, in order to reduce the chance of white screen. When it is set to 'auto', it will automatically calculate the rendering distance in preload-buffer when asynchronous list is enabled to ensure there is no white screen, but the inertia scrolling experience may be abnormal.
   * @defaultValue undefined
   * @experimental
   * @Android
   * @iOS
   * @since 2.17
   */
  'experimental-max-fling-distance-ratio'?: 'auto' | number;

  /**
   * Attempt to increase the CPU frequency if a gesture is set to the list
   * @defaultValue false
   * @experimental
   * @iOS
   * @since 2.18
   */
  'experimental-ios-gesture-frequency-increasing'?: boolean;

  /**
   * On iOS, force-can-scroll should be used with ios-block-gesture-class and ios-recognized-view-tag. Can be used alone on Android.
   * @defaultValue false
   * @iOS
   * @Android
   * @since 3.1
   */
  'force-can-scroll'?: boolean;

  /**
   * Whether to scroll to the top when the iOS status bar is clicked. Note that even if it is set to true, it may not scroll to the top. In such cas, you need to check the behavior of other scrollable components.{@link https://developer.apple.com/documentation/uikit/uiscrollview/1619421-scrollstotop?language=objc | Reference }
   * @defaultValue undefined
   * @iOS
   * @since 2.16
   */
  'ios-scrolls-to-top'?: boolean;

  /**
   * There are stability issues. Please use `<Component align-height={true}/>` instead. It is not recommended to use.
   * @defaultValue false
   * @iOS
   * @deprecated since 2.10
   */
  'ios-enable-align-height'?: boolean;

  /**
   * Deprecated
   * @defaultValue true
   * @iOS
   * @deprecated since 2.10
   */
  'ios-no-recursive-layout'?: boolean;

  /**
   * Deprecated
   * @defaultValue true
   * @iOS
   * @deprecated since 2.10
   */
  'ios-force-reload-data'?: boolean;

  /**
   * Deprecated
   * @defaultValue true
   * @iOS
   * @deprecated since 2.10
   */
  'ios-update-valid-layout'?: boolean;

  /**
   * Deprecated
   * @defaultValue false
   * @iOS
   * @deprecated since 2.10
   */
  'ios-enable-adjust-offset-for-selfsizing'?: boolean;

  /**
   * In Android, this attribute controls whether the list uses the RecyclerView's ItemDecoration to implement the main-axis-gap and cross-axis-gap.
   * @defaultValue false
   * @Android
   */
  'android-enable-gap-item-decoration'?: boolean;

  /**
   * In some Android scenarios, the element that is currently being ceilinged or floored may not be correctly measured and laid out, resulting in blank rendering. Therefore, it is recommended to enable it.
   * @defaultValue false
   * @Android
   */
  'android-trigger-sticky-layout'?: boolean;

  /**
   * Specify this property as 'default' to enable update animation for the list.
   * @defaultValue  "none"
   * @Android
   * @iOS
   */
  'update-animation'?: 'none' | 'default';

  /**
   * Indicates whether to use the new exposure strategy. The new exposure strategy handles the situation where the node is covered, such as the list nested in x-viewpager-ng or x-swiper. The performance of the new exposure strategy is slightly worse than the old one, but the result is more accurate.
   * @defaultValue  false
   * @Android
   */
  'enable-new-exposure-strategy'?: boolean;

  /**
   * Allows notification to send disappear event. It is recommended to enable.
   * @defaultValue false
   * @Android
   */
  'enable-disappear'?: boolean;

  /**
   * Enable asynchronous strategy
   * @defaultValue false
   * @Android
   * @iOS
   */
  'enable-async-list'?: boolean;

  /**
   * Cache the size when rendering components in child nodes of List. It is only allowed to enable under asynchronous mode.
   * @defaultValue false
   * @Android
   */
  'enable-size-cache'?: boolean;

  /**
   * The maximum scrolling speed of the list, with a value between [0,1].
   * @defaultValue 1.0
   * @Android
   */
  'max-fling-velocity-percent'?: number;

  /**
   * When scrolling down, when the number of remaining displayable child nodes at the bottom is first less than lower-threshold-item-count, a scrolltolower event is triggered. If lower-threshold-item-count is specified, lower-threshold is not effective.
   * @defaultValue  none
   * @Android
   * @iOS
   */
  'lower-threshold-item-count'?: number;

  /**
   * When scrolling up, when the number of remaining displayable child nodes at the top is first less than upper-threshold-item-count, a scrolltoupper event is triggered. If upper-threshold-item-count is specified, upper-threshold is not effective.
   * @defaultValue  none
   * @Android
   * @iOS
   */
  'upper-threshold-item-count'?: number;

  /**
   * During a single sliding process, when the upper_distance is first smaller than the value specified by upper-threshold, a scrolltoupper event is triggered. When upper_distance is already smaller than the value specified by upper-threshold, the scrolltoupper event will no longer be triggered.
   * @defaultValue  50
   * @Android
   * @iOS
   */
  'upper-threshold'?: number;

  /**
   * During a single sliding process, when the lower_distance is first smaller than the value specified by lower-threshold, a scrolltolower event is triggered. When lower_distance is already smaller than the value specified by lower-threshold, the scrolltolower event will no longer be triggered.
   * @defaultValue  50
   * @Android
   * @iOS
   */
  'lower-threshold'?: number;

  /**
   * Control whether the 'attachedCells' is included in the scroll event callback parameters.
   * @defaultValue  false
   * @Android
   * @iOS
   */
  'needs-visible-cells'?: boolean;

  /**
   * Specify the callback frequency of the scroll event by passing in a value, which specifies how many milliseconds (ms) <list> will call the scroll callback event during scrolling.
   * @defaultValue  200
   * @Android
   * @iOS
   */
  'scroll-event-throttle'?: number;

  /**
   * List scrolling event refactoring switch. It is recommended to enable it.
   * @defaultValue false
   * @iOS
   */
  'ios-scroll-emitter-helper'?: boolean;

  /**
   * Solve the problem of inaccurate sliding distance in the list scrolling event callback. It is recommended to enable it.
   * @defaultValue false
   * @Android
   */
  'android-new-scroll-top'?: boolean;

  /**
   * Allow frontend code to specify the automatic positioning of the list after rendering, only valid for single columns.
   * @defaultValue 0
   * @Android
   * @iOS
   */
  'initial-scroll-index'?: number;

  /**
   * The difference of double-sided page turning effect: On Android, it controls scrolling one child node at a time; on iOS, it controls scrolling one list viewport distance at a time.
   * @defaultValue false
   * @Android
   * @iOS
   * @experimental
   */
  'paging-enabled'?: boolean;

  /**
   * Enable the paging effect, and after each scroll, the list-item will stop at a specified position. For list-type:single only/
   * @defaultValue undefined
   * @Android
   * @iOS
   * @experimental
   */
  'item-snap'?: ListItemSnapAlignment;

  /**
   * Notify the parent view not to intercept the event. Enable this when the list is in a client container (RecyclerView) and needs to be scrollable.
   * @defaultValue false
   * @Android
   */
  'android-preference-consume-gesture'?: boolean;

  /** scroll-view and list can be nested for scrolling. The scrolling of the child view triggered by a single gesture can drive the scrolling of the parent view when the inertial scrolling touches the edge.
   * @defaultValue false
   * @iOS
   */
  'enable-nested-scroll'?: boolean;

   
  /**
   * NestedScrollOptions for scrollForward
   * @since 3.0
   * @H
   */
  'temporary-nested-scroll-forward'?: 'selfOnly' | 'selfFirst'|'parentFirst'|'parallel';

  /**
   * NestedScrollOptions for scrollBackward
   * @since 3.0
   * @H
   */
  'temporary-nested-scroll-backward'?: 'selfOnly' | 'selfFirst'|'parentFirst'|'parallel' ;

  /**
   * Preload count.
   * @defaultValue  0
   * @Android
   * @iOS
   * @experimental
   */
  'preload-buffer-count'?: number;

  /**
   * Whether to enable the self-preloading strategy of the list system.
   * @defaultValue  0
   * @Android
   * @experimental
   */
  'android-enable-item-prefetch'?: boolean;

  /**
   * Determine whether the list is laid out from the top or from the bottom.
   * @defaultValue  'fromBegin'
   * @iOS
   * @experimental
   */
  'anchor-priority'?: 'fromBegin' | 'fromEnd';

  /**
   * When all nodes on the screen are deleted, find the anchor point outside the top/bottom area.
   * @defaultValue  'toTop'
   * @iOS
   * @experimental
   */
  'delete-regress-policy'?: 'toTop' | 'toBottom';

  /**
   * Using 'outside' can make the inserted element visible at the edge of the screen, and using 'inside' can make the inserted element invisible at the edge of the screen.
   * @defaultValue  'inside'
   * @iOS
   * @experimental
   */
  'insert-anchor-mode'?: 'outside' | 'inside';

  /**
   * "The dot element is positioned at the fully visible position (show) or at the point just invisible (hide), and no adjustment is made by default."
   * @defaultValue  'noAdjustment'
   * @iOS
   * @experimental
   */
  'anchor-visibility'?: 'noAdjustment' | 'show' | 'hide';

  /**
   * 'anchor-align-to-bottom' will keep the bottom of the anchor at the same distance from the visible area, while 'anchor-align-to-top' will keep the top of the anchor at the same distance from the visible area.
   * @defaultValue  'toTop'
   * @iOS
   * @experimental
   */
  'anchor-align'?: 'toBottom' | 'toTop';

  /**
   * From version 2.17 and above, you can directly use should-request-state-restore instead of internal-cell-appear-notification/internal-cell-prepare-for-reuse-notification/internal-cell-disappear-notification. The lifecycle of element layer reuse solves the problem of reuse status disorder in the UI layer. It is recommended to enable it when there is <scroll-view/> in the list item.
   * @defaultValue false
   * @deprecated 2.17
   * @iOS
   * @Android
   */
  'internal-cell-appear-notification'?: boolean;

  /**
   * From version 2.17 and above, you can directly use should-request-state-restore instead of internal-cell-appear-notification/internal-cell-prepare-for-reuse-notification/internal-cell-disappear-notification. The lifecycle of element layer reuse solves the problem of reuse status disorder in the UI layer. It is recommended to enable it when there is <scroll-view/> in the list item.
   * @defaultValue false
   * @deprecated 2.17
   * @iOS
   * @Android
   */
  'internal-cell-disappear-notification'?: boolean;

  /**
   * From version 2.17 and above, you can directly use should-request-state-restore instead of internal-cell-appear-notification/internal-cell-prepare-for-reuse-notification/internal-cell-disappear-notification. The lifecycle of element layer reuse solves the problem of reuse status disorder in the UI layer. It is recommended to enable it when there is <scroll-view/> in the list item.
   * @defaultValue false
   * @deprecated 2.17
   * @iOS
   * @Android
   */
  'internal-cell-prepare-for-reuse-notification'?: boolean;

  /**
   * The lifecycle of element layer reuse. It solves the problem of reuse status disorder in the UI layer. It is recommended to enable it when there is <scroll-view/> in the list item. It replaces internal-cell-appear-notification, internal-cell-disappear-notification, and internal-cell-prepare-for-reuse-notification.
   * @defaultValue false
   * @since 2.17
   * @iOS
   * @Android
   */
  'should-request-state-restore'?: boolean;

  /**
   * After the layout of the list is completed, preload the vdom of the remaining nodes in advance to improve the scrolling frame rate of the subsequent list.
   * @defaultValue false
   * @experimental
   * @iOS
   * @Android
   * @since 2.17
   */
  'experimental-enable-preload-section'?: boolean;

  /**
   * Since RecyclerView is a collection ViewGroup that includes virtual children (items that are in the Adapter but not visible in the UI), it employs a more involved focus search strategy that differs from other ViewGroups, which may lead to unexpected scroll behavior. This property is used to control whether the focus search strategy is enabled, and if set to false, the focus search strategy of RecyclerView will be disabled and directly return the current focused view in focusSearch method.
   * @defaultValue true
   * @Android
   * @since 2.17
   */
  'android-enable-focus-search'?: boolean;

  /**
   * Warning: It is not recommended to enable it in the production environment. Adjust the level of detail for bindlistdebugevent, with 0 meaning no output, 1 meaning outputting warnings, 2 meaning outputting information, and 3 meaning outputting detailed debug information.
   * @defaultValue 2
   * @iOS
   * @Android
   * @since 2.18
   */
  'list-debug-info-level'?: number;

  /**
   * The strategy of list batch rendering in Fiber arch. The value of this property can be set to the value of Enum BatchRenderStrategy.
   * @defaultValue BatchRenderStrategy.DEFAULT
   * @iOS
   * @Android
   * @since 3.1
   */
  'experimental-batch-render-strategy'?: BatchRenderStrategy;

  /**
   * Scroll event.
   * @Android
   * @iOS
   */
  bindscroll?: EventHandler<ListScrollEvent>;

  /**
   * Callback function for scrolling to the top.
   * @Android
   * @iOS
   */
  bindscrolltoupper?: EventHandler<ListScrollToUpperEvent>;

  /**
   * Callback function for scrolling to the bottom.
   * @Android
   * @iOS
   */
  bindscrolltolower?: EventHandler<ListScrollToLowerEvent>;

  /**
   * This callback function is triggered when the scrolling state of the list changes.
   * @Android
   * @iOS
   */
  bindscrollstatechange?: EventHandler<ListScrollStateChangeEvent>;
  /**
   * This callback function will be triggered when the first screen of the list has finished rendering. On iOS, this callback function will be triggered when list has an update.
   * @Android
   * @iOS
   */
  bindlayoutcomplete?: EventHandler<ListLayoutFinishEvent> | EventHandler<LayoutCompleteEvent>;
  /**
   * Warning: It is not recommended to enable it in the production environment. Output debug information for the list.
   * @since 2.18
   * @Android
   * @iOS
   */
  bindlistdebugevent?: EventHandler<ListDebugInfoEvent>;
  /**
   * scrollview scrolls to upper edge
   * @since 3.0
   * @iOS
   * @Android
   * @H
   */
  bindscrolltoupperedge?: EventHandler<ListScrollToUpperEdgeEvent>;

  /**
   * scrollview scrolls to lower edge
   * @since 3.0
   * @iOS
   * @Android
   * @H
   */
  bindscrolltoloweredge?: EventHandler<ListScrollToLowerEdgeEvent>;

  /**
   * scrollview scrolls to normal position. Not at upper or lower edge
   * @since 3.0
   * @iOS
   * @Android
   * @H
   */
  bindscrolltonormalstate?: EventHandler<ListScrollToNormalStateEvent>;
}

export interface ListItemProps extends StandardProps {
  /**
   * sticky top effect. Not compatible with flatten.
   * @defaultValue  false
   * @Android
   * @iOS
   */
  'sticky-top'?: boolean;

  /**
   * sticky bottom effect. Not compatible with flatten.
   * @defaultValue  false
   * @Android
   * @iOS
   */
  'sticky-bottom'?: boolean;

  /**
   * Preset height to control the placeholder height of the view while the list component has not finished rendering. The more accurately it is set, the less flickering the list will have.
   * @defaultValue undefined
   * @Android
   * @iOS
   */
  'estimated-height-px'?: number;

  /**
   * Preset size in main scroll axis to control the placeholder size of the view while the list component has not finished rendering. The more accurately it is set, the less flickering the list will have.
   * @defaultValue undefined
   * @Android
   * @iOS
   */
  'estimated-main-axis-size-px'?: number;

  /**
   * Snap callback
   * @Android
   * @iOS
   */
  bindsnap?: EventHandler<ListEvent<'snap'>>;

  /**
   * @Android
   * @iOS
   */
  'item-key': string;

  bindnodeappear?: EventHandler<AppearanceEvent>;
  bindnodedisappear?: EventHandler<AppearanceEvent>;
}

export interface ListRowProps extends ListItemProps {}

/**
 * Automatic scrolling
 * @Android
 * @iOS
 */
export interface AutoScrollMethod extends BaseMethod {
  method: 'autoScroll';
  params: {
    /**
     *  The distance of each second's scrolling, which supports positive and negative values. The unit of distance can be "px", "rpx", "ppx", or null (for iOS, the value must be greater than 1/screen.scale px).
     * @Android
     * @iOS
     */
    rate: string;
    /**
     * Start/stop automatic scrolling.
     * @defaultValue false
     * @Android
     * @iOS
     */
    start: boolean;
    /**
     * Whether to stop automatically when sliding to the bottom.
     * @defaultValue true
     * @Android
     * @iOS
     */
    autoStop: boolean;
  };
}

/**
 * Get the info of the currently displayed list item.
 * @Android
 * @iOS
 */
export interface GetVisibleCellsMethod extends BaseMethod {
  method: 'getVisibleCells';
}

/**
 * Get the index of the currently displayed list item.
 * @Android
 */
export interface GetVisibleItemsPositionsMethod extends BaseMethod {
  method: 'getVisibleItemsPositions';
}

/**
 * Remove current sticky view.
 * @Android
 */
export interface RemoveStickyViewMethod extends BaseMethod {
  method: 'removeStickyView';
}

/**
 * Init the cache of the list.
 * @Android
 */
export interface InitCacheMethod extends BaseMethod {
  method: 'initCache';
}

/**
 * Scroll the list to the specified position.
 * @Android
 * @iOS
 */
export interface ScrollToPositionMethod extends BaseMethod {
  method: 'scrollToPosition';
  params: {
    /**
     * From version 2.17 and above, it is recommended to use 'index' instead. Specify the index of the node to scroll to, with a value range of [0, itemCount).
     * @deprecated 2.17
     * @Android
     * @iOS
     */
    position: number;

    /**
     * Specify the index of the node to scroll to, with a value range of [0, itemCount).
     * @since 2.17
     * @Android
     * @iOS
     */
    index: number;

    /**
     * Specify the alignment method when scrolling to a target position.
     *  "none": Scroll to make the node fully visible in the list. If the node is above the visible range of the list, it will scroll to align the top of the node with the top of the list. If the node is below the visible range of the list, it will scroll to align the bottom of the node with the bottom of the list. If the node is within the visible range of the list, no movement will occur.
     *  "bottom": Scroll to make the node fully visible in the list and align the bottom of the node with the bottom of the list.
     *  "top": Scroll to make the node fully visible in the list and align the top of the node with the top of the list.
     *  "middle": Scroll to make the node fully visible in the list and align the center of the node with the center of the list. This is only supported in LynxSDK 2.12 and above.
     * @defaultValue none
     * @Android
     * @iOS
     */
    alignTo?: 'none' | 'bottom' | 'top' | 'middle';

    /**
     * Align the node with alignTo, and then move the node downward by a length of offset.
     * @Android
     * @iOS
     */
    offset?: number;

    /**
     * Enable scroll animation during scroll.
     * @defaultValue  false
     * @Android
     * @iOS
     */
    smooth?: boolean;

    /**
     * When enabled, it uses the refactored logic which helps to solve some existing bugs. It is recommended to enable it.
     * @defaultValue false
     * @iOS
     */
    useScroller?: boolean;
  };
}

export type ListUIMethods = ScrollToPositionMethod | AutoScrollMethod | GetVisibleCellsMethod | GetVisibleItemsPositionsMethod | RemoveStickyViewMethod | InitCacheMethod;
