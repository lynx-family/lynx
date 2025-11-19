// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { expectType } from 'tsd';

import { UIMethods, ScrollToUpperEvent, ScrollToLowerEvent, ScrollEvent, ScrollEndEvent, IntrinsicElements } from '../../../types';
import { invoke } from '../test-utils';

// Props Types Check
let a;
{
  <scroll-view scroll-orientation={'horizontal'} />;
  <scroll-view scroll-orientation={'vertical'} />;
  assertType<'vertical' | 'horizontal' | undefined>(a as IntrinsicElements['scroll-view']['scroll-orientation']);

  <scroll-view bounces={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-view']['bounces']);

  <scroll-view enable-scroll={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-view']['enable-scroll']);

  <scroll-view scroll-bar-enable={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-view']['scroll-bar-enable']);

  <scroll-view upper-threshold={100} />;
  assertType<number | undefined>(a as IntrinsicElements['scroll-view']['upper-threshold']);

  <scroll-view lower-threshold={100} />;
  assertType<number | undefined>(a as IntrinsicElements['scroll-view']['lower-threshold']);

  <scroll-view initial-scroll-offset={100} />;
  assertType<number | undefined>(a as IntrinsicElements['scroll-view']['initial-scroll-offset']);

  <scroll-view initial-scroll-to-index={1} />;
  assertType<number | undefined>(a as IntrinsicElements['scroll-view']['initial-scroll-to-index']);
}

// Events types check
function noop() {}
{
  <scroll-view bindtap={noop} />;
  <scroll-view
    bindscrolltoupper={(e: ScrollToUpperEvent) => {
      assertType<number>(e.detail.scrollTop);
      assertType<number>(e.detail.scrollLeft);
      assertType<number>(e.detail.deltaX);
      assertType<number>(e.detail.deltaY);
      assertType<number>(e.detail.scrollHeight);
      assertType<number>(e.detail.scrollWidth);
    }}
  />;
  <scroll-view
    bindscrolltolower={(e: ScrollToLowerEvent) => {
      assertType<number>(e.detail.scrollTop);
      assertType<number>(e.detail.scrollLeft);
      assertType<number>(e.detail.deltaX);
      assertType<number>(e.detail.deltaY);
      assertType<number>(e.detail.scrollHeight);
      assertType<number>(e.detail.scrollWidth);
    }}
  />;
  <scroll-view
    bindscroll={(e: ScrollEvent) => {
      assertType<number>(e.detail.scrollTop);
      assertType<number>(e.detail.scrollLeft);
      assertType<number>(e.detail.deltaX);
      assertType<number>(e.detail.deltaY);
      assertType<number>(e.detail.scrollHeight);
      assertType<number>(e.detail.scrollWidth);
    }}
  />;
  <scroll-view
    bindscrollend={(e: ScrollEndEvent) => {
      assertType<number>(e.detail.scrollTop);
      assertType<number>(e.detail.scrollLeft);
      assertType<number>(e.detail.deltaX);
      assertType<number>(e.detail.deltaY);
      assertType<number>(e.detail.scrollHeight);
      assertType<number>(e.detail.scrollWidth);
    }}
  />;
}

{
  invoke<'scroll-view'>({
    method: 'scrollTo',
    params: {
      offset: 0,
      smooth: true,
      index: 0,
    },
  });
  invoke<'scroll-view'>({
    method: 'autoScroll',
    params: {
      rate: 0,
      start: true,
    },
  });
  invoke<'scroll-view'>({
    method: 'scrollBy',
    params: {
      offset: 0,
    },
  });
  {
    invoke<'scroll-view'>({
      method: 'getScrollInfo',
      success: (payload) => {
        assertType<number>(payload.scrollX);
        assertType<number>(payload.scrollY);
        assertType<number>(payload.scrollRange);
      },
    });
  }

  let a: unknown;
  expectType<'scrollTo' | 'autoScroll' | 'scrollBy' | 'getScrollInfo'>(a as UIMethods['scroll-view']['method']);
}
