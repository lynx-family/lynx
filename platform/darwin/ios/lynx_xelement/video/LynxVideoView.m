// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AVFoundation/AVFoundation.h>
#import <XElement/LynxVideoView.h>
#import <stdint.h>

static void *kLynxVideoItemStatusContext = &kLynxVideoItemStatusContext;
static void *kLynxVideoLoadedRangesContext = &kLynxVideoLoadedRangesContext;
static void *kLynxVideoLayerReadyContext = &kLynxVideoLayerReadyContext;
static void *kLynxVideoTimeControlStatusContext = &kLynxVideoTimeControlStatusContext;
static const NSTimeInterval kLynxVideoMaxSeekPosition =
    (NSTimeInterval)INT64_MAX / (NSTimeInterval)NSEC_PER_SEC;

static BOOL LynxVideoIsRepresentableSeekPosition(NSTimeInterval position) {
  return isfinite(position) && position <= kLynxVideoMaxSeekPosition;
}

@interface LynxVideoView ()
@property(nonatomic, strong) AVPlayer *player;
@property(nonatomic, strong, nullable) AVPlayerItem *playerItem;
@property(nonatomic, strong, nullable) id timeObserver;
@property(nonatomic, assign) BOOL observingLayer;
@property(nonatomic, assign) BOOL observingPlayer;
@property(nonatomic, assign) BOOL observingItem;
@property(nonatomic, assign) BOOL firstFrameDispatched;
@property(nonatomic, assign, getter=isPlaying) BOOL playing;
@property(nonatomic, assign) BOOL playbackEnded;
@end

@implementation LynxVideoView

@synthesize delegate = _delegate;
@synthesize src = _src;
@synthesize loop = _loop;
@synthesize volume = _volume;
@synthesize muted = _muted;
@synthesize speed = _speed;
@synthesize objectFit = _objectFit;
@synthesize timeUpdateInterval = _timeUpdateInterval;

+ (Class)layerClass {
  return [AVPlayerLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    _volume = 1.0;
    _speed = 1.0;
    _objectFit = LynxVideoObjectFitContain;
    _timeUpdateInterval = 0.33;
    self.clipsToBounds = YES;
    [self.playerLayer addObserver:self
                       forKeyPath:@"readyForDisplay"
                          options:NSKeyValueObservingOptionNew
                          context:kLynxVideoLayerReadyContext];
    _observingLayer = YES;
    [self applyObjectFit];
  }
  return self;
}

- (void)dealloc {
  [self clearCurrentItem];
  [self clearTimeObserver];
  if (self.observingPlayer) {
    [self.player removeObserver:self forKeyPath:@"timeControlStatus"];
    self.observingPlayer = NO;
  }
  if (self.observingLayer) {
    [self.playerLayer removeObserver:self forKeyPath:@"readyForDisplay"];
    self.observingLayer = NO;
  }
}

- (AVPlayerLayer *)playerLayer {
  return (AVPlayerLayer *)self.layer;
}

- (void)dispatchOnMainThread:(dispatch_block_t)block {
  if (!block) {
    return;
  }
  if ([NSThread isMainThread]) {
    block();
    return;
  }
  dispatch_async(dispatch_get_main_queue(), block);
}

- (NSTimeInterval)duration {
  CMTime duration = self.playerItem.duration;
  if (!CMTIME_IS_NUMERIC(duration) || CMTIME_IS_INDEFINITE(duration)) {
    return 0;
  }
  Float64 seconds = CMTimeGetSeconds(duration);
  return isfinite(seconds) ? seconds : 0;
}

- (void)setSrc:(NSString *)src {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    NSString *srcCopy = [src copy];
    [self dispatchOnMainThread:^{
      weakSelf.src = srcCopy;
    }];
    return;
  }
  if ((_src == src) || [_src isEqualToString:src]) {
    return;
  }
  _src = [src copy];
  [self loadSource:_src];
}

- (void)setLoop:(BOOL)loop {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      weakSelf.loop = loop;
    }];
    return;
  }
  _loop = loop;
}

- (void)setVolume:(CGFloat)volume {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      weakSelf.volume = volume;
    }];
    return;
  }
  _volume = MIN(1.0, MAX(0.0, volume));
  [self applyAudioState];
}

- (void)setMuted:(BOOL)muted {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      weakSelf.muted = muted;
    }];
    return;
  }
  _muted = muted;
  [self applyAudioState];
}

- (void)setSpeed:(CGFloat)speed {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      weakSelf.speed = speed;
    }];
    return;
  }
  _speed = MIN(2.0, MAX(0.1, speed));
  if (self.isPlaying) {
    self.player.rate = _speed;
  }
}

- (void)setObjectFit:(LynxVideoObjectFit)objectFit {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      weakSelf.objectFit = objectFit;
    }];
    return;
  }
  _objectFit = objectFit;
  [self applyObjectFit];
}

- (void)setTimeUpdateInterval:(NSTimeInterval)timeUpdateInterval {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      weakSelf.timeUpdateInterval = timeUpdateInterval;
    }];
    return;
  }
  if (timeUpdateInterval <= 0) {
    return;
  }
  _timeUpdateInterval = timeUpdateInterval;
  [self resetTimeObserverIfNeeded];
}

- (void)play {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      [weakSelf play];
    }];
    return;
  }
  if (!self.playerItem) {
    [self notifyErrorWithCode:LynxVideoErrorInvalidSource message:@"invalid video source"];
    return;
  }
  if (self.playerItem.status == AVPlayerItemStatusFailed) {
    [self notifyErrorWithCode:LynxVideoErrorOperation
                      message:self.playerItem.error.localizedDescription ?: @"video load failed"];
    return;
  }
  if (self.playbackEnded) {
    self.playbackEnded = NO;
    __weak typeof(self) weakSelf = self;
    [self.player seekToTime:kCMTimeZero
            toleranceBefore:kCMTimeZero
             toleranceAfter:kCMTimeZero
          completionHandler:^(BOOL finished) {
            __strong typeof(weakSelf) strongSelf = weakSelf;
            if (!strongSelf) {
              return;
            }
            [strongSelf dispatchOnMainThread:^{
              if (!finished) {
                [strongSelf notifyErrorWithCode:LynxVideoErrorOperation message:@"seek failed"];
                return;
              }
              [strongSelf.player playImmediatelyAtRate:strongSelf.speed];
              if (strongSelf.player.timeControlStatus == AVPlayerTimeControlStatusPlaying) {
                [strongSelf notifyPlayingIfNeeded];
              }
            }];
          }];
    return;
  }
  [self.player playImmediatelyAtRate:self.speed];
  if (self.player.timeControlStatus == AVPlayerTimeControlStatusPlaying) {
    [self notifyPlayingIfNeeded];
  }
}

- (void)pause {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      [weakSelf pause];
    }];
    return;
  }
  BOOL shouldNotify = self.isPlaying || self.player.rate != 0;
  [self.player pause];
  if (shouldNotify) {
    self.playing = NO;
    [self notifyPaused];
  }
}

- (void)stop {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      [weakSelf stop];
    }];
    return;
  }
  if (!self.player) {
    self.playing = NO;
    self.playbackEnded = NO;
    [self notifyStopped];
    return;
  }
  [self.player pause];
  self.playing = NO;
  self.playbackEnded = NO;
  AVPlayerItem *itemAtStop = self.player.currentItem;
  if (!itemAtStop) {
    [self notifyStopped];
    return;
  }
  __weak typeof(self) weakSelf = self;
  [self.player seekToTime:kCMTimeZero
          toleranceBefore:kCMTimeZero
           toleranceAfter:kCMTimeZero
        completionHandler:^(BOOL finished) {
          __strong typeof(weakSelf) strongSelf = weakSelf;
          if (!strongSelf || !finished || strongSelf.player.currentItem != itemAtStop) {
            return;
          }
          [strongSelf notifyStopped];
        }];
}

- (void)seekTo:(NSTimeInterval)position
    completion:(void (^)(BOOL success, NSString *_Nullable errorMsg))completion {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      [weakSelf seekTo:position completion:completion];
    }];
    return;
  }
  if (!self.playerItem) {
    if (completion) {
      completion(NO, @"invalid video source");
    }
    return;
  }

  NSTimeInterval duration = self.duration;
  if (!LynxVideoIsRepresentableSeekPosition(position) || position < 0 ||
      (duration > 0 && position > duration)) {
    if (completion) {
      completion(NO, @"position out of range");
    }
    return;
  }

  BOOL shouldResume = self.isPlaying || self.player.rate != 0;
  [self.player pause];
  CMTime target = CMTimeMakeWithSeconds(position, NSEC_PER_SEC);
  __weak typeof(self) weakSelf = self;
  [self.player seekToTime:target
          toleranceBefore:kCMTimeZero
           toleranceAfter:kCMTimeZero
        completionHandler:^(BOOL finished) {
          __strong typeof(weakSelf) strongSelf = weakSelf;
          if (!strongSelf) {
            if (completion) {
              dispatch_async(dispatch_get_main_queue(), ^{
                completion(NO, @"seek failed");
              });
            }
            return;
          }
          [strongSelf dispatchOnMainThread:^{
            strongSelf.playbackEnded = NO;
            if (finished && shouldResume) {
              [strongSelf.player playImmediatelyAtRate:strongSelf.speed];
            }
            if (completion) {
              completion(finished, finished ? nil : @"seek failed");
            }
          }];
        }];
}

- (void)loadSource:(NSString *)src {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    NSString *srcCopy = [src copy];
    [self dispatchOnMainThread:^{
      [weakSelf loadSource:srcCopy];
    }];
    return;
  }
  [self clearCurrentItem];
  self.firstFrameDispatched = NO;
  self.playing = NO;
  self.playbackEnded = NO;
  [self.player pause];

  if (src.length == 0) {
    [self.player replaceCurrentItemWithPlayerItem:nil];
    return;
  }

  NSURL *url = [NSURL URLWithString:src];
  if (![self isSupportedURL:url]) {
    [self.player replaceCurrentItemWithPlayerItem:nil];
    [self notifyErrorWithCode:LynxVideoErrorInvalidSource message:@"invalid video source"];
    return;
  }

  AVPlayerItem *item = [AVPlayerItem playerItemWithURL:url];
  self.playerItem = item;
  [self observeCurrentItem];
  [self ensurePlayer];
  [self.player replaceCurrentItemWithPlayerItem:item];
  [self resetTimeObserverIfNeeded];
}

- (BOOL)isSupportedURL:(NSURL *)url {
  NSString *scheme = url.scheme.lowercaseString;
  return url && ([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"]) &&
         url.host.length > 0;
}

- (void)ensurePlayer {
  if (self.player) {
    return;
  }
  self.player = [[AVPlayer alloc] init];
  self.player.actionAtItemEnd = AVPlayerActionAtItemEndPause;
  [self.player addObserver:self
                forKeyPath:@"timeControlStatus"
                   options:NSKeyValueObservingOptionNew
                   context:kLynxVideoTimeControlStatusContext];
  self.observingPlayer = YES;
  self.playerLayer.player = self.player;
  [self applyAudioState];
}

- (void)observeCurrentItem {
  if (!self.playerItem || self.observingItem) {
    return;
  }
  [self.playerItem addObserver:self
                    forKeyPath:@"status"
                       options:NSKeyValueObservingOptionNew
                       context:kLynxVideoItemStatusContext];
  [self.playerItem addObserver:self
                    forKeyPath:@"loadedTimeRanges"
                       options:NSKeyValueObservingOptionNew
                       context:kLynxVideoLoadedRangesContext];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(playerItemDidPlayToEnd:)
                                               name:AVPlayerItemDidPlayToEndTimeNotification
                                             object:self.playerItem];
  self.observingItem = YES;
}

- (void)clearCurrentItem {
  if (!self.playerItem || !self.observingItem) {
    self.playerItem = nil;
    return;
  }
  [[NSNotificationCenter defaultCenter] removeObserver:self
                                                  name:AVPlayerItemDidPlayToEndTimeNotification
                                                object:self.playerItem];
  [self.playerItem removeObserver:self forKeyPath:@"status"];
  [self.playerItem removeObserver:self forKeyPath:@"loadedTimeRanges"];
  self.observingItem = NO;
  self.playerItem = nil;
}

- (void)resetTimeObserverIfNeeded {
  if (!self.player) {
    return;
  }
  [self clearTimeObserver];
  __weak typeof(self) weakSelf = self;
  CMTime interval = CMTimeMakeWithSeconds(self.timeUpdateInterval, NSEC_PER_SEC);
  self.timeObserver = [self.player addPeriodicTimeObserverForInterval:interval
                                                                queue:dispatch_get_main_queue()
                                                           usingBlock:^(CMTime time) {
                                                             __strong typeof(weakSelf) strongSelf =
                                                                 weakSelf;
                                                             [strongSelf dispatchTimeUpdate:time];
                                                           }];
}

- (void)clearTimeObserver {
  if (self.timeObserver && self.player) {
    [self.player removeTimeObserver:self.timeObserver];
  }
  self.timeObserver = nil;
}

- (void)applyAudioState {
  self.player.volume = self.muted ? 0 : self.volume;
  self.player.muted = self.muted;
}

- (void)applyObjectFit {
  switch (self.objectFit) {
    case LynxVideoObjectFitCover:
      self.playerLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
      break;
    case LynxVideoObjectFitFill:
      self.playerLayer.videoGravity = AVLayerVideoGravityResize;
      break;
    case LynxVideoObjectFitContain:
    default:
      self.playerLayer.videoGravity = AVLayerVideoGravityResizeAspect;
      break;
  }
}

- (void)dispatchTimeUpdate:(CMTime)time {
  if (!self.playerItem || !self.isPlaying) {
    return;
  }
  Float64 current = CMTimeGetSeconds(time);
  if (!isfinite(current)) {
    current = 0;
  }
  NSTimeInterval duration = self.duration;
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf.delegate playable:strongSelf didUpdateTime:current duration:duration];
  }];
}

- (void)dispatchBufferingUpdate {
  [self dispatchBufferingUpdateForItem:self.playerItem];
}

- (void)dispatchBufferingUpdateForItem:(AVPlayerItem *)item {
  if (!item || item != self.playerItem) {
    return;
  }
  NSTimeInterval bufferedEnd = 0;
  for (NSValue *rangeValue in item.loadedTimeRanges) {
    CMTimeRange range = [rangeValue CMTimeRangeValue];
    Float64 end = CMTimeGetSeconds(CMTimeAdd(range.start, range.duration));
    if (isfinite(end)) {
      bufferedEnd = MAX(bufferedEnd, end);
    }
  }
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (item != strongSelf.playerItem) {
      return;
    }
    [strongSelf.delegate playable:strongSelf didUpdateBuffering:bufferedEnd];
  }];
}

- (void)dispatchFirstFrameIfNeeded {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf.firstFrameDispatched || !strongSelf.playerLayer.readyForDisplay) {
      return;
    }
    strongSelf.firstFrameDispatched = YES;
    [strongSelf.delegate playableDidLoadFirstFrame:strongSelf duration:strongSelf.duration];
  }];
}

- (void)notifyPlayingIfNeeded {
  if (self.playing) {
    return;
  }
  self.playing = YES;
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf.delegate playableDidStartPlaying:strongSelf];
  }];
}

- (void)notifyErrorWithCode:(NSInteger)code message:(NSString *)message {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf.delegate playable:strongSelf
                  didFailWithCode:code
                          message:message ?: @"unknown video error"];
  }];
}

- (void)notifyPaused {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf.delegate playableDidPause:strongSelf];
  }];
}

- (void)notifyStopped {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf.delegate playableDidStop:strongSelf];
  }];
}

- (void)playerItemDidPlayToEnd:(NSNotification *)notification {
  AVPlayerItem *item = (AVPlayerItem *)notification.object;
  if (item != self.playerItem) {
    return;
  }
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (item != strongSelf.playerItem) {
      return;
    }
    strongSelf.playing = NO;
    if (strongSelf.loop) {
      strongSelf.playbackEnded = NO;
      [strongSelf.delegate playableDidLoop:strongSelf];
      [strongSelf.player seekToTime:kCMTimeZero
                    toleranceBefore:kCMTimeZero
                     toleranceAfter:kCMTimeZero
                  completionHandler:^(BOOL finished) {
                    __strong typeof(weakSelf) innerStrongSelf = weakSelf;
                    [innerStrongSelf dispatchOnMainThread:^{
                      if (!finished) {
                        [innerStrongSelf notifyErrorWithCode:LynxVideoErrorOperation
                                                     message:@"seek failed"];
                        return;
                      }
                      innerStrongSelf.playbackEnded = NO;
                      [innerStrongSelf.player playImmediatelyAtRate:innerStrongSelf.speed];
                    }];
                  }];
    } else {
      strongSelf.playbackEnded = YES;
      [strongSelf.delegate playableDidEnd:strongSelf];
    }
  }];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context {
  if (context == kLynxVideoItemStatusContext) {
    AVPlayerItem *item = (AVPlayerItem *)object;
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      __strong typeof(weakSelf) strongSelf = weakSelf;
      if (item != strongSelf.playerItem) {
        return;
      }
      if (item.status == AVPlayerItemStatusFailed) {
        [strongSelf notifyErrorWithCode:LynxVideoErrorOperation
                                message:item.error.localizedDescription ?: @"video load failed"];
      } else if (item.status == AVPlayerItemStatusReadyToPlay) {
        [strongSelf dispatchFirstFrameIfNeeded];
      }
    }];
    return;
  }
  if (context == kLynxVideoLoadedRangesContext) {
    AVPlayerItem *item = (AVPlayerItem *)object;
    [self dispatchBufferingUpdateForItem:item];
    return;
  }
  if (context == kLynxVideoLayerReadyContext) {
    [self dispatchFirstFrameIfNeeded];
    return;
  }
  if (context == kLynxVideoTimeControlStatusContext) {
    AVPlayer *player = (AVPlayer *)object;
    __weak typeof(self) weakSelf = self;
    [self dispatchOnMainThread:^{
      __strong typeof(weakSelf) strongSelf = weakSelf;
      if (player != strongSelf.player) {
        return;
      }
      if (player.timeControlStatus == AVPlayerTimeControlStatusPlaying) {
        [strongSelf notifyPlayingIfNeeded];
      } else if (player.timeControlStatus ==
                 AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate) {
        [strongSelf dispatchBufferingUpdate];
      } else if (player.timeControlStatus == AVPlayerTimeControlStatusPaused &&
                 strongSelf.isPlaying) {
        strongSelf.playing = NO;
        [strongSelf notifyPaused];
      }
    }];
    return;
  }
  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

@end
