// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, it, assertType } from 'vitest';
import {
  ListProps,
  ListAnimationType,
  ListAnimationEntry,
  ListAnimationLegacyStage,
  ListUpdateAnimationConfig,
  ListAnimationStartEvent,
  ListAnimationEndEvent,
  ListAnimationCancelEvent,
  ListAnimationUpdateEvent,
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
      'item-snap': { factor: 0.5, offset: 10, maxSnapCount: 3 },
    });
    assertType<ListProps>({
      // @ts-expect-error - factor should be a number
      'item-snap': { factor: '0.5', offset: 10 },
    });
    assertType<ListProps>({
      // @ts-expect-error - maxSnapCount should be a number
      'item-snap': { factor: 0.5, offset: 10, maxSnapCount: '3' },
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

  it('check scrollbar properties', () => {
    assertType<ListProps>({
      'enable-scrollbar': true,
      'scroll-bar-auto-hide': false,
      'scroll-bar-width': 12,
      'scroll-bar-thumb-width': 8,
      'scroll-bar-thumb-min-length': 18,
      'scroll-bar-thumb-radius': 4,
      'scroll-bar-thumb-color': '#00000066',
      'scroll-bar-thumb-active-color': 'rgba(0, 0, 0, 0.8)',
      'scroll-bar-thumb-hover-color': 'rgba(0, 0, 0, 0.8)',
      'scroll-bar-track-color': 'transparent',
      'scroll-bar-auto-hide-delay': 1000,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'enable-scrollbar': 'true',
    });
    assertType<ListProps>({
      // @ts-expect-error
      'scroll-bar-thumb-min-length': '18',
    });
    assertType<ListProps>({
      // @ts-expect-error
      'scroll-bar-track-color': 0,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'scroll-bar-thumb-hover-color': 0,
    });
    assertType<ListProps>({
      // @ts-expect-error
      'scroll-bar-auto-hide-delay': '1000',
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

describe('List update animation configuration', () => {
  it('accepts uniformly new or legacy stages', () => {
    assertType<ListProps>({
      'experimental-use-new-update-animation': true,
      'experimental-new-update-animation': {
        enable: true,
        stages: [
          { type: 'remove', duration: 1000 },
          [
            { type: 'move', duration: 800 },
            { type: 'add', duration: 1200 },
          ],
        ],
      },
    });
    assertType<ListProps>({
      'experimental-use-new-update-animation': true,
      'experimental-new-update-animation': {
        enable: true,
        stages: [
          { animations: ['remove'], durations: 1000 },
          { animations: ['move', 'add'], durations: [800, 1200] },
        ],
      },
    });
    assertType<ListUpdateAnimationConfig>({});
    assertType<ListUpdateAnimationConfig>({ enable: false });
  });

  it('rejects mixed stage syntax in either order', () => {
    assertType<ListUpdateAnimationConfig>({
      // @ts-expect-error
      stages: [{ animations: ['remove'], durations: 1000 }, [{ type: 'add', duration: 1200 }]],
    });
    assertType<ListUpdateAnimationConfig>({
      // @ts-expect-error
      stages: [[{ type: 'add', duration: 1200 }], { animations: ['remove'], durations: 1000 }],
    });
    assertType<ListUpdateAnimationConfig>({
      stages: [
        { animations: ['remove'], durations: 1000 },
        // @ts-expect-error
        { type: 'add', duration: 1200 },
      ],
    });
    assertType<ListUpdateAnimationConfig>({
      stages: [
        // @ts-expect-error
        { type: 'add', duration: 1200 },
        { animations: ['remove'], durations: 1000 },
      ],
    });
  });

  it('requires stages to be an array and parallel groups to contain current entries', () => {
    assertType<ListUpdateAnimationConfig>({
      // @ts-expect-error stages must be an array, even for a single animation
      stages: { type: 'remove', duration: 1000 },
    });
    assertType<ListUpdateAnimationConfig>({
      // @ts-expect-error legacy stages cannot be nested in parallel groups
      stages: [[{ animations: ['remove'], durations: 1000 }]],
    });
    assertType<ListUpdateAnimationConfig>({
      // @ts-expect-error parallel groups cannot be nested
      stages: [[[{ type: 'remove', duration: 1000 }]]],
    });
  });

  it('rejects invalid flags, names, and duration fields', () => {
    assertType<ListProps>({
      // @ts-expect-error
      'experimental-use-new-update-animation': 'true',
    });
    assertType<ListUpdateAnimationConfig>({
      // @ts-expect-error
      enable: 1,
    });
    assertType<ListAnimationEntry>({
      // @ts-expect-error
      type: ' move',
      duration: 1000,
    });
    assertType<ListAnimationEntry>({
      type: 'remove',
      // @ts-expect-error
      durations: 1000,
    });
    assertType<ListAnimationEntry>({
      type: 'remove',
      // @ts-expect-error
      duration: [1000],
    });
    assertType<ListAnimationLegacyStage>({
      animations: ['remove'],
      // @ts-expect-error
      duration: 1000,
    });
    // @ts-expect-error
    assertType<ListAnimationEntry>({ type: 'add' });
  });
});

describe('List event check', () => {
  it('infers animation event names and payloads', () => {
    assertType<ListProps>({
      bindlistanimationstart: (e) => {
        assertType<ListAnimationStartEvent>(e);
        assertType<'listanimationstart'>(e.type);
        assertType<number>(e.detail.transactionId);
        assertType<ListAnimationType>(e.detail.type);
        // @ts-expect-error
        e.detail.progress;
      },
      bindlistanimationend: (e) => {
        assertType<ListAnimationEndEvent>(e);
        assertType<'listanimationend'>(e.type);
        assertType<number>(e.detail.transactionId);
        assertType<ListAnimationType>(e.detail.type);
        // @ts-expect-error progress is only present on update events
        e.detail.progress;
      },
      bindlistanimationcancel: (e) => {
        assertType<ListAnimationCancelEvent>(e);
        assertType<'listanimationcancel'>(e.type);
        assertType<number>(e.detail.transactionId);
        assertType<ListAnimationType>(e.detail.type);
        // @ts-expect-error progress is only present on update events
        e.detail.progress;
      },
      bindlistanimationupdate: (e) => {
        assertType<ListAnimationUpdateEvent>(e);
        assertType<'listanimationupdate'>(e.type);
        assertType<number>(e.detail.progress);
        assertType<number>(e.detail.transactionId);
        assertType<ListAnimationType>(e.detail.type);
      },
    });
    assertType<ListProps>({
      // @ts-expect-error
      bindlistanimationstart: (e: ListAnimationUpdateEvent) => {},
    });
    // @ts-expect-error
    assertType<ListAnimationUpdateEvent['detail']>({ transactionId: 1, type: 'move' });
  });

  it('requires valid animation event details', () => {
    assertType<ListAnimationUpdateEvent['detail']>({ transactionId: 1, type: 'move', progress: 0.5 });
    // @ts-expect-error transactionId is required
    assertType<ListAnimationStartEvent['detail']>({ type: 'remove' });
    // @ts-expect-error type is required
    assertType<ListAnimationEndEvent['detail']>({ transactionId: 1 });
    assertType<ListAnimationCancelEvent['detail']>({
      transactionId: 1,
      // @ts-expect-error unknown animation type
      type: 'unknown',
    });
    assertType<ListAnimationUpdateEvent['detail']>({
      transactionId: 1,
      type: 'move',
      // @ts-expect-error progress must be numeric
      progress: '0.5',
    });
  });

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
        itemKey: 'key-1',
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

    invoke<'list'>({
      method: 'scrollToPosition',
      // @ts-expect-error
      params: {
        itemKey: 'key-1',
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

  it('should have correct type for getScrollInfo method', () => {
    invoke<'list'>({
      method: 'getScrollInfo',
      success: (e) => {
        assertType<number>(e.scrollX);
        assertType<number>(e.scrollY);
        assertType<number | undefined>(e.scrollRange);
        assertType<number | undefined>(e.maxScrollOffset);
      },
    });
  });
});
