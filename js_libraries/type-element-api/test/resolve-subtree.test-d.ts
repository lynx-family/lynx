// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, expectTypeOf, it } from 'vitest';
import type { ComposeElementRef, ElementRef } from '../types/index';

describe('Subtree resolve API types', () => {
  it('synchronously resolves raw subtrees and Compose mount roots', () => {
    const owner = {} as ComposeElementRef;
    const raw = {} as ElementRef;
    expectTypeOf<typeof __ResolveElementSubtree>().toBeCallableWith(raw);
    expectTypeOf<typeof __ResolveElementSubtree>().toBeCallableWith(owner, true);
    expectTypeOf<typeof __ResolveElementSubtree>().returns.toBeVoid();
    expectTypeOf<Parameters<typeof __ResolveElementSubtree>>().toEqualTypeOf<[ElementRef | ComposeElementRef, boolean?]>();
  });
});
