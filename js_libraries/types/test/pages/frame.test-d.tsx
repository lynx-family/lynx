// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType, it } from 'vitest';
import { IntrinsicElements } from '../../types';

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
}
