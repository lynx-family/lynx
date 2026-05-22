// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod } from '../events';
import { StandardProps } from '../props';

export interface VideoFirstFrameEvent {
  /**
   * Total video duration in seconds.
   * @Android
   * @iOS
   * @Harmony
   */
  duration: number;
}

export interface VideoTimeUpdateEvent {
  /**
   * Current playback position in seconds.
   * @Android
   * @iOS
   * @Harmony
   */
  current: number;
  /**
   * Total video duration in seconds.
   * @Android
   * @iOS
   * @Harmony
   */
  duration: number;
}

export interface VideoErrorEvent {
  /**
   * Platform playback error code.
   * @Android
   * @iOS
   * @Harmony
   */
  errorCode: number;
  /**
   * Platform playback error message.
   * @Android
   * @iOS
   * @Harmony
   */
  errorMsg: string;
}

export interface VideoBufferingEvent {
  /**
   * Buffered playback duration in seconds.
   * @Android
   * @iOS
   * @Harmony
   */
  buffering: number;
}

export interface VideoProps extends Omit<StandardProps, 'binderror'> {
  /**
   * Video source URL. Only online network URLs are supported.
   * @Android
   * @iOS
   * @Harmony
   */
  src?: string;
  /**
   * Whether to loop playback.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   */
  loop?: boolean;
  /**
   * Volume from 0 to 1.
   * @defaultValue 1
   * @Android
   * @iOS
   * @Harmony
   */
  volume?: number;
  /**
   * Whether the video is muted.
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   */
  muted?: boolean;
  /**
   * Playback speed from 0.1 to 2.0.
   * @defaultValue 1
   * @Android
   * @iOS
   * @Harmony
   */
  speed?: number;
  /**
   * Video scaling strategy.
   * @defaultValue contain
   * @Android
   * @iOS
   * @Harmony
   */
  'object-fit'?: 'contain' | 'cover' | 'fill';
  /**
   * UIMethod execution mode.
   * @defaultValue queue
   * @Android
   * @iOS
   * @Harmony
   */
  mode?: 'queue' | 'direct' | 'latest';
  /**
   * Minimum dispatch interval for bindtimeupdate in seconds.
   * @defaultValue 0.33
   * @Android
   * @iOS
   * @Harmony
   */
  'timeupdate-interval'?: number;
  /**
   * Fired when the first video frame has loaded.
   * @Android
   * @iOS
   * @Harmony
   */
  bindfirstframe?: (e: BaseEvent<'firstframe', VideoFirstFrameEvent>) => void;
  /**
   * Fired when playback starts or resumes.
   * @Android
   * @iOS
   * @Harmony
   */
  bindplaying?: (e: BaseEvent<'playing'>) => void;
  /**
   * Fired when playback pauses.
   * @Android
   * @iOS
   * @Harmony
   */
  bindpaused?: (e: BaseEvent<'paused'>) => void;
  /**
   * Fired only when playback is stopped by the stop UIMethod.
   * @Android
   * @iOS
   * @Harmony
   */
  bindstopped?: (e: BaseEvent<'stopped'>) => void;
  /**
   * Fired when playback position updates.
   * @Android
   * @iOS
   * @Harmony
   */
  bindtimeupdate?: (e: BaseEvent<'timeupdate', VideoTimeUpdateEvent>) => void;
  /**
   * Fired when playback fully ends.
   * @Android
   * @iOS
   * @Harmony
   */
  bindended?: (e: BaseEvent<'ended'>) => void;
  /**
   * Fired at the end of each loop iteration.
   * @Android
   * @iOS
   * @Harmony
   */
  bindlooped?: (e: BaseEvent<'looped'>) => void;
  /**
   * Fired when playback error occurs.
   * @Android
   * @iOS
   * @Harmony
   */
  binderror?: (e: BaseEvent<'error', VideoErrorEvent>) => void;
  /**
   * Fired while the video is buffering.
   * @Android
   * @iOS
   * @Harmony
   */
  bindbuffering?: (e: BaseEvent<'buffering', VideoBufferingEvent>) => void;
}

export interface VideoPlayMethod extends BaseMethod {
  method: 'play';
}

export interface VideoPauseMethod extends BaseMethod {
  method: 'pause';
}

export interface VideoStopMethod extends BaseMethod {
  method: 'stop';
}

export interface VideoSeekMethod extends BaseMethod {
  method: 'seek';
  params: {
    /**
     * Target playback position in seconds.
     * @Android
     * @iOS
     * @Harmony
     */
    position: number;
  };
}

export type VideoUIMethods = VideoPlayMethod | VideoPauseMethod | VideoStopMethod | VideoSeekMethod;
