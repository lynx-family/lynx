// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { expectError } from 'tsd';
import { OverlayTouchState, OverlayError, OverlayTouch, OverlayProps, OverlayTouchEvent, OverlayErrorEvent, IntrinsicElements } from '../../types';

import { BaseEvent } from '../../types';

// Props Types Check
let a;
{
  // Test overlay JSX element with various props
  <overlay visible={true} />;
  <overlay level={1} />;
  <overlay mode="window" />;
  <overlay mode="page" />;
  <overlay visible={false} level={2} mode="top" />;

  assertType<boolean | undefined>(a as IntrinsicElements['overlay']['visible']);
  assertType<1 | 2 | 3 | 4 | undefined>(a as IntrinsicElements['overlay']['level']);
  assertType<boolean | undefined>(a as IntrinsicElements['overlay']['ios-enable-swipe-back']);
  assertType<'window' | 'top' | 'page' | string | undefined>(a as IntrinsicElements['overlay']['mode']);

  expectError(() => {
    // @ts-expect-error type error
    <overlay visible={'text'} />;
    // @ts-expect-error type error
    <overlay level={5} />;
    // @ts-expect-error type error
    <overlay mode={true} />;
    // @ts-expect-error type error
    <overlay ios-enable-swipe-back={'text'} />;
  });
}

// Events types check
function noop() {}
{
  <overlay bindtap={noop} />;
  <overlay
    bindshowoverlay={(e: BaseEvent) => {
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e);
      });
    }}
  />;
  <overlay
    binddismissoverlay={(e: BaseEvent) => {
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e);
      });
    }}
  />;
  <overlay
    bindrequestclose={(e: BaseEvent) => {
      expectError(() => {
        // @ts-expect-error type error
        assertType<number>(e);
      });
    }}
  />;
  <overlay
    bindoverlaytouch={(e: OverlayTouchEvent) => {
      assertType<OverlayTouch>(e.detail);
      assertType<number>(e.detail.x);
      assertType<number>(e.detail.y);
      assertType<OverlayTouchState>(e.detail.state);
    }}
  />;
  <overlay
    binderror={(e: OverlayErrorEvent) => {
      assertType<OverlayError>(e.detail);
      assertType<string>(e.detail.errorCode);
      assertType<string>(e.detail.errorMsg);
    }}
  />;
}

// OverlayTouchState Enum Check
assertType<OverlayTouchState>(OverlayTouchState.OverlayTouchStateDown);
assertType<OverlayTouchState>(OverlayTouchState.OverlayTouchStateMove);
assertType<OverlayTouchState>(OverlayTouchState.OverlayTouchStateUp);
assertType<OverlayTouchState>(OverlayTouchState.OverlayTouchStateCancel);

expectError(() => {
  // @ts-expect-error type error
  assertType<OverlayTouchState>(4);
});
