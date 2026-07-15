// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type {} from '@lynx-js/types/strict';
import { assertType, expectTypeOf } from 'vitest';
import type {
  CSSProperties,
  InlineStyleProperties,
  IntrinsicElements,
  StrictCSSProperties,
} from '@lynx-js/types';

expectTypeOf<InlineStyleProperties>().toEqualTypeOf<StrictCSSProperties>();

assertType<CSSProperties>({
  aspectRatio: 4 / 3,
  position: 'static',
  zIndex: '0',
});

assertType<InlineStyleProperties>({
  display: 'linear',
  position: 'absolute',
});

assertType<InlineStyleProperties>({
  // @ts-expect-error: the project entry selects strict metadata
  position: 'static',
});

assertType<IntrinsicElements['view']['style']>('display: linear');

<view style={{ display: 'linear', position: 'absolute' }} />;
<view
  style={{
    // @ts-expect-error: project-level strict mode reaches JSX style objects
    position: 'static',
  }}
/>;

const componentAttributes: JSX.IntrinsicAttributes = {
  style: { position: 'absolute' },
};
assertType<JSX.IntrinsicAttributes>(componentAttributes);

const invalidComponentAttributes: JSX.IntrinsicAttributes = {
  style: {
    // @ts-expect-error: strict mode reaches component intrinsic attributes
    position: 'static',
  },
};
assertType<JSX.IntrinsicAttributes>(invalidComponentAttributes);
