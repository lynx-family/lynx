// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { IntrinsicElements, ViewPagerChangeEvent, ViewPagerOffsetChangeEvent, ViewPagerUIMethods, ViewPagerWillChangeEvent } from '../../types';

// Props Types Check
let a: unknown;
{
  <viewpager />;
  <viewpager-item id="page-1" />;
  assertType<string | undefined>(a as IntrinsicElements['viewpager-item']['id']);

  <viewpager android-always-overscroll={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['viewpager']['android-always-overscroll']);

  <viewpager android-force-can-scroll={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['viewpager']['android-force-can-scroll']);

  <viewpager ios-recognized-view-tag={123} />;
  assertType<number | undefined>(a as IntrinsicElements['viewpager']['ios-recognized-view-tag']);

  <viewpager ios-recognized-gesture-class="UIPanGestureRecognizer" />;
  assertType<string | undefined>(a as IntrinsicElements['viewpager']['ios-recognized-gesture-class']);

  <viewpager ios-gesture-offset={50} />;
  assertType<number | undefined>(a as IntrinsicElements['viewpager']['ios-gesture-offset']);

  <viewpager ios-gesture-direction={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['viewpager']['ios-gesture-direction']);

  <viewpager initial-select-index={1} />;
  assertType<number | undefined>(a as IntrinsicElements['viewpager']['initial-select-index']);

  <viewpager enable-scroll={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['viewpager']['enable-scroll']);

  <viewpager bounces={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['viewpager']['bounces']);

  <viewpager keep-item-view={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['viewpager']['keep-item-view']);
}

// Events types check
function noop() {}
{
  <viewpager-item bindtap={noop} />;

  <viewpager bindtap={noop} />;

  <viewpager
    bindchange={(e: ViewPagerChangeEvent) => {
      assertType<number>(e.detail.index);
      assertType<boolean>(e.detail.isDragged);
    }}
  />;

  <viewpager
    bindwillchange={(e: ViewPagerWillChangeEvent) => {
      assertType<number>(e.detail.index);
      assertType<boolean>(e.detail.isDragged);
    }}
  />;

  <viewpager
    bindoffsetchange={(e: ViewPagerOffsetChangeEvent) => {
      assertType<number>(e.detail.offset);
    }}
  />;
}

// UIMethods types check
function invoke<T extends keyof { viewpager: ViewPagerUIMethods }>(_param: { viewpager: ViewPagerUIMethods }[T]) {}

{
  invoke<'viewpager'>({
    method: 'selectTab',
    params: {
      index: 2,
      smooth: true,
    },
    success: () => {},
    fail: () => {},
  });

  let methodType: unknown;
  assertType<'selectTab'>(methodType as ViewPagerUIMethods['method']);
}
