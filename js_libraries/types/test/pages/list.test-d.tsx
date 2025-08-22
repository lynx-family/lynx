// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { expectType } from 'tsd';
import { IntrinsicElements } from '../../types';

// Props Types Check
let a;

// Props Types Check
{
  <list harmony-scroll-edge-effect={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['list']['harmony-scroll-edge-effect']);
}
