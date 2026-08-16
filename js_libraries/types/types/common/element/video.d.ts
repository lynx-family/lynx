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
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @Web
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  src?: string;

  /**
   * Whether to loop playback.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @Web
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   * @defaultValue false
   */
  loop?: boolean;

  /**
   * Playback volume from 0 to 1.
   * @Android 4.1
   * @iOS 4.1
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @defaultValue 1.0
   */
  volume?: number;

  /**
   * Whether the video is muted.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @Web
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @defaultValue false
   */
  muted?: boolean;

  /**
   * Playback speed from 0.1 to 2.0.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @defaultValue 1.0
   */
  speed?: number;

  /**
   * Video scaling strategy.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @defaultValue 'contain'
   */
  'object-fit'?: VideoObjectFit;

  /**
   * UIMethod execution mode.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @defaultValue 'queue'
   */
  mode?: VideoUIMethodMode;

  /**
   * Minimum interval for timeupdate dispatch, in seconds.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @defaultValue 0.33
   */
  'timeupdate-interval'?: number;

  /**
   * Fired when the first video frame has loaded.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   */
  bindfirstframe?: (e: VideoFirstFrameEvent) => void;

  /**
   * Fired when video playback starts or resumes.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @Web
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   */
  bindplaying?: (e: VideoPlayingEvent) => void;

  /**
   * Fired when video playback pauses.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   */
  bindpaused?: (e: VideoPausedEvent) => void;

  /**
   * Fired when video playback is stopped by the stop UIMethod.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   */
  bindstopped?: (e: VideoStoppedEvent) => void;

  /**
   * Fired when the playback position updates.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @Web
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   */
  bindtimeupdate?: (e: VideoTimeUpdateEvent) => void;

  /**
   * Fired when video playback fully ends.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @Web
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   */
  bindended?: (e: VideoEndedEvent) => void;

  /**
   * Fired at the end of each loop iteration.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   */
  bindlooped?: (e: VideoLoopedEvent) => void;

  /**
   * Fired when a video playback error occurs.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
   * @Web
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  binderror?: (e: VideoErrorEvent) => void;

  /**
   * Fired while the video is buffering.
   * @Android 4.1
   * @iOS 4.1
   * @Harmony 4.1
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
 * @Android 4.1
 * @iOS 4.1
 * @Harmony 4.1
 * @Web
 * @ClayIOS 2.14
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 * @ClayHarmony 2.14
 */
export interface VideoPlayMethod extends VideoBaseMethod {
  method: 'play';
}

/**
 * Pause video playback.
 * @Android 4.1
 * @iOS 4.1
 * @Harmony 4.1
 * @Web
 * @ClayIOS 2.14
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 */
export interface VideoPauseMethod extends VideoBaseMethod {
  method: 'pause';
}

/**
 * Stop video playback.
 * @Android 4.1
 * @iOS 4.1
 * @Harmony 4.1
 * @ClayIOS 2.14
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 */
export interface VideoStopMethod extends VideoBaseMethod {
  method: 'stop';
}

/**
 * Seek to the target playback position.
 * @Android 4.1
 * @iOS 4.1
 * @Harmony 4.1
 * @ClayIOS 2.14
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
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
