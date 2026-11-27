// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, Callback } from '../events';
import { StandardProps } from '../props';

/**
 * A geographic coordinate in degrees.
 * @Android 4.5
 * @iOS 4.5
 * @Harmony 4.5
 */
export interface LatLng {
  /**
   * Latitude in degrees.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  latitude: number;
  /**
   * Longitude in degrees.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  longitude: number;
}

export interface MapProps extends StandardProps {
  /**
   * Geographic coordinates at the center of the map.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  center?: LatLng;
  /**
   * Camera zoom level.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  scale?: number;
  /**
   * Enable pinch-to-zoom gestures.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  'enable-scale'?: boolean;
  /**
   * Enable panning gestures.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  'enable-drag'?: boolean;
  /**
   * Enable rotation gestures.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  'enable-rotate'?: boolean;
  /**
   * Enable camera tilt gestures.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  'enable-tilt'?: boolean;
  /**
   * Called when the map finishes loading.
   * @Android 4.5
   * @iOS 4.5
   * @Harmony 4.5
   */
  bindmapload?: (e: BaseEvent<'mapload'>) => void;
}

/**
 * Set the map center.
 * @Android 4.5
 * @iOS 4.5
 * @Harmony 4.5
 */
export interface MapSetCenterMethod extends BaseMethod {
  method: 'setCenter';
  params: {
    /**
     * Set the map scale together with its center.
     * @Android 4.5
     * @iOS 4.5
     * @Harmony 4.5
     */
    scale?: number;
    /**
     * Specify the center of map
     * @Android 4.5
     * @iOS 4.5
     * @Harmony 4.5
     */
    center: LatLng;
    /**
     * Specify whether to animate the map viewport change
     * @defaultValue true
     * @Android 4.5
     * @iOS 4.5
     * @Harmony 4.5
     */
    animated?: boolean;
    /**
     * Specify the animation duration in milliseconds
     * @defaultValue 250
     * @Android 4.5
     * @iOS 4.5
     * @Harmony 4.5
     */
    duration?: number;
  };
}

/**
 * Get the map center.
 * @Android 4.5
 * @iOS 4.5
 * @Harmony 4.5
 */
export interface MapGetCenterMethod extends BaseMethod {
  method: 'getCenter';
  success: Callback<LatLng>;
}

/**
 * Set the map scale.
 * @Android 4.5
 * @iOS 4.5
 * @Harmony 4.5
 */
export interface MapSetScaleMethod extends BaseMethod {
  method: 'setScale';
  params: {
    /**
     * Specify the scale of map
     * @Android 4.5
     * @iOS 4.5
     * @Harmony 4.5
     */
    scale: number;
    /**
     * Specify whether to animate the map viewport change
     * @defaultValue true
     * @Android 4.5
     * @iOS 4.5
     * @Harmony 4.5
     */
    animated?: boolean;
    /**
     * Specify the animation duration in milliseconds
     * @defaultValue 250
     * @Android 4.5
     * @iOS 4.5
     * @Harmony 4.5
     */
    duration?: number;
  };
}

/**
 * Get the map scale.
 * @Android 4.5
 * @iOS 4.5
 * @Harmony 4.5
 */
export interface MapGetScaleMethod extends BaseMethod {
  method: 'getScale';
  success: Callback<{
    /**
     * The scale of map
     * @Android 4.5
     * @iOS 4.5
     * @Harmony 4.5
     */
    scale: number;
  }>;
}

export type MapUIMethods = MapSetCenterMethod | MapGetCenterMethod | MapSetScaleMethod | MapGetScaleMethod;
