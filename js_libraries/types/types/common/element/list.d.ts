// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, EventHandler } from '../events';
import { StandardProps } from '../props';

/**
 * The scroll state of the list.
 * SCROLL_STATE_STOP: In an idle, settled state.
 * SCROLL_STATE_DRAGGING: Is currently being dragged by outside input such as user touch.
 * SCROLL_STATE_DECELERATE: Is currently animating to a final position triggered by fling.
 * SCROLL_STATE_ANIMATION: Is currently animating to a final position triggered by smooth scroll.
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
export enum ListScrollState {
  SCROLL_STATE_STOP = 1,
  SCROLL_STATE_DRAGGING = 2,
  SCROLL_STATE_DECELERATE = 3,
  SCROLL_STATE_ANIMATION = 4,
}

/**
 * The source of the scroll event.
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
export enum ListEventSource {
  DIFF = 0,
  LAYOUT = 1,
  SCROLL = 2,
}

/**
 * The strategy to search the reference anchor for removed on-screen children.
 * NONE: Do not search the reference anchor.
 * TO_START: Search the reference anchor to the start of the list.
 * TO_END: Search the reference anchor to the end of the list.
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
export enum ListSearchRefAnchorStrategy {
  NONE = 0,
  TO_START = 1,
  TO_END = 2,
}

export interface ListAttachedCell {
  /**
   * id of list item
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  id: string;
  /**
   * item-key of list item
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  itemKey: string;
  /**
   * index of list item
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  index: number;
  /**
   * left position of list item relative to list, in px
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  left: number;
  /**
   * top position of list item relative to list, in px
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  top: number;
  /**
   * right position of list item relative to list, in px
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  right: number;
  /**
   * bottom position of list item relative to list, in px
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  bottom: number;
}

export interface ListScrollInfo {
  /**
   * Horizontal scroll offset since the last scroll, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  deltaX: number;
  /**
   * Vertical scroll offset since the last scroll, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  deltaY: number;
  /**
   * Current horizontal scroll offset, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  scrollLeft: number;
  /**
   * Current vertical scroll offset, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  scrollTop: number;
  /**
   * Current content area width, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  scrollWidth: number;
  /**
   * Current content area height, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  scrollHeight: number;
  /**
   * list width, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  listWidth: number;
  /**
   * list height, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  listHeight: number;
  /**
   * Source of the event.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  eventSource: ListEventSource;
  /**
   * Information of the currently rendering list items.
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  attachedCells: ListAttachedCell[];
}

export interface ListItemInfo {
  /**
   * Height of the list item, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  height: number;
  /**
   * Width of the list item, in px.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  width: number;
  /**
   * ItemKey of the list-item.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  itemKey: string;
  /**
   * When the list is running in multi-thread mode, this property indicates whether the item is currently being rendered and has not finished rendering. In this state, the item will have a default height that is the same as the list's height or an estimated height if one has been set.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  isBinding: boolean;
  /**
   * The x coordinate of the item's top-left corner, in px. The axis is based on the total length of the content rather than the visible area or screen.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  originX: number;
  /**
   * The y coordinate of the item's top-left corner, in px. The axis is based on the total length of the content rather than the visible area or screen.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  originY: number;
  /**
   * Show if the list-item is currently being updated in this layout.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  updated: boolean;
}

export interface ListScrollStateInfo {
  /**
   * scroll state
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  state: ListScrollState;
}

export interface ListSnapInfo {
  /**
   * The index of the node that will be paginated to.
   * @iOS
   * @Android
   * @Harmony
   */
  position: number;
  /**
   * Current scrolling left offset, in px.
   * @iOS
   * @Android
   * @Harmony
   */
  currentScrollLeft: number;
  /**
   * Current scrolling top offset, in px.
   * @iOS
   * @Android
   * @Harmony
   */
  currentScrollTop: number;
  /**
   * Snap scrolling left offset, in px.
   * @iOS
   * @Android
   * @Harmony
   */
  targetScrollLeft: number;
  /**
   * Snap scrolling top offset, in px.
   * @iOS
   * @Android
   * @Harmony
   */
  targetScrollTop: number;
}

export interface ListLayoutCompleteInfo {
  /**
   * Connected to a setState.
   * @defaultValue -1
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'layout-id'?: number;

  /**
   * The diffResult of current layout change. If this diffResult is undefined, it means that the layout is triggered by a list-item update.
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  diffResult?: {
    insertions: number[];
    removals: number[];
    // eslint-disable-next-line
    move_from: number[];
    // eslint-disable-next-line
    move_to: number[];
    // eslint-disable-next-line
    update_from: number[];
    // eslint-disable-next-line
    update_to: number[];
  };

  /**
   * Cell info of the list before layout.
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  visibleItemBeforeUpdate?: ListItemInfo[];

  /**
   * Cell info of the list after layout.
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  visibleItemAfterUpdate?: ListItemInfo[];

  /**
   * The scroll info after layout.
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  scrollInfo: ListScrollInfo;
}

export interface ListScrollEvent extends BaseEvent<'scroll', ListScrollInfo> {}

export interface ListScrollToLowerEvent extends BaseEvent<'scrolltolower', ListScrollInfo> {}

export interface ListScrollToUpperEvent extends BaseEvent<'scrolltoupper', ListScrollInfo> {}

export interface ListScrollStateChangeEvent extends BaseEvent<'scrollstatechange', ListScrollStateInfo> {}

export interface ListSnapEvent extends BaseEvent<'snap', ListSnapInfo> {}

export interface ListLayoutCompleteEvent extends BaseEvent<'layoutcomplete', ListLayoutCompleteInfo> {}

export interface ListItemSnapAlignment {
  /**
   * Paging factor, 0.0 means align to the top, while 1.0 means align to the bottom.
   * @Android
   * @iOS
   * @Harmony
   */
  factor: number;

  /**
   * In addition to the parameter of factor, an offset for alignment can be set, in px.
   * @Android
   * @iOS
   * @Harmony
   */
  offset: number;
}

/**
 * list. A list does not support adaptive height and the 'height' property must be specified for the <list> tag in order to set its height.
 */
export interface ListProps extends StandardProps {
  /**
   * Sets the scrolling direction and layout direction.
   * @defaultValue 'vertical'
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'scroll-orientation'?: 'vertical' | 'horizontal';

  /**
   * The number of columns or rows for the list, and only effective when list-type is 'flow' or 'waterfall'.
   * @defaultValue 1
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'span-count'?: number;

  /**
   * layout type of the list
   * @defaultValue  'single'
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'list-type'?: 'single' | 'flow' | 'waterfall';

  /**
   * Whether to allow scrolling for a list.
   * @defaultValue true
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'enable-scroll'?: boolean;

  /**
   * Indicates whether list can achieve nested scrolling with other scrollable containers. When enabled, the inner container scrolls first, followed by the outer container.
   * @defaultValue  false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'enable-nested-scroll'?: boolean;

  /**
   * Declared on the list to control whether the list as a whole is allowed to be sticky at the top or bottom.
   * @defaultValue  false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  sticky?: boolean;

  /**
   * The offset distance from the top or bottom of list for sticky positioning, in px.
   * @defaultValue  0
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'sticky-offset'?: number;

  /**
   * Control whether list should produce bounces effect when scrolling past its content boundaries.
   * @defaultValue  true
   * @iOS
   * @Harmony
   * @PC
   */
  bounces?: boolean;

  /**
   * Specifies the node position to which list automatically scrolls after rendering.
   * @defaultValue  0
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'initial-scroll-index'?: number;

  /**
   * Controls whether the scroll event callback parameters include the position information of the currently rendering node.
   * @defaultValue  false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'need-visible-item-info'?: boolean;

  /**
   * When scrolling down, when the number of remaining displayable child nodes at the bottom is first less than lower-threshold-item-count, a scrolltolower event is triggered. If lower-threshold-item-count is specified, lower-threshold is not effective.
   * @defaultValue  0
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'lower-threshold-item-count'?: number;

  /**
   * When scrolling up, when the number of remaining displayable child nodes at the top is first less than upper-threshold-item-count, a scrolltoupper event is triggered. If upper-threshold-item-count is specified, upper-threshold is not effective.
   * @defaultValue  0
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'upper-threshold-item-count'?: number;

  /**
   * Specify the callback frequency of the scroll event by passing in a value, which specifies how many milliseconds (ms) list will call the scroll callback event during scrolling.
   * @defaultValue  200
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'scroll-event-throttle'?: number;

  /**
   * Enable the paging effect, and after each scroll, the list-item will stop at a specified position. For list-type:single only.
   * @Android
   * @iOS
   * @Harmony
   */
  'item-snap'?: ListItemSnapAlignment;

  /**
   * Controls whether the layoutcomplete event includes the node layout information before and after this layout, and the list Diff information that triggered this layout, and the current list scroll state information.
   * @defaultValue  false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'need-layout-complete-info'?: boolean;

  /**
   * Used to mark the unique identifier for this data source update, which will be returned in the layoutcomplete event callback.
   * @defaultValue  -1
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'layout-id'?: number;

  /**
   * This attribute controls the number of nodes outside list that are preloaded.
   * @defaultValue  0
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'preload-buffer-count'?: number;

  /**
   * Determine the strategy to search the reference anchor for removed on-screen children. If set to TO_START or TO_END, we will update reference anchor child for all removed on-screen children before consuming diff info. And in list layout pass, if no valid anchor child can be found from all on screen children, we use the reference anchor child in layout.
   * @defaultValue ListSearchRefAnchorStrategy.NONE
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'experimental-search-ref-anchor-strategy'?: ListSearchRefAnchorStrategy;

  /**
   * Whether to display the scroll bar of the list, with false on Harmony platform and true on other platforms.
   * @defaultValue undefined
   * @iOS
   * @Harmony
   * @PC
   */
  'scroll-bar-enable'?: boolean;

  /**
   * The property to control whether recycle sticky item. The default value is true if sdk version >= 3.4 and false if sdk version < 3.4.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  'experimental-recycle-sticky-item'?: boolean;

  /**
   * When the content size of a component is smaller than the component itself, decide whether to enable scrolling.
   * @defaultValue false
   * @Harmony
   * @since 3.4
   */
  'harmony-scroll-edge-effect'?: boolean;

  /**
   * Scroll event.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindscroll?: EventHandler<ListScrollEvent>;

  /**
   * Callback function for scrolling to the top.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindscrolltoupper?: EventHandler<ListScrollToUpperEvent>;

  /**
   * Callback function for scrolling to the bottom.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindscrolltolower?: EventHandler<ListScrollToLowerEvent>;

  /**
   * This callback function is triggered when the scrolling state of the list changes.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindscrollstatechange?: EventHandler<ListScrollStateChangeEvent>;

  /**
   * This callback function will be triggered when the first screen of the list has finished rendering. On iOS, this callback function will be triggered when list has an update.
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindlayoutcomplete?: EventHandler<ListLayoutCompleteEvent>;

  /**
   * Snap callback
   * @since 2.16
   * @Android
   * @iOS
   * @Harmony
   */
  bindsnap?: EventHandler<ListSnapEvent>;
}

export interface ScrollToPositionParams {
  /**
   * Specify the index of the node to scroll to, with a value range of [0, itemCount).
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  index: number;

  /**
   * Specify the alignment method when scrolling to a target position.
   *  "bottom": Scroll to make the node fully visible in the list and align the bottom of the node with the bottom of the list.
   *  "top": Scroll to make the node fully visible in the list and align the top of the node with the top of the list.
   *  "middle": Scroll to make the node fully visible in the list and align the center of the node with the center of the list. This is only supported in LynxSDK 2.12 and above.
   * @defaultValue 'top'
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  alignTo?: 'bottom' | 'top' | 'middle';

  /**
   * Align the node with alignTo, and then move the node downward by a length of offset.
   * @defaultValue 0
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  offset?: number;

  /**
   * Specify the unique identifier of the node to scroll to. If `item-key` is specified, `index` will be ignored.
   * @Android
   * @iOS
   * @Harmony
   * @since 3.6
   */
  itemKey?: string;

  /**
   * Enable scroll animation during scroll.
   * @defaultValue  false
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  smooth?: boolean;
}

/**
 * Scroll the list to the specified position.
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
export interface ScrollToPositionMethod extends BaseMethod {
  method: 'scrollToPosition';

  params: ScrollToPositionParams;
}

/**
 * Automatic scrolling.
 * @Android
 * @iOS
 * @Harmony
 */
export interface AutoScrollMethod extends BaseMethod {
  method: 'autoScroll';
  params: {
    /**
     * Start/stop automatic scrolling.
     * @defaultValue false
     * @Android
     * @iOS
     * @Harmony
     */
    start?: boolean;

    /**
     *  The distance of each second's scrolling, which supports positive and negative values. The unit of distance can be "px", "rpx", "ppx", or null (for iOS, the value must be greater than 1/screen.scale px).
     * @Android
     * @iOS
     * @Harmony
     */
    rate: string;

    /**
     * Whether to stop automatically when sliding to the bottom.
     * @defaultValue true
     * @Android
     * @iOS
     * @Harmony
     */
    autoStop?: boolean;
  };
}

/**
 * Get the info of the currently displayed list item.
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
export interface GetVisibleCellsMethod extends BaseMethod {
  method: 'getVisibleCells';
}

/**
 * Scroll by specified offset
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
export interface ScrollByMethod extends BaseMethod {
  method: 'scrollBy';

  params: {
    /**
     * Offset to scroll, in px.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    offset: number;
  };
}

export type ListUIMethods = ScrollToPositionMethod | AutoScrollMethod | GetVisibleCellsMethod | ScrollByMethod;
