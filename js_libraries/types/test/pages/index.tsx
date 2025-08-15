// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { MainThread, UIMethods, MessageEvent } from '../../types';

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
      rate: 1,
      start: true,
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

// fetch api
{
  const streamToArrayBuffer = async (stream: ReadableStream) => {
    const reader = stream.getReader();
    while (true) {
      const { done, value } = await reader.read();
      if (done) {
        break;
      } else {
        const text = TextCodecHelper.decode(value);
        const firstChar = text.charAt(0);
        const arrayBuffer = TextCodecHelper.encode(text);
      }
    }
  };

  lynx
    .fetch('url', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        a: 1,
      }),
      lynxExtension: {
        useStreaming: true,
      },
    })
    .then((response) => {
      streamToArrayBuffer(response.body);
    });

  const eventSource = new lynx.EventSource('https://sse.dev/test');

  eventSource.onmessage = (event: MessageEvent) => {
    console.log('Received message:', event.data);
  };

  // @ts-expect-error message should follow MessageEvent
  eventSource.onmessage = (event: Event) => {
    console.log('Received message:', event);
  };

  eventSource.onerror = (error: Event) => {
    console.error('EventSource failed:', error);
  };

  const listener = (event: MessageEvent) => {
    console.log(event.data);
  };

  eventSource.addEventListener('system', listener);
  eventSource.removeEventListener('system', listener);

  const errorListener = (event: Event) => {
    console.log(event);
  };
  eventSource.addEventListener('error', errorListener);
  eventSource.removeEventListener('error', errorListener);
}

// MTS Animate
function startAnimation(ele: MainThread.Element) {
  'main thread';
  const animation = ele.animate([{ opacity: 0 }, { opacity: 1 }], {
    duration: 3000,
  });

  animation.pause();
  animation.play();
  animation.cancel();

  // @ts-expect-error Should always have keyframes
  ele.animate(undefined, {
    duration: 3000,
  });

  // Should work if no option is provided
  ele.animate([{ opacity: 0 }, { opacity: 1 }]);

  // Should work if option is number
  ele.animate([{ opacity: 0 }, { opacity: 1 }], 200);
}
