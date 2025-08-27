// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { UIMethods } from '../../types';

// StandardProps Types Check
{
  <component is="" style={{ backgroundColor: 'red' }} class="" className="" />;
  <filter-image style={{ backgroundColor: 'red' }} class="" className="" />;
  <image style={{ backgroundColor: 'red' }} class="" className="" />;
  <inline-image style={{ backgroundColor: 'red' }} class="" className="" />;
  <inline-text style={{ backgroundColor: 'red' }} class="" className="" />;
  <image style={{ backgroundColor: 'red' }} class="" className="" />;
  <list style={{ backgroundColor: 'red' }} class="" className="" />;
  <list-item style={{ backgroundColor: 'red' }} class="" className="" item-key={'a'} />;
  <list-row style={{ backgroundColor: 'red' }} class="" className="" item-key={'b'} />;
  <page style={{ backgroundColor: 'red' }} class="" className="" />;
  <scroll-view style={{ backgroundColor: 'red' }} class="" className="" />;
  <text style={{ backgroundColor: 'red' }} class="" className="" />;
  <view style={{ backgroundColor: 'red' }} class="" className="" />;
  <raw-text style={{ backgroundColor: 'red' }} class="" className="" text={''} />;
  // Np Props
  <inline-truncation />;
}

//bindtap types check
function noop() {}
{
  <view bindtap={noop}></view>;
}

// UIMethods types check
function invoke<T extends keyof UIMethods>(_param: UIMethods[T]) {}

// list methods Check
{
  invoke<'list'>({
    method: 'autoScroll',
    params: {
      rate: '',
      start: true,
      autoStop: false,
    },
  });

  invoke<'list'>({
    method: 'scrollToPosition',
    params: {
      position: 1,
      index: 1,
    },
  });

  let s: unknown;
  invoke<'list'>({
    method: s as 'initCache' | 'removeStickyView' | 'getVisibleCells' | 'getVisibleItemsPositions',
  });
}

// scroll-view methods
{
  invoke<'scroll-view'>({
    method: 'autoScroll',
    params: {
      rate: '',
      start: true,
      autoStop: false,
    },
  });

  invoke<'scroll-view'>({
    method: 'scrollTo',
    params: {
      offset: 0,
      smooth: false,
      index: 0,
    },
  });
}
