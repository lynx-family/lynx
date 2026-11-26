// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod } from '../events';
import { StandardProps } from '../props';
import { LatLng } from './map';

export interface MapMarkerProps extends StandardProps {
  /**
   * Geographic coordinates of the marker.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  position?: LatLng;
  /**
   * Normalized anchor within the marker, measured from its top-left corner.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  anchor?: [number, number];
  /**
   * Marker stacking order. Defaults to 1 on Android and Harmony.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  'z-index'?: number;
  /**
   * Specify the alpha of map marker, range is [0, 1]
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  alpha?: number;
  /**
   * Specify whether the map marker is clickable
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  'enable-click'?: boolean;
  /**
   * Map marker click event
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  bindclickmarker?: (e: BaseEvent<'clickmarker'>) => void;
}

/**
 * Refresh the custom map marker content.
 * @Android 4.5
 * @iOS 4.5
 * @Harmony 4.5
 */
export interface MapMarkerRefreshMethod extends BaseMethod {
  method: 'refreshMarker';
}

export type MapMarkerUIMethods = MapMarkerRefreshMethod;
