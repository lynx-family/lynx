// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { IntrinsicElements, ScrollCoordinatorOffsetEvent, ScrollCoordinatorUIMethods } from '../../types';

let a: unknown;
{
  <scroll-coordinator />;
  <scroll-coordinator enable-scroll={true} bounces={true} enable-scroll-bar={true} />;
  <scroll-coordinator-header />;
  <scroll-coordinator-slot />;
  <scroll-coordinator-slot-drag enable-drag={true} />;
  <scroll-coordinator-toolbar />;

  assertType<boolean | undefined>(a as IntrinsicElements['scroll-coordinator']['ios-scrolls-to-top']);
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-coordinator']['header-over-slot']);
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-coordinator']['enable-scroll']);
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-coordinator']['bounces']);
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-coordinator']['enable-scroll-bar']);
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-coordinator']['android-nested-scroll-as-child']);
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-coordinator']['ios-force-scroll-detach']);
  assertType<number | undefined>(a as IntrinsicElements['scroll-coordinator']['granularity']);
  assertType<'none' | 'page' | 'fold' | undefined>(a as IntrinsicElements['scroll-coordinator']['refresh-mode']);
  assertType<boolean | undefined>(a as IntrinsicElements['scroll-coordinator-slot-drag']['enable-drag']);
}

function noop() {}
{
  <scroll-coordinator bindtap={noop} />;

  <scroll-coordinator
    bindoffset={(e: ScrollCoordinatorOffsetEvent) => {
      assertType<number>(e.detail.offset);
      assertType<number>(e.detail.height);
    }}
  />;
}

function invoke<T extends keyof { 'scroll-coordinator': ScrollCoordinatorUIMethods }>(_param: { 'scroll-coordinator': ScrollCoordinatorUIMethods }[T]) {}

{
  invoke<'scroll-coordinator'>({
    method: 'setFoldExpanded',
    params: {
      offset: '100px',
      smooth: true,
    },
  });

  let methodType: unknown;
  assertType<'setFoldExpanded'>(methodType as ScrollCoordinatorUIMethods['method']);
}
