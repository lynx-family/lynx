// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { IntrinsicElements } from '../../types';

// Props Types Check
let a: unknown;
{
  <blur-view />;

  <blur-view blur-radius="16px" />;
  assertType<string | undefined>(a as IntrinsicElements['blur-view']['blur-radius']);

  <blur-view blur-sampling={4} />;
  assertType<number | undefined>(a as IntrinsicElements['blur-view']['blur-sampling']);

  <blur-view enable-auto-blur={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['blur-view']['enable-auto-blur']);

  <blur-view android-capture-target="backdrop" />;
  assertType<string | undefined>(a as IntrinsicElements['blur-view']['android-capture-target']);

  <blur-view experimental-update-blur-radius={false} />;
  assertType<boolean | undefined>(a as IntrinsicElements['blur-view']['experimental-update-blur-radius']);

  <blur-view blur-effect="glass-container" />;
  assertType<'light' | 'extra-light' | 'dark' | 'glass' | 'glass-container' | undefined>(a as IntrinsicElements['blur-view']['blur-effect']);

  <blur-view spacing={8} />;
  assertType<number | undefined>(a as IntrinsicElements['blur-view']['spacing']);

  <blur-view glass-interactive={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['blur-view']['glass-interactive']);

  <blur-view glass-tint-color="rgba(255, 255, 255, 0.2)" />;
  assertType<string | undefined>(a as IntrinsicElements['blur-view']['glass-tint-color']);

  <blur-view glass-style="clear" />;
  assertType<'regular' | 'clear' | undefined>(a as IntrinsicElements['blur-view']['glass-style']);
}
