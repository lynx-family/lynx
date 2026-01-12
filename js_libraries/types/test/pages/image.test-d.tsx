// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { LayoutEvent, TextLineInfo, UIMethods, ImageErrorEvent, BaseEvent, ErrorEvent } from '../../types';
import type { ImageProps, ImageUIMethods, LoadEvent } from '../../types/common/element/image';

// Props Types Check
{
  <image src={'1'} />;
  <image mode={'scaleToFill'} />;
  <image mode={'aspectFit'} />;
  <image mode={'aspectFill'} />;
  <image mode={'center'} />;
  <image image-config={'ARGB_8888'} />;
  <image image-config={'RGB_565'} />;
  <image placeholder={'top'} />;
  <image blur-radius={'center'} />; // TODO: format to {`${number}`px}
  <image cap-insets={'bottom'} />; // TODO: it is an enum, not a string
  <image cap-insets-scale={1} />;
  <image loop-count={1} />;
  <image prefetch-width={'bottom'} />; // TODO: format to {`${number}`px}
  <image prefetch-height={'bottom'} />; // TODO: format to {`${number}`px}
  <image auto-size={false} />;
  <image defer-src-invalidation={false} />;
  <image autoplay={true} />;
  <image ios-frame-cache-automatically={true} />;
  <image tint-color={'true'} />; // TODO: it is a <color>, not a string
}

// Events types check
function noop() {}
{
  <image bindtap={noop}></image>;
  <image
    bindload={(e: LoadEvent) => {
      assertType<number>(e.detail.width);
      assertType<number>(e.detail.height);
    }}
  />;
  <image
    binderror={(e: ErrorEvent) => {
      assertType<string>(e.detail.errMsg);
      assertType<number>(e.detail.error_code);
      assertType<number>(e.detail.lynx_categorized_code);
    }}
  />;
  <image bindstartplay={() => {}} bindcurrentloopcomplete={() => {}} bindfinalloopcomplete={() => {}} />;
}

// UIMethods types check
function invoke<T extends keyof UIMethods>(_param: UIMethods[T]) {}

{
  invoke<'image'>({
    method: 'startAnimate',
  });
  invoke<'image'>({
    method: 'resumeAnimation',
  });
  invoke<'image'>({
    method: 'pauseAnimation',
  });
  invoke<'image'>({
    method: 'stopAnimation',
  });
}
