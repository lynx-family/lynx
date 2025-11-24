// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType, it } from 'vitest';
import { IntrinsicElements, FrameLoadEvent } from '../../types';

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
}
