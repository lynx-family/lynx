// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { expectAssignable, expectNotAssignable } from 'tsd';
import { IntrinsicElements, UIMethods } from '../../../types';

const harbor = { latitude: 37.7955, longitude: -122.3937 };
expectAssignable<IntrinsicElements['map']>({ center: harbor, scale: 15 });
expectAssignable<IntrinsicElements['map-marker']>({ position: harbor, anchor: [0.5, 1] });
expectNotAssignable<IntrinsicElements['map-marker']>({ anchor: [1, 2, 3] });
expectNotAssignable<keyof IntrinsicElements['map']>('route-label');
expectAssignable<UIMethods['map-marker']>({ method: 'refreshMarker' });
