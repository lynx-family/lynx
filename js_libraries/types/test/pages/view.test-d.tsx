// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { IntrinsicElements } from '../../types';

// Props Types Check
let a;

// Props Types Check
{
  <view exposure-id="test" />;
  <view exposure-scene="test" />;
  <view exposure-area="50%" />;
  <view enable-exposure-ui-margin={true} />;
  <view enable-exposure-ui-clip={true} />;
  <view exposure-screen-margin-top="10px" />;
  <view exposure-screen-margin-right="10px" />;
  <view exposure-screen-margin-bottom="10px" />;
  <view exposure-screen-margin-left="10px" />;
  <view exposure-ui-margin-top="10px" />;
  <view exposure-ui-margin-right="10px" />;
  <view exposure-ui-margin-bottom="10px" />;
  <view exposure-ui-margin-left="10px" />;
  <view user-interaction-enabled={true} />;
  <view native-interaction-enabled={true} />;
  <view block-native-event={true} />;
  <view block-native-event-areas={[['0px', '0px', '100%', '300px']]} />;
  <view
    consume-slide-event={[
      [-135, -180],
      [135, 180],
      [-45, 45],
    ]}
  />;
  <view event-through={false} />;
  <view enable-touch-pseudo-propagation={true} />;
  <view hit-slop="10px" />;
  <view hit-slop={{ top: 10, bottom: 10, left: 10, right: 10 }} />;
  <view ignore-focus={true} />;
  <view ios-enable-simultaneous-touch={true} />;
  <view event-through-active-regions={[['0px', '0px', '100%', '300px']]} />;
  // @ts-expect-error: event-through-active-regions only accept string array
  <view event-through-active-regions={[[0, 0, 100, 300]]} />;
  // @ts-expect-error: event-through-active-regions only accept string array of 4 elements.
  <view event-through-active-regions={[['0px', '0px', '100%']]} />;
}
