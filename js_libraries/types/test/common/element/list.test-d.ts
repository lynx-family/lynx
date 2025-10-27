// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, it, assertType } from 'vitest';
import {
  ListProps,
  StandardProps,
  ListEventSource,
  ListScrollEvent,
  ListScrollToLowerEvent,
  ListScrollToUpperEvent,
  ListScrollStateChangeEvent,
  ListScrollState,
  ListSnapEvent,
  ListLayoutCompleteEvent,
} from '../../../types';
import { invoke } from '../test-utils';

declare const listProps: ListProps;

describe('ListItemProps type test', () => {
  it('should extend StandardProps', () => {
    assertType<StandardProps>(listProps);
  });

  it('check scroll-orientation', () => {
    assertType<ListProps>({
      'scroll-orientation': 'vertical',
    });
    assertType<ListProps>({
      'scroll-orientation': 'horizontal',
    });
    assertType<ListProps>({
      // @ts-expect-error
      'scroll-orientation': 'invalid-orientation',
    });
  });

  it('check list-type', () => {
    assertType<ListProps>({
      'list-type': 'single',
    });
    assertType<ListProps>({
      'list-type': 'flow',
    });
    assertType<ListProps>({
      // @ts-expect-error
      'list-type': 'invalid-type',
    });
  });

  it('check span-count', () => {
    assertType<ListProps>({
      'span-count': 2,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'span-count': '2',
    });
  });

  it('check enable-scroll', () => {
    assertType<ListProps>({
      'enable-scroll': true,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'enable-scroll': 'true',
    });
  });

  it('check enable-nested-scroll', () => {
    assertType<ListProps>({
      'enable-nested-scroll': false,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'enable-nested-scroll': 0,
    });
  });

  it('check sticky', () => {
    assertType<ListProps>({
      sticky: true,
    });
    assertType<ListProps>({
      // @ts-expect-error
      sticky: 'false',
    });
  });

  it('check sticky-offset', () => {
    assertType<ListProps>({
      'sticky-offset': 10,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'sticky-offset': '10px',
    });
  });

  it('check bounces', () => {
    assertType<ListProps>({
      bounces: true,
    });
    assertType<ListProps>({
      // @ts-expect-error
      bounces: 1,
    });
  });

  it('check initial-scroll-index', () => {
    assertType<ListProps>({
      'initial-scroll-index': 5,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'initial-scroll-index': '5',
    });
  });

  it('check need-visible-item-info', () => {
    assertType<ListProps>({
      'need-visible-item-info': true,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'need-visible-item-info': 'true',
    });
  });

  it('check lower-threshold-item-count', () => {
    assertType<ListProps>({
      'lower-threshold-item-count': 3,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'lower-threshold-item-count': false,
    });
  });

  it('check upper-threshold-item-count', () => {
    assertType<ListProps>({
      'upper-threshold-item-count': 3,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'upper-threshold-item-count': '3',
    });
  });

  it('check scroll-event-throttle', () => {
    assertType<ListProps>({
      'scroll-event-throttle': 200,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'scroll-event-throttle': '200ms',
    });
  });

  it('check item-snap', () => {
    assertType<ListProps>({
      'item-snap': { factor: 0.5, offset: 10 },
    });
    assertType<ListProps>({
      // @ts-expect-error - factor should be a number
      'item-snap': { factor: '0.5', offset: 10 },
    });
    assertType<ListProps>({
      // @ts-expect-error - missing factor property
      'item-snap': { offset: 10 },
    });
  });

  it('check need-layout-complete-info', () => {
    assertType<ListProps>({
      'need-layout-complete-info': true,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'need-layout-complete-info': 1,
    });
  });

  it('check layout-id', () => {
    assertType<ListProps>({
      'layout-id': 12345,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'layout-id': 'id-123',
    });
  });

  it('check preload-buffer-count', () => {
    assertType<ListProps>({
      'preload-buffer-count': 5,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'preload-buffer-count': true,
    });
  });

  it('check scroll-bar-enable', () => {
    assertType<ListProps>({
      'scroll-bar-enable': false,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'scroll-bar-enable': 'false',
    });
  });

  it('check experimental-recycle-sticky-item', () => {
    assertType<ListProps>({
      'experimental-recycle-sticky-item': true,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'experimental-recycle-sticky-item': 0,
    });
  });

  it('check harmony-scroll-edge-effect', () => {
    assertType<ListProps>({
      'harmony-scroll-edge-effect': true,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'harmony-scroll-edge-effect': 'true',
    });
  });
});

describe('List event check', () => {
  it('check bind scroll event', () => {
    assertType<ListProps>({
      bindscroll: (e: ListScrollEvent) => {},
    });
  });

  it('check bind scrolltolower event', () => {
    assertType<ListProps>({
      bindscrolltolower: (e: ListScrollToLowerEvent) => {},
    });
  });

  it('check bind scrolltoupper event', () => {
    assertType<ListProps>({
      bindscrolltoupper: (e: ListScrollToUpperEvent) => {},
    });
  });

  it('check bind scrollstatechange event', () => {
    assertType<ListProps>({
      bindscrollstatechange: (e: ListScrollStateChangeEvent) => {},
    });
  });

  it('check bind snap event', () => {
    assertType<ListProps>({
      bindsnap: (e: ListSnapEvent) => {},
    });
  });

  it('check bind layoutcomplete event', () => {
    assertType<ListProps>({
      bindlayoutcomplete: (e: ListLayoutCompleteEvent) => {},
    });
  });

  it('should have correct type for ListScrollInfo (scroll event detail)', () => {
    assertType<ListScrollEvent>({
      type: 'scroll',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      detail: {
        deltaX: 10,
        deltaY: -5,
        scrollLeft: 100,
        scrollTop: 200,
        scrollWidth: 1000,
        scrollHeight: 2000,
        listWidth: 375,
        listHeight: 667,
        eventSource: ListEventSource.SCROLL,
        attachedCells: [
          {
            id: 'cell-1',
            itemKey: 'key-1',
            index: 0,
            left: 0,
            top: 0,
            right: 375,
            bottom: 100,
          },
        ],
      },
    });

    assertType<ListScrollEvent>({
      type: 'scroll',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      // @ts-expect-error
      detail: {
        deltaX: 10,
      },
    });
  });

  it('should have correct type for ListScrollToLowerEvent', () => {
    assertType<ListScrollToLowerEvent>({
      type: 'scrolltolower',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      detail: {
        deltaX: 0,
        deltaY: 10,
        scrollLeft: 0,
        scrollTop: 1800,
        scrollWidth: 375,
        scrollHeight: 2000,
        listWidth: 375,
        listHeight: 667,
        eventSource: ListEventSource.SCROLL,
        attachedCells: [],
      },
    });
  });

  it('should have correct type for ListScrollToUpperEvent', () => {
    assertType<ListScrollToUpperEvent>({
      type: 'scrolltoupper',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      detail: {
        deltaX: 0,
        deltaY: -10,
        scrollLeft: 0,
        scrollTop: 0,
        scrollWidth: 375,
        scrollHeight: 2000,
        listWidth: 375,
        listHeight: 667,
        eventSource: ListEventSource.SCROLL,
        attachedCells: [],
      },
    });
  });

  it('should have correct type for ListScrollStateChangeEvent', () => {
    assertType<ListScrollStateChangeEvent>({
      type: 'scrollstatechange',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      detail: {
        state: ListScrollState.SCROLL_STATE_DRAGGING,
      },
    });

    assertType<ListScrollStateChangeEvent>({
      type: 'scrollstatechange',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      detail: {
        // @ts-expect-error
        state: 99,
      },
    });
  });

  it('should have correct type for ListSnapEvent', () => {
    assertType<ListSnapEvent>({
      type: 'snap',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      detail: {
        position: 5,
        currentScrollLeft: 0,
        currentScrollTop: 490,
        targetScrollLeft: 0,
        targetScrollTop: 500,
      },
    });

    assertType<ListSnapEvent>({
      type: 'snap',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      // @ts-expect-error
      detail: {
        currentScrollTop: 490,
        targetScrollTop: 500,
      },
    });
  });

  it('should have correct type for LayoutCompleteEvent', () => {
    assertType<ListLayoutCompleteEvent>({
      type: 'layoutcomplete',
      timestamp: Date.now(),
      target: { id: 'list1', uid: 1, dataset: {} },
      currentTarget: { id: 'list1', uid: 1, dataset: {} },
      detail: {
        'layout-id': 123,
        diffResult: {
          insertions: [1, 2],
          removals: [],
          move_from: [],
          move_to: [],
          update_from: [],
          update_to: [],
        },
        visibleItemBeforeUpdate: [],
        visibleItemAfterUpdate: [],
        scrollInfo: {
          deltaX: 0,
          deltaY: 0,
          scrollLeft: 0,
          scrollTop: 0,
          scrollWidth: 375,
          scrollHeight: 2000,
          listWidth: 375,
          listHeight: 667,
          eventSource: ListEventSource.DIFF,
          attachedCells: [],
        },
      },
    });
  });
});

describe('List method test', () => {
  it('should have correct type for scrollToPosition method', () => {
    invoke<'list'>({
      method: 'scrollToPosition',
      params: {
        index: 1,
        alignTo: 'top',
        offset: 0,
        smooth: true,
      },
    });

    invoke<'list'>({
      method: 'scrollToPosition',
      params: {
        index: 1,
        // @ts-expect-error
        alignTo: 'invalid-align',
      },
    });

    invoke<'list'>({
      method: 'scrollToPosition',
      // @ts-expect-error
      params: {
        smooth: true,
      },
    });
  });

  it('should have correct type for autoScroll method', () => {
    invoke<'list'>({
      method: 'autoScroll',
      params: {
        rate: '1000px',
        start: true,
        autoStop: false,
      },
    });

    invoke<'list'>({
      method: 'autoScroll',
      params: {
        rate: '1000px',
      },
    });
  });

  it('should have correct type for scrollBy method', () => {
    invoke<'list'>({
      method: 'scrollBy',
      params: {
        offset: 200,
      },
    });

    invoke<'list'>({
      method: 'scrollBy',
      params: {
        // @ts-expect-error
        offset: '200',
      },
    });
  });

  it('should have correct type for getVisibleCells method', () => {
    invoke<'list'>({
      method: 'getVisibleCells',
    });

    invoke<'list'>({
      method: 'getVisibleCells',
      // @ts-expect-error
      params: {
        x: '200',
      },
    });
  });
});
