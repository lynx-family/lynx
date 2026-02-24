// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { RefreshStartRefreshEvent, RefreshHeaderOffsetEvent, RefreshStateChangeEvent, RefreshState, RefreshUIMethods, IntrinsicElements, BaseEvent } from '../../types';

export type XRefreshViewHeaderReleasedEvent = BaseEvent<'headerreleased', {}>;

// Props Types Check
let a: unknown;
{
  <refresh enable-refresh={true} />;
  assertType<boolean | undefined>(a as IntrinsicElements['refresh']['enable-refresh']);
}

// Events types check
function noop() {}
{
  <refresh-header />;

  <refresh bindtap={noop} />;

  <refresh
    bindstartrefresh={(e: RefreshStartRefreshEvent) => {
      assertType<boolean>(e.detail.isManual);
    }}
  />;

  <refresh
    bindheaderoffset={(e: RefreshHeaderOffsetEvent) => {
      assertType<boolean>(e.detail.isDragging);
      assertType<number>(e.detail.offsetPercent);
    }}
  />;

  <refresh
    bindrefreshstatechange={(e: RefreshStateChangeEvent) => {
      assertType<RefreshState>(e.detail.state);
    }}
  />;
}

// UIMethods types check
function invoke<T extends keyof { 'refresh': RefreshUIMethods }>(_param: { 'refresh': RefreshUIMethods }[T]) {}

{
  // finishRefresh method
  invoke<'refresh'>({
    method: 'finishRefresh',
    success: () => {},
    fail: () => {},
  });

  // autoStartRefresh method
  invoke<'refresh'>({
    method: 'autoStartRefresh',
    success: () => {},
    fail: () => {},
  });

  // Test method types
  let methodType: unknown;
  assertType<'finishRefresh' | 'autoStartRefresh'>(methodType as RefreshUIMethods['method']);
}
