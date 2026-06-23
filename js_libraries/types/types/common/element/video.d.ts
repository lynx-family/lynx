// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, Callback } from '../events';
import { StandardProps } from '../props';

export type VideoObjectFit = 'contain' | 'cover' | 'fill';

export type VideoUIMethodMode = 'queue' | 'direct' | 'latest';

export interface VideoFirstFrameEventDetail {
  /**
   * Total video duration, in seconds.
   * @Android
   * @iOS
   * @Harmony
   */
  duration: number;
}

export interface VideoTimeUpdateEventDetail {
  /**
   * Current playback position, in seconds.
   * @Android
   * @iOS
   * @Harmony
   */
  current: number;

  /**
   * Total video duration, in seconds.
   * @Android
   * @iOS
   * @Harmony
   */
  duration: number;
}

export interface VideoErrorEventDetail {
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

export interface VideoBufferingEventDetail {
  /**
   * Buffered end position on the media timeline, in seconds.
   * @Android
   * @iOS
   * @Harmony
   */
  buffering: number;
}

export type VideoFirstFrameEvent = BaseEvent<'bindfirstframe', VideoFirstFrameEventDetail>;
export type VideoPlayingEvent = BaseEvent<'bindplaying'>;
export type VideoPausedEvent = BaseEvent<'bindpaused'>;
export type VideoStoppedEvent = BaseEvent<'bindstopped'>;
export type VideoTimeUpdateEvent = BaseEvent<'bindtimeupdate', VideoTimeUpdateEventDetail>;
export type VideoEndedEvent = BaseEvent<'bindended'>;
export type VideoLoopedEvent = BaseEvent<'bindlooped'>;
export type VideoErrorEvent = BaseEvent<'binderror', VideoErrorEventDetail>;
export type VideoBufferingEvent = BaseEvent<'bindbuffering', VideoBufferingEventDetail>;

/**
 * Experimental video playback element.
 * @experimental
 */
export interface VideoProps extends StandardProps {
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
   * Playback volume from 0 to 1.
   * @defaultValue 1.0
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
   * @defaultValue 1.0
   * @Android
   * @iOS
   * @Harmony
   */
  speed?: number;

  /**
   * Video scaling strategy.
   * @defaultValue 'contain'
   * @Android
   * @iOS
   * @Harmony
   */
  'object-fit'?: VideoObjectFit;

  /**
   * UIMethod execution mode.
   * @defaultValue 'queue'
   * @Android
   * @iOS
   * @Harmony
   */
  mode?: VideoUIMethodMode;

  /**
   * Minimum interval for timeupdate dispatch, in seconds.
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
  bindfirstframe?: (e: VideoFirstFrameEvent) => void;

  /**
   * Fired when video playback starts or resumes.
   * @Android
   * @iOS
   * @Harmony
   */
  bindplaying?: (e: VideoPlayingEvent) => void;

  /**
   * Fired when video playback pauses.
   * @Android
   * @iOS
   * @Harmony
   */
  bindpaused?: (e: VideoPausedEvent) => void;

  /**
   * Fired when video playback is stopped by the stop UIMethod.
   * @Android
   * @iOS
   * @Harmony
   */
  bindstopped?: (e: VideoStoppedEvent) => void;

  /**
   * Fired when the playback position updates.
   * @Android
   * @iOS
   * @Harmony
   */
  bindtimeupdate?: (e: VideoTimeUpdateEvent) => void;

  /**
   * Fired when video playback fully ends.
   * @Android
   * @iOS
   * @Harmony
   */
  bindended?: (e: VideoEndedEvent) => void;

  /**
   * Fired at the end of each loop iteration.
   * @Android
   * @iOS
   * @Harmony
   */
  bindlooped?: (e: VideoLoopedEvent) => void;

  /**
   * Fired when a video playback error occurs.
   * @Android
   * @iOS
   * @Harmony
   */
  binderror?: (e: VideoErrorEvent) => void;

  /**
   * Fired while the video is buffering.
   * @Android
   * @iOS
   * @Harmony
   */
  bindbuffering?: (e: VideoBufferingEvent) => void;
}

export interface VideoMethodResponse {
  /**
   * Whether the operation succeeded when provided by the platform callback.
   * @Android
   * @iOS
   */
  success?: boolean;

  /**
   * Error code when the operation fails.
   * @Android
   * @iOS
   */
  errorCode?: number;

  /**
   * Error message when the operation fails.
   * @Android
   * @iOS
   */
  msg?: string;

  /**
   * Error message when the operation fails.
   * @Harmony
   */
  errorMsg?: string;
}

export interface VideoBaseMethod extends BaseMethod {
  success?: Callback<VideoMethodResponse>;
  fail?: Callback<VideoMethodResponse>;
}

/**
 * Play the video.
 * @Android
 * @iOS
 * @Harmony
 */
export interface VideoPlayMethod extends VideoBaseMethod {
  method: 'play';
}

/**
 * Pause video playback.
 * @Android
 * @iOS
 * @Harmony
 */
export interface VideoPauseMethod extends VideoBaseMethod {
  method: 'pause';
}

/**
 * Stop video playback.
 * @Android
 * @iOS
 * @Harmony
 */
export interface VideoStopMethod extends VideoBaseMethod {
  method: 'stop';
}

/**
 * Seek to the target playback position.
 * @Android
 * @iOS
 * @Harmony
 */
export interface VideoSeekMethod extends VideoBaseMethod {
  method: 'seek';
  params: {
    /**
     * Target playback position, in seconds.
     * @Android
     * @iOS
     * @Harmony
     */
    position: number;
  };
}

export type VideoUIMethods =
  | VideoPlayMethod
  | VideoPauseMethod
  | VideoStopMethod
  | VideoSeekMethod;
