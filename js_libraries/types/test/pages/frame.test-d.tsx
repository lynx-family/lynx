// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType, it } from 'vitest';
import { IntrinsicElements, FrameLoadEvent, FrameLoadMetricsEvent } from '../../types';

// Props Types Check
let a;
{
  <frame src={'1'} />;
  assertType<string>((a as unknown) as IntrinsicElements['frame']['src']);
  // @ts-expect-error: src is required
  <frame />;
  // @ts-expect-error: src shoudl be string not number
  <frame src={1} />;
  // @ts-expect-error: src shoudl be string not boolean
  <frame src={true} />;

  <frame src={'1'} />;
  assertType<Record<string, unknown> | undefined>((a as unknown) as IntrinsicElements['frame']['data']);
  <frame src={'1'} data={{}} />;
  assertType<Record<string, unknown> | undefined>((a as unknown) as IntrinsicElements['frame']['data']);
  <frame src={'1'} data={{ data1: 'data1' }} />;
  assertType<Record<string, unknown> | undefined>((a as unknown) as IntrinsicElements['frame']['data']);
  // @ts-expect-error: data shoudl be object not string
  <frame src={'1'} data={'data1'} />;
  // @ts-expect-error: data shoudl be object not boolean
  <frame src={'1'} data={true} />;
  // @ts-expect-error: data shoudl be object not number
  <frame src={'1'} data={1} />;
  // @ts-expect-error: data shoudl be object not array
  <frame src={'1'} data={[1, 2, 3]} />;

  <frame src={'1'} />;
  assertType<Record<string, unknown> | undefined>((a as unknown) as IntrinsicElements['frame']['global-props']);
  <frame src={'1'} global-props={{}} />;
  assertType<Record<string, unknown> | undefined>((a as unknown) as IntrinsicElements['frame']['global-props']);
  <frame src={'1'} global-props={{ data1: 'data1' }} />;
  assertType<Record<string, unknown> | undefined>((a as unknown) as IntrinsicElements['frame']['global-props']);
  // @ts-expect-error: global-props shoudl be object not string
  <frame src={'1'} global-props={'data1'} />;
  // @ts-expect-error: global-props shoudl be object not boolean
  <frame src={'1'} global-props={true} />;
  // @ts-expect-error: global-props shoudl be object not number
  <frame src={'1'} global-props={1} />;
  // @ts-expect-error: global-props shoudl be object not array
  <frame src={'1'} global-props={[1, 2, 3]} />;

  <frame src={'1'} auto-width={true} />;
  assertType<boolean | undefined>((a as unknown) as IntrinsicElements['frame']['auto-width']);
  <frame src={'1'} auto-height={true} />;
  assertType<boolean | undefined>((a as unknown) as IntrinsicElements['frame']['auto-height']);
  // @ts-expect-error: auto-height shoudl be boolean not number
  <frame src={'1'} auto-height={1} />;
  // @ts-expect-error: auto-height shoudl be boolean not string
  <frame src={'1'} auto-height={'1'} />;
  // @ts-expect-error: auto-width shoudl be boolean not number
  <frame src={'1'} auto-width={1} />;
  // @ts-expect-error: auto-width shoudl be boolean not string
  <frame src={'1'} auto-width={'1'} />;

  <frame src={'1'} preset-width={'100px'} />;
  <frame src={'1'} preset-width={'100rpx'} />;
  assertType<`${number}px` | `${number}rpx` | undefined>((a as unknown) as IntrinsicElements['frame']['preset-width']);
  <frame src={'1'} preset-height={'100px'} />;
  <frame src={'1'} preset-height={'100rpx'} />;
  assertType<`${number}px` | `${number}rpx` | undefined>((a as unknown) as IntrinsicElements['frame']['preset-height']);
  // @ts-expect-error: preset-width shoudl be string not number
  <frame src={'1'} preset-width={100} />;
  // @ts-expect-error: preset-width shoudl be string not boolean
  <frame src={'1'} preset-width={true} />;
  // @ts-expect-error: preset-width shoudl use px or rpx unit
  <frame src={'1'} preset-width={'100'} />;
  // @ts-expect-error: preset-width shoudl use px or rpx unit
  <frame src={'1'} preset-width={'100ppx'} />;
  // @ts-expect-error: preset-height shoudl be string not number
  <frame src={'1'} preset-height={100} />;
  // @ts-expect-error: preset-height shoudl be string not boolean
  <frame src={'1'} preset-height={true} />;
  // @ts-expect-error: preset-height shoudl use px or rpx unit
  <frame src={'1'} preset-height={'100'} />;
  // @ts-expect-error: preset-height shoudl use px or rpx unit
  <frame src={'1'} preset-height={'100ppx'} />;

  <frame src={'1'} enable-multi-async-thread={true} />;
  <frame src={'1'} enable-multi-async-thread={false} />;
  assertType<boolean | undefined>((a as unknown) as IntrinsicElements['frame']['enable-multi-async-thread']);
  // @ts-expect-error: enable-multi-async-thread shoudl be boolean not number
  <frame src={'1'} enable-multi-async-thread={1} />;
  // @ts-expect-error: enable-multi-async-thread shoudl be boolean not string
  <frame src={'1'} enable-multi-async-thread={'true'} />;

  type HasEmbeddedMode = 'embedded-mode' extends keyof IntrinsicElements['frame'] ? true : false;
  assertType<false>((a as unknown) as HasEmbeddedMode);
}

// Events types check
function noop() {}
{
  <frame
    src={'1'}
    bindload={(e: FrameLoadEvent) => {
      assertType<string>(e.detail.url);
      assertType<number>(e.detail.statusCode);
      assertType<string>(e.detail.statusMessage);
    }}
  />;
  <frame
    src={'1'}
    bindloadmetrics={(e: FrameLoadMetricsEvent) => {
      assertType<'bindloadmetrics'>(e.type);
      assertType<string>(e.detail.url);
      assertType<string>(e.detail.mode);
      assertType<string | undefined>(e.detail.entry.name);
      assertType<string | undefined>(e.detail.entry.entryType);
      assertType<number | undefined>(e.detail.entry.loadBundleStart);
      assertType<unknown>(e.detail.entry.customMetric);
    }}
  />;
}
