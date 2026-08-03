import { expectType } from 'tsd';
import type { JSX as ReactJSX } from 'react';
import { CoverViewProps, IntrinsicElements } from '../../types';

let a: unknown;

// Props types check
{
  <cover-view id="cover" />;
  expectType<string | undefined>(a as IntrinsicElements['cover-view']['id']);
  expectType<CoverViewProps>(a as IntrinsicElements['cover-view']);
}

// React JSX types check
{
  expectType<CoverViewProps>(a as ReactJSX.IntrinsicElements['cover-view']);
}

// Events types check
function noop() {}
{
  <cover-view bindtap={noop} />;
}
