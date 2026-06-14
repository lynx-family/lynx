// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BusinessError } from '@ohos.base';
import { BuilderNode, FrameNode, NodeController, UIContext } from '@kit.ArkUI';
import {
  EventHandlerArray,
  LynxResourceRequest,
  LynxResourceType,
  LynxUIMethodConstants,
  UIBase
} from '@lynx/lynx';
import { LynxVideoCommandScheduler, LynxVideoPlayable } from './VideoCommandScheduler';
import {
  cloneLynxVideoAttributes,
  createDefaultLynxVideoAttributes,
  isOnlineVideoSrc,
  LynxVideoAttributes,
  LynxVideoObjectFit,
  shouldMuteVideo,
  updateLynxVideoAttributes
} from './VideoOptions';
import {
  createBufferingEvent,
  createErrorEvent,
  createFirstFrameEvent,
  createTimeUpdateEvent,
  LynxVideoTimeUpdateThrottle,
  shouldEmitEnded
} from './VideoEvents';
import {
  LynxVideoMethodCodes,
  LynxVideoMethodErrorResponse,
  parseVideoSeekPosition,
  respondToVideoMethod
} from './VideoMethodResponse';

const INVALID_SRC_ERROR_CODE = -1;
const PLAYBACK_ERROR_CODE = -2;
const METHOD_TIMEOUT_MS = 30000;
const MISSING_SRC_ERROR_MSG = 'missing video source';

class VideoMethodWaiter {
  resolve: () => void;
  reject: (reason: Error) => void;
  timer: number = -1;
  emitStopped: boolean = true;

  constructor(resolve: () => void, reject: (reason: Error) => void) {
    this.resolve = resolve;
    this.reject = reject;
  }
}

export interface LynxVideoViewDelegate {
  onPrepared(duration: number): void;

  onStart(): void;

  onPause(): void;

  onStop(emitStopped: boolean): void;

  onFinish(): void;

  onSeeked(current: number): void;

  onTimeUpdate(current: number): void;

  onError(errorCode: number, errorMsg: string): void;
}

@ObservedV2
export class VideoViewParams {
  @Trace src: string = '';
  @Trace muted: boolean = false;
  @Trace speed: number = 1.0;
  @Trace objectFit: LynxVideoObjectFit = LynxVideoObjectFit.Contain;
}

export class LynxVideoViewInitParams {
  nodeController: LynxVideoView;
  params: VideoViewParams;

  constructor(nodeController: LynxVideoView, params: VideoViewParams) {
    this.nodeController = nodeController;
    this.params = params;
  }
}

@ComponentV2
struct LynxDefaultVideo {
  @BuilderParam initParams: LynxVideoViewInitParams;

  build() {
    Stack() {
      Video({
        src: this.initParams.params.src,
        currentProgressRate: this.initParams.params.speed,
        posterOptions: { showFirstFrame: true },
        controller: this.initParams.nodeController.controller
      })
        .controls(false)
        .muted(this.initParams.params.muted)
        .loop(false)
        .objectFit(toArkImageFit(this.initParams.params.objectFit))
        .onPrepared((info: PreparedInfo) => {
          this.initParams.nodeController.handlePrepared(info.duration);
        })
        .onStart(() => {
          this.initParams.nodeController.handleStart();
        })
        .onPause(() => {
          this.initParams.nodeController.handlePause();
        })
        .onStop(() => {
          this.initParams.nodeController.handleStop();
        })
        .onFinish(() => {
          this.initParams.nodeController.handleFinish();
        })
        .onSeeked((info: PlaybackInfo) => {
          this.initParams.nodeController.handleSeeked(info.time);
        })
        .onUpdate((info: PlaybackInfo) => {
          this.initParams.nodeController.handleTimeUpdate(info.time);
        })
        .onError((error?: BusinessError) => {
          this.initParams.nodeController.handleError(error);
        })
        .width('100%')
        .height('100%')
    }
    .width('100%')
    .height('100%')
  }
}

@Builder
function lynxDefaultVideoBuilder(initParams: LynxVideoViewInitParams) {
  LynxDefaultVideo({ initParams });
}

@Builder
export function buildVideoNodeContainer(ui: UIBase) {
  if (ui as UIVideo) {
    NodeContainer((ui as UIVideo).videoView)
      .width('100%')
      .height('100%')
  }
}

export class LynxVideoView extends NodeController implements LynxVideoPlayable {
  readonly controller: VideoController = new VideoController();
  private params: VideoViewParams;
  private delegate: LynxVideoViewDelegate;
  private videoNode: BuilderNode<[LynxVideoViewInitParams]> | null = null;
  private initParams: LynxVideoViewInitParams;
  private attributes: LynxVideoAttributes = createDefaultLynxVideoAttributes();
  private duration: number = 0;
  private current: number = 0;
  private playing: boolean = false;
  private playWaiters: VideoMethodWaiter[] = [];
  private pauseWaiters: VideoMethodWaiter[] = [];
  private stopWaiters: VideoMethodWaiter[] = [];
  private seekWaiters: VideoMethodWaiter[] = [];

  constructor(params: VideoViewParams, delegate: LynxVideoViewDelegate) {
    super();
    this.params = params;
    this.delegate = delegate;
    this.initParams = new LynxVideoViewInitParams(this, this.params);
  }

  makeNode(uiContext: UIContext): FrameNode | null {
    this.videoNode = new BuilderNode(uiContext);
    this.videoNode.build(wrapBuilder<[LynxVideoViewInitParams]>(lynxDefaultVideoBuilder), this.initParams);
    return this.videoNode.getFrameNode();
  }

  setAttributes(attributes: LynxVideoAttributes): void {
    this.attributes = cloneLynxVideoAttributes(attributes);
    this.params.src = attributes.src;
    this.params.muted = shouldMuteVideo(attributes);
    this.params.speed = attributes.speed;
    this.params.objectFit = attributes.objectFit;
    this.videoNode?.update(this.initParams);
  }

  play(): Promise<void> {
    if (!this.hasSource()) {
      this.delegate.onError(LynxUIMethodConstants.OPERATION_ERROR, MISSING_SRC_ERROR_MSG);
      return Promise.reject(new Error(MISSING_SRC_ERROR_MSG));
    }
    if (this.playing) {
      return Promise.resolve();
    }
    return this.waitForEvent(this.playWaiters, () => {
      this.controller.start();
    });
  }

  pause(): Promise<void> {
    if (!this.hasSource()) {
      return Promise.reject(new Error('video src is empty'));
    }
    if (!this.playing) {
      return Promise.resolve();
    }
    return this.waitForEvent(this.pauseWaiters, () => {
      this.controller.pause();
    });
  }

  stop(emitStopped: boolean = true): Promise<void> {
    if (!this.hasSource()) {
      this.playing = false;
      this.delegate.onStop(emitStopped);
      return Promise.resolve();
    }
    return this.waitForEvent(this.stopWaiters, () => {
      this.controller.stop();
    }, (waiter: VideoMethodWaiter) => {
      waiter.emitStopped = emitStopped;
    });
  }

  seek(position: number): Promise<void> {
    if (!this.hasSource()) {
      return Promise.reject(new Error('video src is empty'));
    }
    if (position < 0 || (this.duration > 0 && position > this.duration)) {
      return Promise.reject(new Error('seek position out of range'));
    }
    return this.waitForEvent(this.seekWaiters, () => {
      this.controller.setCurrentTime(position, SeekMode.Accurate);
    });
  }

  replayFromStart(): boolean {
    if (!this.hasSource()) {
      return false;
    }
    try {
      this.controller.setCurrentTime(0, SeekMode.Accurate);
      this.controller.start();
      return true;
    } catch (error) {
      this.delegate.onError(PLAYBACK_ERROR_CODE, `${error}`);
      return false;
    }
  }

  cancelPending(reason: string): void {
    this.playing = false;
    const error = new Error(reason);
    this.rejectAll(this.playWaiters, error);
    this.rejectAll(this.pauseWaiters, error);
    this.rejectAll(this.stopWaiters, error);
    this.rejectAll(this.seekWaiters, error);
  }

  disposePlayable(reason: string = 'video disposed'): void {
    this.cancelPending(reason);
  }

  handlePrepared(duration: number): void {
    this.duration = Math.max(0, duration);
    this.delegate.onPrepared(this.duration);
  }

  handleStart(): void {
    this.playing = true;
    this.delegate.onStart();
    this.resolveAll(this.playWaiters);
  }

  handlePause(): void {
    this.playing = false;
    this.delegate.onPause();
    this.resolveAll(this.pauseWaiters);
  }

  handleStop(): void {
    this.playing = false;
    const emitStopped = this.stopWaiters.some((waiter: VideoMethodWaiter) => waiter.emitStopped);
    this.delegate.onStop(emitStopped);
    this.resolveAll(this.stopWaiters);
  }

  handleFinish(): void {
    this.playing = false;
    this.delegate.onFinish();
  }

  handleSeeked(current: number): void {
    this.current = Math.max(0, current);
    this.delegate.onSeeked(this.current);
    this.resolveAll(this.seekWaiters);
  }

  handleTimeUpdate(current: number): void {
    this.current = Math.max(0, current);
    this.delegate.onTimeUpdate(this.current);
  }

  handleError(error?: BusinessError): void {
    const message = error === undefined ? 'video playback error' : error.message;
    const code = error === undefined ? PLAYBACK_ERROR_CODE : error.code;
    this.rejectAll(this.playWaiters, new Error(message));
    this.rejectAll(this.pauseWaiters, new Error(message));
    this.rejectAll(this.stopWaiters, new Error(message));
    this.rejectAll(this.seekWaiters, new Error(message));
    this.delegate.onError(code, message);
  }

  private hasSource(): boolean {
    return this.params.src.length > 0;
  }

  private waitForEvent(
    waiters: VideoMethodWaiter[],
    trigger: () => void,
    configureWaiter?: (waiter: VideoMethodWaiter) => void
  ): Promise<void> {
    return new Promise<void>((resolve: () => void, reject: (reason: Error) => void) => {
      const waiter = new VideoMethodWaiter(resolve, reject);
      configureWaiter?.(waiter);
      waiter.timer = setTimeout(() => {
        this.removeWaiter(waiters, waiter);
        reject(new Error('video operation timed out'));
      }, METHOD_TIMEOUT_MS);
      waiters.push(waiter);
      try {
        trigger();
      } catch (error) {
        this.removeWaiter(waiters, waiter);
        clearTimeout(waiter.timer);
        reject(error as Error);
      }
    });
  }

  private resolveAll(waiters: VideoMethodWaiter[]): void {
    const pending = waiters.splice(0, waiters.length);
    for (const waiter of pending) {
      clearTimeout(waiter.timer);
      waiter.resolve();
    }
  }

  private rejectAll(waiters: VideoMethodWaiter[], error: Error): void {
    const pending = waiters.splice(0, waiters.length);
    for (const waiter of pending) {
      clearTimeout(waiter.timer);
      waiter.reject(error);
    }
  }

  private removeWaiter(waiters: VideoMethodWaiter[], waiter: VideoMethodWaiter): void {
    const index = waiters.indexOf(waiter);
    if (index >= 0) {
      waiters.splice(index, 1);
    }
  }
}

export class UIVideo extends UIBase implements LynxVideoViewDelegate {
  readonly builder: WrappedBuilder<[UIBase]> = wrapBuilder<[UIBase]>(buildVideoNodeContainer);
  private videoParams: VideoViewParams = new VideoViewParams();
  public videoView: LynxVideoView = new LynxVideoView(this.videoParams, this);
  private attributes: LynxVideoAttributes = createDefaultLynxVideoAttributes();
  private scheduler: LynxVideoCommandScheduler = new LynxVideoCommandScheduler();
  private timeUpdateThrottle: LynxVideoTimeUpdateThrottle = new LynxVideoTimeUpdateThrottle();
  private firstFrameSent: boolean = false;
  private suppressNextPlaying: boolean = false;
  private duration: number = 0;
  private methodCodes: LynxVideoMethodCodes = {
    success: LynxUIMethodConstants.SUCCESS,
    paramInvalid: LynxUIMethodConstants.PARAM_INVALID,
    operationError: LynxUIMethodConstants.OPERATION_ERROR,
  };

  layout(x: number, y: number, width: number, height: number, paddingLeft: number, paddingTop: number,
    paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number, marginRight: number,
    marginBottom: number, sticky?: number[] | undefined): void {
  }

  update(props: Record<string, Object>, events?: EventHandlerArray[]): void {
    super.update(props, events);
    const update = updateLynxVideoAttributes(this.attributes, props);
    this.attributes = update.attributes;
    this.scheduler.setMode(this.attributes.mode);

    if (update.srcChanged) {
      this.firstFrameSent = false;
      this.suppressNextPlaying = false;
      this.timeUpdateThrottle.reset();
      this.scheduler.cancelAll('video operation canceled by src change');
      this.videoView.cancelPending('video operation canceled by src change');
      this.videoView.stop(false).catch(() => {
      });
    }

    const playableAttributes = this.createPlayableAttributes(this.attributes);
    this.videoView.setAttributes(playableAttributes);

    if (update.srcChanged && this.attributes.src.length > 0) {
      if (isOnlineVideoSrc(this.attributes.src)) {
        this.sendBuffering(0);
      } else {
        this.sendError(INVALID_SRC_ERROR_CODE, 'invalid video src');
      }
    }
  }

  invokeMethod(method: string, params: Object, callback: (code: number, res: Object) => void): boolean {
    switch (method) {
      case 'play':
        this.respondToMethod(callback, this.scheduler.schedule({
          name: method,
          run: () => this.videoView.play(),
        }));
        return true;
      case 'pause':
        this.respondToMethod(callback, this.scheduler.schedule({
          name: method,
          run: () => this.videoView.pause(),
        }));
        return true;
      case 'stop':
        this.respondToMethod(callback, this.scheduler.schedule({
          name: method,
          run: () => this.videoView.stop(true),
        }));
        return true;
      case 'seek':
        const position = parseVideoSeekPosition(params);
        if (position === undefined) {
          callback(LynxUIMethodConstants.PARAM_INVALID, new LynxVideoMethodErrorResponse('seek position is invalid'));
          return true;
        }
        this.respondToMethod(callback, this.scheduler.schedule({
          name: method,
          run: () => this.videoView.seek(position),
        }));
        return true;
      default:
        return false;
    }
  }

  dispose(): void {
    this.scheduler.cancelAll('video operation canceled by destroy');
    this.videoView.disposePlayable('video operation canceled by destroy');
    super.dispose();
  }

  onPrepared(duration: number): void {
    this.duration = duration;
    if (!this.firstFrameSent) {
      this.firstFrameSent = true;
      this.sendEvent('firstframe', createFirstFrameEvent(duration));
    }
  }

  onStart(): void {
    if (this.suppressNextPlaying) {
      this.suppressNextPlaying = false;
      return;
    }
    this.sendEvent('playing', new Object());
  }

  onPause(): void {
    this.sendEvent('paused', new Object());
  }

  onStop(emitStopped: boolean): void {
    if (emitStopped) {
      this.sendEvent('stopped', new Object());
    }
  }

  onFinish(): void {
    if (shouldEmitEnded(this.attributes.loop)) {
      this.sendEvent('ended', new Object());
      return;
    }
    this.sendEvent('looped', new Object());
    this.suppressNextPlaying = this.videoView.replayFromStart();
  }

  onSeeked(current: number): void {
    this.onTimeUpdate(current);
  }

  onTimeUpdate(current: number): void {
    if (this.timeUpdateThrottle.shouldDispatch(new Date().getTime(), this.attributes.timeUpdateInterval)) {
      this.sendEvent('timeupdate', createTimeUpdateEvent(current, this.duration));
    }
  }

  onError(errorCode: number, errorMsg: string): void {
    this.suppressNextPlaying = false;
    this.sendError(errorCode, errorMsg);
  }

  private createPlayableAttributes(attributes: LynxVideoAttributes): LynxVideoAttributes {
    const playableAttributes = cloneLynxVideoAttributes(attributes);
    if (!isOnlineVideoSrc(attributes.src)) {
      playableAttributes.src = '';
      return playableAttributes;
    }
    playableAttributes.src = this.resolveVideoSrc(attributes.src);
    return playableAttributes;
  }

  private resolveVideoSrc(src: string): string {
    const fetcher = this.lynxContext.getMediaResourceFetcher();
    if (fetcher === undefined) {
      return src;
    }
    return fetcher.shouldRedirectUrl(new LynxResourceRequest(src, LynxResourceType.LYNX_RESOURCE_TYPE_VIDEO));
  }

  private respondToMethod(callback: (code: number, res: Object) => void, promise: Promise<void>): void {
    respondToVideoMethod(callback, promise, this.methodCodes);
  }

  private sendError(errorCode: number, errorMsg: string): void {
    this.sendEvent('error', createErrorEvent(errorCode, errorMsg));
  }

  private sendBuffering(buffering: number): void {
    this.sendEvent('buffering', createBufferingEvent(buffering));
  }

  private sendEvent(name: string, params: Object): void {
    super.sendCustomEvent(name, params, 'detail');
  }
}

function toArkImageFit(objectFit: LynxVideoObjectFit): ImageFit {
  switch (objectFit) {
    case LynxVideoObjectFit.Cover:
      return ImageFit.Cover;
    case LynxVideoObjectFit.Fill:
      return ImageFit.Fill;
    case LynxVideoObjectFit.Contain:
    default:
      return ImageFit.Contain;
  }
}
